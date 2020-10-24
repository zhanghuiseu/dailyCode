/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    encrypt.cpp
* @author  jackszhang
* @date    2020/11/08
* @brief   The interface of blodwfish
*
**************************************************************************/
#pragma once

#include "encrypt.h"
#include <string>
#include <stdint.h>

namespace dailycode {

class Blowfish : public baseEncrypt {
 public:
    virtual void setKey(const unsigned char* key, int byte_length);
    virtual void encrypt(unsigned char* dst, const unsigned char* src, int byte_length);
    virtual void decrypt(unsigned char* dst, const unsigned char* src, int byte_length);

 private:
    void encryptBlock(uint32_t* left, uint32_t* right);
    void decryptBlock(uint32_t* left, uint32_t* right);
    uint32_t feistel(uint32_t value);

 private:
    uint32_t m_pary_[18];
    uint32_t m_sbox_[4][256];
};

}  // end namespace dailycode