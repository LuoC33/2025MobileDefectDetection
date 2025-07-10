#ifndef __WIFI_CONN_H__
#define __WIFI_CONN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

// WiFi连接函数
rt_err_t wifi_connect(const char *ssid, const char *password, rt_uint32_t timeout_ms);

// 检查WiFi连接状态
rt_bool_t wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_CONN_H__ */

// #include "wifi_conn.h"

// int main()
// {
//     // 连接WiFi
//     if (wifi_connect("your_ssid", "your_password", 15000) == RT_EOK) {
//         // 连接成功
//         char ip[16];
//         wifi_get_ip(ip, sizeof(ip));
//         rt_kprintf("Connected! IP: %s\n", ip);
//     }
    
//     // 检查连接状态
//     while (1) {
//         if (wifi_is_connected()) {
//             // 执行网络操作
//         }
//         rt_thread_mdelay(1000);
//     }
//     return 0;
// }