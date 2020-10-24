/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    log_file.cpp
* @author  jackszhang
* @date    2020/10/25
* @brief   The interface of log_file
*
**************************************************************************/

#include <map>
#include <vector>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cctype>
#include "log_file.h"
#include "utils.h"
#include "zip.h"
#include "unzip.h"
#include "xor.h"
#include "blowfish.h"

namespace dailycode {

bool LogFile::m_isInit = false;

void LogFile::Init() {
    LogFile* logFilePtr = SingleTon<LogFile>::Instance();
    std::lock_guard<std::mutex> lock(logFilePtr->m_logMutex);
    if (LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d Log file is already initialized, not init twice\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return;
    }
    logFilePtr->m_logConfIntMap[LC_LOG_LEVEL] = defaultLogLevel;
    logFilePtr->m_logConfIntMap[LC_LOG_ROW_LENGTH] = defaultLogRowLength;
    logFilePtr->m_logConfIntMap[LC_LOG_FILE_MAX_NUM] = defaultLogFilesMaxCnt;
    logFilePtr->m_logConfIntMap[LC_LOG_FILE_MAX_SIEZ] = defaultLogMaxFileSize;
    logFilePtr->m_logConfIntMap[LC_LOG_NEED_REGULAR_CLEAN] = defaultLogNeedClear;
    logFilePtr->m_logConfIntMap[LC_LOG_NEED_PRINT_CONSOLE] = defaultLogNeedPrintConsole;
    logFilePtr->m_logConfIntMap[LC_LOG_NEED_ENCRYPTION] = defauleLogNeedEncryption;
    logFilePtr->m_logConfIntMap[LC_LOG_MAX_CONCURRENT_CNT] = defaultLogMaxConcurrentCnt;
    logFilePtr->m_logConfIntMap[LC_LOG_ENABLE_COMPRESS] = defauleLogEnableCompress;
    logFilePtr->m_logConfIntMap[LC_LOG_COMPRESS_INTERVAL] = defauleLogCompressInterval;

    logFilePtr->m_logConfStrMap[LC_LOG_OUTPUT_PATH] = defaultLogOutputPath;
    logFilePtr->m_logConfStrMap[LC_LOG_FILE_NAME] = defaultLogFileName;
    logFilePtr->m_logConfStrMap[LC_LOG_APP_NAME] = defaultAppName;

    logFilePtr->m_logBuffer = new char[defaultLogRowLength];
    logFilePtr->m_logBuffer[defaultLogRowLength - 1] = '\0';

    logFilePtr->m_logFd = nullptr;
    logFilePtr->m_lastCompressStamp = 0;

    logFilePtr->m_encryptTools[ET_XOR_ENCRYPTION] = std::shared_ptr<Xor>(new Xor());
    std::string key = std::string(defaultXorEncryptKey);
    logFilePtr->m_encryptTools[ET_XOR_ENCRYPTION]
        ->setKey((const unsigned char*)key.c_str(), key.size());

    logFilePtr->m_encryptTools[ET_BLOWFISH_ENCRYPTION] = std::shared_ptr<Blowfish>(new Blowfish());
    key = std::string(defaultBlowfishEncryptKey);
    logFilePtr->m_encryptTools[ET_BLOWFISH_ENCRYPTION]
        ->setKey((const unsigned char*)key.c_str(), key.size());

    logFilePtr->m_stopThreadFlag.store(false);
    logFilePtr->m_logThread =
        std::make_shared<std::thread>(std::thread(&LogFile::threadFunc, logFilePtr));
    LogFile::m_isInit = true;
}

void LogFile::DeInit() {
    LogFile* logFilePtr = SingleTon<LogFile>::Instance();
    {
        std::lock_guard<std::mutex> lock(logFilePtr->m_logMutex);
        if (!LogFile::m_isInit) {
            fprintf(stderr, "%s [ERROR] %s-%d Log file is not init\n",
                    Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
            return;
        }
        LogFile::m_isInit = false;
        logFilePtr->m_stopThreadFlag.store(true);
        logFilePtr->m_logThread->join();

        logFilePtr->m_encryptTools.clear();
        logFilePtr->m_lastCompressStamp = 0;
        if (logFilePtr->m_logFd) {
            fclose(logFilePtr->m_logFd);
        }
        if (logFilePtr->m_logBuffer) {
            delete[] logFilePtr->m_logBuffer;
        }

        logFilePtr->m_logConfStrMap.clear();
        logFilePtr->m_logConfIntMap.clear();
    }
    logFilePtr->Release();
}

void LogFile::set(const int32_t key, const int32_t value) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile not init\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return;
    }
    if (key == LC_LOG_ROW_LENGTH && m_logConfIntMap[key] != value) {
        delete[] m_logBuffer;
        m_logBuffer = new char[value];
        m_logBuffer[value - 1] = '\0';
    }
    m_logConfIntMap[key] = value;
}

