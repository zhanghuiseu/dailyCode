/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    utils.cpp
* @author  jackszhang
* @date    2020/10/25
* @brief   The interface of utils
*
**************************************************************************/
#include "utils.h"

#include <string>
#include <map>
#include <queue>
#include <sys/types.h>
#include <cctype>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <net/if.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <sstream>
#include <iomanip>

namespace dailycode {

const std::string Utils::getCurrentSystemTime() {
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    char date[100] = {0};
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));
    return std::string(date) + "." + std::to_string(curTime.tv_usec / 1000);
}

const std::string Utils::getCurrentSystemDate() {
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    char date[100] = {0};
    strftime(date, sizeof(date), "%Y-%m-%d", localtime(&curTime.tv_sec));
    return std::string(date);
}

uint32_t Utils::getTickCount() {
    struct timespec tsNowTime;
    clock_gettime(CLOCK_MONOTONIC, &tsNowTime);
    return (uint32_t)((uint64_t)tsNowTime.tv_sec * 1000 + (uint64_t)tsNowTime.tv_nsec / 1000000);
}

const void Utils::split(std::string target, const std::string delimter,
                        std::vector<std::string>& res) {
    res.clear();
    std::string::size_type pos = target.find_first_of(delimter);
    while (pos != std::string::npos) {
        std::string tmp = target.substr(0, pos);
        res.push_back(tmp);
        target = target.substr(pos + 1);
        pos = target.find_first_of(delimter);
    }
    if (target.size() > 0) {
        res.push_back(target);
    }
    return;
}

bool Utils::mkdirRecursive(const std::string path) {
    if (path.size() <= 0 || path == "." || path == "./" || path == "..") {
        return true;
    }
    if (0 == access(path.c_str(), F_OK)) {
        return true;
    }
    std::vector<std::string> res;
    Utils::split(path, "/", res);

    std::vector<std::string> pathVec;
    for (size_t i = 0; i < res.size(); ++i) {
        if (res[i].size() <= 0) {
            continue;
        }
        if (pathVec.size() > 0 && res[i] == ".") {
            continue;
        }
        if (res[i] == "..") {
            if (pathVec.empty()) {
                continue;
            } else {
                pathVec.pop_back();
            }
        } else {
            pathVec.push_back(res[i]);
        }
    }

    bool isSucc = true;
    std::string subPath = "/";
    for (size_t i = 0; i < pathVec.size(); ++i) {
        subPath += pathVec[i] + "/";
        if (subPath == "./") {
            continue;
        }

        if (access(subPath.c_str(), 0) != 0 && mkdir(subPath.c_str(), 0755) != 0) {
            isSucc = false;
            break;
        }
    }
    return isSucc;
}

void Utils::getDirFiles(std::string path, std::vector<std::string>& res) {
    res.clear();
    if (access(path.c_str(), 0) != 0) {
        return;
    }
    DIR* dir = opendir(path.c_str());
    struct dirent* ptr;
    while ((ptr = readdir(dir)) != NULL) {
        res.push_back(std::string(ptr->d_name));
    }
    closedir(dir);
}

bool Utils::isDigit(const std::string num) {
    for (size_t i = 0; i < num.size(); ++i) {
        if (!isdigit(num[i])) {
            return false;
        }
    }
    return true;
}

bool Utils::isBiggerUint32(uint32_t src, uint32_t dest) {
    return (src != dest && src - dest < 0x7fffffff);
}

bool Utils::isEqualOrBiggerUint32(uint32_t src, uint32_t dest) { return (src - dest < 0x7fffffff); }

uint32_t Utils::GetLocalHost() {
    uint32_t hostIP = 0;
    int s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return 0;
    }

    ifconf.ifc_buf = (char*)ifr;
    ifconf.ifc_len = sizeof ifr;
    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
        close(s);
        return 0;
    }

    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    for (i = 0; i < ifs; ++i) {
        // not use IFNAMSIZ = 16
        if (strcmp(ifr[i].ifr_name, "wlan0") == 0 ||   // for mobile
            strcmp(ifr[i].ifr_name, "br-lan") == 0 ||  // for newifi
            strcmp(ifr[i].ifr_name, "eth0") == 0)      // for linux ikuai gehua
        {
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in* s_in = (struct sockaddr_in*)&ifr[i].ifr_addr;
            if (inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip))) {
                // equal to s_in->sin_addr.s_addr
                hostIP = inet_addr(ip);
                break;
            }
        }
    }
    close(s);
    return hostIP;
}

std::string Utils::ipToString(uint32_t ip) {
    char ipStr[100];
    // 网络序
    snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", ip & 0xFF, (ip & 0xFF00) >> 8,
             (ip & 0xFF0000) >> 16, ip >> 24);
    // 主机序
    // sprintf(ipStr, "[%d.%d.%d.%d]", ip >> 24, (ip & 0xFF0000) >> 16, (ip & 0xFF00) >> 8, ip &
    // 0xFF);
    return std::string(ipStr);
}

}  // end of namespace dailycode