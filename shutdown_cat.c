#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#include <uxtheme.h>
#include <shellapi.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shell32.lib")

// 控件ID定义
#define ID_TIMER 1001
#define ID_CHECK_SCHEDULED 2001
#define ID_CHECK_COUNTDOWN 2002
#define ID_EDIT_HOUR 2003
#define ID_EDIT_MINUTE 2004
#define ID_EDIT_HOURS 2005
#define ID_EDIT_MINUTES 2006
#define ID_BUTTON_START 2007
#define ID_BUTTON_CANCEL 2008
#define ID_BUTTON_HELP 2009
#define ID_TRAY_ICON 3001
#define ID_TRAY_OPEN 3002
#define ID_TRAY_CANCEL 3003
#define ID_TRAY_EXIT 3004
#define ID_DESKTOP_TIMER 2004
#define WM_TRAYICON (WM_USER + 1)
#define IDI_MAIN_ICON 1
#define IDI_TRAY_ICON 2
#define ID_HELP_DIALOG 4001
#define ID_HELP_OK 4002

// UpDown控件ID
#define ID_UPDOWN_HOUR 5001
#define ID_UPDOWN_MINUTE 5002
#define ID_UPDOWN_HOURS 5003
#define ID_UPDOWN_MINUTES 5004

// 现代Windows风格颜色定义
#define COLOR_BG            RGB(240, 240, 240)  // #f0f0f0
#define COLOR_TITLEBAR      RGB(0, 120, 212)    // #0078d4
#define COLOR_WHITE         RGB(255, 255, 255)  // #ffffff
#define COLOR_BORDER        RGB(209, 209, 209)  // #d1d1d1
#define COLOR_SEPARATOR     RGB(225, 223, 221)  // #e1dfdd
#define COLOR_TEXT          RGB(50, 49, 48)     // #323130
#define COLOR_TEXT_LIGHT    RGB(96, 94, 92)     // #605e5c
#define COLOR_PRIMARY       RGB(0, 120, 212)    // #0078d4
#define COLOR_PRIMARY_HOVER RGB(16, 110, 190)   // #106ebe
#define COLOR_SECONDARY     RGB(243, 242, 241)  // #f3f2f1

#define COLOR_TIME_DISPLAY  RGB(0, 0, 0)        // 黑色背景
#define COLOR_TIME_TEXT     RGB(0, 255, 0)      // 绿色文字

static const wchar_t HELP_TEXT[] = 
    L"关机猫,一个简单好用的定时关机程序\n"
    L"支持Windows XP及以上版本\n\n"
    L"【定时关机】\n"
    L"• 设定具体时间点关机（如22:30），可跨日\n\n"
    L"【倒计时关机】\n"
    L"• 设定多少时间后关机（如1小时30分）\n\n"
    L"【桌面倒计时】\n"
    L"• 定时开始后桌面右下角显示半透明倒计时\n"
    L"• 可拖拽移动位置，右键显示菜单\n\n"
    L"【系统托盘】\n"
    L"• 定时开始后自动最小化到托盘\n"
    L"• 左键托盘图标打开主界面\n"
    L"• 右键托盘图标显示快捷菜单\n\n"
    L"注意：\n"
    L"• 停止计时才能关闭程序\n"
    L"• 系统会在最后60秒显示关机倒计时提示框\n"
    L"• 建议不要超过24小时，程序会自动验证输入范围\n\n"
    L"版本：V2.0 作者：喵言喵语 by.52pojie";

// 全局变量
HWND hWnd, hCheckScheduled, hCheckCountdown, hEditHour, hEditMinute, hEditHours, hEditMinutes, hButtonStart, hButtonCancel, hButtonHelp, hTimeDisplay, hDesktopTimer = NULL;
HWND hUpDownHour, hUpDownMinute, hUpDownHours, hUpDownMinutes;
WNDPROC OriginalEditProc;
BOOL useScheduled = FALSE, useCountdown = TRUE, isTimerActive = FALSE, isInTray = FALSE, showDesktopTimer = FALSE;
time_t shutdownTime = 0;
NOTIFYICONDATA nid;
HFONT hFont, hTimeFont, hDesktopFont = NULL;

// 窗口过程函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 函数声明
void UpdateControlStates();
void CreateTrayIcon();
void RemoveTrayIcon();
void UpdateTrayTooltip();
void ShowFromTray();
void HideToTray();
void ShowTrayMenu();
void CreateDesktopTimer();
void DestroyDesktopTimer();
void UpdateDesktopTimer(int totalSeconds);
LRESULT CALLBACK DesktopTimerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 数值输入框增减函数
void AdjustEditValue(HWND hEdit, int delta, int minVal, int maxVal);
void HandleMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