int32_t LogFile::getIntConf(const int32_t key, const int32_t defaultValue) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile not init\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return 0;
    }
    if (m_logConfIntMap.find(key) != m_logConfIntMap.end()) {
        return m_logConfIntMap[key];
    }
    return defaultValue;
}

void LogFile::set(const int32_t key, const std::string value) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile not init\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return;
    }
    m_logConfStrMap[key] = value;
}

std::string LogFile::getStrConf(const int32_t key, const std::string defaultValue) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile not init\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return "LogNotInit";
    }
    if (m_logConfStrMap.find(key) != m_logConfStrMap.end()) {
        return m_logConfStrMap[key];
    }
    return defaultValue;
}

void LogFile::setEncryptKey(const int32_t encryptType, const std::string encryptKey) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile not init\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return;
    }

    if (0 == m_logConfIntMap[LC_LOG_NEED_ENCRYPTION] || ET_NO_ENCRYPTION == encryptType) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile disable encrypt\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return;
    }

    if (ET_XOR_ENCRYPTION != encryptType && ET_BLOWFISH_ENCRYPTION != encryptType) {
        return;
    }
    if (m_encryptTools.find(encryptType) == m_encryptTools.end()) {
        if (ET_XOR_ENCRYPTION == encryptType) {
            m_encryptTools[ET_XOR_ENCRYPTION] = std::shared_ptr<Xor>(new Xor());
        } else if (ET_BLOWFISH_ENCRYPTION) {
            m_encryptTools[ET_BLOWFISH_ENCRYPTION] = std::shared_ptr<Blowfish>(new Blowfish());
        } else {
            return;
        }
    }
    m_encryptTools[encryptType]
        ->setKey((const unsigned char*)encryptKey.c_str(), encryptKey.size());
    return;
}

std::string LogFile::getEncryptKey(const int32_t encryptType) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile not init\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return "";
    }

    if (m_logConfIntMap[LC_LOG_NEED_ENCRYPTION] == 0 || ET_NO_ENCRYPTION == encryptType) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile disable encrypt\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return "";
    }

    if (ET_XOR_ENCRYPTION != encryptType && ET_BLOWFISH_ENCRYPTION != encryptType) {
        return "";
    }

    if (m_encryptTools.find(encryptType) == m_encryptTools.end()) {
        return "";
    }
    return m_encryptTools[encryptType]->getKey();
}

void LogFile::addZipRequest(std::shared_ptr<ZipLogCallBack> callBack) {
    std::lock_guard<std::mutex> lock(m_zipMutex);
    std::weak_ptr<ZipLogCallBack> wpCallback(callBack);
    m_zipCallBacks.insert(wpCallback);
}

void LogFile::recviveOneLog(LogLevel level, const char* level_str, const char* fileName,
                            const char* format, ...) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!LogFile::m_isInit) {
        fprintf(stderr, "%s [ERROR] %s-%d LogFile not init\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return;
    }

    if (m_stopThreadFlag) {
        return;
    }

    int32_t curLevel = m_logConfIntMap[LC_LOG_LEVEL];
    if (LL_LOG_NONE == curLevel || level < curLevel) {
        return;
    }
    const char* finalfileName = strrchr(fileName, '/');
    if (finalfileName) {
        finalfileName++;
    } else {
        finalfileName = fileName;
    }
    int32_t rowLen = m_logConfIntMap[LC_LOG_ROW_LENGTH];
    if (rowLen <= 0) {
        return;
    }

    std::string appName = m_logConfStrMap[LC_LOG_APP_NAME];
    snprintf(m_logBuffer, rowLen, " %s [%d:%p] ", appName.c_str(), (int32_t)getpid(), (void*)this);
    int32_t len = strlen(m_logBuffer);
    snprintf((char*)(m_logBuffer + len), rowLen - len, "%s [%s", level_str, finalfileName);
    len = strlen(m_logBuffer);

    va_list args;
    va_start(args, format);
    vsnprintf((char*)(m_logBuffer + len), rowLen - len, format, args);
    va_end(args);
    len = strlen(m_logBuffer);

    const std::string finalLog = Utils::getCurrentSystemTime() + std::string(m_logBuffer, len);
    if (m_allLogs.size() > m_logConfIntMap[LC_LOG_MAX_CONCURRENT_CNT]) {
        fprintf(stderr, "%s [ERROR] %s-%d too much logs(%u)\n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__, m_allLogs.size());
        return;
    }
    m_allLogs.push_back(finalLog);
}

bool LogFile::writeOneLog(const std::string& log) {
    if (log.size() <= 0) {
        return false;
    }
    if (0 != getIntConf(LC_LOG_NEED_PRINT_CONSOLE)) {
        fprintf(stdout, "%s\n", log.c_str());
    }

    if (!openFile()) {
        return false;
    }

    std::string encryptLog = log;
    int32_t encryptType = getIntConf(LC_LOG_NEED_ENCRYPTION);

    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        if (ET_NO_ENCRYPTION != encryptType &&
            m_encryptTools.find(encryptType) != m_encryptTools.end()) {
            unsigned char* encryptData = new unsigned char[log.size()];
            int totalLen = log.size();
            m_encryptTools[encryptType]
                ->encrypt(encryptData, (const unsigned char*)log.c_str(), totalLen);
            encryptLog = std::string((const char*)encryptData, totalLen);
        }
    }
    if (m_logFd != nullptr && fprintf(m_logFd, "%s\n", encryptLog.c_str()) < 0) {
        return false;
    }
    fflush(m_logFd);
    return true;
}

