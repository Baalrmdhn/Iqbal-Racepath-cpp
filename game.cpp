#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp> // Library JSON modern C++ (https://github.com/nlohmann/json)
#include <mmsystem.h>        // Tambahkan untuk PlaySound
#pragma comment(lib, "winmm.lib")
bool soundEnabled = true;
using namespace std;

struct CustomQueue
{
    int top;
    string isi[50];
    static const int ukuran = 50;
};

void CustomQueue_init(CustomQueue *q)
{
    q->top = 0;
}

void CustomQueue_clear(CustomQueue *q)
{
    q->top = 0;
}

bool CustomQueue_isEmpty(CustomQueue *q)
{
    return (q->top == 0);
}

bool CustomQueue_isFull(CustomQueue *q)
{
    return (q->top >= CustomQueue::ukuran);
}

void CustomQueue_enqueue(CustomQueue *q, const string &data)
{
    if (!CustomQueue_isFull(q))
    {
        q->isi[q->top] = data;
        q->top++;
    }
}

void CustomQueue_dequeue(CustomQueue *q)
{
    if (!CustomQueue_isEmpty(q))
    {
        for (int i = 0; i < q->top - 1; i++)
        {
            q->isi[i] = q->isi[i + 1];
        }
        q->top--;
    }
}

vector<string> CustomQueue_getContents(CustomQueue *q)
{
    vector<string> contents;
    for (int i = 0; i < q->top; i++)
    {
        contents.push_back(q->isi[i]);
    }
    return contents;
}

void CustomQueue_updateBack(CustomQueue *q, const string &data)
{
    if (!CustomQueue_isEmpty(q))
    {
        q->isi[q->top - 1] = data;
    }
}

void CustomQueue_setAt(CustomQueue *q, int idx, const string &val)
{
    if (idx >= 0 && idx < q->top)
    {
        q->isi[idx] = val;
    }
}

struct Graph
{
    int numVertices;
    vector<int> *adjLists;
};

void Graph_init(Graph *g, int vertices)
{
    g->numVertices = vertices;
    g->adjLists = new vector<int>[vertices];
}

void Graph_destroy(Graph *g)
{
    delete[] g->adjLists;
    g->adjLists = nullptr;
    g->numVertices = 0;
}

void Graph_addEdge(Graph *g, int src, int dest)
{
    if (src >= 0 && src < g->numVertices && dest >= 0 && dest < g->numVertices)
    {
        g->adjLists[src].push_back(dest);
        g->adjLists[dest].push_back(src);
    }
}

