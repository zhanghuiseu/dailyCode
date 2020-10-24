/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    utils.h
* @author  jackszhang
* @date    2020/10/25
* @brief   The interface of utils
*
**************************************************************************/
#include <map>
#include <vector>
#include <string>
#include <stdint.h>

namespace dailycode {
class Utils {
 public:
    static const std::string getCurrentSystemTime();
    static const std::string getCurrentSystemDate();
    static uint32_t getTickCount();
    static const void split(std::string target, std::string delimter,
                            std::vector<std::string>& res);
    static bool mkdirRecursive(const std::string path);
    static void getDirFiles(std::string path, std::vector<std::string>& res);
    static bool isDigit(const std::string num);

    static bool isBiggerUint32(uint32_t src, uint32_t dest);
    static bool isEqualOrBiggerUint32(uint32_t src, uint32_t dest);

    static uint32_t GetLocalHost();
    static std::string ipToString(uint32_t ip);
};
}  // end namespace dailycode