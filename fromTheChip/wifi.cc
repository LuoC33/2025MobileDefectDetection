#include "wifi.h"

// 连接Wi-Fi函数
bool connectToWiFi(const std::string& ssid, const std::string& password) {
    // 构建命令
    std::string command = "wifi join " + ssid + " " + password;
    
    std::cout << "Executing: " << command << std::endl;
    
    // 执行命令
    int result = system(command.c_str());
    
    if (result == 0) {
        std::cout << "Wi-Fi connection initiated successfully." << std::endl;
        
        // 等待连接建立
        for (int i = 0; i < 15; i++) {
            if (system("ping -c 1 www.baidu.com > /dev/null 2>&1") == 0) {
                std::cout << "Internet connection verified!" << std::endl;
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cerr << "Connection timeout! Internet not available." << std::endl;
    } else {
        std::cerr << "Wi-Fi connection failed with error code: " << result << std::endl;
    }
    return false;
}

// 从配置文件读取Wi-Fi凭证
bool readWiFiConfig(std::string& ssid, std::string& password) {
    std::ifstream configFile(CONFIG_FILE);
    if (!configFile.is_open()) {
        std::cerr << "Error: Could not open config file: " << CONFIG_FILE << std::endl;
        return false;
    }
    
    if (!std::getline(configFile, ssid) || !std::getline(configFile, password)) {
        std::cerr << "Error: Invalid config file format." << std::endl;
        configFile.close();
        return false;
    }
    
    configFile.close();
    return true;
}

// int main(int argc, char* argv[]) {
//     std::string ssid, password;
    
//     // 检查是否通过命令行参数传递凭证
//     if (argc == 3) {
//         ssid = argv[1];
//         password = argv[2];
//     } 
//     // 尝试从配置文件读取
//     else if (!readWiFiConfig(ssid, password)) {
//         std::cerr << "Usage: " << argv[0] << " <SSID> <Password>" << std::endl;
//         std::cerr << "Or create config file: " << CONFIG_FILE << std::endl;
//         return 1;
//     }
    
//     // 尝试连接Wi-Fi
//     if (connectToWiFi(ssid, password)) {
//         std::cout << "Wi-Fi connected successfully!" << std::endl;
//         return 0;
//     }
    
//     std::cerr << "Failed to connect to Wi-Fi." << std::endl;
//     return 2;
// }