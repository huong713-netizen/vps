#pragma once
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <string>

inline void InstallLinuxStartup() {
    // Lấy đường dẫn file thực thi hiện tại
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        std::cerr << "[-] Cannot get executable path\n";
        return;
    }
    exe_path[len] = '\0';
    std::string exec_path(exe_path);

    std::string home = std::getenv("HOME") ? std::getenv("HOME") : "~";
    std::string systemd_dir = home + "/.config/systemd/user";
    std::string service_file = systemd_dir + "/fastbot.service";

    // Tạo thư mục nếu chưa có
    std::string mkdir_cmd = "mkdir -p " + systemd_dir;
    system(mkdir_cmd.c_str());

    // Tạo file service
    std::ofstream ofs(service_file);
    if (!ofs.is_open()) {
        std::cerr << "[-] Failed to create service file: " << service_file << "\n";
        return;
    }

    ofs << "[Unit]\n";
    ofs << "Description=FastBot Auto Start\n";
    ofs << "After=network.target\n\n";
    ofs << "[Service]\n";
    ofs << "Type=simple\n";
    ofs << "ExecStart=" << exec_path << "\n";
    ofs << "Restart=on-failure\n\n";
    ofs << "[Install]\n";
    ofs << "WantedBy=default.target\n";
    ofs.close();

    // Reload systemd user daemon
    system("systemctl --user daemon-reload");

    // Enable service
    system("systemctl --user enable fastbot.service");

    std::cout << "[+] Auto-start installed via systemd: " << service_file << "\n";
    std::cout << "[+] Use 'systemctl --user start fastbot.service' to start manually\n";
}
