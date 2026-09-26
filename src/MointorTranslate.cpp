#include "MointorTranslate.h"
#include <windows.h>
#include <vector>
#include <string>

MointorTranslate MointorTranslate::m_instance;

MointorTranslate::MointorTranslate()
{
    m_config.protocol_type = 0; //default is uart
    memcpy(m_config.cfg.net.ip, "127.0.0.1", sizeof("127.0.0.1")); //default is uart
    m_config.cfg.net.port = 7890; //default is uart
}

IPluginItem* MointorTranslate::GetItem(int index)
{
    //No data return
    return nullptr;
}

ITMPlugin::OptionReturn MointorTranslate::ShowOptionsDialog(void* hParent){
    HMODULE hMod = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)this, &hMod);
    
    // 2. 直接调用封装好的 C 接口
    if (ShowConfigDialog(hMod, (HWND)hParent, &m_config)) {
        // 3. 用户点了 OK，把 config 里的值存回插件
        return OR_OPTION_CHANGED;
    }

    return OR_OPTION_UNCHANGED;
}

void MointorTranslate::DataRequired()
{
}

void MointorTranslate::OnMonitorInfo(const MonitorInfo& monitor_info){
    if (!m_comm.Send(m_config, (char *)&monitor_info, sizeof(MonitorInfo))) {
        
    }
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

