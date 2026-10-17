#include "OptionsDialog.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>

static void ScanPorts(HWND hCombo) {
    for (int i = 1; i <= 20; i++) {
        char name[10];
        sprintf(name, "COM%d", i);
        char path[20];
        sprintf(path, "\\\\.\\%s", name);
        HANDLE hFile = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)name);
            CloseHandle(hFile);
        }
    }
    if (SendMessage(hCombo, CB_GETCOUNT, 0, 0) > 0) 
        SendMessage(hCombo, CB_SETCURSEL, 0, 0);
}

// 内部函数：根据选择切换控件显示
static void UpdateUI(HWND hDlg, int protocolIndex) {
    // 0:UDP, 1:TCP -> Network mode
    // 2:UART      -> Serial mode
    int isNetwork = (protocolIndex == 0 || protocolIndex == 1);

    // 切换槽位 1
    ShowWindow(GetDlgItem(hDlg, IDC_EDIT_IP), isNetwork ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_COMBO_COM), isNetwork ? SW_HIDE : SW_SHOW);
    SetDlgItemTextA(hDlg, IDC_STATIC_LABEL_1, isNetwork ? "IP Address:" : "COM Port:");

    // 切换槽位 2
    ShowWindow(GetDlgItem(hDlg, IDC_EDIT_PORT), isNetwork ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_COMBO_BAUD), isNetwork ? SW_HIDE : SW_SHOW);
    SetDlgItemTextA(hDlg, IDC_STATIC_LABEL_2, isNetwork ? "Port:" : "Baud Rate:");
}

static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static PluginConfig* pCfg = NULL;

    switch (message) {
    case WM_INITDIALOG: {
        pCfg = (PluginConfig*)lParam;

        // --- 1. 初始化协议类型 ---
        HWND hType = GetDlgItem(hDlg, IDC_COMBO_TYPE);
        SendMessageA(hType, CB_ADDSTRING, 0, (LPARAM)"UDP");
        SendMessageA(hType, CB_ADDSTRING, 0, (LPARAM)"TCP");
        SendMessageA(hType, CB_ADDSTRING, 0, (LPARAM)"UART");
        SendMessage(hType, CB_SETCURSEL, pCfg->protocol_type, 0);

        // --- 2. 初始化波特率 ---
        HWND hBaud = GetDlgItem(hDlg, IDC_COMBO_BAUD);
        const char* bauds[] = {"9600", "19200", "38400", "57600", "115200", "2000000"};
        for(int i=0; i<6; i++) SendMessageA(hBaud, CB_ADDSTRING, 0, (LPARAM)bauds[i]);
        
        // 如果已有波特率则显示，否则默认 115200
        if (pCfg->protocol_type == 2 && pCfg->cfg.uart.baud > 0) {
            char bBuf[16];
            sprintf(bBuf, "%d", pCfg->cfg.uart.baud);
            int index = (int)SendMessageA(hBaud, CB_FINDSTRINGEXACT, -1, (LPARAM)bBuf);
            if (index != CB_ERR) {
                SendMessageA(hBaud, CB_SETCURSEL, (WPARAM)index, 0);
            }
        } else {
            SendMessage(hBaud, CB_SETCURSEL, 5, 0); 
        }

        
        // 如果之前存过串口名，尝试选中它
        if (pCfg->protocol_type == 2 && pCfg->cfg.uart.port_name[0] != '\0') {
            SendMessage(GetDlgItem(hDlg, IDC_COMBO_COM), CB_RESETCONTENT, 0, 0);
            SendMessageA(GetDlgItem(hDlg, IDC_COMBO_COM), CB_ADDSTRING, 0, (LPARAM)pCfg->cfg.uart.port_name);
            SendMessageA(GetDlgItem(hDlg, IDC_COMBO_COM), CB_SELECTSTRING, -1, (LPARAM)pCfg->cfg.uart.port_name);
        }else{
            SendMessage(GetDlgItem(hDlg, IDC_COMBO_COM), CB_RESETCONTENT, 0, 0);
        }
        // --- 3. 初始化串口名 ---
        ScanPorts(GetDlgItem(hDlg, IDC_COMBO_COM));

        // --- 4. 网络信息回填 ---
        // 判断 IP 是否为空，不为空则填入
        if (pCfg->protocol_type < 2 && pCfg->cfg.net.ip[0] != '\0') {
            SetDlgItemTextA(hDlg, IDC_EDIT_IP, pCfg->cfg.net.ip);
        } else {
            SetDlgItemTextA(hDlg, IDC_EDIT_IP, "127.0.0.1"); // 默认值
        }

        // 判断 Port 是否有效
        if (pCfg->protocol_type < 2 &&pCfg->cfg.net.port > 0) {
            SetDlgItemInt(hDlg, IDC_EDIT_PORT, pCfg->cfg.net.port, FALSE);
        } else {
            SetDlgItemInt(hDlg, IDC_EDIT_PORT, 8080, FALSE); // 默认值
        }

        // --- 5. 同步 UI 状态 ---
        UpdateUI(hDlg, pCfg->protocol_type);

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);

        if (id == IDC_COMBO_TYPE && event == CBN_SELCHANGE) {
            int idx = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
            UpdateUI(hDlg, idx);
        }

        if (id == IDOK) {
            pCfg->protocol_type = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_TYPE), CB_GETCURSEL, 0, 0);
            
            if (pCfg->protocol_type == 2) { // UART
                GetDlgItemTextA(hDlg, IDC_COMBO_COM, pCfg->cfg.uart.port_name, 16);
                char bStr[16];
                GetDlgItemTextA(hDlg, IDC_COMBO_BAUD, bStr, 16);
                pCfg->cfg.uart.baud = atoi(bStr);
            } else { // Network
                GetDlgItemTextA(hDlg, IDC_EDIT_IP, pCfg->cfg.net.ip, 32);
                pCfg->cfg.net.port = GetDlgItemInt(hDlg, IDC_EDIT_PORT, NULL, FALSE);
            }
            EndDialog(hDlg, 1);
            return (INT_PTR)TRUE;
        }

        if (id == IDCANCEL) {
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}

int ShowConfigDialog(HINSTANCE hInst, HWND hParent, PluginConfig* config) {
    return (int)DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_OPTIONS_DLG), hParent, DlgProc, (LPARAM)config);
}
