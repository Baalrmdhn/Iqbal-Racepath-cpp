#include <iostream>
#include "game.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <conio.h>
#include <nlohmann/json.hpp>
extern bool soundEnabled;
using json = nlohmann::json;
using namespace std;

void tampilkanMenuPengaturan() {
    char pilihan;
    do {
        system("CLS");
        cout << "========================================" << endl;
        cout << "|            PENGATURAN SUARA          |" << endl;
        cout << "========================================" << endl;
        cout << "| 1. Sound Effects   :   " << (soundEnabled ? "[ ON ] " : "[ OFF ]") << "     |" << endl;
        cout << "|                                      |" << endl;
        cout << "| 2. Kembali ke Menu Utama             |" << endl;
        cout << "========================================" << endl << endl;
        cout << "Pilih opsi (1-2): ";

        pilihan = _getch(); // Langsung ngabaca input tanpa perlu Enter

        if (pilihan == '1') {
            // Toggle: Jika true menjadi false, jika false menjadi true
            soundEnabled = !soundEnabled;
        }

    } while (pilihan != '2');
}

void tampilkanTopScore() {
    std::ifstream in("scores.json");
    json data;
    if (in.is_open()) {
        try { in >> data; } catch (...) { data = json::array(); }
        in.close();
    } else {
        data = json::array();
    }
    std::vector<std::pair<int, std::string>> daftar;
    for (const auto &entry : data) {
        if (entry.contains("user") && entry.contains("score")) {
            daftar.emplace_back(entry["score"].get<int>(), entry["user"].get<std::string>());
        }
    }
    std::sort(daftar.begin(), daftar.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
    std::cout << "========== TOP 10 SCORE ==========" << std::endl;
    std::cout << "| No |       Nama        |  Score   |" << std::endl;
    std::cout << "----------------------------------" << std::endl;
    int n = std::min(10, (int)daftar.size());
    for (int i = 0; i < n; ++i) {
        std::cout << "| " << (i + 1) << "  | ";
        std::cout.width(17);
        std::cout << std::left << daftar[i].second << " | ";
        std::cout.width(8);
        std::cout << std::right << daftar[i].first << " |" << std::endl;
    }
    if (n == 0)
        std::cout << "|         Belum ada skor         |" << std::endl;
    std::cout << "----------------------------------" << std::endl;
    system("pause");
}


void menu() {
    int difficulty = 1;
    float spawnMultiplier = 1.0f;
    float scoreMultiplier = 1.0f;
    string user;

    while (true) {
        system("CLS");
        cout << "========================================" << endl;
        cout << "| No |           MENU UTAMA             |" << endl;
        cout << "========================================" << endl;
        cout << "| 1  | Play Game                        |" << endl;
        cout << "| 2  | Pengaturan                       |" << endl;
        cout << "| 3  | Top Scores                       |" << endl;
        cout << "| 0  | Exit                             |" << endl;
        cout << "========================================" << endl << endl;
        cout << "Pilih opsi (0-3): ";

        char pilihan;
        cin >> pilihan;
        cin.ignore();
        switch (pilihan) {
        case '1':
            system("CLS");
            while (true) {
            cout << "Masukkan nama Anda (maks 12 karakter, huruf dan spasi saja): ";
            getline(cin, user);
            // Validasi: hanya huruf, spasi, dan maksimal 12 karakter
            if (user.length() <= 12 && !user.empty() &&
                all_of(user.begin(), user.end(), [](char c) { return isalpha(c) || c == ' '; }))
                break;
            cout << "Nama hanya boleh huruf, spasi, dan maksimal 12 karakter! Coba lagi.\n";
            system("pause");
            system("CLS");
            }
            cin.ignore();
            while (true) {
                cout << "\nPilih Difficulty:" << endl;
                cout << "1. Normal" << endl;
                cout << "2. Hard" << endl;
                cout << "3. Extreme" << endl;
                cout << "Pilihan (1-3): ";
                char diffChoice = _getch();
                if (diffChoice == '1') {
                    spawnMultiplier = 1.0f; scoreMultiplier = 1.0f;
                    difficulty = 1; break;
                } else if (diffChoice == '2') {
                    spawnMultiplier = 0.8f; scoreMultiplier = 1.5f;
                    difficulty = 2; break;
                } else if (diffChoice == '3') {
                    spawnMultiplier = 0.6f; scoreMultiplier = 2.0f;
                    difficulty = 3; break;
                } else {
                    cout << "\nPilihan tidak valid, coba lagi!\n";
                }
            }
            cout << "\nDifficulty di-set ke ";
            if (difficulty == 1) cout << "Normal\n";
            else if (difficulty == 2) cout << "Hard\n";
            else cout << "Extreme\n";

            cout << "Tekan tombol apa saja untuk memulai permainan... ";
            _getch();

            mainGame(difficulty, spawnMultiplier, scoreMultiplier, user);
            break;
        case '2':
            tampilkanMenuPengaturan();
            break;
        case '3':
            system("CLS");
            tampilkanTopScore();
            break;
        case '0':
            cout << "\nKeluar dari program\n";
            return;
        default:
            cout << "\nOpsi tidak valid, tekan tombol apa saja untuk coba lagi...";
            _getch();
            break;
        }
    }
}

int main() {
    menu();
    return 0;
}