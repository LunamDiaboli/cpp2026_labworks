// cpp2026_orlyk_lab3.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "cpp2026_orlyk_lab3.h"

#include <vector>
#include <cstdlib>
#include <ctime>
#include <cwchar>
#include <algorithm>

#define MAX_LOADSTRING 100
const int N_NUMBERS = 10; // Кількість останніх чисел N для обчислення середнього

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Спільні ресурси та об'єкти синхронізації
HWND g_hWnd = NULL;
HANDLE g_hMutex = NULL;
HANDLE g_hThreads[3] = { NULL, NULL, NULL };
volatile BOOL g_bRunning = TRUE;

std::vector<int> g_buffer;  // Спільний буфер чисел
int g_lastNumber = 0;       // Останнє згенероване число
double g_average = 0.0;     // Обчислене середнє значення

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// --- Потокові функції ---

// Потік 1: Генерує випадкові числа та записує їх у спільний буфер
DWORD WINAPI GeneratorThread(LPVOID lpParam)
{
    while (g_bRunning)
    {
        int num = rand() % 100 + 1; // Генерація числа від 1 до 100

        // Вхід у критичну секцію під захистом м'ютекса
        WaitForSingleObject(g_hMutex, INFINITE);
        g_buffer.push_back(num);
        g_lastNumber = num;
        ReleaseMutex(g_hMutex);

        Sleep(3000); // Затримка 3000 мс
    }
    return 0;
}

// Потік 2: Обчислює середнє значення з останніх N чисел
DWORD WINAPI CalculatorThread(LPVOID lpParam)
{
    while (g_bRunning)
    {
        WaitForSingleObject(g_hMutex, INFINITE);
        if (!g_buffer.empty())
        {
            int count = (std::min)((int)g_buffer.size(), N_NUMBERS);
            double sum = 0;
            for (size_t i = g_buffer.size() - count; i < g_buffer.size(); ++i)
            {
                sum += g_buffer[i];
            }
            g_average = sum / count;
        }
        ReleaseMutex(g_hMutex);

        Sleep(3000); // Затримка 3000 мс
    }
    return 0;
}

// Потік 3: Періодично надсилає сигнал для оновлення GUI
DWORD WINAPI DisplayThread(LPVOID lpParam)
{
    while (g_bRunning)
    {
        if (g_hWnd)
        {
            // Виклик перемалювання вікна
            InvalidateRect(g_hWnd, NULL, TRUE);
        }
        Sleep(3000); // Оновлення інтерфейсу кожні 3000 мс
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CPP2026ORLYKLAB3, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CPP2026ORLYKLAB3));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CPP2026ORLYKLAB3));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CPP2026ORLYKLAB3);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   g_hWnd = hWnd;

   // Ініціалізація генератора випадкових чисел
   srand((unsigned int)time(NULL));

   // Створення м'ютекса для синхронізації
   g_hMutex = CreateMutex(NULL, FALSE, NULL);

   // Запуск 3-х робочих потоків
   g_hThreads[0] = CreateThread(NULL, 0, GeneratorThread, NULL, 0, NULL);
   g_hThreads[1] = CreateThread(NULL, 0, CalculatorThread, NULL, 0, NULL);
   g_hThreads[2] = CreateThread(NULL, 0, DisplayThread, NULL, 0, NULL);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            wchar_t strBuffer[256];
            int currentNum = 0;
            double currentAvg = 0.0;
            size_t totalCount = 0;

            // Зчитуємо дані з захищеного буфера
            WaitForSingleObject(g_hMutex, INFINITE);
            currentNum = g_lastNumber;
            currentAvg = g_average;
            totalCount = g_buffer.size();
            ReleaseMutex(g_hMutex);

            // Вивід інформації на екран
            swprintf_s(strBuffer, L"Усього згенеровано чисел у буфері: %zu", totalCount);
            TextOutW(hdc, 20, 30, strBuffer, (int)wcslen(strBuffer));

            swprintf_s(strBuffer, L"Поточне (останнє) число: %d", currentNum);
            TextOutW(hdc, 20, 60, strBuffer, (int)wcslen(strBuffer));

            swprintf_s(strBuffer, L"Середнє значення з останніх %d чисел: %.2f", N_NUMBERS, currentAvg);
            TextOutW(hdc, 20, 90, strBuffer, (int)wcslen(strBuffer));
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        // Зупиняємо цикл виконання потоків
        g_bRunning = FALSE;

        // Чекаємо завершення всіх потоків
        WaitForMultipleObjects(3, g_hThreads, TRUE, 1000);

        // Закриваємо дескриптори
        for (int i = 0; i < 3; ++i)
        {
            if (g_hThreads[i]) CloseHandle(g_hThreads[i]);
        }
        if (g_hMutex) CloseHandle(g_hMutex);

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
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