// 创建系统默认字体
HFONT CreateSystemFont(int size) {
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    
    ncm.lfMessageFont.lfWeight = FW_NORMAL;  // 统一使用普通字重
    ncm.lfMessageFont.lfHeight = -MulDiv(size, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    
    return CreateFontIndirect(&ncm.lfMessageFont);
}

// 创建等宽字体用于时间显示
HFONT CreateMonospaceFont(int size) {
    return CreateFont(
        -MulDiv(size, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas"
    );
}

// 设置控件字体
void SetControlFont(HWND hwnd, HFONT font) {
    SendMessage(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
}

// 自定义按钮绘制
void DrawModernButton(HDC hdc, RECT rect, LPCWSTR text, BOOL isPressed, BOOL isHovered, BOOL isPrimary, BOOL isEnabled) {
    // 选择颜色
    COLORREF bgColor, borderColor, textColor;
    BOOL shouldShowRaised = FALSE;  // 是否显示凸起效果
    
    // 按照用户要求的状态逻辑
    if (isPrimary) {
        // 定时开始按钮
        if (!isTimerActive && isEnabled) {
            // 状态1：尚未开始关机倒计时时 - 凸起蓝色可点击
            bgColor = isHovered ? COLOR_PRIMARY_HOVER : COLOR_PRIMARY;
            borderColor = COLOR_PRIMARY;
            textColor = RGB(255, 255, 255);
            shouldShowRaised = TRUE;
        } else {
            // 状态2：关机倒计时进行中时 - 按下灰色无法点击
            bgColor = RGB(180, 180, 180);
            borderColor = RGB(160, 160, 160);
            textColor = RGB(120, 120, 120);
            shouldShowRaised = FALSE;
        }
    } else {
        // 取消关机按钮
        if (!isTimerActive) {
            // 状态1：尚未开始关机倒计时时 - 按下灰色无法点击
            bgColor = RGB(180, 180, 180);
            borderColor = RGB(160, 160, 160);
            textColor = RGB(120, 120, 120);
            shouldShowRaised = FALSE;
        } else {
            // 状态2：关机倒计时进行中时 - 凸起蓝色可点击
            bgColor = isHovered ? COLOR_PRIMARY_HOVER : COLOR_PRIMARY;
            borderColor = COLOR_PRIMARY;
            textColor = RGB(255, 255, 255);
            shouldShowRaised = TRUE;
        }
    }
    
    // 鼠标按下效果（只在可点击时生效）
    if (isPressed && isEnabled) {
        bgColor = RGB(0, 86, 153);
    }
    
    // 填充背景
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
    
    // 绘制3D效果边框
    HPEN hLightPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HPEN hDarkPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, borderColor);
    
    if (shouldShowRaised) {
        // 凸起效果
        SelectObject(hdc, hLightPen);
        MoveToEx(hdc, rect.left, rect.bottom - 1, NULL);
        LineTo(hdc, rect.left, rect.top);
        LineTo(hdc, rect.right - 1, rect.top);
        
        SelectObject(hdc, hDarkPen);
        MoveToEx(hdc, rect.right - 1, rect.top, NULL);
        LineTo(hdc, rect.right - 1, rect.bottom - 1);
        LineTo(hdc, rect.left, rect.bottom - 1);
    } else {
        // 按下效果或平面效果
        SelectObject(hdc, hDarkPen);
        MoveToEx(hdc, rect.left, rect.bottom - 1, NULL);
        LineTo(hdc, rect.left, rect.top);
        LineTo(hdc, rect.right - 1, rect.top);
        
        SelectObject(hdc, hLightPen);
        MoveToEx(hdc, rect.right - 1, rect.top, NULL);
        LineTo(hdc, rect.right - 1, rect.bottom - 1);
        LineTo(hdc, rect.left, rect.bottom - 1);
    }
    
    DeleteObject(hLightPen);
    DeleteObject(hDarkPen);
    DeleteObject(hBorderPen);
    
    // 绘制文字
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    SelectObject(hdc, hFont);
    DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void AdjustEditValue(HWND hEdit, int delta, int minVal, int maxVal) {
    WCHAR buf[10];
    GetWindowText(hEdit, buf, 10);
    int val = _wtoi(buf) + delta;
    if (val < minVal) val = minVal;
    if (val > maxVal) val = maxVal;
    swprintf(buf, 10, L"%d", val);
    SetWindowText(hEdit, buf);
}

void HandleMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    ScreenToClient(hWnd, &pt);
    HWND hChild = ChildWindowFromPoint(hWnd, pt);
    if (!hChild) return;
    int delta = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : -1;
    if (hChild == hEditHour) AdjustEditValue(hEditHour, delta, 0, 23);
    else if (hChild == hEditMinute) AdjustEditValue(hEditMinute, delta, 0, 59);
    else if (hChild == hEditHours) AdjustEditValue(hEditHours, delta, 0, 23);
    else if (hChild == hEditMinutes) AdjustEditValue(hEditMinutes, delta, 0, 59);
}

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_MOUSEWHEEL) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : -1;
        if (hwnd == hEditHour) AdjustEditValue(hEditHour, delta, 0, 23);
        else if (hwnd == hEditMinute) AdjustEditValue(hEditMinute, delta, 0, 59);
        else if (hwnd == hEditHours) AdjustEditValue(hEditHours, delta, 0, 23);
        else if (hwnd == hEditMinutes) AdjustEditValue(hEditMinutes, delta, 0, 59);
        return 0;
    }
    return CallWindowProc(OriginalEditProc, hwnd, uMsg, wParam, lParam);
}

