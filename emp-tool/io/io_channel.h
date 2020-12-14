#ifndef EMP_IO_CHANNEL_H__
#define EMP_IO_CHANNEL_H__
#include "emp-tool/utils/block.h"
#include "emp-tool/utils/prg.h"
#include "emp-tool/utils/group.h"
#include "emp-tool/utils/hash.h"
#include <memory>

namespace emp {
template<typename T> 
class IOChannel { public:
	uint64_t counter = 0;
	void send_data(const void * data, int nbyte) {
		if(bool_fast_need_flush)
			flush_bool_fast();
		counter +=nbyte;
		derived().send_data_internal(data, nbyte);
	}

	void recv_data(void * data, int nbyte) {
		if(bool_fast_need_flush)
			flush_bool_fast();
		derived().recv_data_internal(data, nbyte);
	}

	void send_block(const block* data, int nblock) {
		send_data(data, nblock*sizeof(block));
	}

	void recv_block(block* data, int nblock) {
		recv_data(data, nblock*sizeof(block));
	}

	void send_pt(Point *A, int num_pts = 1) {
		for(int i = 0; i < num_pts; ++i) {
			size_t len = A[i].size();
			A[i].group->resize_scratch(len);
			unsigned char * tmp = A[i].group->scratch;
			send_data(&len, 4);
			A[i].to_bin(tmp, len);
			send_data(tmp, len);
		}
	}

	void recv_pt(Group * g, Point *A, int num_pts = 1) {
		size_t len = 0;
		for(int i = 0; i < num_pts; ++i) {
			recv_data(&len, 4);
			g->resize_scratch(len);
			unsigned char * tmp = g->scratch;
			recv_data(tmp, len);
			A[i].from_bin(g, tmp, len);
		}
	}	

	void send_bool(bool * data, int length) {
		void * ptr = (void *)data;
		size_t space = length;
		const void * aligned = std::align(alignof(uint64_t), sizeof(uint64_t), ptr, space);
		if(aligned == nullptr)
			send_data(data, length);
		else{
			int diff = length - space;
			send_data(data, diff);
			send_bool_aligned((const bool*)aligned, length - diff);
		}
	}

	void recv_bool(bool * data, int length) {
		void * ptr = (void *)data;
		size_t space = length;
		void * aligned = std::align(alignof(uint64_t), sizeof(uint64_t), ptr, space);
		if(aligned == nullptr)
			recv_data(data, length);
		else{
			int diff = length - space;
			recv_data(data, diff);
			recv_bool_aligned((bool*)aligned, length - diff);
		}
	}


	void send_bool_aligned(const bool * data, int length) {
		unsigned long long * data64 = (unsigned long long * )data;
		int i = 0;
		for(; i < length/8; ++i) {
			unsigned long long mask = 0x0101010101010101ULL;
			unsigned long long tmp = 0;
#if defined(__BMI2__)
			tmp = _pext_u64(data64[i], mask);
#else
			// https://github.com/Forceflow/libmorton/issues/6
			for (unsigned long long bb = 1; mask != 0; bb += bb) {
				if (data64[i] & mask & -mask) { tmp |= bb; }
				mask &= (mask - 1);
			}
#endif
			send_data(&tmp, 1);
		}
		if (8*i != length)
			send_data(data + 8*i, length - 8*i);
	}
	void recv_bool_aligned(bool * data, int length) {
		unsigned long long * data64 = (unsigned long long *) data;
		int i = 0;
		for(; i < length/8; ++i) {
			unsigned long long mask = 0x0101010101010101ULL;
			unsigned long long tmp = 0;
			recv_data(&tmp, 1);
#if defined(__BMI2__)
			data64[i] = _pdep_u64(tmp, mask);
#else
			data64[i] = 0;
			for (unsigned long long bb = 1; mask != 0; bb += bb) {
				if (tmp & bb) {data64[i] |= mask & (-mask); }
				mask &= (mask - 1);
			}
#endif
		}
		if (8*i != length)
			recv_data(data + 8*i, length - 8*i);
	}

	bool * bool_buf = nullptr;
	size_t bool_ptr;
	const size_t buf_size = NETWORK_BUFFER_SIZE2;
	bool is_sender;
	bool bool_fast_need_flush = false;
	Hash hash;
	void start_bool_fast(bool _is_sender) {
		is_sender = _is_sender;
		if(bool_buf != nullptr)
		bool_buf = (bool * )(new uint64_t[buf_size/8]);
		if(is_sender)
			bool_ptr = 0;
		else bool_ptr = buf_size;
	}
	void send_bool_fast(bool data) {
		bool_fast_need_flush = true;
		bool_buf[bool_ptr] = data;
		bool_ptr++;
		if(bool_ptr == buf_size) {
			send_bool_aligned(bool_buf, buf_size);
			bool_ptr = 0;
		}
	}
	bool recv_bool_fast() {
		bool_fast_need_flush = true;
		if(bool_ptr == buf_size) {
			recv_bool_aligned(bool_buf, buf_size);
			bool_ptr = 0;
		}
		bool res = bool_buf[bool_ptr];
		bool_ptr++;
		return res;
	}
	void flush_bool_fast() {
		if(is_sender) {
			bool data = true;
			while(bool_ptr!=0)
				send_bool_fast(data);
		} else {
			bool_ptr = buf_size;
		}
		bool_fast_need_flush = false;
	}
	~IOChannel() {
		if(bool_buf != nullptr)
			delete[] bool_buf;
	}


	private:
	T& derived() {
		return *static_cast<T*>(this);
	}
};
}
#endif