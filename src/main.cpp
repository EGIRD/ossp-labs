#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>

struct SharedData {
    int A;
    int B;
    int C;
    int version;
};

SharedData g_data = { 0, 0, 0, 0 };
volatile bool g_stop = false;
auto g_startTime = std::chrono::steady_clock::now();

long long GetTimeMs() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - g_startTime).count();
}

void Log(const std::string& msg) {
    if (g_stop) return;
    std::cout << "[" << std::setfill('0') << std::setw(6) << GetTimeMs() << " ms] " << msg << std::endl;
}

void DoWork(int ms) { Sleep(ms); }

DWORD WINAPI ReaderThread(LPVOID param) {
    int id = (int)(intptr_t)param;
    for (int i = 0; i < 200 && !g_stop; ++i) {
        Log("READER " + std::to_string(id) + " READING");
        // Проверяем инварианты (в этой версии они будут нарушаться!)
        if (g_data.B != 2 * g_data.A || g_data.C != 3 * g_data.A) {
            Log("!!! READER " + std::to_string(id) + " DETECTED INCONSISTENCY !!!");
        }
        DoWork(50);
    }
    return 0;
}

DWORD WINAPI WriterThread(LPVOID param) {
    int id = (int)(intptr_t)param;
    for (int i = 0; i < 200 && !g_stop; ++i) {
        Log("WRITER " + std::to_string(id) + " WRITING");
        g_data.A++;
        DoWork(20);
        g_data.B = 2 * g_data.A;
        g_data.C = 3 * g_data.A;
        g_data.version++;
        DoWork(30);
    }
    return 0;
}

int main() {
    g_startTime = std::chrono::steady_clock::now();
    std::vector<HANDLE> threads;
    for (int i = 1; i <= 3; ++i) threads.push_back(CreateThread(NULL, 0, ReaderThread, (LPVOID)(intptr_t)i, 0, NULL));
    for (int i = 1; i <= 2; ++i) threads.push_back(CreateThread(NULL, 0, WriterThread, (LPVOID)(intptr_t)i, 0, NULL));

    WaitForMultipleObjects((DWORD)threads.size(), threads.data(), TRUE, 15000);
    for (HANDLE h : threads) CloseHandle(h);
    return 0;
}