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
#pragma once

#include "encrypt.h"
#include <string>
#include <stdint.h>

namespace dailycode {

class Xor : public baseEncrypt {
 public:
    virtual void setKey(const unsigned char* key, int byte_length);
    virtual void encrypt(unsigned char* dst, const unsigned char* src, int byte_length);
    virtual void decrypt(unsigned char* dst, const unsigned char* src, int byte_length);
};

}  // end namespace dailycode