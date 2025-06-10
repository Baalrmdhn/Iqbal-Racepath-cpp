#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
bool soundEnabled = true;
using namespace std;

struct CustomQueue {
    int top;
    string isi[50];
    static const int ukuran = 50;
};

void CustomQueue_init(CustomQueue *q) {
    q->top = 0;
}

void CustomQueue_clear(CustomQueue *q) {
    q->top = 0;
}

bool CustomQueue_isEmpty(CustomQueue *q) {
    return (q->top == 0);
}

bool CustomQueue_isFull(CustomQueue *q) {
    return (q->top >= CustomQueue::ukuran);
}

void CustomQueue_enqueue(CustomQueue *q, const string &data) {
    if (!CustomQueue_isFull(q)) {
        q->isi[q->top] = data;
        q->top++;
    }
}

void CustomQueue_dequeue(CustomQueue *q) {
    if (!CustomQueue_isEmpty(q)) {
        for (int i = 0; i < q->top - 1; i++) {
            q->isi[i] = q->isi[i + 1];
        }
        q->top--;
    }
}

vector<string> CustomQueue_getContents(CustomQueue *q) {
    vector<string> contents;
    for (int i = 0; i < q->top; i++) {
        contents.push_back(q->isi[i]);
    }
    return contents;
}

void CustomQueue_updateBack(CustomQueue *q, const string &data) {
    if (!CustomQueue_isEmpty(q)) {
        q->isi[q->top - 1] = data;
    }
}

void CustomQueue_setAt(CustomQueue *q, int idx, const string &val) {
    if (idx >= 0 && idx < q->top) {
        q->isi[idx] = val;
    }
}

struct Graph {
    int numVertices;
    vector<int> *adjLists;
};

void Graph_init(Graph *g, int vertices) {
    g->numVertices = vertices;
    g->adjLists = new vector<int>[vertices];
}

void Graph_destroy(Graph *g) {
    delete[] g->adjLists;
    g->adjLists = nullptr;
    g->numVertices = 0;
}

void Graph_addEdge(Graph *g, int src, int dest) {
    if (src >= 0 && src < g->numVertices && dest >= 0 && dest < g->numVertices) {
        g->adjLists[src].push_back(dest);
        g->adjLists[dest].push_back(src);
    }
}

bool Graph_isConnected(Graph *g, int src, int dest) {
    if (src < 0 || src >= g->numVertices || dest < 0 || dest >= g->numVertices) {
        return false;
    }
    for (int neighbor : g->adjLists[src]) {
        if (neighbor == dest) {
            return true;
        }
    }
    return false;
}

const int LEBAR_LAYAR = 110;
const int TINGGI_LAYAR = 20;
CHAR_INFO consoleBuffer[LEBAR_LAYAR * TINGGI_LAYAR];
HANDLE hConsole;

const int PANJANG_JALAN = 50;
const string SIMBOL_JALAN = "- ";
const string SIMBOL_KOSONG = "  ";
const int TOTAL_LINTASAN = 6;
const int TOTAL_CABANG = 3;

CustomQueue jalur[TOTAL_LINTASAN];
Graph jalanGraph;

const vector<string> MOBIL = {"  . - - ` - .", "  '- O - O -'"};
const string RINTANGAN_ART = " /|||\\ ";
const string RINTANGAN_MARKER = "##";
const string KOIN_MARKER = "$$";
const string KOIN_ART = "  $  ";
const int KOIN_SCORE = 300;
const int KOIN_SHOW_FRAMES = 15;
const string FINISH_LINE_MARKER = "=";
const string FINISH_LINE_ART = "=";

int posisiMobil = 1;
int nyawa = 3;
bool invulnerable = false;
DWORD invulnerableStart = 0;
const DWORD INVULNERABLE_DURATION = 2000;

int koinShowCounter = 0;
int distanceToFinish = 0;
bool finishLineInserted = false;
bool finishLineCrossed = false;
int currentLevel = 1;
bool afterFinishLine = false;
int finishLinePos = -1;

using json = nlohmann::json;

void simpanScore(const string &user, int score) {
    json data;
    ifstream in("scores.json");
    if (in.is_open()) {
        try {
            in >> data;
        } catch (...) {
            data = json::array();
        }
        in.close();
    } else {
        data = json::array();
    }
    data.push_back({{"user", user}, {"score", score}});
    ofstream out("scores.json");
    out << data.dump(4);
    out.close();
}

