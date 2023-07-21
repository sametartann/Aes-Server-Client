#ifndef AES_H
#define AES_H

#include <string>

class AES {
public:
    AES(const unsigned char* key);
    std::string Encrypt(const std::string& message, const unsigned char* iv);
    std::string Decrypt(const std::string& ciphertext, const unsigned char* iv);

private:
    unsigned char state[4][4];
    unsigned char roundKey[240];

    unsigned char multiply(unsigned char a, unsigned char b); // Decrypt
    void incrementCounter(unsigned char* counter); // Decrypt
    void encryptCounterMode(const unsigned char* counter, unsigned char* output);
    void decryptCounterMode(const unsigned char* counter, unsigned char* output);
    void EncryptCTR(const std::string& message, const unsigned char* iv, std::string& ciphertext);
    void DecryptCTR(const std::string& ciphertext, const unsigned char* iv, std::string& decryptedMessage);
    unsigned char xtime(unsigned char x); // Encrypt
    void KeyExpansion(); // Encrypt, // Decrypt
    void AddRoundKey(int round); // Encrypt, // Decrypt
    void SubBytes(); // Encrypt
    void ShiftRows(); // Encrypt
    void MixColumns(); // Encrypt
    void InvSubBytes(); // Decrypt
    void InvShiftRows(); // Decrypt
    void InvMixColumns(); // Decrypt
    void EncryptBlock(const unsigned char* input, unsigned char* output);
    void DecryptBlock(const unsigned char* input, unsigned char* output);


};

#endif  // AES_H
