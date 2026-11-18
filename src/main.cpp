#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <random>

struct SharedData { int A; int B; int C; int version; };
SharedData g_data = { 0, 0, 0, 0 };

CRITICAL_SECTION cs_log;
CRITICAL_SECTION cs_counters;
HANDLE semReaders;
HANDLE semWriters;

int activeReaders = 0;
int waitingWriters = 0;
int waitingReaders = 0;
bool writerActive = false;

volatile bool g_stop = false;
long g_invariantViolations = 0;
long g_totalReads = 0;
long g_totalWrites = 0;

auto g_startTime = std::chrono::steady_clock::now();
long long GetTimeMs() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - g_startTime).count();
}

void Log(const std::string& msg) {
    if (g_stop) return;
    EnterCriticalSection(&cs_log);
    std::cout << "[" << std::setfill('0') << std::setw(6) << GetTimeMs() << " ms] " << msg << std::endl;
    LeaveCriticalSection(&cs_log);
}

void DoWork(int ms) { Sleep(ms); }

DWORD WINAPI ReaderThread(LPVOID param) {
    int id = (int)(intptr_t)param;
    std::mt19937 rng(id);
    for (int i = 0; i < 50 && !g_stop; ++i) {
        EnterCriticalSection(&cs_counters);
        if (writerActive || waitingWriters > 0) {
            waitingReaders++;
            LeaveCriticalSection(&cs_counters);
            WaitForSingleObject(semReaders, INFINITE);
            EnterCriticalSection(&cs_counters);
            waitingReaders--;
        }
        activeReaders++;
        LeaveCriticalSection(&cs_counters);

        Log("READER " + std::to_string(id) + " READING");
        if (g_data.B != 2 * g_data.A || g_data.C != 3 * g_data.A) {
            InterlockedIncrement(&g_invariantViolations);
        }
        DoWork(50 + rng() % 50);
        InterlockedIncrement(&g_totalReads);

        EnterCriticalSection(&cs_counters);
        activeReaders--;
        if (activeReaders == 0 && waitingWriters > 0) ReleaseSemaphore(semWriters, 1, NULL);
        LeaveCriticalSection(&cs_counters);
    }
    return 0;
}

DWORD WINAPI WriterThread(LPVOID param) {
    int id = (int)(intptr_t)param;
    std::mt19937 rng(id + 1000);
    for (int i = 0; i < 50 && !g_stop; ++i) {
        EnterCriticalSection(&cs_counters);
        waitingWriters++;
        if (activeReaders > 0 || writerActive) {
            LeaveCriticalSection(&cs_counters);
            WaitForSingleObject(semWriters, INFINITE);
            EnterCriticalSection(&cs_counters);
        }
        waitingWriters--;
        writerActive = true;
        LeaveCriticalSection(&cs_counters);

        Log("WRITER " + std::to_string(id) + " WRITING");
        g_data.A++;
        DoWork(20 + rng() % 30);
        g_data.B = 2 * g_data.A;
        g_data.C = 3 * g_data.A;
        g_data.version++;
        InterlockedIncrement(&g_totalWrites);

        EnterCriticalSection(&cs_counters);
        writerActive = false;
        if (waitingWriters > 0) ReleaseSemaphore(semWriters, 1, NULL);
        else if (waitingReaders > 0) ReleaseSemaphore(semReaders, waitingReaders, NULL);
        LeaveCriticalSection(&cs_counters);
    }
    return 0;
}

int main() {
    InitializeCriticalSection(&cs_log);
    InitializeCriticalSection(&cs_counters);
    semReaders = CreateSemaphore(NULL, 0, 10000, NULL);
    semWriters = CreateSemaphore(NULL, 0, 10000, NULL);
    g_startTime = std::chrono::steady_clock::now();

    std::vector<HANDLE> threads;
    for (int i = 1; i <= 3; ++i) threads.push_back(CreateThread(NULL, 0, ReaderThread, (LPVOID)(intptr_t)i, 0, NULL));
    for (int i = 1; i <= 2; ++i) threads.push_back(CreateThread(NULL, 0, WriterThread, (LPVOID)(intptr_t)i, 0, NULL));

    WaitForMultipleObjects((DWORD)threads.size(), threads.data(), TRUE, 15000);
    for (HANDLE h : threads) CloseHandle(h);
    CloseHandle(semReaders); CloseHandle(semWriters);
    DeleteCriticalSection(&cs_log); DeleteCriticalSection(&cs_counters);

    std::cout << "\n=== STATISTICS ===\nViolations: " << g_invariantViolations << "\nTime: " << GetTimeMs() << " ms\n";
    return 0;
}