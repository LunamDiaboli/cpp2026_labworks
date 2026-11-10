// Server.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Server.h"
#include <stdio.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

#define IDT_TIMER 1001
#define ID_BTN_CREATE 2001
#define ID_BTN_CLOSE  2002

LPCTSTR MailslotName = TEXT("\\\\.\\mailslot\\lab5_orlyk");
HANDLE hMailslot = INVALID_HANDLE_VALUE;

HINSTANCE hInst;
HWND hwndEdit;
char mess[4096] = "";
char szBuf[1024];

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void CreateServerMailslot(HWND hWnd)
{
    if (hMailslot != INVALID_HANDLE_VALUE)
    {
        MessageBox(hWnd, TEXT("Поштову скриньку вже створено!"), TEXT("Інформація"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    hMailslot = CreateMailslot(
        MailslotName,
        0,                      // Необмежений розмір повідомлення
        MAILSLOT_WAIT_FOREVER,  // Необмежене очікування
        NULL
    );

    if (hMailslot == INVALID_HANDLE_VALUE)
    {
        sprintf_s(szBuf, "Помилка створення поштової скриньки! Error: %lu\r\n", GetLastError());
        strcat_s(mess, sizeof(mess), szBuf);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        return;
    }

    SetTimer(hWnd, IDT_TIMER, 1000, NULL); // Таймер перевірки кожну 1 секунду

    strcat_s(mess, sizeof(mess), "Поштовий сервер запущено (\\.\\mailslot\\lab5_orlyk)...\r\n Очікування повідомлень...\r\n");
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

void CloseServerMailslot(HWND hWnd)
{
    if (hMailslot != INVALID_HANDLE_VALUE)
    {
        KillTimer(hWnd, IDT_TIMER);
        CloseHandle(hMailslot);
        hMailslot = INVALID_HANDLE_VALUE;

        strcat_s(mess, sizeof(mess), "\r\nПоштовий сервер закрито.\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    }
}

void ReadMailslotData()
{
    if (hMailslot == INVALID_HANDLE_VALUE) return;

    DWORD cbMessage = 0, cMessage = 0, cbRead = 0;
    BOOL fResult = GetMailslotInfo(hMailslot, NULL, &cbMessage, &cMessage, NULL);

    if (fResult && cMessage > 0)
    {
        while (cMessage > 0)
        {
            char readBuffer[1024] = { 0 };
            if (ReadFile(hMailslot, readBuffer, sizeof(readBuffer) - 1, &cbRead, NULL) && cbRead > 0)
            {
                readBuffer[cbRead] = '\0';
                sprintf_s(szBuf, "\r\n--- Нове повідомлення ---\r\n%s\r\n", readBuffer);
                strcat_s(mess, sizeof(mess), szBuf);
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
            }
            GetMailslotInfo(hMailslot, NULL, &cbMessage, &cMessage, NULL);
        }
    }
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
    wcex.lpszClassName = L"MailslotServerClass";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"MailslotServerClass", L"Сервер Mailslot", WS_OVERLAPPEDWINDOW,
        100, 100, 480, 420, nullptr, nullptr, hInstance, nullptr);

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
            10, 10, 440, 300, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Запустити сервер"), WS_VISIBLE | WS_CHILD,
            10, 320, 150, 30, hWnd, (HMENU)ID_BTN_CREATE, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Зупинити сервер"), WS_VISIBLE | WS_CHILD,
            170, 320, 150, 30, hWnd, (HMENU)ID_BTN_CLOSE, hInst, NULL);

        sprintf_s(mess, "Поштові скриньки (Mailslot)\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

        CreateServerMailslot(hWnd);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_CREATE) CreateServerMailslot(hWnd);
        if (LOWORD(wParam) == ID_BTN_CLOSE)  CloseServerMailslot(hWnd);
        break;

    case WM_TIMER:
        if (wParam == IDT_TIMER) ReadMailslotData();
        break;

    case WM_DESTROY:
        CloseServerMailslot(hWnd);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}