void LogFile::threadFunc() {
    while (!m_stopThreadFlag) {
        std::deque<std::string> tmpQueue;
        {
            std::lock_guard<std::mutex> lock(m_logMutex);
            tmpQueue.swap(m_allLogs);
        }
        while (!tmpQueue.empty()) {
            const std::string log = tmpQueue.front();
            tmpQueue.pop_front();
            if (!writeOneLog(log)) {
                break;
            }
        }
        compressLogs();
    }

    std::deque<std::string> tmpQueue;
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        tmpQueue.swap(m_allLogs);
    }
    while (!tmpQueue.empty()) {
        const std::string log = tmpQueue.front();
        tmpQueue.pop_front();
        if (!writeOneLog(log)) {
            break;
        }
    }
}

void LogFile::updateLogFiles() {
    std::vector<std::string> files;
    std::string fileName = getStrConf(LC_LOG_FILE_NAME);
    Utils::getDirFiles(getStrConf(LC_LOG_OUTPUT_PATH), files);
    for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); it++) {
        std::string logFileName = (*it);
        if (logFileName.find(fileName) == std::string::npos) {
            continue;
        }
        if ((logFileName + ".log") == fileName) {
            continue;
        }
        // test_2020-10-01_1245.log
        std::size_t pos = logFileName.find(".log");
        if (pos == std::string::npos) {
            continue;
        }
        std::string name = logFileName.substr(0, pos);
        std::vector<std::string> subInfos;
        Utils::split(name, "_", subInfos);
        if (subInfos.size() != 3 || !Utils::isDigit(subInfos[2])) {
            continue;
        }
        { m_allFiles[std::stoul(subInfos[2])] = logFileName; }
    }
}

void LogFile::cleanOldFiles() {
    if (0 == getIntConf(LC_LOG_NEED_REGULAR_CLEAN)) {
        return;
    }
    int maxFilesNum = std::max(getIntConf(LC_LOG_FILE_MAX_NUM), 1);
    std::string path = getStrConf(LC_LOG_OUTPUT_PATH);

    while (m_allFiles.size() > maxFilesNum - 1) {
        std::map<uint32_t, std::string>::iterator it = m_allFiles.begin();
        std::string fileName = path + "/" + it->second;
        if (0 == access(fileName.c_str(), F_OK)) {
            if (unlink(fileName.c_str()) < 0) {
                fprintf(stderr, "%s [ERROR] %s-%d unlink %s failed \n",
                        Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__,
                        fileName.c_str());
            }
        }
        m_allFiles.erase(m_allFiles.begin());
    }
}

