// Client1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Client1.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

#define BUF_SIZE 1024
#define ID_BTN_CONNECT    1001
#define ID_BTN_SEND       1002
#define ID_BTN_DISCONNECT 1003

LPCTSTR PIPE_NAME = TEXT("\\\\.\\pipe\\lab7_orlyk");

HINSTANCE hInst;
HWND hwndEdit = NULL;
HANDLE hNamedPipe = INVALID_HANDLE_VALUE;
char mess[4096] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void AppendLog(const char* text)
{
    strcat_s(mess, sizeof(mess), text);
    SetWindowTextA(hwndEdit, mess);
    SendMessageA(hwndEdit, EM_SETSEL, -1, -1);
    SendMessageA(hwndEdit, EM_SCROLLCARET, 0, 0);
}

void ConnectToPipe(HWND hWnd)
{
    if (hNamedPipe != INVALID_HANDLE_VALUE)
    {
        MessageBox(hWnd, TEXT("Канал вже відкрито!"), TEXT("Інформація"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    hNamedPipe = CreateFile(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hNamedPipe == INVALID_HANDLE_VALUE)
    {
        char errBuf[128];
        sprintf_s(errBuf, "Помилка відкриття каналу (сервер не запущено): %lu\r\n", GetLastError());
        AppendLog(errBuf);
        return;
    }

    DWORD dwMode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hNamedPipe, &dwMode, NULL, NULL);

    AppendLog("З'єднання з каналом \\\\.\\pipe\\lab7_orlyk встановлено!\r\n");
}

void SendClient1Data(HWND hWnd)
{
    if (hNamedPipe == INVALID_HANDLE_VALUE)
    {
        MessageBox(hWnd, TEXT("Спочатку відкрийте канал (підключіться до сервера)!"), TEXT("Увага"), MB_OK | MB_ICONWARNING);
        return;
    }

    // Підготовка даних за Варіантом №8
    int isServerR2 = GetSystemMetrics(SM_SERVERR2);
    int monitorsCount = GetSystemMetrics(SM_CMONITORS);

    HDC hdc = GetDC(hWnd);
    int colorDepth = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(hWnd, hdc);

    char sendBuf[512];
    sprintf_s(sendBuf, sizeof(sendBuf),
        "Дані від Клієнта №1:\r\n- Версія ОС (SM_SERVERR2): %d\r\n- Кількість моніторів (SM_CMONITORS): %d\r\n- Глибина кольору (BITSPIXEL): %d bit",
        isServerR2, monitorsCount, colorDepth);

    DWORD cbWritten = 0;
    if (!WriteFile(hNamedPipe, sendBuf, (DWORD)strlen(sendBuf) + 1, &cbWritten, NULL))
    {
        char errBuf[128];
        sprintf_s(errBuf, "Помилка запису в канал: %lu\r\n", GetLastError());
        AppendLog(errBuf);
        CloseHandle(hNamedPipe);
        hNamedPipe = INVALID_HANDLE_VALUE;
        return;
    }

    AppendLog("\r\n[Надіслано на сервер]:\r\n");
    AppendLog(sendBuf);
    AppendLog("\r\n");

    // Читання відповіді-підтвердження від сервера
    char replyBuf[256] = { 0 };
    DWORD cbRead = 0;
    if (ReadFile(hNamedPipe, replyBuf, sizeof(replyBuf) - 1, &cbRead, NULL) && cbRead > 0)
    {
        replyBuf[cbRead] = '\0';
        char replyLog[320];
        sprintf_s(replyLog, "[Відповідь сервера]: %s\r\n", replyBuf);
        AppendLog(replyLog);
    }
}

void DisconnectPipe()
{
    if (hNamedPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hNamedPipe);
        hNamedPipe = INVALID_HANDLE_VALUE;
        AppendLog("Канал закрито клієнтом.\r\n");
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
    wcex.lpszClassName = L"NamedPipeClient1Class";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"NamedPipeClient1Class", L"Клієнт 1", WS_OVERLAPPEDWINDOW,
        620, 100, 440, 330, nullptr, nullptr, hInstance, nullptr);

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
            10, 10, 400, 210, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Відкрити канал"), WS_VISIBLE | WS_CHILD,
            10, 235, 125, 30, hWnd, (HMENU)ID_BTN_CONNECT, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Надіслати дані"), WS_VISIBLE | WS_CHILD,
            145, 235, 130, 30, hWnd, (HMENU)ID_BTN_SEND, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Закрити канал"), WS_VISIBLE | WS_CHILD,
            285, 235, 125, 30, hWnd, (HMENU)ID_BTN_DISCONNECT, hInst, NULL);

        AppendLog("Клієнт №1 готовий.\r\n");
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_CONNECT)    ConnectToPipe(hWnd);
        if (LOWORD(wParam) == ID_BTN_SEND)       SendClient1Data(hWnd);
        if (LOWORD(wParam) == ID_BTN_DISCONNECT) DisconnectPipe();
        break;

    case WM_DESTROY:
        DisconnectPipe();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}