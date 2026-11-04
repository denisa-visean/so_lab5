#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

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

int main() {
    int p2c[NUM_PROCESSES][2]; // Parent to Child pipe
    int c2p[NUM_PROCESSES][2]; // Child to Parent pipe
    pid_t pids[NUM_PROCESSES];

    // 1. Creare procese si legaturi pipe
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (pipe(p2c[i]) == -1 || pipe(c2p[i]) == -1) {
            perror("Eroare la crearea pipe-ului");
            return 1;
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Eroare la fork");
            return 1;
        }

        if (pids[i] == 0) {
            // --- PROCES COPIL ---
            close(p2c[i][1]); // Inchide capatul de scriere parinte->copil
            close(c2p[i][0]); // Inchide capatul de citire copil->parinte

            int range[2];
            read(p2c[i][0], range, sizeof(range));
            close(p2c[i][0]);

            for (int num = range[0]; num <= range[1]; ++num) {
                if (is_prime(num)) {
                    write(c2p[i][1], &num, sizeof(num));
                }
            }

            // Inchide capatul de scriere pentru a semnala EOF parintelui
            close(c2p[i][1]);
            _exit(0);
        } else {
            // --- PROCES PARINTE ---
            close(p2c[i][0]); // Inchide capatul de citire al copilului
            close(c2p[i][1]); // Inchide capatul de scriere al copilului

            int range[2] = { i * STEP + 1, (i + 1) * STEP };
            write(p2c[i][1], range, sizeof(range));
            close(p2c[i][1]); // Inchide imediat dupa transmiterea intervalului
        }
    }

    // 2. Colectare si afisare din pipe-uri
    std::cout << "--- Numere prime gasite (Linux Ubuntu) ---\n";
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int prime_val;
        while (read(c2p[i][0], &prime_val, sizeof(prime_val)) > 0) {
            std::cout << prime_val << " ";
        }
        close(c2p[i][0]);
    }
    std::cout << "\n\n";

    // 3. Asteptare terminare procese copil
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], nullptr, 0);
    }

    return 0;
}
