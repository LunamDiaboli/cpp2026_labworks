// Server.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Server.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

#define BUF_SIZE 1024
#define ID_BTN_START 1001
#define ID_BTN_STOP  1002

LPCTSTR PIPE_NAME = TEXT("\\\\.\\pipe\\lab7_orlyk");

HINSTANCE hInst;
HWND hwndEdit = NULL;
HANDLE hListenerThread = NULL;
volatile bool bServerRunning = false;
CRITICAL_SECTION csLog;

char mess[8192] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void AppendLog(const char* text)
{
    EnterCriticalSection(&csLog);
    strcat_s(mess, sizeof(mess), text);
    if (hwndEdit != NULL)
    {
        SetWindowTextA(hwndEdit, mess);
        SendMessageA(hwndEdit, EM_SETSEL, -1, -1);
        SendMessageA(hwndEdit, EM_SCROLLCARET, 0, 0);
    }
    LeaveCriticalSection(&csLog);
}

// Потік обслуговування окремого підключеного клієнта
DWORD WINAPI ClientHandlerThread(LPVOID lpParam)
{
    HANDLE hPipe = (HANDLE)lpParam;
    char readBuf[BUF_SIZE];
    DWORD cbRead = 0, cbWritten = 0;

    AppendLog("[Сервер]: Новий клієнт підключився до каналу.\r\n");

    while (bServerRunning)
    {
        ZeroMemory(readBuf, sizeof(readBuf));
        BOOL fSuccess = ReadFile(hPipe, readBuf, sizeof(readBuf) - 1, &cbRead, NULL);

        if (!fSuccess || cbRead == 0)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                AppendLog("[Сервер]: Клієнт закрив з'єднання.\r\n");
            }
            break;
        }

        readBuf[cbRead] = '\0';

        char logEntry[BUF_SIZE + 64];
        sprintf_s(logEntry, sizeof(logEntry), "\r\nОтримано через Named Pipe\r\n%s\r\n", readBuf);
        AppendLog(logEntry);

        // Надсилаємо підтвердження прийому клієнту
        const char* ack = "Дані успішно прийнято сервером";
        WriteFile(hPipe, ack, (DWORD)strlen(ack) + 1, &cbWritten, NULL);
    }

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    return 0;
}

// Потік очікування нових підключень клієнтів
DWORD WINAPI ServerListenerThread(LPVOID lpParam)
{
    while (bServerRunning)
    {
        HANDLE hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUF_SIZE,
            BUF_SIZE,
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            char errBuf[128];
            sprintf_s(errBuf, "Помилка CreateNamedPipe: %lu\r\n", GetLastError());
            AppendLog(errBuf);
            break;
        }

        BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!bServerRunning)
        {
            CloseHandle(hPipe);
            break;
        }

        if (fConnected)
        {
            HANDLE hClientThread = CreateThread(NULL, 0, ClientHandlerThread, (LPVOID)hPipe, 0, NULL);
            if (hClientThread != NULL)
            {
                CloseHandle(hClientThread);
            }
        }
        else
        {
            CloseHandle(hPipe);
        }
    }
    return 0;
}

void StartPipeServer(HWND hWnd)
{
    if (bServerRunning)
    {
        MessageBox(hWnd, TEXT("Сервер іменованих каналів вже запущено!"), TEXT("Інформація"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    bServerRunning = true;
    hListenerThread = CreateThread(NULL, 0, ServerListenerThread, NULL, 0, NULL);
    AppendLog("Сервер запущено. Канал \\\\.\\pipe\\lab7_orlyk створено.\r\nОчікування клієнтів...\r\n");
}

void StopPipeServer()
{
    if (!bServerRunning) return;

    bServerRunning = false;

    // Фіктивне підключення для коректного розблокування ConnectNamedPipe
    HANDLE hDummy = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDummy != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDummy);
    }

    if (hListenerThread != NULL)
    {
        WaitForSingleObject(hListenerThread, 2000);
        CloseHandle(hListenerThread);
        hListenerThread = NULL;
    }

    AppendLog("\r\nСервер зупинено. Канал закрито.\r\n");
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    InitializeCriticalSection(&csLog);
    hInst = hInstance;

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"NamedPipeServerClass";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"NamedPipeServerClass", L"Сервер", WS_OVERLAPPEDWINDOW,
        100, 100, 500, 420, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteCriticalSection(&csLog);
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
            10, 10, 460, 300, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Створити канал"), WS_VISIBLE | WS_CHILD,
            10, 325, 160, 30, hWnd, (HMENU)ID_BTN_START, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Закрити канал"), WS_VISIBLE | WS_CHILD,
            180, 325, 160, 30, hWnd, (HMENU)ID_BTN_STOP, hInst, NULL);

        AppendLog("Канали обміну даними.\r\n");
        StartPipeServer(hWnd);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_START) StartPipeServer(hWnd);
        if (LOWORD(wParam) == ID_BTN_STOP)  StopPipeServer();
        break;

    case WM_DESTROY:
        StopPipeServer();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