void gambarKeBuffer(int x, int y, const string &teks, WORD atribut = 7) {
    if (y >= TINGGI_LAYAR || x >= LEBAR_LAYAR)
        return;
    for (size_t i = 0; i < teks.length(); ++i) {
        if (x + i < LEBAR_LAYAR) {
            consoleBuffer[x + i + LEBAR_LAYAR * y].Char.AsciiChar = teks[i];
            consoleBuffer[x + i + LEBAR_LAYAR * y].Attributes = atribut;
        }
    }
}

void tampilkanBuffer() {
    COORD bufferSize = {LEBAR_LAYAR, TINGGI_LAYAR};
    COORD characterPos = {0, 0};
    SMALL_RECT writeArea = {0, 0, LEBAR_LAYAR - 1, TINGGI_LAYAR - 1};
    WriteConsoleOutputA(hConsole, consoleBuffer, bufferSize, characterPos, &writeArea);
}

void isiJalur() {
    srand((unsigned int)time(0));
    for (int i = 0; i < TOTAL_LINTASAN; i++) {
        CustomQueue_init(&jalur[i]);
        for (int j = 0; j < PANJANG_JALAN; j++) {
            CustomQueue_enqueue(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
        }
    }
}

bool cekFinishLineCross() {
    int lintasanMobil = posisiMobil * 2 + 1;
    vector<string> contents = CustomQueue_getContents(&jalur[lintasanMobil]);
    if (!contents.empty() && contents[0] == FINISH_LINE_MARKER)
        return true;
    return false;
}

void cekTabrakan() {
    int lintasanMobil = posisiMobil * 2 + 1;
    vector<string> contents = CustomQueue_getContents(&jalur[lintasanMobil]);
    if (contents.size() > 1 && !invulnerable) {
        if (contents[0] != FINISH_LINE_MARKER && contents[1] != FINISH_LINE_MARKER) {
            string depan = contents[0] + contents[1];
            if (depan.find(RINTANGAN_MARKER) != string::npos) {
                if (soundEnabled) {
                    PlaySound(TEXT("hit.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                nyawa--;
                invulnerable = true;
                invulnerableStart = GetTickCount();
            }
        }
    }
    if (contents.size() > 0) {
        int maxCek = min(4, (int)contents.size());
        string depanKoin = "";
        for (int i = 0; i < maxCek; ++i) {
            if (contents[i] == FINISH_LINE_MARKER)
                continue;
            depanKoin += contents[i];
        }
        size_t posKoin = depanKoin.find(KOIN_MARKER);
        if (posKoin != string::npos) {
            koinShowCounter = KOIN_SHOW_FRAMES;
            if (soundEnabled) {
                PlaySound(TEXT("coin.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
            int cellIdx = 0, charCount = 0;
            for (; cellIdx < maxCek; ++cellIdx) {
                if (contents[cellIdx] == FINISH_LINE_MARKER)
                    continue;
                if (posKoin < charCount + contents[cellIdx].size())
                    break;
                charCount += contents[cellIdx].size();
            }
            if (cellIdx < maxCek && contents[cellIdx] != FINISH_LINE_MARKER) {
                CustomQueue_setAt(&jalur[lintasanMobil], cellIdx, (lintasanMobil % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
            }
        }
    }
}

void updateInvulnerable() {
    if (invulnerable && (GetTickCount() - invulnerableStart >= INVULNERABLE_DURATION)) {
        invulnerable = false;
    }
}

void pindahMobil() {
    if (_kbhit()) {
        char input = _getch();
        if (input == 'w' || input == 'W') {
            if (Graph_isConnected(&jalanGraph, posisiMobil, posisiMobil - 1)) {
                posisiMobil--;
            }
        } else if (input == 's' || input == 'S') {
            if (Graph_isConnected(&jalanGraph, posisiMobil, posisiMobil + 1)) {
                posisiMobil++;
            }
        }
    }
}

void jalurBerjalan() {
    for (int i = 0; i < TOTAL_LINTASAN; i++) {
        CustomQueue_dequeue(&jalur[i]);
        CustomQueue_enqueue(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
    }
}

void tampilkanEndScreen(int score, bool failed, const string &user) {
    for (int i = 0; i < LEBAR_LAYAR * TINGGI_LAYAR; ++i) {
        consoleBuffer[i] = {' ', 7};
    }
    string msg = "GAME OVER!";
    string msg3 = "Nama: " + user;
    string msg4 = "Skor: " + to_string(score);
    int boxWidth = max({msg.length(), msg3.length(), msg4.length()}) + 6;
    int boxHeight = 6;
    int boxX = LEBAR_LAYAR / 2 - boxWidth / 2;
    int boxY = TINGGI_LAYAR / 2 - boxHeight / 2;
    string topBot = "+" + string(boxWidth - 2, '-') + "+";
    gambarKeBuffer(boxX, boxY, topBot, 14);
    for (int i = 1; i < boxHeight - 1; ++i) {
        gambarKeBuffer(boxX, boxY + i, "|" + string(boxWidth - 2, ' ') + "|", 14);
    }
    gambarKeBuffer(boxX, boxY + boxHeight - 1, topBot, 14);
    gambarKeBuffer(boxX + (boxWidth - msg.length()) / 2, boxY + 1, msg, 12);
    gambarKeBuffer(boxX + (boxWidth - msg3.length()) / 2, boxY + 3, msg3, 11);
    gambarKeBuffer(boxX + (boxWidth - msg4.length()) / 2, boxY + 4, msg4, 11);
    tampilkanBuffer();
    Sleep(3000);
}

void tampilkanFinishLine() {
    for (int baris = 0; baris < TOTAL_LINTASAN; ++baris) {
        gambarKeBuffer(4 + (PANJANG_JALAN - 1) * 2, 4 + baris, FINISH_LINE_ART, 13);
    }
}

int getTrackLength(int level) {
    return 300 + (level - 1) * 50;
}

void mainGame(int difficulty, float spawnMultiplier, float scoreMultiplier, string user) {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    Graph_init(&jalanGraph, TOTAL_CABANG);
    Graph_addEdge(&jalanGraph, 0, 1);
    Graph_addEdge(&jalanGraph, 1, 2);

    nyawa = 3;
    int score = 0;
    isiJalur();
    posisiMobil = 1;

    int spawnCounter = 0, koinSpawnCounter = 0;
    invulnerable = false;
    invulnerableStart = 0;
    koinShowCounter = 0;

    currentLevel = 1;
    distanceToFinish = getTrackLength(currentLevel);
    finishLineInserted = false;
    finishLineCrossed = false;
    afterFinishLine = false;
    finishLinePos = -1;

    while (nyawa > 0) {
        if (!finishLineInserted && distanceToFinish <= 0) {
            for (int i = 0; i < TOTAL_LINTASAN; ++i) {
                CustomQueue_setAt(&jalur[i], PANJANG_JALAN - 1, FINISH_LINE_MARKER);
            }
            finishLineInserted = true;
            finishLinePos = PANJANG_JALAN - 1;
        }

        for (int i = 0; i < LEBAR_LAYAR * TINGGI_LAYAR; ++i) {
            consoleBuffer[i] = {' ', 7};
        }

        gambarKeBuffer(0, 0, "Kontrol: W = atas, S = bawah", 15);
        gambarKeBuffer(0, 1, "Level: " + to_string(currentLevel), 14);
        gambarKeBuffer(25, 1, "| Jarak ke finish: " + to_string(distanceToFinish), 14);
        string nyawaStr = "Nyawa: ";
        for (int i = 0; i < nyawa; ++i)
            nyawaStr += "\3 ";
        gambarKeBuffer(80, 1, nyawaStr, 12);
        gambarKeBuffer(50, 1, "| Score: " + to_string(score), 11);
        if (koinShowCounter > 0) {
            gambarKeBuffer(65, 1, "+300", 14);
            koinShowCounter--;
        }

        string bingkai(PANJANG_JALAN * 2 + 6, '=');
        gambarKeBuffer(2, 3, bingkai, 8);
        gambarKeBuffer(2, 4 + TOTAL_LINTASAN, bingkai, 8);
        for (int i = 0; i < TOTAL_LINTASAN; ++i) {
            gambarKeBuffer(1, 4 + i, "||", 8);
            gambarKeBuffer(4 + PANJANG_JALAN * 2, 4 + i, "||", 8);
        }

        for (int baris = 0; baris < TOTAL_LINTASAN; baris++) {
            vector<string> contents = CustomQueue_getContents(&jalur[baris]);
            string lineContent = "";
            for (const auto &s : contents)
                lineContent += s;

            gambarKeBuffer(4, 4 + baris, lineContent, 7);

            size_t pos = 0;
            while ((pos = lineContent.find(RINTANGAN_MARKER, pos)) != string::npos) {
                gambarKeBuffer(4 + pos, 4 + baris, RINTANGAN_ART, 12);
                pos += RINTANGAN_MARKER.length();
            }

            pos = lineContent.find(KOIN_MARKER);
            while (pos != string::npos) {
                gambarKeBuffer(4 + pos, 4 + baris, KOIN_ART, 14);
                pos = lineContent.find(KOIN_MARKER, pos + KOIN_MARKER.length());
            }

            pos = lineContent.find(FINISH_LINE_MARKER);
            while (pos != string::npos) {
                gambarKeBuffer(4 + pos, 4 + baris, FINISH_LINE_ART, 13);
                pos = lineContent.find(FINISH_LINE_MARKER, pos + FINISH_LINE_MARKER.length());
            }
        }

        int posisiY_mobil = 4 + posisiMobil * 2;
        bool tampilkanMobil = true;
        if (invulnerable) {
            DWORD blinkTime = (GetTickCount() - invulnerableStart) / 150;
            tampilkanMobil = (blinkTime % 2 == 0);
        }
        if (tampilkanMobil) {
            gambarKeBuffer(6, posisiY_mobil, MOBIL[0], invulnerable ? 14 : 10);
            gambarKeBuffer(6, posisiY_mobil + 1, MOBIL[1], invulnerable ? 14 : 10);
        }

        tampilkanBuffer();

        pindahMobil();
        jalurBerjalan();
        cekTabrakan();
        updateInvulnerable();

        spawnCounter++;
        koinSpawnCounter++;

        if (!afterFinishLine && finishLineInserted) {
            int lintasanMobil = posisiMobil * 2 + 1;
            vector<string> contents = CustomQueue_getContents(&jalur[lintasanMobil]);
            if (!contents.empty() && contents[0] == FINISH_LINE_MARKER)
                afterFinishLine = true;
        }

        if (finishLineInserted && finishLinePos >= 0) {
            for (int i = 0; i < TOTAL_LINTASAN; ++i) {
                for (int j = finishLinePos + 1; j < PANJANG_JALAN; ++j) {
                    CustomQueue_setAt(&jalur[i], j, (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
                }
            }
        }

        bool allowSpawn = !finishLineInserted;
        if (allowSpawn) {
            int kecepatan = max(10, 40 - currentLevel * 2);
            int spawnRate = max(5, static_cast<int>((30 - currentLevel * 2) * spawnMultiplier));

            if (spawnCounter >= spawnRate) {
                spawnCounter = 0;
                if (!finishLineInserted || finishLinePos == -1 || finishLinePos == PANJANG_JALAN - 1) {
                    int mode = rand() % 10;
                    if (mode < 3) {
                        int lajurAman = rand() % TOTAL_CABANG;
                        vector<int> tidakAman;
                        for (int i = 0; i < TOTAL_CABANG; ++i)
                            if (i != lajurAman)
                                tidakAman.push_back(i);
                        if (tidakAman.size() >= 2) {
                            int idx1 = rand() % tidakAman.size();
                            int idx2;
                            do {
                                idx2 = rand() % tidakAman.size();
                            } while (idx2 == idx1);
                            if (!finishLineInserted || finishLinePos == -1 || finishLinePos == PANJANG_JALAN - 1) {
                                CustomQueue_updateBack(&jalur[tidakAman[idx1] * 2 + 1], RINTANGAN_MARKER);
                                CustomQueue_updateBack(&jalur[tidakAman[idx2] * 2 + 1], RINTANGAN_MARKER);
                            }
                        } else {
                            if (!finishLineInserted || finishLinePos == -1 || finishLinePos == PANJANG_JALAN - 1) {
                                CustomQueue_updateBack(&jalur[tidakAman[0] * 2 + 1], RINTANGAN_MARKER);
                            }
                        }
                    } else {
                        int lajurAman = rand() % TOTAL_CABANG;
                        vector<int> tidakAman;
                        for (int i = 0; i < TOTAL_CABANG; ++i)
                            if (i != lajurAman)
                                tidakAman.push_back(i);
                        int target = tidakAman[rand() % tidakAman.size()];
                        if (!finishLineInserted || finishLinePos == -1 || finishLinePos == PANJANG_JALAN - 1) {
                            CustomQueue_updateBack(&jalur[target * 2 + 1], RINTANGAN_MARKER);
                        }
                    }
                }
            }

            static int nextKoinFrame = 0;
            if (nextKoinFrame == 0)
                nextKoinFrame = 45 + rand() % 26;
            if (koinSpawnCounter >= nextKoinFrame) {
                koinSpawnCounter = 0;
                nextKoinFrame = 45 + rand() % 26;
                int lajurKoin = rand() % TOTAL_CABANG;
                int lintasanKoin = lajurKoin * 2 + 1;
                vector<string> isi = CustomQueue_getContents(&jalur[lintasanKoin]);
                bool aman = true;
                int cekN = 3;
                if (isi.size() >= cekN) {
                    for (int i = 1; i <= cekN; ++i) {
                        const string &cell = isi[isi.size() - i];
                        if (cell.find(RINTANGAN_MARKER) != string::npos || cell.find(KOIN_MARKER) != string::npos) {
                            aman = false;
                            break;
                        }
                    }
                } else
                    aman = false;
                if (aman && (!finishLineInserted || finishLinePos == -1 || finishLinePos == PANJANG_JALAN - 1)) {
                    CustomQueue_updateBack(&jalur[lintasanKoin], KOIN_MARKER);
                }
            }
        } else {
            for (int i = 0; i < TOTAL_LINTASAN; i++) {
                CustomQueue_updateBack(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
            }
        }

        score += static_cast<int>(scoreMultiplier * currentLevel);
        if (koinShowCounter == KOIN_SHOW_FRAMES - 1)
            score += KOIN_SCORE;

        if (finishLineInserted && cekFinishLineCross()) {
            finishLineCrossed = true;
            afterFinishLine = true;
            if (soundEnabled) {
                PlaySound(TEXT("level.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
            int transitionFrames = 50;
            for (int t = 0; t < transitionFrames; ++t) {
                for (int i = 0; i < LEBAR_LAYAR * TINGGI_LAYAR; ++i)
                    consoleBuffer[i] = {' ', 7};
                string msg = "Level " + to_string(currentLevel) + " clear!";
                gambarKeBuffer(LEBAR_LAYAR / 2 - msg.length() / 2, 0, msg, 14);
                string bingkai(PANJANG_JALAN * 2 + 6, '=');
                gambarKeBuffer(2, 3, bingkai, 8);
                gambarKeBuffer(2, 4 + TOTAL_LINTASAN, bingkai, 8);
                for (int i = 0; i < TOTAL_LINTASAN; ++i) {
                    gambarKeBuffer(1, 4 + i, "||", 8);
                    gambarKeBuffer(4 + PANJANG_JALAN * 2, 4 + i, "||", 8);
                }
                for (int baris = 0; baris < TOTAL_LINTASAN; baris++) {
                    vector<string> contents = CustomQueue_getContents(&jalur[baris]);
                    string lineContent = "";
                    for (const auto &s : contents)
                        lineContent += s;
                    gambarKeBuffer(4, 4 + baris, lineContent, 7);
                    size_t pos = lineContent.find(FINISH_LINE_MARKER);
                    while (pos != string::npos) {
                        gambarKeBuffer(4 + pos, 4 + baris, FINISH_LINE_ART, 13);
                        pos = lineContent.find(FINISH_LINE_MARKER, pos + FINISH_LINE_MARKER.length());
                    }
                }
                int posisiY_mobil = 4 + posisiMobil * 2;
                gambarKeBuffer(6, posisiY_mobil, MOBIL[0], 10);
                gambarKeBuffer(6, posisiY_mobil + 1, MOBIL[1], 10);
                tampilkanBuffer();
                pindahMobil();
                jalurBerjalan();
                cekTabrakan();
                updateInvulnerable();
                for (int i = 0; i < TOTAL_LINTASAN; i++) {
                    CustomQueue_updateBack(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
                }
                if (finishLineInserted && finishLinePos >= 0) {
                    for (int i = 0; i < TOTAL_LINTASAN; ++i) {
                        for (int j = finishLinePos + 1; j < PANJANG_JALAN; ++j) {
                            CustomQueue_setAt(&jalur[i], j, (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
                        }
                    }
                }
                Sleep(max(10, 30));
            }
            currentLevel++;
            distanceToFinish = getTrackLength(currentLevel);
            finishLineInserted = false;
            finishLineCrossed = false;
            afterFinishLine = false;
            finishLinePos = -1;
            isiJalur();
            posisiMobil = 1;
            invulnerable = false;
            invulnerableStart = 0;
            koinShowCounter = 0;
            spawnCounter = 0;
            koinSpawnCounter = 0;
            continue;
        }

        if (!finishLineInserted && distanceToFinish > 0)
            distanceToFinish--;

        Sleep(max(10, 40 - currentLevel * 2));
    }

    tampilkanEndScreen(score, user.empty(), user);
    simpanScore(user, score);

    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    SetConsoleTextAttribute(hConsole, 7);
}