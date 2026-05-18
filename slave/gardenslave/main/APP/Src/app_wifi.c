/**
 * @file    app_wifi.c
 * @brief   存放 SSID、密码，并触发网络连接
 */
#include "app_wifi.h"
#include "bsp_wifi.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_netif.h"

static const char *TAG = "APP_WIFI";

// 在这里修改你的网络配置
#define ROUTER_SSID      "Ll"
#define ROUTER_PASSWORD  "ljc12345"
#define MAX_RETRY_TIMES  5

void App_WiFi_System_Init(void) {
    bsp_wifi_config_t my_wifi_cfg = {
        .max_retry = MAX_RETRY_TIMES,
        .ssid = {0},
        .password = {0}
    };
    
    // 拷贝账号密码
    strncpy(my_wifi_cfg.ssid, ROUTER_SSID, sizeof(my_wifi_cfg.ssid) - 1);
    my_wifi_cfg.ssid[sizeof(my_wifi_cfg.ssid) - 1] = '\0';
    strncpy(my_wifi_cfg.password, ROUTER_PASSWORD, sizeof(my_wifi_cfg.password) - 1);
    my_wifi_cfg.password[sizeof(my_wifi_cfg.password) - 1] = '\0';

ESP_LOGI(TAG, "网络子系统启动，正在连接热点: %s", ROUTER_SSID);

    // 调用底层开始阻塞连接
    if (BSP_WiFi_Init_And_Connect(&my_wifi_cfg)) {
        // 获取并打印分配到的 IP 地址
        esp_netif_ip_info_t ip_info;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"); // 修正大小写错误
        if (netif) {
            esp_netif_get_ip_info(netif, &ip_info);
            ESP_LOGI(TAG, "WiFi 连接成功! IP: " IPSTR, IP2STR(&ip_info.ip));
        } else {
            ESP_LOGI(TAG, "WiFi 连接成功 (IP获取未知)");
        }
        
        // 【核心修复】关闭 WiFi 省电模式，确保图传网页响应飞快
        esp_wifi_set_ps(WIFI_PS_NONE);
        
    } else {
ESP_LOGE(TAG, "网络连接失败，请检查密码或路由器状态。系统将在脱机模式下运行。");
    }
}

void App_Wifi_Scan(void) {
    ESP_LOGW(TAG, "App_Wifi_Scan() 已禁用 - 与已初始化的WiFi栈冲突");
}
