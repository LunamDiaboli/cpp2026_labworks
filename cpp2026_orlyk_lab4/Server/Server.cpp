// Server.cpp : Defines the entry point for the application.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <ws2tcpip.h>
#include "framework.h"
#include "Server.h"
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#define SERV_PORT 5000
#define WSA_ACCEPT (WM_USER + 0)
#define WSA_NETEVENT (WM_USER + 1)
#define MAX_CLIENTS 3

#define ID_SERVER_START 1001
#define ID_SERVER_STOP  1002

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

static HWND hwndEdit;
WSADATA wsaData;
SOCKET srv_socket = INVALID_SOCKET;

SOCKET sock[MAX_CLIENTS] = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET };
SOCKADDR_IN sockaddr[MAX_CLIENTS];

char mess[4096] = "";
char szBuf[1024];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void ServerStart(HWND hWnd)
{
    if (srv_socket != INVALID_SOCKET)
    {
        MessageBoxA(hWnd, "Сервер вже запущено!", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }

    srv_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv_socket == INVALID_SOCKET)
    {
        MessageBoxA(hWnd, "Помилка створення сокета!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    SOCKADDR_IN srv_address;
    srv_address.sin_family = AF_INET;
    srv_address.sin_port = htons(SERV_PORT);
    srv_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv_socket, (LPSOCKADDR)&srv_address, sizeof(srv_address)) == SOCKET_ERROR)
    {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        MessageBoxA(hWnd, "Помилка зв'язування з портом!", "Error", MB_OK | MB_ICONSTOP);
        return;
    }

    if (listen(srv_socket, MAX_CLIENTS) == SOCKET_ERROR)
    {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        MessageBoxA(hWnd, "Помилка режиму прослуховування!", "Error", MB_OK);
        return;
    }

    WSAAsyncSelect(srv_socket, hWnd, WSA_ACCEPT, FD_ACCEPT);

    strcat_s(mess, sizeof(mess), "Сервер запущено на порту 5000...\r\n");
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

void ServerStop(HWND hWnd)
{
    WSAAsyncSelect(srv_socket, hWnd, 0, 0);
    if (srv_socket != INVALID_SOCKET)
    {
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (sock[i] != INVALID_SOCKET)
        {
            WSAAsyncSelect(sock[i], hWnd, 0, 0);
            closesocket(sock[i]);
            sock[i] = INVALID_SOCKET;
        }
    }

    strcat_s(mess, sizeof(mess), "Сервер зупинено.\r\n");
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

void WndProc_OnWSAAccept(HWND hWnd, LPARAM lParam)
{
    if (WSAGETSELECTERROR(lParam))
    {
        MessageBoxA(hWnd, "Помилка прийому підключення (accept error)", "Error", MB_OK);
        return;
    }

    int freeIdx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (sock[i] == INVALID_SOCKET)
        {
            freeIdx = i;
            break;
        }
    }

    if (freeIdx == -1)
    {
        MessageBoxA(hWnd, "Досягнуто ліміт підключень (макс. 3 клієнти)!", "Увага", MB_OK);
        return;
    }

    int addr_len = sizeof(sockaddr[freeIdx]);
    sock[freeIdx] = accept(srv_socket, (LPSOCKADDR)&sockaddr[freeIdx], &addr_len);

    if (sock[freeIdx] != INVALID_SOCKET)
    {
        WSAAsyncSelect(sock[freeIdx], hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE);

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sockaddr[freeIdx].sin_addr), ipStr, sizeof(ipStr));

        sprintf_s(szBuf, "Додано Клієнта %d (IP: %s, Port: %u)\r\n",
            freeIdx + 1, ipStr, ntohs(sockaddr[freeIdx].sin_port));
        strcat_s(mess, sizeof(mess), szBuf);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    }
}

void WndProc_OnWSANetEvent(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int clientIdx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (sock[i] == (SOCKET)wParam)
        {
            clientIdx = i;
            break;
        }
    }

    if (clientIdx == -1) return;

    if (WSAGETSELECTEVENT(lParam) == FD_READ)
    {
        char tempBuf[512] = { 0 };
        int rc = recv((SOCKET)wParam, tempBuf, sizeof(tempBuf) - 1, 0);
        if (rc > 0)
        {
            tempBuf[rc] = '\0';
            sprintf_s(szBuf, "\r\n[Дані від Клієнта %d]:\r\n%s\r\n", clientIdx + 1, tempBuf);
            strcat_s(mess, sizeof(mess), szBuf);
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        }
    }
    else if (WSAGETSELECTEVENT(lParam) == FD_CLOSE)
    {
        WSAAsyncSelect(sock[clientIdx], hWnd, 0, 0);
        closesocket(sock[clientIdx]);
        sock[clientIdx] = INVALID_SOCKET;

        sprintf_s(szBuf, "Клієнт %d відключився.\r\n", clientIdx + 1);
        strcat_s(mess, sizeof(mess), szBuf);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SERVER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVER));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SERVER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, L"Сервер", WS_OVERLAPPEDWINDOW,
        100, 100, 480, 360, nullptr, nullptr, hInstance, nullptr);

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
            10, 10, 440, 280, hWnd, NULL, hInst, NULL);

        WORD wVersionRequested = MAKEWORD(2, 2);
        int err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0)
        {
            MessageBoxA(hWnd, "Помилка WSAStartup!", "ERROR", MB_OK | MB_ICONSTOP);
            return FALSE;
        }

        sprintf_s(mess, "Ініціалізовано Winsock 2.2\r\nОпис: %s\r\nСтатус: %s\r\n\r\n",
            wsaData.szDescription, wsaData.szSystemStatus);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);

        // Автоматичний запуск сервера при створенні вікна
        ServerStart(hWnd);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WSA_ACCEPT:
        WndProc_OnWSAAccept(hWnd, lParam);
        break;
    case WSA_NETEVENT:
        WndProc_OnWSANetEvent(hWnd, wParam, lParam);
        break;
    case WM_DESTROY:
        ServerStop(hWnd);
        WSACleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}