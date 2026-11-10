// Client3.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Client3.h"
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

#define ID_BTN_CALC 1001

LPCTSTR ServerMailslotName = TEXT("\\\\.\\mailslot\\lab5_orlyk");

HINSTANCE hInst;
HWND hwndLog, hwndInputA, hwndInputB;
char mess[2048] = "";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void CalculateAndSend(HWND hWnd)
{
    char bufA[64] = { 0 }, bufB[64] = { 0 };
    GetWindowTextA(hwndInputA, bufA, sizeof(bufA));
    GetWindowTextA(hwndInputB, bufB, sizeof(bufB));

    double a = atof(bufA);
    double b = atof(bufB);

    if (a <= 0 || b <= 0)
    {
        MessageBox(hWnd, TEXT("Введіть коректні додатні числа для сторін a та b!"), TEXT("Увага"), MB_OK | MB_ICONWARNING);
        return;
    }

    double area = a * b; // Обчислення площі прямокутника

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
        SendMessageA(hwndLog, WM_SETTEXT, 0, (LPARAM)mess);
        return;
    }

    char sendBuf[512];
    sprintf_s(sendBuf, sizeof(sendBuf),
        "Дані Клієнта #3 (Завдання *):\r\n- Математична операція: Площа прямокутника (S = a * b)\r\n- Сторона a = %.2f\r\n- Сторона b = %.2f\r\n- Обчислена площа S = %.2f",
        a, b, area);

    DWORD cbWritten = 0;
    if (WriteFile(hFile, sendBuf, (DWORD)strlen(sendBuf) + 1, &cbWritten, NULL))
    {
        strcat_s(mess, sizeof(mess), "\r\n[Результати обчислень надіслано на сервер]:\r\n");
        strcat_s(mess, sizeof(mess), sendBuf);
        strcat_s(mess, sizeof(mess), "\r\n");
        SendMessageA(hwndLog, WM_SETTEXT, 0, (LPARAM)mess);
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
    wcex.lpszClassName = L"MailslotClient3Class";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"MailslotClient3Class", L"Клієнт 3", WS_OVERLAPPEDWINDOW,
        1040, 100, 420, 360, nullptr, nullptr, hInstance, nullptr);

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
        hwndLog = CreateWindow(TEXT("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 380, 150, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("STATIC"), TEXT("Сторона a:"), WS_VISIBLE | WS_CHILD, 10, 175, 70, 20, hWnd, NULL, hInst, NULL);
        hwndInputA = CreateWindow(TEXT("EDIT"), TEXT("12.5"), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT, 85, 172, 90, 22, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("STATIC"), TEXT("Сторона b:"), WS_VISIBLE | WS_CHILD, 190, 175, 70, 20, hWnd, NULL, hInst, NULL);
        hwndInputB = CreateWindow(TEXT("EDIT"), TEXT("4.0"), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT, 265, 172, 90, 22, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Обчислити та надіслати S"), WS_VISIBLE | WS_CHILD,
            10, 215, 220, 30, hWnd, (HMENU)ID_BTN_CALC, hInst, NULL);

        sprintf_s(mess, "Клієнт #3 готовий.\r\n");
        SendMessageA(hwndLog, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_CALC) CalculateAndSend(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}