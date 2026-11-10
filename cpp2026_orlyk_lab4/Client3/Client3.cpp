// Client3.cpp : Defines the entry point for the application.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <ws2tcpip.h>
#include "framework.h"
#include "Client3.h"
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#define SERV_PORT 5000
#define WSA_NETEVENT (WM_USER + 1)

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

static HWND hwndLog;
static HWND hwndInput;
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
    SendMessageA(hwndLog, WM_SETTEXT, 0, (LPARAM)mess);
    return TRUE;
}

void SendCustomText(HWND hWnd)
{
    if (cln_socket == INVALID_SOCKET)
    {
        MessageBoxA(hWnd, "Спочатку підключіться до сервера!", "Увага", MB_OK);
        return;
    }

    char userText[512] = { 0 };
    GetWindowTextA(hwndInput, userText, sizeof(userText));

    if (strlen(userText) == 0)
    {
        MessageBoxA(hWnd, "Введіть текст для відправки!", "Увага", MB_OK);
        return;
    }

    if (send(cln_socket, userText, (int)strlen(userText), 0) != SOCKET_ERROR)
    {
        char infoBuf[600];
        sprintf_s(infoBuf, sizeof(infoBuf), "\r\n[Надіслано серверу]: %s\r\n", userText);
        strcat_s(mess, sizeof(mess), infoBuf);
        SendMessageA(hwndLog, WM_SETTEXT, 0, (LPARAM)mess);
        SetWindowTextA(hwndInput, ""); // Очистити поле введення
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT3, szWindowClass, MAX_LOADSTRING);
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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT3));
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
    HWND hWnd = CreateWindowW(szWindowClass, L"Клієнт 3", WS_OVERLAPPEDWINDOW,
        1040, 100, 420, 360, nullptr, nullptr, hInstance, nullptr);

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
        hwndLog = CreateWindow(TEXT("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 380, 140, hWnd, NULL, hInst, NULL);

        hwndInput = CreateWindow(TEXT("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
            10, 160, 380, 25, hWnd, NULL, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Підключитись"), WS_VISIBLE | WS_CHILD,
            10, 200, 120, 30, hWnd, (HMENU)1, hInst, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Надіслати текст"), WS_VISIBLE | WS_CHILD,
            140, 200, 140, 30, hWnd, (HMENU)2, hInst, NULL);

        WORD wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);

        sprintf_s(mess, "Клієнт 3 готовий\r\n");
        SendMessageA(hwndLog, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) ConnectToServer(hWnd);
        if (LOWORD(wParam) == 2) SendCustomText(hWnd);
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