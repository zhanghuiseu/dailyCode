/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    log_file.h
* @author  jackszhang
* @date    2020/10/25
* @brief   The interface of log file 日志模块接口
*
**************************************************************************/

#pragma once
// 流式日志输出
#include <iostream>
#include <sstream>

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <map>
#include <set>
#include <string>
#include <deque>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include "encrypt.h"

#include "singleton.hpp"

namespace dailycode {

// 压缩文件回调，目前仅仅支持zip压缩
class ZipLogCallBack {
 public:
    virtual void onRecvZipLog(const std::string filePath, const std::string& zipLogs) = 0;
};

#define defaultLogLevel LL_LOG_INFO            // 默认Info级别日志
#define defaultLogRowLength 1024               // 默认每行的log大小1024Byte
#define defaultLogFilesMaxCnt 3                // 默认3个日志文件
#define defaultLogMaxFileSize 5 * 1024 * 1024  // 默认每个日志文件最多5M
#define defaultLogNeedClear 1                  // 默认需要清理过过期日志
#define defaultLogNeedPrintConsole 0           // 默认不需要输出到终端
#define defauleLogNeedEncryption \
    0  // 默认不需要加密日志，默认不需要，目前提供xor、blowfish等加密策略
#define defaultXorEncryptKey "qwertyuiop"       // 默认xor加密算法的key
#define defaultBlowfishEncryptKey "qwertyuiop"  // 默认blowfish加密算法的key
#define defaultLogMaxConcurrentCnt 10000        // 最大并发log数量，默认1000个
#define defauleLogEnableCompress 0              // 默认允许压缩日志
#define defauleLogCompressInterval 300          // 默认压缩间隔300s，单位秒
#define defaultLogOutputPath "./"               // 日志文件输出到当前目录
#define defaultLogFileName "logsdk"             // 日志文件名字，默认为logsdk.log
#define defaultAppName "logsdk"                 // 日志APP名称，默认logsdk

enum LogConfigInt {
    LC_LOG_LEVEL = 0,           // 日志级别，默认Info
    LC_LOG_ROW_LENGTH,          // 日志每行的Length
    LC_LOG_FILE_MAX_NUM,        // 输出日志文件的数量
    LC_LOG_FILE_MAX_SIEZ,       // 每个日志文件的最大大小
    LC_LOG_NEED_REGULAR_CLEAN,  // 是否需要定期清理日志文件
    LC_LOG_NEED_PRINT_CONSOLE,  // 是否需要输出到终端
    LC_LOG_NEED_ENCRYPTION,     // 是否需要加密日志
    LC_LOG_MAX_CONCURRENT_CNT,  // 最大并发log数量，默认1000个
    LC_LOG_ENABLE_COMPRESS,     // 是否允许压缩日志
    LC_LOG_COMPRESS_INTERVAL,   // 压缩日志间隔
};

enum LogConfigStr {
    LC_LOG_OUTPUT_PATH = 0,  // 日志输出路径，建议使用绝对路径
    LC_LOG_FILE_NAME,        // 日志文件名字
    LC_LOG_APP_NAME,         // 日志app名称，会输出到每行日志，便于日志染色
};

enum LogLevel {
    LL_LOG_TRACE = 0,
    LL_LOG_INFO,
    LL_LOG_WARN,
    LL_LOG_ERROR,
    LL_LOG_NONE,
};

enum EncryptionType {
    ET_NO_ENCRYPTION,
    ET_XOR_ENCRYPTION,
    ET_BLOWFISH_ENCRYPTION,
};

class LogFile : public SingleTon<LogFile> {
 public:
    static void Init();
    static void DeInit();

    // LogConfigKey --> LogConfigIntValue
    void set(const int32_t key, const int32_t value);
    int32_t getIntConf(const int32_t key, const int32_t defaultValue = 0);

    // LogConfigKey --> LogConfigStrValue
    void set(const int32_t key, const std::string value);
    std::string getStrConf(const int32_t key, const std::string defaultValue = "");

    // encryptType --> key
    void setEncryptKey(const int32_t encryptType, const std::string encryptKey);
    std::string getEncryptKey(const int32_t encryptType);

    // 发起压缩文件请求
    void addZipRequest(std::shared_ptr<ZipLogCallBack> callBack);

    // 写日志
    void recviveOneLog(LogLevel level, const char* levelStr, const char* fileName,
                       const char* format, ...);

 private:
    bool writeOneLog(const std::string& log);
    void threadFunc();
    void updateLogFiles();
    void cleanOldFiles();
    bool enableCompress();
    void compressLogs();
    void onCompressData(std::string compressLogPath);

 private:
    bool openFile();

 private:
    friend class SingleTon<LogFile>;
    LogFile(void) {};
    ~LogFile() {};

