#include "MointorTranslate.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

static bool find_first_serial_port(char *port_name, size_t port_name_size)
{
    for (int port_number = 1; port_number <= 20; ++port_number) {
        char port_path[16];
        char candidate[16];
        sprintf_s(candidate, sizeof(candidate), "COM%d", port_number);
        sprintf_s(port_path, sizeof(port_path), "\\\\.\\%s", candidate);

        HANDLE handle = CreateFileA(port_path, GENERIC_READ | GENERIC_WRITE,
                                    0, NULL, OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE) continue;

        CloseHandle(handle);
        strncpy_s(port_name, port_name_size, candidate, _TRUNCATE);
        return true;
    }

    return false;
}

static std::string trim_value(const std::string& value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return std::string();

    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static bool parse_integer(const std::string& value, int& result)
{
    if (value.empty()) return false;

    char* end = NULL;
    const long parsed = strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' || parsed < 0 || parsed > 2147483647L) {
        return false;
    }

    result = (int)parsed;
    return true;
}

static bool parse_protocol(const std::string& value, int& protocol)
{
    if (parse_integer(value, protocol)) return true;

    if (_stricmp(value.c_str(), "UDP") == 0) {
        protocol = 0;
        return true;
    }
    if (_stricmp(value.c_str(), "TCP") == 0) {
        protocol = 1;
        return true;
    }
    if (_stricmp(value.c_str(), "UART") == 0 ||
        _stricmp(value.c_str(), "SERIAL") == 0) {
        protocol = 2;
        return true;
    }

    return false;
}

static bool read_config_file(const std::wstring& path, PluginConfig& config)
{
    FILE* file = NULL;
    if (path.empty() || _wfopen_s(&file, path.c_str(), L"rb") != 0 || file == NULL) {
        return false;
    }

    char buffer[4096];
    const size_t length = fread(buffer, 1, sizeof(buffer) - 1, file);
    const bool read_error = ferror(file) != 0;
    fclose(file);
    if (read_error) return false;
    buffer[length] = '\0';

    int protocol_type = 2;
    int network_port = 8080;
    int baud = 2000000;
    bool has_protocol_type = false;
    bool has_config_section = false;
    bool invalid = false;
    std::string ip;
    std::string port_name;
    const std::string text(buffer, length);

    size_t begin = 0;
    while (begin <= text.size()) {
        size_t end = text.find('\n', begin);
        if (end == std::string::npos) end = text.size();

        std::string line = trim_value(text.substr(begin, end - begin));
        if (begin == 0 && line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            line.erase(0, 3);
            line = trim_value(line);
        }

        if (!line.empty() && line[0] == '[' && line[line.size() - 1] == ']') {
            const std::string section = trim_value(line.substr(1, line.size() - 2));
            has_config_section = (_stricmp(section.c_str(), "CONFIG") == 0);
        } else if (has_config_section && !line.empty() &&
                   line[0] != ';' && line[0] != '#') {
            const size_t separator = line.find('=');
            if (separator == std::string::npos) {
                invalid = true;
            } else {
                const std::string key = trim_value(line.substr(0, separator));
                const std::string value = trim_value(line.substr(separator + 1));
                if (_stricmp(key.c_str(), "protocol") == 0 ||
                    _stricmp(key.c_str(), "protocol_type") == 0) {
                    has_protocol_type = parse_protocol(value, protocol_type);
                    if (!has_protocol_type) invalid = true;
                } else if (_stricmp(key.c_str(), "ip") == 0) {
                    ip = value;
                } else if (_stricmp(key.c_str(), "port") == 0) {
                    if (!parse_integer(value, network_port)) invalid = true;
                } else if (_stricmp(key.c_str(), "port_name") == 0) {
                    port_name = value;
                } else if (_stricmp(key.c_str(), "baud") == 0) {
                    if (!parse_integer(value, baud)) invalid = true;
                }
            }
        }

        if (end == text.size()) break;
        begin = end + 1;
    }

    if (invalid || !has_config_section || !has_protocol_type ||
        protocol_type < 0 || protocol_type > 2) {
        return false;
    }

    memset(&config, 0, sizeof(config));
    config.protocol_type = protocol_type;
    if (protocol_type == 2) {
        strncpy_s(config.cfg.uart.port_name, sizeof(config.cfg.uart.port_name),
                  port_name.c_str(), _TRUNCATE);
        config.cfg.uart.baud = baud > 0 ? baud : 2000000;
    } else {
        strncpy_s(config.cfg.net.ip, sizeof(config.cfg.net.ip),
                  ip.empty() ? "127.0.0.1" : ip.c_str(), _TRUNCATE);
        config.cfg.net.port = network_port > 0 ? network_port : 8080;
    }

    return true;
}

static bool write_config_file(const std::wstring& path, const PluginConfig& config)
{
    if (path.empty()) return false;

    const char* ip = "";
    const char* port_name = "";
    int network_port = 0;
    int baud = 0;
    if (config.protocol_type < 2) {
        ip = config.cfg.net.ip;
        network_port = config.cfg.net.port;
    } else {
        port_name = config.cfg.uart.port_name;
        baud = config.cfg.uart.baud;
    }

    FILE* file = NULL;
    if (_wfopen_s(&file, path.c_str(), L"wb") != 0 || file == NULL) {
        return false;
    }

    const int written = fprintf(file,
        "[CONFIG]\r\n"
        "protocol=%d\r\n"
        "ip=%s\r\n"
        "port=%d\r\n"
        "port_name=%s\r\n"
        "baud=%d\r\n",
        config.protocol_type, ip, network_port, port_name, baud);
    const int close_result = fclose(file);
    return written >= 0 && close_result == 0;
}

