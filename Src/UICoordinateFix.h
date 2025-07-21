#pragma once

#include <Windows.h>

// UI 座標系統修復工具類
class UICoordinateFix {
public:
    // 獲取 DPI 縮放因子
    static void GetDPIScale(HWND hwnd, float& scaleX, float& scaleY) {
        HDC hdc = GetDC(hwnd);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hwnd, hdc);
        
        // 96 DPI 是 Windows 的標準 DPI
        scaleX = static_cast<float>(dpiX) / 96.0f;
        scaleY = static_cast<float>(dpiY) / 96.0f;
    }
    
    // 將螢幕座標轉換為客戶區座標
    static void ScreenToClientCoords(HWND hwnd, int& x, int& y) {
        POINT pt = {x, y};
        ScreenToClient(hwnd, &pt);
        x = pt.x;
        y = pt.y;
    }
    
    // 調試輸出視窗資訊
    static void DebugWindowInfo(HWND hwnd) {
        RECT windowRect, clientRect;
        GetWindowRect(hwnd, &windowRect);
        GetClientRect(hwnd, &clientRect);
        
        // 客戶區在螢幕上的位置
        POINT clientOrigin = {0, 0};
        ClientToScreen(hwnd, &clientOrigin);
        
        char debugMsg[512];
        sprintf_s(debugMsg, 
            "Window Debug Info:\n"
            "  Window Rect: (%d,%d)-(%d,%d)\n"
            "  Client Rect: (%d,%d)-(%d,%d)\n"
            "  Client Origin on Screen: (%d,%d)\n"
            "  Border Width: %d, Title Height: %d\n",
            windowRect.left, windowRect.top, windowRect.right, windowRect.bottom,
            clientRect.left, clientRect.top, clientRect.right, clientRect.bottom,
            clientOrigin.x, clientOrigin.y,
            clientOrigin.x - windowRect.left,
            clientOrigin.y - windowRect.top);
        
        OutputDebugStringA(debugMsg);
        
        // DPI 資訊
        float scaleX, scaleY;
        GetDPIScale(hwnd, scaleX, scaleY);
        sprintf_s(debugMsg, "  DPI Scale: X=%.2f, Y=%.2f\n", scaleX, scaleY);
        OutputDebugStringA(debugMsg);
    }
};