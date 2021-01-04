#include "emp-tool/emp-tool.h"

// expected out: 7ca51614425c3ba8ce54dd2fc2020ae7b6e574d198136d0fae7e26ccbf0be7a6
void sha2_check() {
    std::cout << "Testing\n";
    emp::Integer inp(512 + 256, 0, emp::ALICE);

    emp::Integer out(256, 0, emp::PUBLIC);

    std::string filepath = "/usr/local/include/emp-tool/circuits/files/bristol_fashion/sha256.txt";
    emp::BristolFashion cf(filepath.c_str());

    vector<emp::Bit> reverse(512 + 256);
    vector<emp::Bit> result(256);
    // Invert bits: 0->0,511; 1->1,510, ... 511->511,0
    for (int i=0; i<512; i++)
        reverse[i] = inp.bits[511-i];

    // Invert bits: 0->512,767; 1->513,766; ... 255->767,512
    for (int i=0; i<256; i++)
        reverse[i+512] = inp.bits[767-i];

    cf.compute(result.data(), reverse.data());

    // Invert bits    
    for (int i=0; i<256; i++)
        out.bits[i] = result[255-i];
    
    std::string expected = "0111110010100101000101100001010001000010010111000011101110101000110011100101010011011101001011111100001000000010000010101110011110110110111001010111010011010001100110000001001101101101000011111010111001111110001001101100110010111111000010111110011110100110";
    if (expected != out.reveal<std::string>()) {
        std::cout << "Failing\n";
        // std::cout << out.reveal<std::string>() << std::endl;
        assert(false);
    }
    else {
        std::cout << "GOOD!\n";
    }
}

void sha2_stateful() {
    emp::Integer inp(512 + 256, 0, emp::ALICE);

    emp::Integer out(256, 0, emp::PUBLIC);

    std::string filepath = "/usr/local/include/emp-tool/circuits/files/bristol_fashion/sha256.txt";
    emp::BristolFashion cf(filepath.c_str());
    vector<emp::Bit> reverse(512 + 256);
    // Invert bits: 0->511; 1->510, ... 511->0
    for (int i=0; i<512; i++)
        reverse[i] = inp.bits[511-i];

    // Invert bits: 512->767; 513->766; ... 767->512
    for (int i=0; i<256; i++)
        reverse[i+512] = inp.bits[767-i];

    vector<emp::Bit> result(256);
    cf.compute(result.data(), reverse.data());

    // Invert bits
    for (int i=0; i<256; i++)
        out.bits[i] = result[255-i];

    out.reveal<std::string>();
}

void modadder256() {
    emp::Integer inp(256 * 2, 0, emp::ALICE);

    emp::Integer out(256, 0, emp::PUBLIC);

    std::string filepath = "/usr/local/include/emp-tool/circuits/files/bristol_fashion/ModAdd512.txt";
    emp::BristolFashion cf(filepath.c_str());

    vector<emp::Bit> reverse(512*3);
    // Invert bits: 0->511; 1->510, ... 511->0
    for (int i=0; i<256; i++) {
        reverse[i] = inp.bits[255-i];
    }

    // Invert bits: 512->512 + 511; ... 1023->512
    for (int i=0; i<256; i++)
        reverse[i+512] = inp.bits[(256+255)-i];

    // Invert bits: 1024->512*2 + 511; ... 3*512-1->1024
    const char* modulus = "1111111111111111111111111111111100000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
    for (int i=0; i<256; i++) {
        reverse[i+512*2] = (modulus[255-i] == '1');
    }

    vector<emp::Bit> result(512);
    cf.compute(result.data(), reverse.data());

    // Invert bits  
    // 3 2 1 0 
    // 0a0b0c0d00000
    for (int i=0; i<256; i++)
        out.bits[i] = result[255-i];

    out.reveal<std::string>();
}

int main(int argc, char** argv) {
	emp::setup_plain_prot(true, "ModAdd256.txt");
    modadder256();
	emp::finalize_plain_prot();
}