 private:
    static bool m_isInit;
    char* m_logBuffer;
    std::mutex m_logMutex;
    std::map<int32_t, int32_t> m_logConfIntMap;
    std::map<int32_t, std::string> m_logConfStrMap;
    std::map<int32_t, std::shared_ptr<baseEncrypt>> m_encryptTools;

 private:
    std::shared_ptr<std::thread> m_logThread;
    std::atomic<bool> m_stopThreadFlag;
    std::deque<std::string> m_allLogs;

 private:
    FILE* m_logFd;
    std::map<uint32_t, std::string> m_allFiles;

    std::mutex m_zipMutex;
    uint32_t m_lastCompressStamp;
    std::set<std::weak_ptr<ZipLogCallBack>, std::owner_less<std::weak_ptr<ZipLogCallBack>>>
        m_zipCallBacks;
};

// 流失输出日志辅助类
class StreamLogHelper {
 public:
    StreamLogHelper(LogLevel level, const char* levelStr, const char* codeFileName,
                    const char* codeFunction, const int32_t codeLine)
        : m_level(level),
          m_levelStr(levelStr),
          m_codeFileName(codeFileName),
          m_codeFunction(codeFunction),
          m_codeLine(codeLine) {}

    template <typename T>
    StreamLogHelper& operator<<(const T& t) {
        ss << t;
        return *this;
    }

    ~StreamLogHelper() {
        SingleTon<LogFile>::Instance()->recviveOneLog(m_level, m_levelStr, m_codeFileName,
                                                      "-%s:%d] %s", m_codeFunction, m_codeLine,
                                                      ss.str().c_str());
        ss.clear();
    }

 private:
    LogLevel m_level;
    const char* m_levelStr;
    const char* m_codeFileName;
    const char* m_codeFunction;
    const int32_t m_codeLine;
    std::stringstream ss;
};

}  // end namespace dailycode

/*************  LOG CONF API  *************/
#define LOG_CONF_SET(key, value) SingleTon<LogFile>::Instance()->set(key, value)
#define LOG_CONF_GET_INT(key) SingleTon<LogFile>::Instance()->getIntConf(key)
#define LOG_CONF_GET_STR(key) SingleTon<LogFile>::Instance()->getStrConf(key)
#define LOG_SET_ENCRYPT_KEY(encryptType, key) \
    SingleTon<LogFile>::Instance()->setEncryptKey(encryptType, key)
#define LOG_GET_ENCRYPT_KEY(encryptType) SingleTon<LogFile>::Instance()->getEncryptKey(encryptType)

// 压缩日志请求
#define LOG_ZIP_REQUEST(callback) SingleTon<LogFile>::Instance()->addZipRequest(callback)

/*************  LOG API  *************/
// C风格日志输出
#define LOGT(format, args...)                                                                \
    SingleTon<LogFile>::Instance()->recviveOneLog(LogLevel::LL_LOG_TRACE, "T", __FILE__,     \
                                                  "-%s:%d] " format, __FUNCTION__, __LINE__, \
                                                  ##args)
#define LOGI(format, args...)                                                                \
    SingleTon<LogFile>::Instance()->recviveOneLog(LogLevel::LL_LOG_INFO, "I", __FILE__,      \
                                                  "-%s:%d] " format, __FUNCTION__, __LINE__, \
                                                  ##args)
#define LOGW(format, args...)                                                                \
    SingleTon<LogFile>::Instance()->recviveOneLog(LogLevel::LL_LOG_WARN, "W", __FILE__,      \
                                                  "-%s:%d] " format, __FUNCTION__, __LINE__, \
                                                  ##args)
#define LOGE(format, args...)                                                                \
    SingleTon<LogFile>::Instance()->recviveOneLog(LogLevel::LL_LOG_ERROR, "E", __FILE__,     \
                                                  "-%s:%d] " format, __FUNCTION__, __LINE__, \
                                                  ##args)

// 日志流方式输出接口
#define STREAM_LOG_HELPER(LOGLEVEL, LEVEL_STR, FILE, FUNCTION, LINE) \
    StreamLogHelper(LOGLEVEL, LEVEL_STR, FILE, FUNCTION, LINE)

#define SLOGT() STREAM_LOG_HELPER(LogLevel::LL_LOG_TRACE, "T", __FILE__, __FUNCTION__, __LINE__)
#define SLOGI() STREAM_LOG_HELPER(LogLevel::LL_LOG_INFO, "I", __FILE__, __FUNCTION__, __LINE__)
#define SLOGW() STREAM_LOG_HELPER(LogLevel::LL_LOG_WARN, "W", __FILE__, __FUNCTION__, __LINE__)
#define SLOGE() STREAM_LOG_HELPER(LogLevel::LL_LOG_ERROR, "E", __FILE__, __FUNCTION__, __LINE__)