bool Graph_isConnected(Graph *g, int src, int dest)
{
    if (src < 0 || src >= g->numVertices || dest < 0 || dest >= g->numVertices)
    {
        return false;
    }
    for (int neighbor : g->adjLists[src])
    {
        if (neighbor == dest)
        {
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
const int KOIN_SHOW_FRAMES = 15; // Tampilkan "+300" selama 15 frame (~0.3-0.5 detik)

int posisiMobil = 1;

// Tambah variabel global untuk nyawa dan invulnerable
int nyawa = 3;
bool invulnerable = false;
DWORD invulnerableStart = 0;
const DWORD INVULNERABLE_DURATION = 2000; // 2 detik

// Tambah variabel global untuk efek koin
int koinShowCounter = 0;

using json = nlohmann::json;

// Fungsi untuk menyimpan skor ke file JSON
void simpanScore(const string &user, int score)
{
    json data;
    ifstream in("scores.json");
    if (in.is_open())
    {
        try
        {
            in >> data;
        }
        catch (...)
        {
            data = json::array();
        }
        in.close();
    }
    else
    {
        data = json::array();
    }
    data.push_back({{"user", user}, {"score", score}});
    ofstream out("scores.json");
    out << data.dump(4);
    out.close();
}

void gambarKeBuffer(int x, int y, const string &teks, WORD atribut = 7)
{
    if (y >= TINGGI_LAYAR || x >= LEBAR_LAYAR)
        return;
    for (size_t i = 0; i < teks.length(); ++i)
    {
        if (x + i < LEBAR_LAYAR)
        {
            consoleBuffer[x + i + LEBAR_LAYAR * y].Char.AsciiChar = teks[i];
            consoleBuffer[x + i + LEBAR_LAYAR * y].Attributes = atribut;
        }
    }
}
void tampilkanBuffer()
{
    COORD bufferSize = {LEBAR_LAYAR, TINGGI_LAYAR};
    COORD characterPos = {0, 0};
    SMALL_RECT writeArea = {0, 0, LEBAR_LAYAR - 1, TINGGI_LAYAR - 1};
    WriteConsoleOutputA(hConsole, consoleBuffer, bufferSize, characterPos, &writeArea);
}

void isiJalur()
{
    srand((unsigned int)time(0));
    for (int i = 0; i < TOTAL_LINTASAN; i++)
    {
        CustomQueue_init(&jalur[i]);
        for (int j = 0; j < PANJANG_JALAN; j++)
        {
            CustomQueue_enqueue(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
        }
    }
}

void cekTabrakan()
{
    // Cek apakah posisi mobil bertabrakan dengan rintangan atau koin
    int lintasanMobil = posisiMobil * 2 + 1;
    vector<string> contents = CustomQueue_getContents(&jalur[lintasanMobil]);
    // Cek collision rintangan (2 cell terdepan)
    if (contents.size() > 1 && !invulnerable)
    {
        string depan = contents[0] + contents[1];
        if (depan.find(RINTANGAN_MARKER) != string::npos)
        {
            // Sound effect rintangan
            if (soundEnabled)
            {
                PlaySound(TEXT("hit.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
            nyawa--;
            invulnerable = true;
            invulnerableStart = GetTickCount();
        }
    }
    // Cek collision koin (perbesar: 4 cell terdepan)
    if (contents.size() > 0)
    {
        int maxCek = min(4, (int)contents.size());
        string depanKoin = "";
        for (int i = 0; i < maxCek; ++i)
        {
            depanKoin += contents[i];
        }
        size_t posKoin = depanKoin.find(KOIN_MARKER);
        if (posKoin != string::npos)
        {
            koinShowCounter = KOIN_SHOW_FRAMES;
            // Sound effect koin
            if (soundEnabled)
            {
                PlaySound(TEXT("coin.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
            // Tentukan cell mana yang kena koin, lalu hapus koin dari cell tersebut
            int cellIdx = 0, charCount = 0;
            for (; cellIdx < maxCek; ++cellIdx)
            {
                if (posKoin < charCount + contents[cellIdx].size())
                    break;
                charCount += contents[cellIdx].size();
            }
            if (cellIdx < maxCek)
            {
                CustomQueue_setAt(&jalur[lintasanMobil], cellIdx, (lintasanMobil % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
            }
        }
    }
}

void updateInvulnerable()
{
    if (invulnerable && (GetTickCount() - invulnerableStart >= INVULNERABLE_DURATION))
    {
        invulnerable = false;
    }
}

void pindahMobil()
{
    if (_kbhit())
    {
        char input = _getch();
        if (input == 'w' || input == 'W')
        {
            if (Graph_isConnected(&jalanGraph, posisiMobil, posisiMobil - 1))
            {
                posisiMobil--;
            }
        }
        else if (input == 's' || input == 'S')
        {
            // Cek ke graph apakah ada jalur dari posisi sekarang ke bawah
            if (Graph_isConnected(&jalanGraph, posisiMobil, posisiMobil + 1))
            {
                posisiMobil++;
            }
        }
    }
}

void jalurBerjalan()
{
    for (int i = 0; i < TOTAL_LINTASAN; i++)
    {
        CustomQueue_dequeue(&jalur[i]);
        CustomQueue_enqueue(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
    }
}

void tampilkanEndScreen(int score, bool failed, const string &user)
{
    for (int i = 0; i < LEBAR_LAYAR * TINGGI_LAYAR; ++i)
    {
        consoleBuffer[i] = {' ', 7};
    }

    string msg = "GAME OVER!";
    string msg2 = "";
    string msg3 = "Nama: " + user;
    string msg4 = "Skor: " + to_string(score);

    // Hitung lebar kotak (ambil yang terpanjang)
    int boxWidth = max({msg.length(), msg3.length(), msg4.length()}) + 6;
    int boxHeight = 6;
    int boxX = LEBAR_LAYAR / 2 - boxWidth / 2;
    int boxY = TINGGI_LAYAR / 2 - boxHeight / 2;

    // Gambar kotak
    string topBot = "+" + string(boxWidth - 2, '-') + "+";
    gambarKeBuffer(boxX, boxY, topBot, 14);
    for (int i = 1; i < boxHeight - 1; ++i)
    {
        gambarKeBuffer(boxX, boxY + i, "|" + string(boxWidth - 2, ' ') + "|", 14);
    }
    gambarKeBuffer(boxX, boxY + boxHeight - 1, topBot, 14);

    // Gambar pesan di tengah kotak (benar-benar di tengah)
    gambarKeBuffer(boxX + (boxWidth - msg.length()) / 2, boxY + 1, msg, 12);
    gambarKeBuffer(boxX + (boxWidth - msg3.length()) / 2, boxY + 3, msg3, 11);
    gambarKeBuffer(boxX + (boxWidth - msg4.length()) / 2, boxY + 4, msg4, 11);

    tampilkanBuffer();
    Sleep(3000);
}

void transisiKosong(int durasiMs)
{
    DWORD start = GetTickCount();
    while (GetTickCount() - start < durasiMs && nyawa > 0)
    {
        for (int i = 0; i < TOTAL_LINTASAN; i++)
        {
            CustomQueue_dequeue(&jalur[i]);
            CustomQueue_enqueue(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
        }

        for (int i = 0; i < LEBAR_LAYAR * TINGGI_LAYAR; ++i)
            consoleBuffer[i] = {' ', 7};

        gambarKeBuffer(0, 0, "Transisi ke Level Berikutnya...", 14);
        string nyawaStr = "Nyawa: ";
        for (int i = 0; i < nyawa; ++i)
            nyawaStr += "\3 ";
        gambarKeBuffer(80, 1, nyawaStr, 12);

        int posisiY_mobil = 4 + posisiMobil * 2;
        gambarKeBuffer(6, posisiY_mobil, MOBIL[0], 10);
        gambarKeBuffer(6, posisiY_mobil + 1, MOBIL[1], 10);

        tampilkanBuffer();
        pindahMobil();
        cekTabrakan();
        updateInvulnerable();

        Sleep(30);
    }
}

// Tambah parameter untuk difficulty dan spawnMultiplier
void mainGame(int difficulty, float spawnMultiplier, float scoreMultiplier, string user)
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    Graph_init(&jalanGraph, TOTAL_CABANG);
    Graph_addEdge(&jalanGraph, 0, 1);
    Graph_addEdge(&jalanGraph, 1, 2);

    nyawa = 3;
    int level = 1;
    int score = 0;
    DWORD waktuMulaiLevel = GetTickCount();
    DWORD durasiLevel = 20000; // 20 detik awal
    DWORD noSpawnUntil = 0;

    isiJalur();
    posisiMobil = 1;

    int spawnCounter = 0, koinSpawnCounter = 0;
    invulnerable = false;
    invulnerableStart = 0;
    koinShowCounter = 0;

    while (nyawa > 0)
    {
        DWORD sekarang = GetTickCount();

        // Naik level
        if (sekarang - waktuMulaiLevel >= durasiLevel)
        {
            level++;
            waktuMulaiLevel = sekarang;
            durasiLevel += 2000;
            noSpawnUntil = sekarang + 3500; // durasi lintasan bersih
        }

        int kecepatan = max(10, 40 - level * 2);
        int spawnRate = max(5, static_cast<int>((30 - level * 2) * spawnMultiplier));

        // Bersihkan layar
        for (int i = 0; i < LEBAR_LAYAR * TINGGI_LAYAR; ++i)
        {
            consoleBuffer[i] = {' ', 7};
        }

        // HUD
        gambarKeBuffer(0, 0, "Kontrol: W = atas, S = bawah", 15);
        gambarKeBuffer(0, 1, "Level: " + to_string(level), 14);
        DWORD waktuTersisa = durasiLevel - (sekarang - waktuMulaiLevel);
        gambarKeBuffer(25, 1, "| Waktu level: " + to_string(waktuTersisa / 1000) + " detik", 14);
        string nyawaStr = "Nyawa: ";
        for (int i = 0; i < nyawa; ++i)
            nyawaStr += "\3 ";
        gambarKeBuffer(80, 1, nyawaStr, 12);
        gambarKeBuffer(50, 1, "| Score: " + to_string(score), 11);
        if (koinShowCounter > 0)
        {
            gambarKeBuffer(65, 1, "+300", 14);
            koinShowCounter--;
        }

        // Bingkai lintasan
        string bingkai(PANJANG_JALAN * 2 + 6, '=');
        gambarKeBuffer(2, 3, bingkai, 8);
        gambarKeBuffer(2, 4 + TOTAL_LINTASAN, bingkai, 8);
        for (int i = 0; i < TOTAL_LINTASAN; ++i)
        {
            gambarKeBuffer(1, 4 + i, "||", 8);
            gambarKeBuffer(4 + PANJANG_JALAN * 2, 4 + i, "||", 8);
        }

        // Gambar lintasan, rintangan, koin
        for (int baris = 0; baris < TOTAL_LINTASAN; baris++)
        {
            vector<string> contents = CustomQueue_getContents(&jalur[baris]);
            string lineContent = "";
            for (const auto &s : contents)
                lineContent += s;

            gambarKeBuffer(4, 4 + baris, lineContent, 7);

            size_t pos = 0;
            while ((pos = lineContent.find(RINTANGAN_MARKER, pos)) != string::npos)
            {
                gambarKeBuffer(4 + pos, 4 + baris, RINTANGAN_ART, 12);
                pos += RINTANGAN_MARKER.length();
            }

            pos = lineContent.find(KOIN_MARKER);
            while (pos != string::npos)
            {
                gambarKeBuffer(4 + pos, 4 + baris, KOIN_ART, 14);
                pos = lineContent.find(KOIN_MARKER, pos + KOIN_MARKER.length());
            }
        }

        // Gambar mobil
        int posisiY_mobil = 4 + posisiMobil * 2;
        bool tampilkanMobil = true;
        if (invulnerable)
        {
            DWORD blinkTime = (sekarang - invulnerableStart) / 150;
            tampilkanMobil = (blinkTime % 2 == 0);
        }
        if (tampilkanMobil)
        {
            gambarKeBuffer(6, posisiY_mobil, MOBIL[0], invulnerable ? 14 : 10);
            gambarKeBuffer(6, posisiY_mobil + 1, MOBIL[1], invulnerable ? 14 : 10);
        }

        tampilkanBuffer();

        // Gerakan & tabrakan
        pindahMobil();
        jalurBerjalan();
        cekTabrakan();
        updateInvulnerable();

        spawnCounter++;
        koinSpawnCounter++;

        // Bersihkan belakang lintasan jika masa transisi
        if (sekarang < noSpawnUntil)
        {
            for (int i = 0; i < TOTAL_LINTASAN; i++)
            {
                CustomQueue_updateBack(&jalur[i], (i % 2 == 1) ? SIMBOL_JALAN : SIMBOL_KOSONG);
            }
        }

        // Spawn rintangan dan koin hanya jika sudah lewat masa transisi
        if (sekarang >= noSpawnUntil)
        {
            // === Spawn Rintangan ===
            if (spawnCounter >= spawnRate)
            {
                spawnCounter = 0;
                int mode = rand() % 10;
                if (mode < 3)
                {
                    int lajurAman = rand() % TOTAL_CABANG;
                    vector<int> tidakAman;
                    for (int i = 0; i < TOTAL_CABANG; ++i)
                        if (i != lajurAman)
                            tidakAman.push_back(i);
                    if (tidakAman.size() >= 2)
                    {
                        int idx1 = rand() % tidakAman.size();
                        int idx2;
                        do
                        {
                            idx2 = rand() % tidakAman.size();
                        } while (idx2 == idx1);
                        CustomQueue_updateBack(&jalur[tidakAman[idx1] * 2 + 1], RINTANGAN_MARKER);
                        CustomQueue_updateBack(&jalur[tidakAman[idx2] * 2 + 1], RINTANGAN_MARKER);
                    }
                    else
                    {
                        CustomQueue_updateBack(&jalur[tidakAman[0] * 2 + 1], RINTANGAN_MARKER);
                    }
                }
                else
                {
                    int lajurAman = rand() % TOTAL_CABANG;
                    vector<int> tidakAman;
                    for (int i = 0; i < TOTAL_CABANG; ++i)
                        if (i != lajurAman)
                            tidakAman.push_back(i);
                    int target = tidakAman[rand() % tidakAman.size()];
                    CustomQueue_updateBack(&jalur[target * 2 + 1], RINTANGAN_MARKER);
                }
            }

            // === Spawn Koin ===
            static int nextKoinFrame = 0;
            if (nextKoinFrame == 0)
                nextKoinFrame = 45 + rand() % 26;
            if (koinSpawnCounter >= nextKoinFrame)
            {
                koinSpawnCounter = 0;
                nextKoinFrame = 45 + rand() % 26;
                int lajurKoin = rand() % TOTAL_CABANG;
                int lintasanKoin = lajurKoin * 2 + 1;
                vector<string> isi = CustomQueue_getContents(&jalur[lintasanKoin]);
                bool aman = true;
                int cekN = 3;
                if (isi.size() >= cekN)
                {
                    for (int i = 1; i <= cekN; ++i)
                    {
                        const string &cell = isi[isi.size() - i];
                        if (cell.find(RINTANGAN_MARKER) != string::npos || cell.find(KOIN_MARKER) != string::npos)
                        {
                            aman = false;
                            break;
                        }
                    }
                }
                else
                    aman = false;
                if (aman)
                    CustomQueue_updateBack(&jalur[lintasanKoin], KOIN_MARKER);
            }
        }

        // Skor per frame
        score += static_cast<int>(scoreMultiplier * level);
        if (koinShowCounter == KOIN_SHOW_FRAMES - 1)
            score += KOIN_SCORE;

        Sleep(kecepatan);
    }

    tampilkanEndScreen(score, user.empty(), user);
    simpanScore(user, score);

    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    SetConsoleTextAttribute(hConsole, 7);
}
