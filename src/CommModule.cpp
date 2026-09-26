#include "CommModule.hpp"
#include <stdio.h>

CommModule::CommModule() : m_hSerial(INVALID_HANDLE_VALUE), m_socket(INVALID_SOCKET), m_lastProtocol(-1) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData); // 初始化网络环境
}

CommModule::~CommModule() {
    CloseAll();
    WSACleanup();
}

void CommModule::CloseAll() {
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
    }
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool CommModule::EnsureSerial(const PluginConfig& config) {
    if (m_hSerial != INVALID_HANDLE_VALUE) return true;

    char path[32];
    sprintf(path, "\\\\.\\%s", config.cfg.uart.port_name);
    m_hSerial = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (m_hSerial == INVALID_HANDLE_VALUE) return false;

    DCB dcb = { sizeof(DCB) };
    if (GetCommState(m_hSerial, &dcb)) {
        dcb.BaudRate = config.cfg.uart.baud;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity = NOPARITY;
        SetCommState(m_hSerial, &dcb);
    }
    return true;
}

bool CommModule::EnsureSocket(const PluginConfig& config, int type) {
    if (m_socket != INVALID_SOCKET) return true;

    m_socket = socket(AF_INET, (type == 1 ? SOCK_STREAM : SOCK_DGRAM), 0);
    if (m_socket == INVALID_SOCKET) return false;

    if (type == 1) { // TCP 需要连接
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(config.cfg.net.port);
        addr.sin_addr.s_addr = inet_addr(config.cfg.net.ip);
        if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
    }
    return true;
}

bool CommModule::Send(const PluginConfig& config, const char* pData, int len) {
    // 1. 如果协议变了，强制关闭之前的连接
    if (m_lastProtocol != config.protocol_type) {
        CloseAll();
        m_lastProtocol = config.protocol_type;
    }

    // 2. 分发执行
    switch (config.protocol_type) {
        case 0: // UDP
            if (!EnsureSocket(config, 0)) return false;
            sockaddr_in udpAddr;
            udpAddr.sin_family = AF_INET;
            udpAddr.sin_port = htons(config.cfg.net.port);
            udpAddr.sin_addr.s_addr = inet_addr(config.cfg.net.ip);
            return sendto(m_socket, pData, len, 0, (sockaddr*)&udpAddr, sizeof(udpAddr)) != SOCKET_ERROR;

        case 1: // TCP
            if (!EnsureSocket(config, 1)) return false;
            if (send(m_socket, pData, len, 0) == SOCKET_ERROR) {
                CloseAll(); // 发送失败尝试重连
                return false;
            }
            return true;

        case 2: // UART
            if (!EnsureSerial(config)) return false;
            DWORD written;
            return WriteFile(m_hSerial, pData, len, &written, NULL) && (written == (DWORD)len);
    }
    return false;
}