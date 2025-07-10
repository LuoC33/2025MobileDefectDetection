/*
 * WiFi连接实现 - 使用RT-Thread WiFi管理API
 */

#include "wifi_conn.h"
#include <rthw.h>
#include <rtthread.h>
#include <wlan_mgnt.h>
#include <wlan_cfg.h>
#include <wlan_prot.h>
 
 // 网络就绪信号量
 static struct rt_semaphore net_ready;
 #define RT_UNUSED(x) ((void)x)
 
 // WiFi状态回调
 static void wifi_ready_handler(int event, struct rt_wlan_buff *ebuf, void *eparameter)
 {
     RT_UNUSED(event);
     RT_UNUSED(ebuf);
     RT_UNUSED(eparameter);
     
     rt_sem_release(&net_ready);
 }
 
 static void wifi_connect_handler(int event, struct rt_wlan_buff *abuf, void *aparameter)
 {
     RT_UNUSED(aparameter);
     
     if (abuf && abuf->len == sizeof(struct rt_wlan_info)) {
         struct rt_wlan_info *info = (struct rt_wlan_info *)abuf->data;
         rt_kprintf("[WiFi] Connected to: %s\n", info->ssid.val);
     }
 }
 
 static void wifi_disconnect_handler(int event, struct rt_wlan_buff *bbuf, void *bparameter)
 {
     RT_UNUSED(bparameter);
     
     if (bbuf && bbuf->len == sizeof(struct rt_wlan_info)) {
         struct rt_wlan_info *info = (struct rt_wlan_info *)bbuf->data;
         rt_kprintf("[WiFi] Disconnected from: %s\n", info->ssid.val);
     }
 }
 
 static void wifi_connect_fail_handler(int event, struct rt_wlan_buff *cbuf, void *cparameter)
 {
     RT_UNUSED(cparameter);
     
     if (cbuf && cbuf->len == sizeof(struct rt_wlan_info)) {
         struct rt_wlan_info *info = (struct rt_wlan_info *)cbuf->data;
         rt_kprintf("[WiFi] Failed to connect: %s\n", info->ssid.val);
     }
 }
 
 // 初始化WiFi连接
 rt_err_t wifi_connect_init(void)
 {
     static rt_bool_t initialized = RT_FALSE;
     
     if (initialized) {
         return RT_EOK;
     }
     
     // 初始化WiFi模块
     if (rt_wlan_init() != RT_EOK) {
         rt_kprintf("[WiFi] Initialization failed\n");
         return -RT_ERROR;
     }
     
     // 创建网络就绪信号量
     if (rt_sem_init(&net_ready, "net_ready", 0, RT_IPC_FLAG_PRIO) != RT_EOK) {
         rt_kprintf("[WiFi] Semaphore init failed\n");
         return -RT_ERROR;
     }
     
     // 注册事件处理
     rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wifi_ready_handler, RT_NULL);
     rt_wlan_register_event_handler(RT_WLAN_EVT_STA_CONNECTED, wifi_connect_handler, RT_NULL);
     rt_wlan_register_event_handler(RT_WLAN_EVT_STA_DISCONNECTED, wifi_disconnect_handler, RT_NULL);
     rt_wlan_register_event_handler(RT_WLAN_EVT_STA_CONNECTED_FAIL, wifi_connect_fail_handler, RT_NULL);
     
     initialized = RT_TRUE;
     return RT_EOK;
 }
 
 // 连接WiFi
 rt_err_t wifi_connect(const char *ssid, const char *password, rt_uint32_t timeout_ms)
 {
     rt_err_t result;
     
     // 初始化连接环境
     if (wifi_connect_init() != RT_EOK) {
         return -RT_ERROR;
     }
     
     // 设置工作模式为STA
     rt_wlan_set_mode("w0", RT_WLAN_STATION);
     
     rt_kprintf("[WiFi] Connecting to %s...\n", ssid);
     
     // 发起连接
     result = rt_wlan_connect(ssid, password);
     if (result != RT_EOK) {
         rt_kprintf("[WiFi] Connect command failed: %d\n", result);
         return result;
     }
     
     // 等待网络就绪（获取IP）
     if (rt_sem_take(&net_ready, rt_tick_from_millisecond(timeout_ms))) {
         rt_kprintf("[WiFi] Connection timeout\n");
         return -RT_ETIMEOUT;
     }
     
     rt_kprintf("[WiFi] Connected successfully!\n");
     return RT_EOK;
 }
 
 // 检查是否已连接
 rt_bool_t wifi_is_connected(void)
 {
     return rt_wlan_is_connected();
 }
 
 // MSH命令
 #ifdef RT_USING_FINSH
 #include <finsh.h>
 
 static void wifi_cmd(int argc, char *argv[])
 {
     if (argc < 3) {
         rt_kprintf("Usage: wifi_connect <ssid> <password> [timeout_ms]\n");
         return;
     }
     
     const char *ssid = argv[1];
     const char *password = argv[2];
     rt_uint32_t timeout = (argc > 3) ? atoi(argv[3]) : 15000; // 默认15秒
     
     rt_err_t ret = wifi_connect(ssid, password, timeout);
     if (ret == RT_EOK) {
         char ip[16];
         if (wifi_get_ip(ip, sizeof(ip)) == RT_EOK) {
             rt_kprintf("IP Address: %s\n", ip);
         }
     } else {
         rt_kprintf("Connection failed: %d\n", ret);
     }
 }
 MSH_CMD_EXPORT(wifi_cmd, Connect to WiFi network);
 #endif