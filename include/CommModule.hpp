#ifndef COMM_MODULE_HPP
#define COMM_MODULE_HPP

#include <winsock2.h>
#include <windows.h>
#include "OptionsDialog.h" // 引用你之前的 PluginConfig 结构体

class CommModule {
public:
    CommModule();
    ~CommModule();

    // 核心接口：根据配置发送数据
    bool Send(const PluginConfig& config, const char* pData, int len);

    // 关闭所有连接
    void CloseAll();

private:
    // 内部连接管理
    bool EnsureSerial(const PluginConfig& config);
    bool EnsureSocket(const PluginConfig& config, int type); // 0:UDP, 1:TCP

    HANDLE m_hSerial;
    SOCKET m_socket;
    int m_lastProtocol; // 记录上次协议，如果用户切换了协议，我们需要重连
};

#endif