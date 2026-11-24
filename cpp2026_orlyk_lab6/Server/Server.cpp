// Server.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Server.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

#define BUF_SIZE 1024
#define ID_BTN_START 1001
#define ID_BTN_READ  1002
#define ID_BTN_STOP  1003
#define WM_MAPPING_UPDATED (WM_USER + 10)

LPCTSTR MAPPING_NAME = TEXT("myFileMapping");
LPCTSTR MUTEX_NAME = TEXT("myFileMappingMutex");
LPCTSTR EVENT_NAME = TEXT("myFileMappingEvent");

HINSTANCE hInst;
HWND hwndMain = NULL;
HWND hwndEdit = NULL;

HANDLE hMapFile = NULL;
HANDLE hMutex = NULL;
HANDLE hUpdateEvent = NULL;
HANDLE hStopEvent = NULL;
HANDLE hWatchThread = NULL;

char mess[8192] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void AppendLog(const char* text)
{
    strcat_s(mess, sizeof(mess), text);
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    SendMessageA(hwndEdit, EM_SETSEL, -1, -1);
    SendMessageA(hwndEdit, EM_SCROLLCARET, 0, 0);
}

// Потік автоматичного відстеження змін у проекції пам'яті
DWORD WINAPI WatcherThread(LPVOID lpParam)
{
    HANDLE waitHandles[2] = { hStopEvent, hUpdateEvent };
    while (true)
    {
        DWORD dwWait = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (dwWait == WAIT_OBJECT_0)
        {
            // Отримано сигнал зупинки сервера
            break;
        }
        else if (dwWait == WAIT_OBJECT_0 + 1)
        {
            // Клієнт оновив дані у FileMapping
            // сповіщення головного вікна
            PostMessage(hwndMain, WM_MAPPING_UPDATED, 0, 0);
        }
    }
    return 0;
}

void StartServerMapping(HWND hWnd)
{
    if (hMapFile != NULL)
    {
        MessageBox(hWnd, TEXT("Сервер FileMapping вже запущено!"), TEXT("Інформація"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Створення об'єкта відображення файлу в пам'ять
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        BUF_SIZE,
        MAPPING_NAME
    );

    if (hMapFile == NULL)
    {
        MessageBox(hWnd, TEXT("Неможливо створити об'єкт відображення в пам'яті!"), TEXT("Помилка"), MB_OK | MB_ICONERROR);
        return;
    }

    // Очищення виділеної області пам'яті
    LPVOID pInitBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
    if (pInitBuf != NULL)
    {
        ZeroMemory(pInitBuf, BUF_SIZE);
        UnmapViewOfFile(pInitBuf);
    }

    // Створення м'ютекса та подій для синхронізації
    hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
    hUpdateEvent = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME);
    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    hWatchThread = CreateThread(NULL, 0, WatcherThread, NULL, 0, NULL);

    AppendLog("Сервер FileMapping запущено (автоматичне відстеження активне)...\r\n");
}

void ReadServerMapping(HWND hWnd, bool isAutoEvent)
{
    if (hMapFile == NULL)
    {
        MessageBox(hWnd, TEXT("Запустіть сервер FileMapping!"), TEXT("Увага"), MB_OK | MB_ICONWARNING);
        return;
    }

    if (hMutex != NULL)
    {
        WaitForSingleObject(hMutex, 2000);
    }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, BUF_SIZE);
    if (pBuf == NULL)
    {
        if (hMutex != NULL) ReleaseMutex(hMutex);
        MessageBox(hWnd, TEXT("Подання проектованого файлу неможливе!"), TEXT("Помилка"), MB_OK | MB_ICONERROR);
        return;
    }

    char readData[BUF_SIZE] = { 0 };
    strncpy_s(readData, (const char*)pBuf, BUF_SIZE - 1);
    UnmapViewOfFile(pBuf);

    if (hMutex != NULL)
    {
        ReleaseMutex(hMutex);
    }

    if (strlen(readData) == 0)
    {
        if (!isAutoEvent)
        {
            AppendLog("\r\nПроекція пам'яті зараз порожня.\r\n");
        }
        return;
    }

    char formatted[BUF_SIZE + 128];
    sprintf_s(formatted, sizeof(formatted), "\r\n[%s]:\r\n%s\r\n",
        isAutoEvent ? "Автоматичне виявлення змін" : "Читання за подією користувача",
        readData);
    AppendLog(formatted);
}

void StopServerMapping()
{
    if (hStopEvent != NULL)
    {
        SetEvent(hStopEvent);
    }
    if (hWatchThread != NULL)
    {
        WaitForSingleObject(hWatchThread, 1000);
        CloseHandle(hWatchThread);
        hWatchThread = NULL;
    }
    if (hStopEvent != NULL)
    {
        CloseHandle(hStopEvent);
        hStopEvent = NULL;
    }
    if (hUpdateEvent != NULL)
    {
        CloseHandle(hUpdateEvent);
        hUpdateEvent = NULL;
    }
    if (hMutex != NULL)
    {
        CloseHandle(hMutex);
        hMutex = NULL;
    }
    if (hMapFile != NULL)
    {
        CloseHandle(hMapFile);
        hMapFile = NULL;
        AppendLog("\r\nСервер FileMapping зупинено.\r\n");
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
    wcex.lpszClassName = L"FileMappingServerClass";

    RegisterClassExW(&wcex);

    hwndMain = CreateWindowW(L"FileMappingServerClass", L"Сервер", WS_OVERLAPPEDWINDOW,
        100, 100, 500, 430, nullptr, nullptr, hInstance, nullptr);

    if (!hwndMain) return FALSE;

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

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
            10, 10, 460, 310, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Запустити"), WS_VISIBLE | WS_CHILD,
            10, 335, 140, 30, hWnd, (HMENU)ID_BTN_START, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Прочитати"), WS_VISIBLE | WS_CHILD,
            160, 335, 145, 30, hWnd, (HMENU)ID_BTN_READ, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Зупинити"), WS_VISIBLE | WS_CHILD,
            315, 335, 140, 30, hWnd, (HMENU)ID_BTN_STOP, hInst, NULL);

        sprintf_s(mess, "Відображення файлу в пам'ять.\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

        StartServerMapping(hWnd);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_START) StartServerMapping(hWnd);
        if (LOWORD(wParam) == ID_BTN_READ)  ReadServerMapping(hWnd, false);
        if (LOWORD(wParam) == ID_BTN_STOP)  StopServerMapping();
        break;

    case WM_MAPPING_UPDATED:
        ReadServerMapping(hWnd, true);
        break;

    case WM_DESTROY:
        StopServerMapping();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}