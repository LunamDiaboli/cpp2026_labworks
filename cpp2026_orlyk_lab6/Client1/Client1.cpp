// Client1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Client1.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

#define BUF_SIZE 1024
#define ID_BTN_SEND 1001

LPCTSTR MAPPING_NAME = TEXT("myFileMapping");
LPCTSTR MUTEX_NAME = TEXT("myFileMappingMutex");
LPCTSTR EVENT_NAME = TEXT("myFileMappingEvent");

HINSTANCE hInst;
HWND hwndEdit;
char mess[2048] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void SendClient1Data(HWND hWnd)
{
    // 1. Відкриття існуючого об'єкта відображення файлу в пам'ять
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAME);
    if (hMapFile == NULL)
    {
        MessageBox(hWnd, TEXT("Не можливо відкрити відображений у пам'яті об'єкт (сервер не запущено)!"), TEXT("Map"), MB_OK | MB_ICONERROR);
        return;
    }

    // 2. Підготовка даних за Варіантом №8
    int isServerR2 = GetSystemMetrics(SM_SERVERR2);
    int monitorsCount = GetSystemMetrics(SM_CMONITORS);

    HDC hdc = GetDC(hWnd);
    int colorDepth = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(hWnd, hdc);

    char sendBuf[512];
    sprintf_s(sendBuf, sizeof(sendBuf),
        "Дані від Клієнта №1:\r\n- Версія ОС (SM_SERVERR2): %d\r\n- Кількість моніторів (SM_CMONITORS): %d\r\n- Глибина кольору (BITSPIXEL): %d bit",
        isServerR2, monitorsCount, colorDepth);

    // 3. Синхронізований запис у спільну пам'ять
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_NAME);
    if (hMutex != NULL)
    {
        WaitForSingleObject(hMutex, 3000);
    }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
    if (pBuf == NULL)
    {
        if (hMutex != NULL) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
        CloseHandle(hMapFile);
        MessageBox(hWnd, TEXT("Подання проектованого файлу неможливе!"), TEXT("Map"), MB_OK | MB_ICONERROR);
        return;
    }

    ZeroMemory(pBuf, BUF_SIZE);
    CopyMemory(pBuf, sendBuf, strlen(sendBuf) + 1);
    UnmapViewOfFile(pBuf);

    if (hMutex != NULL)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    // 4. Подача сигналу серверу про оновлення даних
    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, EVENT_NAME);
    if (hEvent != NULL)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    CloseHandle(hMapFile);

    strcat_s(mess, sizeof(mess), "\r\n[Записано у FileMapping]:\r\n");
    strcat_s(mess, sizeof(mess), sendBuf);
    strcat_s(mess, sizeof(mess), "\r\n");
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
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
    wcex.lpszClassName = L"FileMappingClient1Class";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"FileMappingClient1Class", L"Клієнт 1", WS_OVERLAPPEDWINDOW,
        620, 100, 420, 320, nullptr, nullptr, hInstance, nullptr);

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

        CreateWindow(TEXT("BUTTON"), TEXT("Відправити дані у FileMapping"), WS_VISIBLE | WS_CHILD,
            10, 220, 220, 30, hWnd, (HMENU)ID_BTN_SEND, hInst, NULL);

        sprintf_s(mess, "Клієнт №1 готовий.\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_SEND) SendClient1Data(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}