// 简洁的浮动帮助按钮
void DrawFloatingHelp(HDC hdc, RECT rect, BOOL isPressed, BOOL isHovered) {
    // 设置透明背景
    SetBkMode(hdc, TRANSPARENT);
    
    // 轻微的阴影效果
    if (!isPressed) {
        RECT shadowRect = {rect.left + 1, rect.top + 1, rect.right + 1, rect.bottom + 1};
        SetTextColor(hdc, RGB(200, 200, 200));
        HFONT shadowFont = CreateFont(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");
        HFONT oldFont = (HFONT)SelectObject(hdc, shadowFont);
        DrawText(hdc, L"?", -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        DeleteObject(shadowFont);
    }
    
    // 主要的问号 - 只有按下效果，无悬停变色
    COLORREF textColor;
    if (isPressed) {
        textColor = RGB(100, 100, 100);  // 按下时变暗
    } else {
        textColor = RGB(60, 60, 60);     // 默认深灰色
    }
    
    SetTextColor(hdc, textColor);
    HFONT helpFont = CreateFont(-14, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hdc, helpFont);
    
    DrawText(hdc, L"?", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, oldFont);
    DeleteObject(helpFont);
}



// 执行关机命令
void ExecuteShutdown(int seconds) {
    WCHAR command[200];
    if (swprintf(command, 200, L"shutdown /s /t %d", seconds) > 0) {
        _wsystem(command);
    }
}

// 取消关机命令
void CancelShutdownTimer() {
    if (isTimerActive) {
        _wsystem(L"shutdown /a");
        KillTimer(hWnd, ID_TIMER);
        isTimerActive = FALSE;
        shutdownTime = 0;
        SetWindowText(hTimeDisplay, L"--:--:--");
        if (hDesktopTimer) {
            DestroyWindow(hDesktopTimer);
            hDesktopTimer = NULL;
        }
        UpdateControlStates();
        if (isInTray) {
            wcscpy(nid.szTip, L"关机猫");
            Shell_NotifyIcon(NIM_MODIFY, &nid);
        }
    }
}

// 更新时间显示
void UpdateTimeDisplay(int seconds) {
    WCHAR timeStr[20];
    swprintf(timeStr, 20, L"%02d:%02d:%02d", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
    SetWindowText(hTimeDisplay, timeStr);
}

// 开始定时关机
void StartScheduledShutdown() {
    WCHAR buffer[10];
    int targetHour = 0, targetMinute = 0;
    
    GetWindowText(hEditHour, buffer, 10);
    targetHour = _wtoi(buffer);
    
    GetWindowText(hEditMinute, buffer, 10);
    targetMinute = _wtoi(buffer);
    
    if (targetHour > 23 || targetMinute > 59) {
        MessageBox(hWnd, L"请输入有效的时间（小时: 0-23, 分钟: 0-59）！", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 获取当前时间
    time_t now;
    struct tm *current_time;
    time(&now);
    current_time = localtime(&now);
    
    // 计算目标时间
    struct tm target_time = *current_time;
    target_time.tm_hour = targetHour;
    target_time.tm_min = targetMinute;
    target_time.tm_sec = 0;
    
    time_t target_timestamp = mktime(&target_time);
    
    // 如果目标时间已过，设置为明天
    if (target_timestamp <= now) {
        target_time.tm_mday += 1;
        target_timestamp = mktime(&target_time);
    }
    
    int secondsUntilShutdown = (int)(target_timestamp - now);
    
    ExecuteShutdown(secondsUntilShutdown);
    UpdateTimeDisplay(secondsUntilShutdown);
    
    shutdownTime = target_timestamp;  // 保存关机时间戳
    isTimerActive = TRUE;
    SetTimer(hWnd, ID_TIMER, 1000, NULL);  // 每秒更新一次
    CreateDesktopTimer();  // 创建桌面倒计时窗口
    UpdateControlStates();  // 更新控件状态
    InvalidateRect(hWnd, NULL, TRUE);
    
    // 自动最小化到托盘
    HideToTray();
}

// 开始倒计时关机
void StartCountdownShutdown() {
    WCHAR buffer[10];
    int hours = 0, minutes = 0;
    
    GetWindowText(hEditHours, buffer, 10);
    hours = _wtoi(buffer);
    
    GetWindowText(hEditMinutes, buffer, 10);
    minutes = _wtoi(buffer);
    
    int totalSeconds = hours * 3600 + minutes * 60;
    
    if (totalSeconds <= 0) {
        MessageBox(hWnd, L"请输入有效的时间！", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    ExecuteShutdown(totalSeconds);
    UpdateTimeDisplay(totalSeconds);
    
    time_t now;
    time(&now);
    shutdownTime = now + totalSeconds;  // 保存关机时间戳
    isTimerActive = TRUE;
    SetTimer(hWnd, ID_TIMER, 1000, NULL);  // 每秒更新一次
    CreateDesktopTimer();  // 创建桌面倒计时窗口
    UpdateControlStates();  // 更新控件状态
    InvalidateRect(hWnd, NULL, TRUE);
    
    // 自动最小化到托盘
    HideToTray();
}

// 更新控件状态
void UpdateControlStates() {
    EnableWindow(hEditHour, useScheduled && !isTimerActive);
    EnableWindow(hEditMinute, useScheduled && !isTimerActive);
    EnableWindow(hEditHours, useCountdown && !isTimerActive);
    EnableWindow(hEditMinutes, useCountdown && !isTimerActive);
    EnableWindow(hCheckScheduled, !isTimerActive);
    EnableWindow(hCheckCountdown, !isTimerActive);
    EnableWindow(hButtonStart, (useScheduled || useCountdown) && !isTimerActive);
    EnableWindow(hButtonCancel, isTimerActive);
}

// 创建托盘图标
void CreateTrayIcon() {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TRAY_ICON), 
                                IMAGE_ICON, 16, 16, LR_SHARED);
    if (!nid.hIcon) nid.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON), 
                                                IMAGE_ICON, 16, 16, LR_SHARED);
    if (!nid.hIcon) nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy(nid.szTip, L"关机猫");
    
    if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
        MessageBox(hWnd, L"托盘图标创建失败", L"关机猫", MB_OK | MB_ICONWARNING);
    }
}

// 移除托盘图标
void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// 更新托盘提示信息
void UpdateTrayTooltip() {
    if (!isTimerActive || !isInTray) return;
    
    time_t now;
    time(&now);
    
    if (shutdownTime > now) {
        int remainingSeconds = (int)(shutdownTime - now);
        int hours = remainingSeconds / 3600;
        int minutes = (remainingSeconds % 3600) / 60;
        int seconds = remainingSeconds % 60;
        
        swprintf(nid.szTip, 128, L"关机猫 - 剩余: %02d:%02d:%02d", hours, minutes, seconds);
        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
}

// 从托盘显示窗口
void ShowFromTray() {
    if (isInTray) {
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        RemoveTrayIcon();
        isInTray = FALSE;
    }
}

// 隐藏到托盘
void HideToTray() {
    if (!isInTray) {
        CreateTrayIcon();  // 先创建托盘图标
        ShowWindow(hWnd, SW_HIDE);  // 再隐藏窗口
        isInTray = TRUE;
    }
}

// 显示托盘右键菜单
void ShowTrayMenu() {
    HMENU hMenu = CreatePopupMenu();
    POINT pt;
    
    // 添加菜单项
    AppendMenu(hMenu, MF_STRING, ID_TRAY_OPEN, L"打开主界面");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    
    if (isTimerActive) {
        AppendMenu(hMenu, MF_STRING, ID_TRAY_CANCEL, L"取消关机");
        AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"取消并退出");
    } else {
        AppendMenu(hMenu, MF_STRING | MF_GRAYED, ID_TRAY_CANCEL, L"取消关机");
        AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出");
    }
    
    // 获取鼠标位置
    GetCursorPos(&pt);
    
    // 设置前台窗口，这样菜单才能正常工作
    SetForegroundWindow(hWnd);
    
    // 显示菜单
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    
    // 清理菜单
    DestroyMenu(hMenu);
}

// 创建桌面倒计时窗口
void CreateDesktopTimer() {
    if (hDesktopTimer) return;
    
    // 获取屏幕尺寸
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // 获取任务栏高度
    APPBARDATA abd = {0};
    abd.cbSize = sizeof(APPBARDATA);
    SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    int taskbarHeight = abd.rc.bottom - abd.rc.top;
    
    // 计算窗口位置（右下角，距离边缘20像素）
    int windowWidth = 180;
    int windowHeight = 60;
    int posX = screenWidth - windowWidth - 20;
    int posY = screenHeight - windowHeight - taskbarHeight - 20;
    
    // 创建桌面倒计时窗口
    hDesktopTimer = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,  // 置顶，不在任务栏显示，支持透明
        L"DesktopTimer",
        L"倒计时",
        WS_POPUP | WS_VISIBLE,
        posX, posY, windowWidth, windowHeight,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (hDesktopTimer) {
        // 设置半透明效果（40%不透明，更透明）
        SetLayeredWindowAttributes(hDesktopTimer, 0, 100, LWA_ALPHA);
        
        // 创建字体
        hDesktopFont = CreateFont(
            -MulDiv(24, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
            0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas"
        );
        
        showDesktopTimer = TRUE;
        ShowWindow(hDesktopTimer, SW_SHOW);
        // 确保窗口置顶
        SetWindowPos(hDesktopTimer, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        UpdateWindow(hDesktopTimer);
    }
}

// 销毁桌面倒计时窗口
void DestroyDesktopTimer() {
    if (hDesktopTimer) {
        DestroyWindow(hDesktopTimer);
        hDesktopTimer = NULL;
        showDesktopTimer = FALSE;
    }
    if (hDesktopFont) {
        DeleteObject(hDesktopFont);
        hDesktopFont = NULL;
    }
}

// 更新桌面倒计时显示
void UpdateDesktopTimer(int totalSeconds) {
    if (hDesktopTimer) {
        // 每10秒检查一次置顶状态，避免过于频繁
        static int lastCheck = 0;
        if (totalSeconds % 10 == 0 && totalSeconds != lastCheck) {
            SetWindowPos(hDesktopTimer, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            lastCheck = totalSeconds;
        }
        
        // 强制重绘窗口
        InvalidateRect(hDesktopTimer, NULL, TRUE);
    }
}

// 桌面倒计时窗口过程
LRESULT CALLBACK DesktopTimerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static BOOL dragging = FALSE;
    static POINT offset;
    
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // 填充黑色背景（整个窗口）
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);
            
            // 计算并显示倒计时
            if (isTimerActive && shutdownTime > 0) {
                time_t now;
                time(&now);
                
                if (shutdownTime > now) {
                    int remainingSeconds = (int)(shutdownTime - now);
                    int hours = remainingSeconds / 3600;
                    int minutes = (remainingSeconds % 3600) / 60;
                    int seconds = remainingSeconds % 60;
                    
                    WCHAR timeText[32];
                    swprintf(timeText, 32, L"%02d:%02d:%02d", hours, minutes, seconds);
                    
                    // 设置文字属性
                    SetTextColor(hdc, RGB(0, 255, 0));  // 绿色
                    SetBkMode(hdc, TRANSPARENT);
                    SelectObject(hdc, hDesktopFont);
                    
                    // 居中绘制文字
                    DrawText(hdc, timeText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            // 开始拖拽
            dragging = TRUE;
            SetCapture(hwnd);
            
            POINT pt;
            GetCursorPos(&pt);
            RECT rect;
            GetWindowRect(hwnd, &rect);
            
            offset.x = pt.x - rect.left;
            offset.y = pt.y - rect.top;
            return 0;
        }
        
        case WM_LBUTTONUP: {
            // 结束拖拽
            if (dragging) {
                dragging = FALSE;
                ReleaseCapture();
            }
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            // 拖拽移动
            if (dragging) {
                POINT pt;
                GetCursorPos(&pt);
                SetWindowPos(hwnd, NULL, 
                    pt.x - offset.x, 
                    pt.y - offset.y, 
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            return 0;
        }
        
        case WM_RBUTTONUP: {
            // 右键显示菜单（临时降低窗口优先级，避免菜单被遮挡）
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            ShowTrayMenu();
            // 菜单关闭后恢复置顶
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            return 0;
        }
        
        case WM_CLOSE:
            // 桌面倒计时窗口不应该被关闭，忽略此消息
            return 0;
            
        case WM_DESTROY:
            showDesktopTimer = FALSE;
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 自定义窗口过程 - 用于按钮
LRESULT CALLBACK CustomButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static BOOL isHovered = FALSE;
    static BOOL isPressed = FALSE;
    
    switch (uMsg) {
        case WM_MOUSEMOVE:
            if (!isHovered) {
                isHovered = TRUE;
                InvalidateRect(hwnd, NULL, TRUE);
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }
            break;
            
        case WM_MOUSELEAVE:
            isHovered = FALSE;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
            
        case WM_LBUTTONDOWN:
            isPressed = TRUE;
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
            
        case WM_LBUTTONUP:
            if (isPressed) {
                isPressed = FALSE;
                ReleaseCapture();
                InvalidateRect(hwnd, NULL, TRUE);
                SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), BN_CLICKED), (LPARAM)hwnd);
            }
            break;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            WCHAR text[100];
            GetWindowText(hwnd, text, 100);
            
            BOOL isPrimary = (GetDlgCtrlID(hwnd) == ID_BUTTON_START);
            BOOL isEnabled = IsWindowEnabled(hwnd);
            DrawModernButton(hdc, rect, text, isPressed, isHovered, isPrimary, isEnabled);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // 创建字体 - 统一大小
            hFont = CreateSystemFont(9);
            hTimeFont = CreateMonospaceFont(16);
            
            // 主卡片区域背景
            CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_WHITERECT,
                        0, 0, 360, 300, hwnd, NULL, NULL, NULL);
            
            // 定时关机设置
            hCheckScheduled = CreateWindow(L"BUTTON", L"定时关机", 
                                         WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
                                         15, 20, 80, 20, hwnd, (HMENU)ID_CHECK_SCHEDULED, NULL, NULL);
            SetControlFont(hCheckScheduled, hFont);
            
            // 定时关机时间输入
            hEditHour = CreateWindow(L"EDIT", L"22", 
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
                                   35, 45, 55, 22, hwnd, (HMENU)ID_EDIT_HOUR, NULL, NULL);
            SetControlFont(hEditHour, hFont);
            
            hUpDownHour = CreateWindow(UPDOWN_CLASS, NULL,
                                     WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                     0, 0, 0, 0, hwnd, (HMENU)ID_UPDOWN_HOUR, NULL, NULL);
            SendMessage(hUpDownHour, UDM_SETBUDDY, (WPARAM)hEditHour, 0);
            SendMessage(hUpDownHour, UDM_SETRANGE, 0, MAKELPARAM(23, 0));
            SendMessage(hUpDownHour, UDM_SETPOS, 0, 22);
            
            CreateWindow(L"STATIC", L":", WS_VISIBLE | WS_CHILD | SS_CENTER,
                        101, 47, 10, 18, hwnd, NULL, NULL, NULL);
            
            hEditMinute = CreateWindow(L"EDIT", L"00", 
                                     WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
                                     118, 45, 55, 22, hwnd, (HMENU)ID_EDIT_MINUTE, NULL, NULL);
            SetControlFont(hEditMinute, hFont);
            
            hUpDownMinute = CreateWindow(UPDOWN_CLASS, NULL,
                                       WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                       0, 0, 0, 0, hwnd, (HMENU)ID_UPDOWN_MINUTE, NULL, NULL);
            SendMessage(hUpDownMinute, UDM_SETBUDDY, (WPARAM)hEditMinute, 0);
            SendMessage(hUpDownMinute, UDM_SETRANGE, 0, MAKELPARAM(59, 0));
            SendMessage(hUpDownMinute, UDM_SETPOS, 0, 0);
            
            HWND hLabelShutdown = CreateWindow(L"STATIC", L"关机", WS_VISIBLE | WS_CHILD,
                        181, 47, 30, 18, hwnd, NULL, NULL, NULL);
            SetControlFont(hLabelShutdown, hFont);
            
            // 分隔线1
            CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
                        15, 80, 330, 1, hwnd, NULL, NULL, NULL);
            
            // 倒计时关机设置
            hCheckCountdown = CreateWindow(L"BUTTON", L"倒计时关机", 
                                         WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                                         15, 95, 90, 20, hwnd, (HMENU)ID_CHECK_COUNTDOWN, NULL, NULL);
            SetControlFont(hCheckCountdown, hFont);
            
            // 倒计时时间输入
            hEditHours = CreateWindow(L"EDIT", L"1", 
                                    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
                                    35, 120, 55, 22, hwnd, (HMENU)ID_EDIT_HOURS, NULL, NULL);
            SetControlFont(hEditHours, hFont);
            
            hUpDownHours = CreateWindow(UPDOWN_CLASS, NULL,
                                      WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                      0, 0, 0, 0, hwnd, (HMENU)ID_UPDOWN_HOURS, NULL, NULL);
            SendMessage(hUpDownHours, UDM_SETBUDDY, (WPARAM)hEditHours, 0);
            SendMessage(hUpDownHours, UDM_SETRANGE, 0, MAKELPARAM(23, 0));
            SendMessage(hUpDownHours, UDM_SETPOS, 0, 1);
            
            HWND hLabelHour = CreateWindow(L"STATIC", L"时", WS_VISIBLE | WS_CHILD,
                        98, 122, 15, 18, hwnd, NULL, NULL, NULL);
            SetControlFont(hLabelHour, hFont);
            
            hEditMinutes = CreateWindow(L"EDIT", L"0", 
                                      WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
                                      118, 120, 55, 22, hwnd, (HMENU)ID_EDIT_MINUTES, NULL, NULL);
            SetControlFont(hEditMinutes, hFont);
            
            hUpDownMinutes = CreateWindow(UPDOWN_CLASS, NULL,
                                        WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                        0, 0, 0, 0, hwnd, (HMENU)ID_UPDOWN_MINUTES, NULL, NULL);
            SendMessage(hUpDownMinutes, UDM_SETBUDDY, (WPARAM)hEditMinutes, 0);
            SendMessage(hUpDownMinutes, UDM_SETRANGE, 0, MAKELPARAM(59, 0));
            SendMessage(hUpDownMinutes, UDM_SETPOS, 0, 0);
            
            HWND hLabelMinute = CreateWindow(L"STATIC", L"分", WS_VISIBLE | WS_CHILD,
                        181, 122, 15, 18, hwnd, NULL, NULL, NULL);
            SetControlFont(hLabelMinute, hFont);
            
            // 分隔线2
            CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
                        15, 155, 330, 1, hwnd, NULL, NULL, NULL);
            
            // 剩余时间显示
            HWND hLabelRemaining = CreateWindow(L"STATIC", L"剩余时间", WS_VISIBLE | WS_CHILD,
                        15, 170, 60, 18, hwnd, NULL, NULL, NULL);
            SetControlFont(hLabelRemaining, hFont);
            
            hTimeDisplay = CreateWindow(L"STATIC", L"--:--:--", 
                                       WS_VISIBLE | WS_CHILD | SS_CENTER,
                                       15, 190, 280, 30, hwnd, NULL, NULL, NULL);
            SetControlFont(hTimeDisplay, hTimeFont);
            
            // 注册自定义按钮类
            WNDCLASS buttonClass = {0};
            buttonClass.lpfnWndProc = CustomButtonProc;
            buttonClass.hInstance = GetModuleHandle(NULL);
            buttonClass.lpszClassName = L"ModernButton";
            buttonClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            buttonClass.hCursor = LoadCursor(NULL, IDC_HAND);
            RegisterClass(&buttonClass);
            
            // 三按钮紧凑分布布局：定时开始、?、取消关机
            hButtonStart = CreateWindow(L"ModernButton", L"定时开始", 
                                       WS_VISIBLE | WS_CHILD,
                                       40, 240, 80, 30, hwnd, (HMENU)ID_BUTTON_START, NULL, NULL);
            
            // 帮助按钮 - 居中位置，缩小间距
            hButtonHelp = CreateWindow(L"STATIC", L"?", 
                                      WS_VISIBLE | WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
                                      133, 242, 30, 26, hwnd, (HMENU)ID_BUTTON_HELP, NULL, NULL);
            
            hButtonCancel = CreateWindow(L"ModernButton", L"取消关机", 
                                        WS_VISIBLE | WS_CHILD,
                                        176, 240, 80, 30, hwnd, (HMENU)ID_BUTTON_CANCEL, NULL, NULL);
            
            OriginalEditProc = (WNDPROC)SetWindowLongPtr(hEditHour, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
            SetWindowLongPtr(hEditMinute, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
            SetWindowLongPtr(hEditHours, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
            SetWindowLongPtr(hEditMinutes, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
            
            // 默认选择倒计时关机
            useCountdown = TRUE;
            SendMessage(hCheckCountdown, BM_SETCHECK, BST_CHECKED, 0);
            
            // 初始化控件状态
            UpdateControlStates();
            break;
        }
            
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;
            
            // 时间显示区域
            if (hwndStatic == hTimeDisplay) {
                SetTextColor(hdcStatic, COLOR_TIME_TEXT);
                SetBkColor(hdcStatic, COLOR_TIME_DISPLAY);
                return (INT_PTR)CreateSolidBrush(COLOR_TIME_DISPLAY);
            }
            
            // 默认样式 - 全部使用白色背景
            SetTextColor(hdcStatic, COLOR_TEXT);
            SetBkColor(hdcStatic, COLOR_WHITE);
            return (INT_PTR)CreateSolidBrush(COLOR_WHITE);
        }
        
        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, COLOR_TEXT);
            SetBkColor(hdcEdit, RGB(255, 255, 255));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // 绘制白色背景
            FillRect(hdc, &rect, CreateSolidBrush(COLOR_WHITE));
            
            // 绘制时间显示区域边框
            RECT timeRect = {15, 190, 295, 220};
            HPEN hPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            MoveToEx(hdc, timeRect.left, timeRect.top, NULL);
            LineTo(hdc, timeRect.right - 1, timeRect.top);
            LineTo(hdc, timeRect.right - 1, timeRect.bottom - 1);
            LineTo(hdc, timeRect.left, timeRect.bottom - 1);
            LineTo(hdc, timeRect.left, timeRect.top);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_TIMER:
            if (wParam == ID_TIMER && isTimerActive) {
                time_t now;
                time(&now);
                
                if (shutdownTime > now) {
                    int remainingSeconds = (int)(shutdownTime - now);
                    UpdateTimeDisplay(remainingSeconds);
                    UpdateTrayTooltip();  // 更新托盘提示
                    UpdateDesktopTimer(remainingSeconds);  // 更新桌面倒计时
                } else {
                    // 时间到了，停止定时器
                    SetWindowText(hTimeDisplay, L"00:00:00");
                    KillTimer(hwnd, ID_TIMER);
                    DestroyDesktopTimer();  // 销毁桌面倒计时窗口
                    if (isInTray) {
                        wcscpy(nid.szTip, L"关机猫 - 已完成");
                        Shell_NotifyIcon(NIM_MODIFY, &nid);
                    }
                }
            }
            break;
            
        case WM_TRAYICON:
            if (lParam == WM_LBUTTONUP) {
                // 左键点击显示窗口
                ShowFromTray();
            } else if (lParam == WM_RBUTTONUP) {
                // 右键点击显示菜单
                ShowTrayMenu();
            }
            break;
            
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* pDrawItem = (DRAWITEMSTRUCT*)lParam;
            if (pDrawItem->CtlID == ID_BUTTON_HELP) {
                DrawFloatingHelp(pDrawItem->hDC, pDrawItem->rcItem, 
                               (pDrawItem->itemState & ODS_SELECTED), FALSE);
            }
            return TRUE;
        }
        
        case WM_LBUTTONDOWN: {
            // 检查是否点击在帮助按钮区域
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            RECT helpRect = {133, 242, 163, 268};
            if (PtInRect(&helpRect, pt)) {
                // 触发帮助
                MessageBox(hwnd, HELP_TEXT, L"作者：喵言喵语 by.52pojie", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            break;
        }
        
        case WM_MOUSEWHEEL:
            HandleMouseWheel(hwnd, wParam, lParam);
            return 0;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_CHECK_SCHEDULED:
                    useScheduled = (SendMessage(hCheckScheduled, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    useCountdown = FALSE;  // 单选按钮自动取消其他选项
                    UpdateControlStates();
                    break;
                    
                case ID_CHECK_COUNTDOWN:
                    useCountdown = (SendMessage(hCheckCountdown, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    useScheduled = FALSE;  // 单选按钮自动取消其他选项
                    UpdateControlStates();
                    break;
                    
                case ID_BUTTON_START:
                    if (useScheduled) {
                        StartScheduledShutdown();
                    } else if (useCountdown) {
                        StartCountdownShutdown();
                    }
                    break;
                    
                case ID_BUTTON_CANCEL:
                    CancelShutdownTimer();
                    break;
                    
                case ID_BUTTON_HELP:
                    MessageBox(hwnd, HELP_TEXT, L"作者：喵言喵语 by.52pojie", MB_OK | MB_ICONINFORMATION);
                    break;
                    
                case ID_TRAY_OPEN:
                    ShowFromTray();
                    break;
                    
                case ID_TRAY_CANCEL:
                    CancelShutdownTimer();
                    ShowFromTray();  // 取消关机后自动打开主界面
                    break;
                    
                case ID_TRAY_EXIT:
                    if (isTimerActive) {
                        CancelShutdownTimer();
                    }
                    DestroyWindow(hwnd);
                    break;
            }
            break;
            
        case WM_CLOSE:
            if (isTimerActive) {
                // 定时器激活时隐藏到托盘，而不是关闭程序
                HideToTray();
                return 0;  // 阻止默认处理
            } else {
                DestroyWindow(hwnd);
            }
            break;
            
        case WM_DESTROY:
            // 清理托盘图标
            if (isInTray) {
                RemoveTrayIcon();
            }
            
            // 清理桌面倒计时窗口
            DestroyDesktopTimer();
            
            // 清理资源
            if (hFont) DeleteObject(hFont);
            if (hTimeFont) DeleteObject(hTimeFont);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    // 启用视觉样式
    InitCommonControls();
    
    // 主窗口类注册
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ShutdownTimer";
    wc.hbrBackground = CreateSolidBrush(COLOR_BG);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // 窗口类图标 - 让系统自动选择合适尺寸
    wc.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON), 
                               IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    
    RegisterClass(&wc);
    
    // 桌面倒计时窗口类注册
    WNDCLASS wcDesktop = {0};
    wcDesktop.lpfnWndProc = DesktopTimerProc;
    wcDesktop.hInstance = hInstance;
    wcDesktop.lpszClassName = L"DesktopTimer";
    wcDesktop.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcDesktop.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcDesktop.style = CS_HREDRAW | CS_VREDRAW;
    
    RegisterClass(&wcDesktop);
    
    // 创建窗口 - 固定大小，不可调整，缩短宽度
    hWnd = CreateWindow(L"ShutdownTimer", L"关机猫 by.52pojie",
                       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                       CW_USEDEFAULT, CW_USEDEFAULT, 320, 320,
                       NULL, NULL, hInstance, NULL);
    
    if (!hWnd) {
        MessageBox(NULL, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // 设置窗口图标 - 让系统自动选择合适尺寸
    HICON hBigIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON), 
                                     IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED);
    HICON hSmallIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON), 
                                       IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    if (hBigIcon) {
        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hBigIcon);
    }
    if (hSmallIcon) {
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
} 