bool LogFile::openFile() {
    std::string outputLogPath = getStrConf(LC_LOG_OUTPUT_PATH);
    std::string logFileName = getStrConf(LC_LOG_FILE_NAME);
    int32_t logFileMaxSize = getIntConf(LC_LOG_FILE_MAX_SIEZ);

    if (0 != access(outputLogPath.c_str(), F_OK) && !Utils::mkdirRecursive(outputLogPath)) {
        return false;
    }

    const std::string logFile = outputLogPath + "/" + logFileName + ".log";
    if (!m_logFd) {
        updateLogFiles();
        cleanOldFiles();
        m_logFd = fopen(logFile.c_str(), "a+");
    }

    long ftellRes = ftell(m_logFd);
    if (ftellRes < 0 || ftellRes > logFileMaxSize) {
        fclose(m_logFd);
        m_logFd = nullptr;
        // test_2020-10-01_1245.log
        uint32_t stamp = Utils::getTickCount();
        std::string stampFile = logFileName + "_" + Utils::getCurrentSystemDate() + "_" +
                                std::to_string(stamp) + ".log";
        std::string newFileName = outputLogPath + "/" + stampFile;
        if (rename(logFile.c_str(), newFileName.c_str()) < 0) {
            fprintf(stderr, "%s [ERROR] %s-%d  rename files name %s failed \n",
                    Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__,
                    newFileName.c_str());
        }
        openFile();
    }
    return true;
}

bool LogFile::enableCompress() {
    // 配置不允许压缩
    if (getIntConf(LC_LOG_ENABLE_COMPRESS) == 0) {
        std::lock_guard<std::mutex> lock(m_zipMutex);
        m_zipCallBacks.clear();
        return false;
    }
    return true;
}

void LogFile::compressLogs() {
    if (!enableCompress()) {
        fprintf(stderr, "%s [ERROR] %s-%d disable zip logs failed \n",
                Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_zipMutex);
        if (m_zipCallBacks.size() == 0) {
            return;
        }
    }

    std::string path = getStrConf(LC_LOG_OUTPUT_PATH);
    std::string compressName = path + "/" + getStrConf(LC_LOG_APP_NAME) + ".zip";
    std::string nowFileName = getStrConf(LC_LOG_FILE_NAME) + ".log";
    std::string nowLogPath = path + "/" + nowFileName;
    // 暂时没有到压缩间隔，此时会使用上次的压缩文件作为callback
    uint32_t now = Utils::getTickCount();
    uint32_t compressInterval = std::max(getIntConf(LC_LOG_COMPRESS_INTERVAL) * 1000, 10 * 1000);
    if (m_lastCompressStamp != 0 &&
        Utils::isBiggerUint32(m_lastCompressStamp + compressInterval, now)) {
        onCompressData(compressName);
        return;
    }
    updateLogFiles();
    cleanOldFiles();
    {
        if (0 == access(compressName.c_str(), F_OK) && unlink(compressName.c_str()) < 0) {
            fprintf(stderr, "%s [ERROR] %s-%d unlink old zip data %s failed \n",
                    Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__,
                    compressName.c_str());
        }

        HZIP hz = CreateZip(compressName.c_str(), 0);
        for (std::map<uint32_t, std::string>::iterator it = m_allFiles.begin();
             it != m_allFiles.end(); ++it) {
            std::string fileName = path + "/" + it->second;
            if (0 == access(fileName.c_str(), F_OK)) {
                ZipAdd(hz, it->second.c_str(), fileName.c_str());
            }
        }
        if (0 == access(nowLogPath.c_str(), F_OK)) {
            fclose(m_logFd);
            m_logFd = nullptr;
            ZipAdd(hz, nowFileName.c_str(), nowLogPath.c_str());
        }
        CloseZip(hz);
        m_lastCompressStamp = Utils::getTickCount();
    }
    onCompressData(compressName);
    return;
}

void LogFile::onCompressData(std::string compressLogPath) {
    std::string zipData = "";
    {
        if (0 == access(compressLogPath.c_str(), F_OK)) {
            FILE* zipFd = fopen(compressLogPath.c_str(), "r");
            if (!zipFd) {
                fprintf(stderr, "%s [ERROR] %s-%d  open zip data %s failed \n",
                        Utils::getCurrentSystemTime().c_str(), __FUNCTION__, __LINE__,
                        compressLogPath.c_str());
                zipData = "";
            } else {
                fseek(zipFd, 0, SEEK_END);
                int32_t length = ftell(zipFd);
                char* data = (char*)malloc((length + 1) * sizeof(char));
                rewind(zipFd);
                length = fread(data, 1, length, zipFd);
                data[length] = '\0';
                zipData = std::string(data, length);
                delete[] data;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_zipMutex);
        for (std::set<std::weak_ptr<ZipLogCallBack>>::iterator it = m_zipCallBacks.begin();
             it != m_zipCallBacks.end(); ++it) {
            if (!(*it).expired()) {
                std::shared_ptr<ZipLogCallBack> callback = (*it).lock();
                callback->onRecvZipLog(compressLogPath, zipData);
            }
        }
        m_zipCallBacks.clear();
    }
}

}  // end namespace dailycode