static std::wstring make_config_file_path(const wchar_t* config_dir)
{
    std::wstring path = config_dir != NULL ? config_dir : L"";
    while (!path.empty() && (path[path.size() - 1] == L'\\' || path[path.size() - 1] == L'/')) {
        path.erase(path.size() - 1);
    }
    if (!path.empty()) path += L'\\';
    path += L"translate.inf";
    return path;
}

static void put_le32(unsigned char *out, int &pos, unsigned int value)
{
    for (int i = 0; i < 4; ++i) out[pos++] = (unsigned char)(value >> (i * 8));
}

static void put_le64(unsigned char *out, int &pos, unsigned long long value)
{
    for (int i = 0; i < 8; ++i) out[pos++] = (unsigned char)(value >> (i * 8));
}

MointorTranslate MointorTranslate::m_instance;

MointorTranslate::MointorTranslate()
{
    memset(&m_config, 0, sizeof(m_config));
    m_config.protocol_type = 2; // default is UART for AiPi-Clock
    m_config.cfg.uart.baud = 2000000;
}

IPluginItem* MointorTranslate::GetItem(int index)
{
    //No data return
    return nullptr;
}

void MointorTranslate::LoadConfig(const wchar_t* config_dir)
{
    if (config_dir == NULL || config_dir[0] == L'\0') return;

    const std::wstring config_file = make_config_file_path(config_dir);
    if (config_file == m_config_file) return;
    m_config_file = config_file;

    PluginConfig saved_config;
    if (read_config_file(m_config_file, saved_config)) {
        m_config = saved_config;
        return;
    }

    memset(&m_config, 0, sizeof(m_config));
    m_config.protocol_type = 2;
    m_config.cfg.uart.baud = 2000000;
    find_first_serial_port(m_config.cfg.uart.port_name, sizeof(m_config.cfg.uart.port_name));
    write_config_file(m_config_file, m_config);
}

void MointorTranslate::OnInitialize(ITrafficMonitor* pApp)
{
    if (pApp != NULL) LoadConfig(pApp->GetPluginConfigDir());
}

void MointorTranslate::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    if (index == EI_CONFIG_DIR) LoadConfig(data);
}

ITMPlugin::OptionReturn MointorTranslate::ShowOptionsDialog(void* hParent){
    HMODULE hMod = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)this, &hMod);
    
    // 2. 直接调用封装好的 C 接口
    if (ShowConfigDialog(hMod, (HWND)hParent, &m_config)) {
        // 3. 用户点了 OK，把 config 里的值存回插件
        write_config_file(m_config_file, m_config);
        return OR_OPTION_CHANGED;
    }

    return OR_OPTION_UNCHANGED;
}

void MointorTranslate::DataRequired()
{
}

void MointorTranslate::OnMonitorInfo(const MonitorInfo& monitor_info){
    /* Clock firmware shares this UART with logs/xcmd.  Send an ASCII framed
     * xcmd extension instead of the old raw struct, so it can be delimited and
     * safely ignored by older firmware. Payload is 52 bytes, little endian:
     * QQ + nine int32 values. */
    static const char hex[] = "0123456789abcdef";
    unsigned char raw[52];
    char frame[116];
    int pos = 0;
    put_le64(raw, pos, monitor_info.up_speed);
    put_le64(raw, pos, monitor_info.down_speed);
    put_le32(raw, pos, (unsigned int)monitor_info.cpu_usage);
    put_le32(raw, pos, (unsigned int)monitor_info.memory_usage);
    put_le32(raw, pos, (unsigned int)monitor_info.gpu_usage);
    put_le32(raw, pos, (unsigned int)monitor_info.hdd_usage);
    put_le32(raw, pos, (unsigned int)monitor_info.cpu_temperature);
    put_le32(raw, pos, (unsigned int)monitor_info.gpu_temperature);
    put_le32(raw, pos, (unsigned int)monitor_info.hdd_temperature);
    put_le32(raw, pos, (unsigned int)monitor_info.main_board_temperature);
    put_le32(raw, pos, (unsigned int)monitor_info.cpu_freq);
    memcpy(frame, "#XMONDATA,", 10);
    for (int i = 0; i < 52; ++i) {
        frame[10 + i * 2] = hex[raw[i] >> 4];
        frame[11 + i * 2] = hex[raw[i] & 0x0f];
    }
    frame[114] = '\r';
    frame[115] = '\n';
    m_comm.Send(m_config, frame, sizeof(frame));
}

const wchar_t* MointorTranslate::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"MointorTranslate";
    case TMI_DESCRIPTION:
        return L"转换monitor信息到tcp/udp/uart";
    case TMI_AUTHOR:
        return L"xingleigao";
    case TMI_COPYRIGHT:
        return L"Copyright (C) by Xinglei Gao 2025";
    case TMI_VERSION:
        return L"1.0";
    case ITMPlugin::TMI_URL:
        return L"https://github.com/xingleigao/MointorTranslate";
        break;
    default:
        break;
    }
    return L"";
}

MointorTranslate& MointorTranslate::Instance()
{
    return m_instance;
}

ITMPlugin* TMPluginGetInstance()
{
    return &MointorTranslate::Instance();
}

