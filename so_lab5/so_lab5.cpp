#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

bool is_prime(int n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}

const int NUM_PROCESSES = 10;
const int STEP = 1000;

void run_child(HANDLE h_in, HANDLE h_out) {
    int range[2];
    DWORD bytesRead, bytesWritten;

    if (ReadFile(h_in, range, sizeof(range), &bytesRead, NULL) && bytesRead == sizeof(range)) {
        for (int num = range[0]; num <= range[1]; ++num) {
            if (is_prime(num)) {
                WriteFile(h_out, &num, sizeof(num), &bytesWritten, NULL);
            }
        }
    }

    CloseHandle(h_in);
    CloseHandle(h_out);
}

int main(int argc, char* argv[]) {
    // Ramura pentru procesul copil
    if (argc >= 4 && std::string(argv[1]) == "--child") {
        HANDLE h_in = (HANDLE)(uintptr_t)std::stoull(argv[2]);
        HANDLE h_out = (HANDLE)(uintptr_t)std::stoull(argv[3]);
        run_child(h_in, h_out);
        return 0;
    }

    // --- PROCESUL PARINTE ---
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE h_p2c_read[NUM_PROCESSES], h_p2c_write[NUM_PROCESSES];
    HANDLE h_c2p_read[NUM_PROCESSES], h_c2p_write[NUM_PROCESSES];
    PROCESS_INFORMATION pi[NUM_PROCESSES];

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        CreatePipe(&h_p2c_read[i], &h_p2c_write[i], &saAttr, 0);
        CreatePipe(&h_c2p_read[i], &h_c2p_write[i], &saAttr, 0);

        SetHandleInformation(h_p2c_write[i], HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(h_c2p_read[i], HANDLE_FLAG_INHERIT, 0);

        std::string cmd = std::string("\"") + exePath + "\" --child " +
            std::to_string((uintptr_t)h_p2c_read[i]) + " " +
            std::to_string((uintptr_t)h_c2p_write[i]);

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi[i]);

        CloseHandle(h_p2c_read[i]);
        CloseHandle(h_c2p_write[i]);

        int range[2] = { i * STEP + 1, (i + 1) * STEP };
        DWORD bytesWritten;
        WriteFile(h_p2c_write[i], range, sizeof(range), &bytesWritten, NULL);
        CloseHandle(h_p2c_write[i]);
    }

    std::cout << "--- Numere prime gasite (Windows) ---\n";
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int prime_val;
        DWORD bytesRead;
        while (ReadFile(h_c2p_read[i], &prime_val, sizeof(prime_val), &bytesRead, NULL) && bytesRead > 0) {
            std::cout << prime_val << " ";
        }
        CloseHandle(h_c2p_read[i]);
    }
    std::cout << "\n\n";

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        WaitForSingleObject(pi[i].hProcess, INFINITE);
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    return 0;
}