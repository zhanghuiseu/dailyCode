/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    encrypt.cpp
* @author  jackszhang
* @date    2020/11/08
* @brief   The interface of xor
*
**************************************************************************/

#include "xor.h"
#include <string>
#include <string.h>
#include <algorithm>

namespace dailycode {

void Xor::setKey(const unsigned char* key, int byte_length) {
    m_key = std::string((const char*)key, byte_length);
}

void Xor::encrypt(unsigned char* dst, const unsigned char* src, int byte_length) {
    if (dst != src) {
        memcpy(dst, src, byte_length);
    }
    int keyIndex = 0;
    int keyLen = m_key.size();
    for (int i = 0; i < byte_length; ++i) {
        dst[i] ^= m_key[keyIndex];
        keyIndex = (keyIndex + 1) % keyLen;
    }
}

void Xor::decrypt(unsigned char* dst, const unsigned char* src, int byte_length) {
    if (dst != src) {
        memcpy(dst, src, byte_length);
    }
    int keyIndex = 0;
    int keyLen = m_key.size();
    for (int i = 0; i < byte_length; ++i) {
        dst[i] ^= m_key[keyIndex];
        keyIndex = (keyIndex + 1) % keyLen;
    }
}

}  // end namespace dailycode