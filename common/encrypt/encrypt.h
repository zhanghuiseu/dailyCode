/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    encrypt.cpp
* @author  jackszhang
* @date    2020/11/08
* @brief   The interface of encrypt
*
**************************************************************************/

#pragma once
#include <string>
#include <stdint.h>

namespace dailycode {

class baseEncrypt {
 public:
    virtual void setKey(const unsigned char* key, int byte_length) = 0;
    virtual void encrypt(unsigned char* dst, const unsigned char* src, int byte_length) = 0;
    virtual void decrypt(unsigned char* dst, const unsigned char* src, int byte_length) = 0;
    virtual std::string getKey() { return m_key; }

 public:
    std::string m_key;
};

}  // end namespace dailycode