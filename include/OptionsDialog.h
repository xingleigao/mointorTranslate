#ifndef OPTIONS_WINDOW_H
#define OPTIONS_WINDOW_H

#include <windows.h>

// 定义一个结构体来传递数据，避免全局变量
typedef struct {
    int protocol_type; // 0:UDP, 1:TCP, 2:UART
    
    // 使用联合体，因为同一时间只会用到其中一组
    union {
        struct {
            char ip[32];
            int  port;
        } net;
        struct {
            char port_name[16]; // "COM3"
            int  baud;          // 115200
        } uart;
    } cfg;
} PluginConfig;

// 只暴露一个简单的函数接口
#ifdef __cplusplus
extern "C" {
#endif

// 弹出对话框，成功返回 1，取消返回 0
int ShowConfigDialog(HINSTANCE hInst, HWND hParent, PluginConfig* config);

#ifdef __cplusplus
}
#endif

#endif