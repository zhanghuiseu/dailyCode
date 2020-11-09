/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    main.cpp
* @author  jackszhang
* @date    2020/10/15
* @brief   The interface of test
*
**************************************************************************/

#include <iostream>
#include "log_prefix.h"
#include "log_file.h"
#include <sys/time.h>
#include "utils.h"
#include <time.h>
#include <stdlib.h>

#include <thread>
#include <chrono>
using namespace std;
using namespace dailycode;

std::mutex globalMutex;
static int globalCnt = 0;
class ZipTest : public ZipLogCallBack, public std::enable_shared_from_this<ZipTest> {
 public:
    ZipTest(int id) { m_id = id; }
    virtual void onRecvZipLog(const std::string filePath, const std::string& zipLogs) {
        cout << "Client recv zip daya " << Utils::ipToString(Utils::GetLocalHost()) << " --> "
             << zipLogs.size() << " THREAD -->" << std::to_string(m_id) << endl;
        string path = "";
        {
            lock_guard<mutex> lock(globalMutex);
            path = "/root/home/jackszhang/dailyCode/build/getData-thread-" + std::to_string(m_id) +
                   to_string(globalCnt++) + ".zip";
        }
        FILE* fd = fopen(path.c_str(), "w");
        fwrite(zipLogs.c_str(), zipLogs.size(), sizeof(char), fd);
        fclose(fd);
    }

    void addZipReq() { LOG_ZIP_REQUEST(shared_from_this()); }
    int m_id;
};

void threadFunc(int id) {
    shared_ptr<ZipTest> zipTest(new ZipTest(id));
    while (true) {
        string tmp = "";
        for (int i = 0; i < rand() % 1000; i++) {
            tmp += to_string(i) + "**** ";
            LOGI("GGGg %d qawturit %s", 1234, tmp.c_str());
            // this_thread::sleep_for(chrono::seconds(5));//sleep 5秒
            // this_thread::sleep_for(chrono::hours(1));//sleep 1小时
            // this_thread::sleep_for(chrono::minutes(1));//sleep 1分钟
            SLOGI() << "11TEST FORM STREAM"
                    << " index-->" << i;
            this_thread::sleep_for(chrono::milliseconds(30));  // sleep 1毫秒
        }
        zipTest->addZipReq();
    }
}

int main() {
    srand(time(0));
    LogFile::Init();
    LOG_CONF_SET(LC_LOG_OUTPUT_PATH, "/root/home/jackszhang/dailyCode/build/log/");
    LOG_CONF_SET(LC_LOG_FILE_NAME, "jackszhangLOG");
    LOG_CONF_SET(LC_LOG_APP_NAME, "SIMIDA-appSdk");
    LOG_CONF_SET(LC_LOG_ENABLE_COMPRESS, 1);
    LOG_CONF_SET(LC_LOG_COMPRESS_INTERVAL, 60);
    LOG_CONF_SET(LC_LOG_NEED_ENCRYPTION,
                 ET_NO_ENCRYPTION);  // ET_XOR_ENCRYPTION   ET_BLOWFISH_ENCRYPTION,
    LOG_SET_ENCRYPT_KEY(ET_XOR_ENCRYPTION, "xor123");
    LOG_SET_ENCRYPT_KEY(ET_BLOWFISH_ENCRYPTION, "fish245");
    thread t1(threadFunc, 1);
    thread t2(threadFunc, 2);
    t1.join();
    t2.join();
    LogFile::DeInit();
    return 1;
}