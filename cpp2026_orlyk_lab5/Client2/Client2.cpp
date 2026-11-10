// Client2.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Client2.h"
#include <stdio.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

#define ID_BTN_SEND 1001

LPCTSTR ServerMailslotName = TEXT("\\\\.\\mailslot\\lab5_orlyk");

HINSTANCE hInst;
HWND hwndEdit;
char mess[2048] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void SendClient2Data(HWND hWnd)
{
    HANDLE hFile = CreateFile(
        ServerMailslotName,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        char errBuf[256];
        sprintf_s(errBuf, "Помилка відкриття поштового сервера: %lu\r\n", GetLastError());
        strcat_s(mess, sizeof(mess), errBuf);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        return;
    }

    // Збір даних
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int iconWidth = GetSystemMetrics(SM_CXICON);

    HDC hdc = GetDC(hWnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);

    char sendBuf[512];
    sprintf_s(sendBuf, sizeof(sendBuf),
        "Дані Клієнта #2:\r\n- Ширина екрану (SM_CXSCREEN): %d px\r\n- Розмір піктограми (SM_CXICON): %d px\r\n- DPI горизонтально (LOGPIXELSX): %d",
        screenWidth, iconWidth, dpiX);

    DWORD cbWritten = 0;
    if (WriteFile(hFile, sendBuf, (DWORD)strlen(sendBuf) + 1, &cbWritten, NULL))
    {
        strcat_s(mess, sizeof(mess), "\r\n[Успішно надіслано на сервер]:\r\n");
        strcat_s(mess, sizeof(mess), sendBuf);
        strcat_s(mess, sizeof(mess), "\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    }

    CloseHandle(hFile);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    hInst = hInstance;
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"MailslotClient2Class";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"MailslotClient2Class", L"Клієнт 2", WS_OVERLAPPEDWINDOW,
        600, 440, 420, 320, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hwndEdit = CreateWindow(TEXT("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 380, 200, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Надіслати дані на сервер"), WS_VISIBLE | WS_CHILD,
            10, 220, 200, 30, hWnd, (HMENU)ID_BTN_SEND, hInst, NULL);

        sprintf_s(mess, "Клієнт #2 готовий.\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_SEND) SendClient2Data(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}