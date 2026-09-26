#include "CommModule.hpp"
#include "PluginInterface.h"
#include <wchar.h>
#include "OptionsDialog.h"

class MointorTranslate : public ITMPlugin
{
private:
    MointorTranslate();

public:
    static MointorTranslate& Instance();

    // 通过 ITMPlugin 继承
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual void OnMonitorInfo(const MonitorInfo& monitor_info) override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;

private:
    CommModule m_comm;     // 刚才写的模块
    PluginConfig m_config;
    static MointorTranslate m_instance;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif