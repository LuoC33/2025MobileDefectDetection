#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <finsh.h>

// 配置文件路径
const std::string CONFIG_FILE = "/sdcard/app/wifi_config.txt";

bool connectToWiFi(const std::string& ssid, const std::string& password);

bool readWiFiConfig(std::string& ssid, std::string& password);