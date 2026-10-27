#include <windows.h>
#include <iostream>
#include <vector>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <sstream>

// Глобальні ресурси та об'єкти синхронізації
const size_t ARRAY_SIZE = 50'000'000;
std::vector<int> g_array(ARRAY_SIZE, 1);

HANDLE g_hMutex = NULL;          // М'ютекс для захисту спільної суми
HANDLE g_hConsoleMutex = NULL;   // М'ютекс для потокобезпечного виводу в консоль
HANDLE g_hStartEvent = NULL;     // Подія для синхронного старту потоків
long long g_globalSum = 0;       // Спільний ресурс (загальна сума)

struct ThreadParam {
    int threadIndex;
    size_t startIdx;
    size_t endIdx;
};

// Потокобезпечна функція для виводу повідомлень у консоль
void SafeLog(const std::string& msg)
{
    WaitForSingleObject(g_hConsoleMutex, INFINITE);
    std::cout << msg << std::endl;
    ReleaseMutex(g_hConsoleMutex);
}

// Функція потоку
DWORD WINAPI ComputeThreadProc(LPVOID lpParam)
{
    ThreadParam* pData = static_cast<ThreadParam*>(lpParam);
    DWORD tid = GetCurrentThreadId();

    // 1. Журналювання старту потоку
    {
        std::ostringstream ss;
        ss << "[START]  Потік ID: " << tid << " (№" << pData->threadIndex << ") створено.";
        SafeLog(ss.str());
    }

    // 2. Очікування події для синхронного запуску
    WaitForSingleObject(g_hStartEvent, INFINITE);

    // 3. Обчислення часткової суми
    long long localSum = 0;
    for (size_t i = pData->startIdx; i < pData->endIdx; ++i)
    {
        localSum += g_array[i];
    }

    // 4. Синхронізований доступ до спільної змінної g_globalSum
    WaitForSingleObject(g_hMutex, INFINITE);
    g_globalSum += localSum;
    ReleaseMutex(g_hMutex);

    // Журналювання факту отримання доступу
    {
        std::ostringstream ss;
        ss << "[ACCESS] Потік ID: " << tid << " (№" << pData->threadIndex << ") додав частку: " << localSum;
        SafeLog(ss.str());
    }

    // 5. Журналювання завершення та виклик ExitThread()
    {
        std::ostringstream ss;
        ss << "[EXIT]   Потік ID: " << tid << " (№" << pData->threadIndex << ") завершує роботу.";
        SafeLog(ss.str());
    }

    delete pData;
    ExitThread(0); // Примусове завершення потоку згідно з вимогами
    return 0;
}

// Функція проведення заміру для N потоків
void BenchmarkThreads(int numThreads)
{
    std::cout << "\n======================================================\n";
    std::cout << "Запуск тестування для кількості потоків: " << numThreads << "\n";
    std::cout << "======================================================\n";

    g_globalSum = 0;

    // Створення об'єктів синхронізації
    g_hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hMutex = CreateMutex(NULL, FALSE, NULL);
    g_hConsoleMutex = CreateMutex(NULL, FALSE, NULL);

    std::vector<HANDLE> hThreads(numThreads);
    size_t chunkSize = ARRAY_SIZE / numThreads;

    // Динамічне створення N потоків
    for (int i = 0; i < numThreads; ++i)
    {
        ThreadParam* pParam = new ThreadParam();
        pParam->threadIndex = i + 1;
        pParam->startIdx = i * chunkSize;
        pParam->endIdx = (i == numThreads - 1) ? ARRAY_SIZE : (i + 1) * chunkSize;

        hThreads[i] = CreateThread(NULL, 0, ComputeThreadProc, pParam, 0, NULL);
    }

    Sleep(50); // Пауза для ініціалізації потоків

    auto startTime = std::chrono::high_resolution_clock::now();

    // Сигнал усім потокам розпочати виконання
    SetEvent(g_hStartEvent);

    // Очікування завершення всіх створених потоків
    WaitForMultipleObjects(numThreads, hThreads.data(), TRUE, INFINITE);

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    std::cout << "\nРезультати для " << numThreads << " потоків:\n";
    std::cout << "Обчислена сума: " << g_globalSum << "\n";
    std::cout << "Час виконання: " << std::fixed << std::setprecision(2) << elapsedTime << " мс\n";

    // Закриття дескрипторів
    for (int i = 0; i < numThreads; ++i)
    {
        CloseHandle(hThreads[i]);
    }
    CloseHandle(g_hMutex);
    CloseHandle(g_hConsoleMutex);
    CloseHandle(g_hStartEvent);
}

int main()
{
    SetConsoleOutputCP(1251);

    std::cout << "Дослідження продуктивності WinAPI потоків\n";
    std::cout << "Обробка масиву розміром: " << ARRAY_SIZE << " елементів.\n";

    int threadCounts[] = { 2, 4, 8, 16 };

    for (int count : threadCounts)
    {
        BenchmarkThreads(count);
    }

    return 0;
}