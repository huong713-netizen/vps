#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <curl/curl.h>

#ifdef _WIN32
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#include <fstream>
#endif

class BotLogger {
private:
    std::string serverUrl;

    // Lấy số core CPU
    std::string getCPU() {
#ifdef _WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return std::to_string(sysinfo.dwNumberOfProcessors) + "core";
#else
        return std::to_string(sysconf(_SC_NPROCESSORS_ONLN)) + "core";
#endif
    }

    // Lấy RAM (GB)
    std::string getRAM() {
#ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return std::to_string(status.ullTotalPhys / (1024 * 1024 * 1024)) + "GB";
#else
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        while(std::getline(meminfo, line)) {
            if(line.find("MemTotal") != std::string::npos) {
                long kb = std::stol(line.substr(line.find_first_of("0123456789")));
                return std::to_string(kb / 1024 / 1024) + "GB";
            }
        }
        return "UnknownRAM";
#endif
    }

    // Lấy OS
    std::string getOS() {
#ifdef _WIN32
        return "Windows";
#else
        struct utsname u;
        if(uname(&u) == 0)
            return u.sysname;
        else
            return "Linux";
#endif
    }

    // Gửi POST request với cURL
    void postData(const std::string& cpu, const std::string& ram, const std::string& os) {
        CURL *curl = curl_easy_init();
        if(curl) {
            std::string postFields = "cpu=" + cpu + "&ram=" + ram + "&os=" + os;
            curl_easy_setopt(curl, CURLOPT_URL, serverUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            CURLcode res = curl_easy_perform(curl);
            if(res != CURLE_OK)
                std::cerr << "[BotLogger] POST failed: " << curl_easy_strerror(res) << "\n";
            curl_easy_cleanup(curl);
        }
    }

public:
    BotLogger(const std::string& url) : serverUrl(url) {}

    // Gửi thông tin ngay lập tức
    void send() {
        std::string cpu = getCPU();
        std::string ram = getRAM();
        std::string os  = getOS();
        postData(cpu, ram, os);
    }

    // Gửi thông tin định kỳ (mỗi intervalSeconds)
    void sendLoop(int intervalSeconds = 60) {
        std::thread([this, intervalSeconds]() {
            while(true) {
                send();
                std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
            }
        }).detach();
    }
};
