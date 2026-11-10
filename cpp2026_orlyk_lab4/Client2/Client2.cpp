// Client2.cpp : Defines the entry point for the application.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <ws2tcpip.h>
#include "framework.h"
#include "Client2.h"
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#define SERV_PORT 5000
#define WSA_NETEVENT (WM_USER + 1)

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

static HWND hwndEdit;
WSADATA wsaData;
SOCKET cln_socket = INVALID_SOCKET;
char mess[2048] = "";

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL ConnectToServer(HWND hWnd)
{
    cln_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (cln_socket == INVALID_SOCKET)
    {
        MessageBoxA(hWnd, "Помилка створення сокета!", "Error", MB_OK);
        return FALSE;
    }

    SOCKADDR_IN dest_sin;
    dest_sin.sin_family = AF_INET;
    dest_sin.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &dest_sin.sin_addr);

    if (connect(cln_socket, (PSOCKADDR)&dest_sin, sizeof(dest_sin)) == SOCKET_ERROR)
    {
        closesocket(cln_socket);
        cln_socket = INVALID_SOCKET;
        MessageBoxA(hWnd, "Помилка підключення до сервера!", "Error", MB_OK);
        return FALSE;
    }

    WSAAsyncSelect(cln_socket, hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE);
    strcat_s(mess, sizeof(mess), "Зв'язок із сервером встановлено!\r\n");
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    return TRUE;
}

void SendData(HWND hWnd)
{
    if (cln_socket == INVALID_SOCKET)
    {
        MessageBoxA(hWnd, "Спочатку підключіться до сервера!", "Увага", MB_OK);
        return;
    }

    // Отримання даних за Варіантом №8
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int iconWidth = GetSystemMetrics(SM_CXICON);

    HDC hdc = GetDC(hWnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);

    char sendBuf[512];
    sprintf_s(sendBuf, sizeof(sendBuf),
        "Ширина екрану (SM_CXSCREEN): %d px\r\nРозмір піктограми (SM_CXICON): %d px\r\nDPI горизонтально (LOGPIXELSX): %d\r\n",
        screenWidth, iconWidth, dpiX);

    if (send(cln_socket, sendBuf, (int)strlen(sendBuf), 0) != SOCKET_ERROR)
    {
        strcat_s(mess, sizeof(mess), "\r\n[Дані надіслано серверу]:\r\n");
        strcat_s(mess, sizeof(mess), sendBuf);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT2, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT2));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, L"Клієнт 2", WS_OVERLAPPEDWINDOW,
        600, 420, 420, 300, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hwndEdit = CreateWindow(TEXT("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 380, 180, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Підключитись"), WS_VISIBLE | WS_CHILD,
            10, 200, 120, 30, hWnd, (HMENU)1, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Надіслати дані"), WS_VISIBLE | WS_CHILD,
            140, 200, 130, 30, hWnd, (HMENU)2, hInst, NULL);

        WORD wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);

        sprintf_s(mess, "Клієнт 2 готовий\r\n");
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) ConnectToServer(hWnd);
        if (LOWORD(wParam) == 2) SendData(hWnd);
        break;
    case WM_DESTROY:
        if (cln_socket != INVALID_SOCKET) closesocket(cln_socket);
        WSACleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}