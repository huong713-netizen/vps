#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <sched.h>

class FastUDP {
public:
    std::string ip;
    int port;
    int payload_size;
    std::atomic<uint64_t> total_bytes_sent{ 0 };

    FastUDP(const std::string& target_ip, int target_port, int payload_size_)
        : ip(target_ip), port(target_port), payload_size(payload_size_) {
    }

    void start() {
        running = true;
        unsigned int ncores = std::thread::hardware_concurrency();
        for (unsigned int i = 0; i < ncores; ++i) {
            workers.emplace_back(&FastUDP::worker, this, i);
        }
    }

    void stop() {
        running = false;
        for (auto& t : workers) if (t.joinable()) t.join();
        workers.clear();
    }

private:
    std::atomic<bool> running{ false };
    std::vector<std::thread> workers;

    void worker(int core_id) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return;
        fcntl(sock, F_SETFL, O_NONBLOCK);

        int bufsize = 16 * 1024 * 1024;
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        std::vector<char> buffer(payload_size, 'A');

        while (running) {
            for (int i = 0; i < 100; ++i) {
                int sent = sendto(sock, buffer.data(), buffer.size(), 0, (sockaddr*)&addr, sizeof(addr));
                if (sent > 0) total_bytes_sent += sent;
            }
        }
        close(sock);
    }
};
