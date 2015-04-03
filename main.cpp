/*
Copyright (C) 2014  surgeon

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

You can find me on Symulator MaSzyna forum at http://eu07.pl/forum
E-mail: surgus at gmail (dot) com
*/

// #include <omp.h>  //testy

#include <dirent.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
// #include <string> //robi problemy podczas kompilacji na syetemie linux
#include <string.h>
#include <sstream>
#include <vector>
#include <math.h>
#include <ctime>
#include <algorithm>
#include <complex>
#include "version.h"

// #ifdef __cplusplus
// extern "C" {
// #endif
// #include "triangle.h"
// #ifdef __cplusplus
// }
// #endif

// #include <random>

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

//Tymczasowy wybor miedzy plikami .hgt (SRTM 3 arc sec) a .dt2 (SRTM 1 arc sec)
#ifndef DT2
#define DT2
#endif // DT2

#ifndef HGT
#define HGT
#endif // HGT

//Automatyczne zalesianie wl/wyl (tworzy dlugo i duzy plik z drzewami
//bardzo dlugo sie wczytuje i zmniejsza fps symulatora)
#ifndef zalesianie
//#define zalesianie
#endif // zalesianie

template < class T >
std::string to_string( T t )
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

struct wierzcholek {
    double x;
    double y;
    double z;
};

struct triangle {
    double x1;
    double y1;
    double z1;
    double x2;
    double y2;
    double z2;
    double x3;
    double y3;
    double z3;
    double normalX;
    double normalY;
    double normalZ;
    double Ziloczyn;
    int nastepnyTrojkat;
};

struct wypukla {
    double x;
    double y;
    double z;
    unsigned int id;
};

struct punktyTorow {
    double xp1;
    double yp1;
    double xp2;
    double yp2;
};

struct by_xy {
    bool operator()(wierzcholek const &a, wierzcholek const &b) {
        if (a.y < b.y) return true;
        else if (b.y < a.y) return false;
        else
            return a.x < b.x;
    }
};

struct by_yx {
    bool operator()(wierzcholek const &a, wierzcholek const &b) {
        if (a.x < b.x) return true;
        else if (b.x < a.x) return false;
        else
            return a.y < b.y;
    }
};

struct by_x {
    bool operator()(wierzcholek const &a, wierzcholek const &b) {
        return a.x < b.x;
    }
};

void sortowanieWierzcholkow(double &a, double &b, double &c) {
    if (a > b) std::swap(a,b);
    if (b > c) std::swap(b,c);
    if (a > b) std::swap(a,b);
}

double sprawdzKatPolarny(double x1,double y1, double x2, double y2, double x3, double y3) {
    return (x2 - x1)*(y3 - y1) - (y2 - y1)*(x3 - x1);
}

bool czyPunktwTrojkacie(double px, double py, double p0x, double p0y, double p1x, double p1y, double p2x, double p2y) {
    double pole = 0, znak = 0, t = 0, s = 0;
    pole = 1/2 * (-p1y * p2x + p0y * (-p1x + p2x) + p0x * (p1y - p2y) + p1x * p2y);
    if (pole < 0) {
        znak = -1;
    } else {
        znak = 1;
    }
    s = (p0y * p2x - p0x * p2y + (p2y - p0y) * px + (p0x - p2x) * py) * znak;
    t = (p0x * p1y - p0y * p1x + (p0y - p1y) * px + (p1x - p0x) * py) * znak;
    if ((s > 0) && (t > 0) && ((s + t) < 2 * pole * znak)) {
        return true;
    } else {
        return false;
    }
}

// Pobranie dlugosci pliku
long getFileSize(FILE *file) {
    long lCurPos, lEndPos;
    lCurPos = ftell(file);
    fseek(file, 0, 2);
    lEndPos = ftell(file);
    fseek(file, lCurPos, 0);
    return lEndPos;
}

void odczytPunktowTorow(std::vector<std::vector<unsigned int> > &refTablica, std::vector<std::vector<unsigned int> > &refTablicaBrakow,
                        double ExportX, double ExportY, unsigned int szerokosc, unsigned int &refKorektaX, unsigned int &refKorektaY, unsigned int &refKorektaXbraki,
                        unsigned int &refKorektaYbraki, unsigned int &refWierszeTablicy, unsigned int &refKolumnyTablicy, unsigned int &refWierszeTablicyBrakow,
                        unsigned int &refKolumnyTablicyBrakow) {
    std::string linia ("");
    std::string ln ("");
    std::string szukanyString (" track ");
    std::string nazwaPlikuZTorami ("EXPORT.SCN");
    unsigned int liczbaLiniiTorow = 0, wyraz = 0, testXmax = 0, testYmax = 0, testXmin = 900000, testYmin = 900000, testXmaxBraki = 0, testYmaxBraki = 0,
                 testXminBraki = 900000, testYminBraki = 900000;
    bool flagTory = false;

    // Otwarcie pliku tylko do odczytu z torami dla sprawdzenia szerokosci scenerii
    std::cout << "Otwieram plik z torami..." << "\n";
    std::ifstream plik1;
    plik1.open(nazwaPlikuZTorami.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPlikuZTorami << "\n";
        std::cin.get();
    }
// W petli szukamy w otwartym pliku torow oraz ich punktow p1.

    while(!plik1.eof()) {
        std::string rodzajTrack ("");
        std::string rodzajNode ("");
        std::getline(plik1, linia);
// ...patrzymy czy zawiera
        int znaleziono = (linia.find(szukanyString));
// Jesli zawiera, to rozbijamy linie na elementy
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            std::istringstream ln(linia);
            wyraz = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po spacji)
            while (!ln.eof()) {
                std::string temp ("");
                std::getline(ln, temp, ' ');
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
                if (temp == " ") {
                    flagTory = true;
                    break;
                }
                if (flagTory == true) break;
                if (wyraz == 4) rodzajNode = temp;
                if (wyraz == 5) rodzajTrack = temp;
// Jesli chodzi o tory lub drogi (ale wywalamy zwrotnice, szkoda na nie czasu).
                if ((rodzajNode == "track") && (rodzajTrack != "switch")) {
                    unsigned int wyraz1 = 0;
// Jesli znalazlo track, to bierze dwie nastepne linijki z pliku tekstowego by uzyskac x1, y1 - czyli p1
//                    #pragma omp parallel for default(shared) private(wyraz1)
                    for (wyraz1 = 0; wyraz1 <= 1; ++wyraz1) {
                        unsigned int trackX = 0, trackY = 0;
                        std::getline(plik1, linia);
                        std::istringstream ln(linia);
                        wyraz = 0;
                        while (!ln.eof()) {
                            std::getline(ln, temp, ' ');
                            if (wyraz1 == 1) { //x
                                if (wyraz == 0) {
                                    trackX = (atoi(temp.c_str()) * -1) + ExportX; // W przypadku osi x chce aby x roslo w "prawo"
// Wyznaczamy szerokosc scenerii w osi x
                                    if (testXmax < trackX) testXmax = trackX;
                                    if (testXmin > trackX) testXmin = trackX;
                                }
                                if (wyraz == 2) { //y
                                    trackY = atoi(temp.c_str()) + ExportY; //Y rosnie w "gore", wiec nie trzeba mnozyc  przez -1
// Wyznaczamy szerokosc w osi y
                                    if (testYmax < trackY) testYmax = trackY;
                                    if (testYmin > trackY) testYmin = trackY;
                                }
                            }
                            ++wyraz;
                        }
                    }
                    if (flagTory == true) break;
                    rodzajNode = "";
                    break;
                }
                ++wyraz;
            }
        }
    }
// Zwiekszamy szerokosc, bo tablica tez jest nieco szersza (tablica o 2, a tutaj zwiekszamy o 5... rozrzutnosc?)
    testXminBraki = testXmin - (65 * 50);
    testXmaxBraki = testXmax + (65 * 50);
    testYminBraki = testYmin - (65 * 50);
    testYmaxBraki = testYmax + (65 * 50);

    testXmin -= 5 * szerokosc;
    testXmax += 5 * szerokosc;
    testYmin -= 5 * szerokosc;
    testYmax += 5 * szerokosc;
// Ustalamy rozmiar tablicy i resetujemy zawartosc
    refWierszeTablicy = (testXmax / szerokosc) - (testXmin / szerokosc);
    refKolumnyTablicy = (testYmax / szerokosc) - (testYmin / szerokosc);
    refTablica.resize(refWierszeTablicy);
    for (unsigned int i = 0; i < refWierszeTablicy; ++i) {
        refTablica[i].resize(refKolumnyTablicy);
    }
    for (unsigned int i = 0; i < refWierszeTablicy; ++i) {
        for (unsigned int ii = 0; ii < refKolumnyTablicy; ++ii) {
            refTablica[i][ii] = 0;
        }
    }
// Te zmienne w dalszym dzialaniu programu beda kluczowe do prawidlowego przeszukiwania tablicy
    refKorektaX = testXmin;
    refKorektaY = testYmin;

    refWierszeTablicyBrakow = (testXmaxBraki / 50) - (testXminBraki / 50);
    refKolumnyTablicyBrakow = (testYmaxBraki / 50) - (testYminBraki / 50);
    refTablicaBrakow.resize(refWierszeTablicyBrakow);
    for (unsigned int i = 0; i < refWierszeTablicyBrakow; ++i) {
        refTablicaBrakow[i].resize(refKolumnyTablicyBrakow);
    }
    for (unsigned int i = 0; i < refWierszeTablicyBrakow; ++i) {
        for (unsigned int ii = 0; ii < refKolumnyTablicyBrakow; ++ii) {
            refTablicaBrakow[i][ii] = 0;
        }
    }
    refKorektaXbraki = testXminBraki;
    refKorektaYbraki = testYminBraki;

    plik1.close();

// POnowne otwarcie pliku tylko do odczytu z torami w celu zapelnienia zoptymalizowanej tablicy
    std::cout << "Ponownie otwieram plik z torami i wczytuje do pamieci punkty x1 i y1 torow..." << "\n";

    plik1.open(nazwaPlikuZTorami.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPlikuZTorami << "\n";
        std::cin.get();
    }
// W petli szukamy w otwartym pliku torow oraz ich punktow p1.

    while(!plik1.eof()) {
        std::string rodzajTrack ("");
        std::string rodzajNode ("");
        std::getline(plik1, linia);
// ...patrzymy czy zawiera
        int znaleziono = (linia.find(szukanyString));
// Jesli zawiera, to rozbijamy linie na elementy
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            std::istringstream ln(linia);
            wyraz = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po spacji)
            while (!ln.eof()) {
                std::string temp ("");
                std::getline(ln, temp, ' ');
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
                if (temp == " ") {
                    flagTory = true;
                    break;
                }
                if (flagTory == true) break;
                if (wyraz == 4) rodzajNode = temp;
                if (wyraz == 5) rodzajTrack = temp;
// Jesli chodzi o tory lub drogi (ale wywalamy zwrotnice, szkoda na nie czasu).
                if ((rodzajNode == "track") && (rodzajTrack != "switch")) {
                    unsigned int wyraz1 = 0;
// Jesli znalazlo track, to bierze dwie nastepne linijki z pliku tekstowego by uzyskac x1, y1 - czyli p1
//                    #pragma omp parallel for default(shared) private(wyraz1)
                    for (wyraz1 = 0; wyraz1 <= 1; ++wyraz1) {
                        unsigned int trackX = 0, trackY = 0;
                        std::getline(plik1, linia);
                        std::istringstream ln(linia);
                        wyraz = 0;
                        while (!ln.eof()) {
                            std::getline(ln, temp, ' ');
                            if (wyraz1 == 1) {
                                if (wyraz == 0) {
                                    trackX = (atoi(temp.c_str()) * -1) + ExportX;
                                }
                                if (wyraz == 2) {
                                    trackY = atoi(temp.c_str()) + ExportY;
                                    for (int j = -2; j < 3; ++j) {
                                        for (int jj = -2; jj < 3; ++jj) {
                                            refTablica[((trackX - refKorektaX) / szerokosc) + j][((trackY - refKorektaY) / szerokosc) + jj] = 1;
                                        }
                                    }
                                    ++liczbaLiniiTorow;
                                }
                            }
                            ++wyraz;
                        }
                    }
                    if (flagTory == true) break;
                    rodzajNode = "";
                    break;
                }
                ++wyraz;
            }
        }
    }
    std::cout << "Liczba znalezionych odcinkow torow i drog (bez zwrotnic): " << liczbaLiniiTorow << "\n\n";
// Zamkniecie pliku z torami
    plik1.close();
}

void odczytPunktowTorowZGwiazdka(std::vector<punktyTorow> &refToryZGwiazdka, double ExportX, double ExportY) {
    std::string linia ("");
    std::string ln ("");
    std::string szukanyString (" track ");
    std::string nazwaPlikuZTorami ("EXPORT.SCN");
    unsigned int liczbaLiniiTorow = 0, wyraz = 0;
// Otwarcie pliku tylko do odczytu z torami
    std::cout << "Otwieram plik z torami i wczytuje do pamieci punkty p1 i p2 torow z gwiazdka..." << "\n";
    std::ifstream plik1;
    plik1.open(nazwaPlikuZTorami.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPlikuZTorami << "\n";
        std::cin.get();
    }
// W petli szukamy w otwartym pliku torow oraz ich punktow p1.
    while(!plik1.eof()) {
        bool flagTory = false;
        std::string rodzajTrack ("");
        std::string rodzajNode ("");
        std::string nazwaToru ("");
        bool mamyGo = false;
        std::getline(plik1, linia);
// ...patrzymy czy zawiera
        int znaleziono = (linia.find(szukanyString));
// Jesli zawiera, to rozbijamy linie na elementy
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            std::istringstream ln(linia);
            wyraz = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po spacji)
            while (!ln.eof()) {
                std::string temp ("");
                std::getline(ln, temp, ' ');
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
                if (temp == " ") {
                    flagTory = true;
                    break;
                }
                if (flagTory == true) break;
                if (temp.size() > 4) {
                    if (wyraz == 3) {
                        nazwaToru = temp;
                        if (nazwaToru.substr(4,1) == "*") {
                            mamyGo = true;
                        }
                    }
                }
                if (wyraz == 4) rodzajNode = temp;
                if (wyraz == 5) rodzajTrack = temp;
// Jesli chodzi o tory lub drogi (ale wywalamy zwrotnice, szkoda na nie czasu).
                if ((rodzajNode == "track") && (rodzajTrack != "switch") && (mamyGo)) {
                    double trackXp1 = 0, trackYp1 = 0, trackXp2 = 0, trackYp2 = 0;
                    unsigned int wyraz1 = 0;
// Jesli znalazlo track, to bierze dwie nastepne linijki z pliku tekstowego by uzyskac x1, y1 - czyli p1
//                    #pragma omp parallel for default(shared) private(wyraz1)
                    for (wyraz1 = 0; wyraz1 < 5; ++wyraz1) {
                        std::getline(plik1, linia);
                        std::istringstream ln(linia);
                        wyraz = 0;
                        while (!ln.eof()) {
                            std::getline(ln, temp, ' ');
                            if (wyraz1 == 1) {
                                if (wyraz == 0) {
                                    trackXp1 = (atof(temp.c_str()) * -1.0) + ExportX;
                                }
                                if (wyraz == 2) {
                                    trackYp1 = atof(temp.c_str()) + ExportY;

                                }
                            }
                            if (wyraz1 == 4) {
                                if (wyraz == 0) {
                                    trackXp2 = (atof(temp.c_str()) * -1.0) + ExportX;
                                }
                                if (wyraz == 2) {
                                    trackYp2 = atof(temp.c_str()) + ExportY;
                                    refToryZGwiazdka.push_back(punktyTorow{trackXp1, trackYp1, trackXp2, trackYp2});
                                    ++liczbaLiniiTorow;
                                    flagTory = true;
                                }
                            }
                            ++wyraz;
                        }
                    }
                }
                ++wyraz;
            }
        }
    }
    std::cout << "Liczba znalezionych odcinkow torow z gwiazdka: " << liczbaLiniiTorow << "\n\n";
// Zamkniecie pliku z torami
    plik1.close();
}

void odczytWierzcholkowTriangles(std::vector<wierzcholek> &refWierzcholki, double ExportX, double ExportY,
                                 std::vector<punktyTorow> &refToryZGwiazdka) {
    std::string linia ("");
    std::string ln ("");
    std::string szukanyString (" triangles ");
    std::string nazwaPlikuZTrojkatami ("EXPORT.SCN");
    unsigned int liczbaLiniiWierzcholkow = 0, nrLinii = 0, llw = 0;
// Otwarcie pliku tylko do odczytu z torami
    std::cout << "Otwieram plik ze sceneria i wczytuje do pamieci wierzcholki x, y i z trojkatow..." << "\n";
    std::ifstream plik1;
    plik1.open(nazwaPlikuZTrojkatami.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPlikuZTrojkatami << "\n";
        std::cin.get();
    }
// W petli szukamy w otwartym pliku torow oraz ich punktow p1.

    while(!plik1.eof()) {
        std::getline(plik1, linia);
        ++nrLinii;
// ...patrzymy czy zawiera
        int znaleziono = (linia.find(szukanyString));
// Jesli zawiera, to rozbijamy linie na elementy
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            std::istringstream ln(linia);
            bool flagPrzerwij = false;
            std::string temp = "";
// Jesli znalazlo triangles, to bierze 100 nastepnych linijek z pliku tekstowego by uzyskac x, y i z - czyli wierzcholki trojkata
//          #pragma omp parallel for default(shared) private(wyraz1)
            for (unsigned int wyraz = 0; wyraz < 100; ++wyraz) {
                bool doOdrzutu = false;
                double x = 0, y = 0, z = 0;
                std::getline(plik1, linia);
                ++nrLinii;
                std::istringstream ln(linia);
                unsigned int wyraz1 = 0;
                if (flagPrzerwij == true) break;
                if (linia == "endtri") break;
                while (std::getline(ln, temp, ' ')) {
                    if (wyraz1 == 0) {
                        x = (atof(temp.c_str()) * -1.0) + ExportX;
                    }
                    if (wyraz1 == 1) {
                        z = atof(temp.c_str());
                    }
                    if (wyraz1 == 2) {
                        y = atof(temp.c_str()) + ExportY;
                        for (unsigned int j = 0; j < refToryZGwiazdka.size(); ++j) {
                            bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
                            double wektorP2P1x = refToryZGwiazdka[j].xp1 - refToryZGwiazdka[j].xp2;
                            double wektorP2P1y = refToryZGwiazdka[j].yp1 - refToryZGwiazdka[j].yp2;

                            int resztax1toru = (int)refToryZGwiazdka[j].xp1 % 1000;
                            int resztay1toru = (int)refToryZGwiazdka[j].yp1 % 1000;

                            if (resztax1toru == 0) waznyX = true;
                            if (resztay1toru == 0) waznyY = true;
                            if (waznyX) {
//debug                       double nieprzesunietyxp1 = refToryZGwiazdka[j].xp1;
//debug                       double przesunietyXp1 = refToryZGwiazdka[j].xp1 + wektorP2P1x;
                                double testX = (refToryZGwiazdka[j].xp1 + wektorP2P1x) / refToryZGwiazdka[j].xp1;
                                if (testX >= 1) {
                                    rosnieX = true;
                                } else malejeX = true;
                            }
                            if (waznyY) {
//debug                        double nieprzesunietyyp1 = refToryZGwiazdka[j].yp1;
//debug                        double przesunietyYp1 = refToryZGwiazdka[j].yp1 + wektorP2P1y;
                                double testY = (refToryZGwiazdka[j].yp1 + wektorP2P1y) / refToryZGwiazdka[j].yp1;
                                if (testY >= 1) {
                                    rosnieY = true;
                                } else malejeY = true;
                            }
                            if (rosnieY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (rosnieX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                        }
                        if (!doOdrzutu) {
                            refWierzcholki.push_back(wierzcholek{x, y, z});
                            ++liczbaLiniiWierzcholkow;
                        }
                    }
                    ++wyraz1;
                }
                if (wyraz >= llw) ++llw;
            }
        }
    }
    std::cout << "Liczba znalezionych wierzcholkow w triangles: " << liczbaLiniiWierzcholkow << ". Najwieksza liczba linii w triangles: " << llw <<
              "\n\n";
// Zamkniecie pliku z torami
    plik1.close();
}

void tablicaWierzcholkowTriangles(std::vector<std::vector<unsigned int> > &refTablica, double ExportX, double ExportY, unsigned int szerokosc,
                                  std::vector<punktyTorow> &refToryZGwiazdka, unsigned int &refKorektaX, unsigned int &refKorektaY, unsigned int &refWierszeTablicy,
                                  unsigned int &refKolumnyTablicy) {
    std::string linia ("");
    std::string ln ("");
    std::string szukanyString (" triangles ");
    std::string nazwaPlikuZTrojkatami ("EXPORT.SCN");
    unsigned int liczbaLiniiWierzcholkow = 0, nrLinii = 0, llw = 0, testXmin = 900000, testXmax = 0, testYmin = 900000, testYmax = 0;
// Otwarcie pliku tylko do odczytu z torami
    std::cout << "Otwieram plik ze sceneria i wczytuje do pamieci wierzcholki x, y i z trojkatow..." << "\n";
    std::ifstream plik1;
    plik1.open(nazwaPlikuZTrojkatami.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPlikuZTrojkatami << "\n";
        std::cin.get();
    }
// W petli szukamy w otwartym pliku torow oraz ich punktow p1.
    while(!plik1.eof()) {
        std::getline(plik1, linia);
        ++nrLinii;
// ...patrzymy czy zawiera
        int znaleziono = (linia.find(szukanyString));
// Jesli zawiera, to rozbijamy linie na elementy
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            std::istringstream ln(linia);
            bool flagPrzerwij = false;
            std::string temp = "";
// Jesli znalazlo triangles, to bierze 100 nastepnych linijek z pliku tekstowego by uzyskac x, y i z - czyli wierzcholki trojkata
//          #pragma omp parallel for default(shared) private(wyraz1)
            for (unsigned int wyraz = 0; wyraz < 100; ++wyraz) {
                double x = 0, y = 0;
                std::getline(plik1, linia);
                ++nrLinii;
                std::istringstream ln(linia);
                unsigned int wyraz1 = 0;
                if (flagPrzerwij == true) break;
                if (linia == "endtri") break;
                while (std::getline(ln, temp, ' ')) {
                    if (wyraz1 == 0) {
                        x = (atof(temp.c_str()) * -1.0) + ExportX;
                        if (testXmax < x) testXmax = x;
                        if (testXmin > x) testXmin = x;
                    }
                    if (wyraz1 == 2) {
                        y = atof(temp.c_str()) + ExportY;
                        if (testYmax < y) testYmax = y;
                        if (testYmin > y) testYmin = y;
                    }
                    ++wyraz1;
                }
            }
        }
    }
    testXmin -= 5 * szerokosc;
    testXmax += 5 * szerokosc;
    testYmin -= 5 * szerokosc;
    testYmax += 5 * szerokosc;
    refWierszeTablicy = (testXmax / szerokosc) - (testXmin / szerokosc);
    refKolumnyTablicy = (testYmax / szerokosc) - (testYmin / szerokosc);
    refTablica.resize(refWierszeTablicy);
    for (unsigned int i = 0; i < refWierszeTablicy; ++i) {
        refTablica[i].resize(refKolumnyTablicy);
    }
    refKorektaX = testXmin;
    refKorektaY = testYmin;
    plik1.close();

    plik1.open(nazwaPlikuZTrojkatami.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPlikuZTrojkatami << "\n";
        std::cin.get();
    }
    while(!plik1.eof()) {
        std::getline(plik1, linia);
        ++nrLinii;
// ...patrzymy czy zawiera
        int znaleziono = (linia.find(szukanyString));
// Jesli zawiera, to rozbijamy linie na elementy
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            std::istringstream ln(linia);
            bool flagPrzerwij = false;
            std::string temp = "";
// Jesli znalazlo triangles, to bierze 100 nastepnych linijek z pliku tekstowego by uzyskac x, y i z - czyli wierzcholki trojkata
//          #pragma omp parallel for default(shared) private(wyraz1)
            for (unsigned int wyraz = 0; wyraz < 100; ++wyraz) {
                bool doOdrzutu = false;
                double x = 0, y = 0;
                std::getline(plik1, linia);
                ++nrLinii;
                std::istringstream ln(linia);
                unsigned int wyraz1 = 0;
                if (flagPrzerwij == true) break;
                if (linia == "endtri") break;
                while (std::getline(ln, temp, ' ')) {
                    if (wyraz1 == 0) {
                        x = (atof(temp.c_str()) * -1.0) + ExportX;
                    }
                    if (wyraz1 == 2) {
                        y = atof(temp.c_str()) + ExportY;
                        for (unsigned int j = 0; j < refToryZGwiazdka.size(); ++j) {
                            bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
                            double wektorP2P1x = refToryZGwiazdka[j].xp1 - refToryZGwiazdka[j].xp2;
                            double wektorP2P1y = refToryZGwiazdka[j].yp1 - refToryZGwiazdka[j].yp2;

                            int resztax1toru = (int)refToryZGwiazdka[j].xp1 % 1000;
                            int resztay1toru = (int)refToryZGwiazdka[j].yp1 % 1000;

                            if (resztax1toru == 0) waznyX = true;
                            if (resztay1toru == 0) waznyY = true;
                            if (waznyX) {
                                double testX = (refToryZGwiazdka[j].xp1 + wektorP2P1x) / refToryZGwiazdka[j].xp1;
                                if (testX >= 1) {
                                    rosnieX = true;
                                } else malejeX = true;
                            }
                            if (waznyY) {
                                double testY = (refToryZGwiazdka[j].yp1 + wektorP2P1y) / refToryZGwiazdka[j].yp1;
                                if (testY >= 1) {
                                    rosnieY = true;
                                } else malejeY = true;
                            }
                            if (rosnieY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (rosnieX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[j].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[j].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                        }
                        if (!doOdrzutu) {
                            for (int j = -2; j < 3; ++j) {
                                for (int jj = -2; jj < 3; ++jj) {
                                    refTablica[((x - refKorektaX) / szerokosc) + j][((y - refKorektaY) / szerokosc) + jj] = 1;
                                }
                            }
                            ++liczbaLiniiWierzcholkow;
                        }
                    }
                    ++wyraz1;
                }
                if (wyraz > llw) llw = wyraz + 1;
            }
        }
    }
    std::cout << "Liczba znalezionych wierzcholkow w triangles: " << liczbaLiniiWierzcholkow << ". Najwieksza liczba linii w triangles: " << llw <<
              "\nTablica zapisana.\n\n";
// Zamkniecie pliku z torami
    plik1.close();
}

void odczytPunktowNode(std::vector<wierzcholek> &refWierzcholki, std::string zarostek, unsigned int &refLicznikWierzcholkow) {
    std::string linia ("");
    std::string ln ("");
    std::string nazwaPliku ("wierzcholki.node");
    nazwaPliku.insert(11,zarostek);
    unsigned int nrLinii = 0;
// Otwarcie pliku tylko do odczytu z danymi NMT
    std::cout << "Otwieram plik " << nazwaPliku << " z wierzcholkami.\n";
    std::ifstream plik1;
    plik1.open(nazwaPliku.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
    nrLinii = 0;
    while(!plik1.eof()) {
        std::getline(plik1, linia);
        std::istringstream ln(linia);
        unsigned int wyraz = 0;
        bool flagPunkty = false;
        std::string temp1 ("");
        std::string temp2 ("");
        std::string temp3 ("");
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po " ")
        while (!ln.eof()) {
            std::string temp ("");
            std::getline(ln, temp, ' ');
            bool znalazlem = false;
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
            if (temp == "") {
                flagPunkty = true;
            } else {
                flagPunkty = false;
            }
            if (temp == "#") break;
            if (!flagPunkty) {
                if (wyraz == 1) temp1 = temp; // Y
                if (wyraz == 2) temp2 = temp; // -X
                if (wyraz == 3) {
                    temp3 = temp.erase (0,1); //Z
                    if (!znalazlem) {
                        refWierzcholki.push_back(wierzcholek{atof (temp1.c_str()), atof (temp2.c_str()), atof (temp3.c_str())});
                        refWierzcholki.push_back(wierzcholek());
                        znalazlem = true;
                        ++refLicznikWierzcholkow;
                    }
                }
                ++wyraz;
            }
        }
        ++nrLinii;
        std::cout << "Linia nr " << nrLinii << "          \r";
    }
    std::cout << "\nLiczba znalezionych wierzcholkow: " << refWierzcholki.size() << "\n";
// Zamyka plik
    plik1.close();
    std::cout << "Wszystkie punkty wczytane. Zamykam plik " << nazwaPliku << "\n\n";
// Zamyka plik ze znalezionymi punktami NMT
}

void odczytPunktowHGT(std::vector<wierzcholek> &refWierzcholki, std::vector<std::vector<unsigned int> > &refTablica,
                      std::vector<std::vector<unsigned int> > &refTablicaBrakow, std::string nazwaPliku, unsigned int &id,
                      unsigned int &refNrPliku, unsigned int &refLiczbaPlikow, unsigned int szerokosc, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX,
                      unsigned int refKorektaY, unsigned int refKorektaXbraki, unsigned int refKorektaYbraki, unsigned int refWierszeTablicy,
                      unsigned int refKolumnyTablicy) {
    unsigned char buffer[2];
    unsigned int licznik = 0;
    unsigned int dlugoscNazwyPliku = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
    const double sekunda = 3.0/3600.0;
    dlugoscNazwyPliku = nazwaPliku.length();
    std::string nrx = nazwaPliku.substr(dlugoscNazwyPliku-10,2);
    std::string nry = nazwaPliku.substr(dlugoscNazwyPliku-6,2);
    std::cout << "nazwaPliku=" << nazwaPliku << "\n";
    const double XwsgPoczatek = atof(nrx.c_str());
    const double YwsgPoczatek = atof(nry.c_str());
    const unsigned int SRTM_SIZE = 1201;
// Kod przeksztalcenia formatu WGS84 do PUWG 1992 zostal zapozyczony i zoptymalizowany. Naglowek autora ponizej
    /*
    Autor: Zbigniew Szymanski
    E-mail: z.szymanski@szymanski-net.eu
    Wersja: 1.1
    Historia zmian:
    		1.1 dodano przeksztalcenie odwrotne PUWG 1992 ->WGS84
    		1.0 przeksztalcenie WGS84 -> PUWG 1992
    Data modyfikacji: 2012-11-27
    Uwagi: Oprogramowanie darmowe. Dozwolone jest wykorzystanie i modyfikacja
           niniejszego oprogramowania do wlasnych celow pod warunkiem
           pozostawienia wszystkich informacji z naglowka. W przypadku
           wykorzystania niniejszego oprogramowania we wszelkich projektach
           naukowo-badawczych, rozwojowych, wdrozeniowych i dydaktycznych prosze
           o zacytowanie nastepujacego artykulu:

           Zbigniew Szymanski, Stanislaw Jankowski, Jan Szczyrek,
           "Reconstruction of environment model by using radar vector field histograms.",
           Photonics Applications in Astronomy, Communications, Industry, and
           High-Energy Physics Experiments 2012, Proc. of SPIE Vol. 8454, pp. 845422 - 1-8,
           doi:10.1117/12.2001354

    Literatura:
           Uriasz, J., "Wybrane odwzorowania kartograficzne", Akademia Morska w Szczecinie,
           http://uriasz.am.szczecin.pl/naw_bezp/odwzorowania.html
    */
// Parametry elipsoidy GRS-80
    const double e = 0.0818191910428;  	//pierwszymimosrod elipsoidy
    const double R0 = 6367449.14577; 		//promien sfery Lagrange.a
    const double Snorm = 2.0E-6;   		//parametr normujacy
    const double xo = 5760000.0; 		//parametr centrujacy
//Wspolczynniki wielomianu
    const double a0 = 5765181.11148097;
    const double a1 = 499800.81713800;
    const double a2 = -63.81145283;
    const double a3 = 0.83537915;
    const double a4 = 0.13046891;
    const double a5 = -0.00111138;
    const double a6 = -0.00010504;
// Parametry odwzorowania Gaussa-Kruegera dla ukladu PUWG92
    const double L0_stopnie = 19.0; 		//Poczatek ukladu wsp. PUWG92 (dlugosc)
    const double m0 = 0.9993;
    const double x0 = -5300000.0;
    const double y0 = 500000.0;
// Otwarcie pliku HGT
    std::cout << "Otwieram plik " << nazwaPliku << " " << refNrPliku + 1 << " z " << refLiczbaPlikow <<" \n";
    std::ifstream file(nazwaPliku.c_str(), std::ios::in|std::ios::binary);
    if(!file)
    {
        std::cout << "Error opening file " << nazwaPliku << "!\n";
        std::cin.get();
    }
    unsigned int nrPunktu = 0;
    for (int i = SRTM_SIZE - 1; i >= 0; --i) {
        double Xwsg = XwsgPoczatek + (i * sekunda);
        double B = Xwsg * M_PI / 180.0;
        double U = 1.0 - e * sin(B);
        double V = 1.0 + e * sin(B);
        double K = pow((U / V),(e / 2.0));
        double C = K * tan(B / 2.0 + M_PI / 4.0);
        double fi = 2.0 * atan(C) - M_PI / 2.0;
        double p = sin(fi);
        double cosfi = cos(fi);
        for (unsigned int j = 0; j < SRTM_SIZE; ++j) {
            if(!file.read( reinterpret_cast<char*>(buffer), sizeof(buffer) )) {
                std::cout << "Error reading file!\n";
                std::cin.get();
            }
            double Ywsg = YwsgPoczatek + (j * sekunda);
            double z = (buffer[0] << 8) | buffer[1];
            if ((z > 5) && ( z < 3000)) {
                double dL_stopnie = Ywsg - L0_stopnie;
                double d_lambda = dL_stopnie * M_PI / 180.0;
// Etap I - elipsoida na kule
// Etap II - kula na walec
                double sindlambda = sin(d_lambda);
                double q = cosfi * cos(d_lambda);
                double r = 1.0 + cosfi * sindlambda;
                double s = 1.0 - cosfi * sindlambda;
                double XMERC = R0 * atan(p / q);
                double YMERC = 0.5 * R0 * log(r / s);
// Etap III - walec na plaszczyzne
                std::complex<double> Z((XMERC - xo) * Snorm,YMERC * Snorm);
                std::complex<double> Zgk;
                Zgk = a0 + Z *(a1 + Z * (a2 + Z * (a3 + Z * (a4 + Z * (a5 + Z * a6)))));
                double Xgk=Zgk.real();
                double Ygk=Zgk.imag();
// Przejscie do ukladu aplikacyjnego
                double Xpuwg = m0 * Xgk + x0;
                double Ypuwg = m0 * Ygk + y0;
                bool doOdrzutu = false, nieSprawdzaj = false;
                if (((Ypuwg - refKorektaX) / szerokosc < 1) || ((Ypuwg - refKorektaX) / szerokosc > refWierszeTablicy)) nieSprawdzaj = true;
                if (((Xpuwg - refKorektaY) / szerokosc < 1) || ((Xpuwg - refKorektaY) / szerokosc > refKolumnyTablicy)) nieSprawdzaj = true;
                if (!nieSprawdzaj) {
                    if (refTablica[(Ypuwg - refKorektaX) / szerokosc][(Xpuwg - refKorektaY) / szerokosc] == 1) {
                        if (refTablicaBrakow[(Ypuwg - refKorektaXbraki) / 50][(Xpuwg - refKorektaYbraki) / 50] == 1) {
                            for (unsigned int jj = 0; jj < liczbaTorowZGwiazdka; ++jj) {
                                bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
                                double wektorP2P1x = refToryZGwiazdka[jj].xp1 - refToryZGwiazdka[jj].xp2;
                                double wektorP2P1y = refToryZGwiazdka[jj].yp1 - refToryZGwiazdka[jj].yp2;
                                int resztax1toru = (int)refToryZGwiazdka[jj].xp1 % 1000;
                                int resztay1toru = (int)refToryZGwiazdka[jj].yp1 % 1000;
                                if (resztax1toru == 0) waznyX = true;
                                if (resztay1toru == 0) waznyY = true;
                                if (waznyX) {
                                    double testX = (refToryZGwiazdka[jj].xp1 + wektorP2P1x) / refToryZGwiazdka[jj].xp1;
                                    if (testX >= 1) {
                                        rosnieX = true;
                                    } else malejeX = true;
                                }
                                if (waznyY) {
                                    double testY = (refToryZGwiazdka[jj].yp1 + wektorP2P1y) / refToryZGwiazdka[jj].yp1;
                                    if (testY >= 1) {
                                        rosnieY = true;
                                    } else malejeY = true;
                                }
                                if (rosnieY) {
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (malejeY) {
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (rosnieX) {
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (malejeX) {
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                            }
                            if (!doOdrzutu) {
                                refWierzcholki.push_back(wierzcholek{Ypuwg, Xpuwg, z});
                                ++id;
                                ++licznik;
                            }
                        }
                    }
                }
            }
        }
        ++nrPunktu;
    }
//    std::cout << "\nLiczba znalezionych wierzcholkow w pliku: " << licznik << "\nW sumie znalezionych punktow: " << id << "\n\n";
    printf ("\nLiczba znalezionych wierzcholkow w pliku: %u", licznik);
    printf ("\nW sumie znalezionych punktow: %u \n\n", id);
//    std::cout << "Wszystkie punkty HGT wczytane. Zamykam ten plik" << "\n\n";
}

void odczytPunktowHGTzUwzglednieniemProfilu(std::vector<wierzcholek> &refWierzcholki, std::vector<wierzcholek> &refWierzcholkiProfilu,
        std::vector<std::vector<unsigned int> > &refTablica1, std::vector<std::vector<unsigned int> > &refTablica2,
        std::vector<std::vector<unsigned int> > &refTablicaBrakow, std::string nazwaPliku, unsigned int &refNrId,
        unsigned int NrPliku, unsigned int LiczbaPlikow, unsigned int szerokosc1, unsigned int szerokosc2, std::vector<punktyTorow> &refToryZGwiazdka,
        unsigned int refKorektaX1, unsigned int refKorektaY1, unsigned int refKorektaX2, unsigned int refKorektaY2, unsigned int refKorektaXbraki,
        unsigned int refKorektaYbraki, unsigned int refWierszeTablicy1, unsigned int refKolumnyTablicy1, unsigned int refWierszeTablicy2,
        unsigned int refKolumnyTablicy2) {
    unsigned char buffer[2];
    unsigned int licznik = 0, dlugoscNazwyPliku = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
    const double sekunda = 3.0/3600.0;
    dlugoscNazwyPliku = nazwaPliku.length();
    std::string nrx = nazwaPliku.substr(dlugoscNazwyPliku-10,2);
    std::string nry = nazwaPliku.substr(dlugoscNazwyPliku-6,2);
    std::cout << "nazwaPliku=" << nazwaPliku << "\n";
    const double XwsgPoczatek = atof(nrx.c_str());
    const double YwsgPoczatek = atof(nry.c_str());
    const unsigned int SRTM_SIZE = 1201;
// Kod przeksztalcenia formatu WGS84 do PUWG 1992 zostal zapozyczony i zoptymalizowany. Naglowek autora ponizej
    /*
    Autor: Zbigniew Szymanski
    E-mail: z.szymanski@szymanski-net.eu
    Wersja: 1.1
    Historia zmian:
    		1.1 dodano przeksztalcenie odwrotne PUWG 1992 ->WGS84
    		1.0 przeksztalcenie WGS84 -> PUWG 1992
    Data modyfikacji: 2012-11-27
    Uwagi: Oprogramowanie darmowe. Dozwolone jest wykorzystanie i modyfikacja
           niniejszego oprogramowania do wlasnych celow pod warunkiem
           pozostawienia wszystkich informacji z naglowka. W przypadku
           wykorzystania niniejszego oprogramowania we wszelkich projektach
           naukowo-badawczych, rozwojowych, wdrozeniowych i dydaktycznych prosze
           o zacytowanie nastepujacego artykulu:

           Zbigniew Szymanski, Stanislaw Jankowski, Jan Szczyrek,
           "Reconstruction of environment model by using radar vector field histograms.",
           Photonics Applications in Astronomy, Communications, Industry, and
           High-Energy Physics Experiments 2012, Proc. of SPIE Vol. 8454, pp. 845422 - 1-8,
           doi:10.1117/12.2001354

    Literatura:
           Uriasz, J., "Wybrane odwzorowania kartograficzne", Akademia Morska w Szczecinie,
           http://uriasz.am.szczecin.pl/naw_bezp/odwzorowania.html
    */
// Parametry elipsoidididy GRS-80
    const double e = 0.0818191910428;  	//pierwszymimosrod elipsoidy
    const double R0 = 6367449.14577; 		//promien sfery Lagrange.a
    const double Snorm = 2.0E-6;   		//parametr normujacy
    const double xo = 5760000.0; 		//parametr centrujacy
//Wspolczynniki wielomianu
    const double a0 = 5765181.11148097;
    const double a1 = 499800.81713800;
    const double a2 = -63.81145283;
    const double a3 = 0.83537915;
    const double a4 = 0.13046891;
    const double a5 = -0.00111138;
    const double a6 = -0.00010504;
// Parametry odwzorowania Gaussa-Kruegera dla ukladu PUWG92
    const double L0_stopnie = 19.0; 		//Poczatek ukladu wsp. PUWG92 (dlugosc)
    const double m0 = 0.9993;
    const double x0 = -5300000.0;
    const double y0 = 500000.0;
// Otwarcie pliku HGT
    std::cout << "Otwieram plik " << nazwaPliku << " " << NrPliku + 1 << " z " << LiczbaPlikow <<" \n";
    std::ifstream file(nazwaPliku.c_str(), std::ios::in|std::ios::binary);
    if(!file)
    {
        std::cout << "Error opening file " << nazwaPliku << "!\n";
        std::cin.get();
    }
    unsigned int nrPunktu = 0;
    for (int i = SRTM_SIZE - 1; i >= 0; --i) {
        double Xwsg = XwsgPoczatek + (i * sekunda);
        double B = Xwsg * M_PI / 180.0;
        double U = 1.0 - e * sin(B);
        double V = 1.0 + e * sin(B);
        double K = pow((U / V),(e / 2.0));
        double C = K * tan(B / 2.0 + M_PI / 4.0);
        double fi = 2.0 * atan(C) - M_PI / 2.0;
        double p = sin(fi);
        double cosfi = cos(fi);
        for (unsigned int j = 0; j < SRTM_SIZE; ++j) {
            if(!file.read( reinterpret_cast<char*>(buffer), sizeof(buffer) )) {
                std::cout << "Error reading file!\n";
                std::cin.get();
            }
            double Ywsg = YwsgPoczatek + (j * sekunda);
            double z = (buffer[0] << 8) | buffer[1];
//            if (z != 32768.0) {
            if ((z > 5) && ( z < 3000)) {
                double dL_stopnie = Ywsg - L0_stopnie;
                double d_lambda = dL_stopnie * M_PI / 180.0;
// Etap I - elipsoida na kule
// Etap II - kula na walec
                double sindlambda = sin(d_lambda);
                double q = cosfi * cos(d_lambda);
                double r = 1.0 + cosfi * sindlambda;
                double s = 1.0 - cosfi * sindlambda;
                double XMERC = R0 * atan(p / q);
                double YMERC = 0.5 * R0 * log(r / s);
// Etap III - walec na plaszczyzne
                std::complex<double> Z((XMERC - xo) * Snorm,YMERC * Snorm);
                std::complex<double> Zgk;
                Zgk = a0 + Z *(a1 + Z * (a2 + Z * (a3 + Z * (a4 + Z * (a5 + Z * a6)))));
                double Xgk=Zgk.real();
                double Ygk=Zgk.imag();
// Przejscie do ukladu aplikacyjnego
                double Xpuwg = m0 * Xgk + x0;
                double Ypuwg = m0 * Ygk + y0;
                bool zaBlisko = false, nieSprawdzaj = false;
                if (((Ypuwg - refKorektaX1) / szerokosc1 < 1) || ((Ypuwg - refKorektaX1) / szerokosc1 > refWierszeTablicy1)) nieSprawdzaj = true;
                if (((Xpuwg - refKorektaY1) / szerokosc1 < 1) || ((Xpuwg - refKorektaY1) / szerokosc1 > refKolumnyTablicy1)) nieSprawdzaj = true;
                if (!nieSprawdzaj) {
                    if (refTablica1[(Ypuwg - refKorektaX1) / szerokosc1][(Xpuwg - refKorektaY1) / szerokosc1] == 1) {
                        if (refTablicaBrakow[(Ypuwg - refKorektaXbraki) / 50][(Xpuwg - refKorektaYbraki) / 50] == 1) {
                            bool doOdrzutu = false;
                            if (((Ypuwg - refKorektaX2) / szerokosc2 < 1) || ((Ypuwg - refKorektaX2) / szerokosc2 > refWierszeTablicy2)) nieSprawdzaj = true;
                            if (((Xpuwg - refKorektaY2) / szerokosc2 < 1) || ((Xpuwg - refKorektaY2) / szerokosc2 > refKolumnyTablicy2)) nieSprawdzaj = true;
                            if (!nieSprawdzaj) {
                                if (refTablica2[(Ypuwg - refKorektaX2) / szerokosc2][(Xpuwg - refKorektaY2) / szerokosc2] == 1) zaBlisko = true;
                            }
                            if (!zaBlisko) {
                                for (unsigned int jj = 0; jj < liczbaTorowZGwiazdka; ++jj) {
                                    bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
                                    double wektorP2P1x = refToryZGwiazdka[jj].xp1 - refToryZGwiazdka[jj].xp2;
                                    double wektorP2P1y = refToryZGwiazdka[jj].yp1 - refToryZGwiazdka[jj].yp2;
                                    int resztax1toru = (int)refToryZGwiazdka[jj].xp1 % 1000;
                                    int resztay1toru = (int)refToryZGwiazdka[jj].yp1 % 1000;

                                    if (resztax1toru == 0) waznyX = true;
                                    if (resztay1toru == 0) waznyY = true;

                                    if (waznyX) {
//debug                                double nieprzesunietyxp1 = refToryZGwiazdka[jj].xp1;
//debug                                double przesunietyXp1 = refToryZGwiazdka[jj].xp1 + wektorP2P1x;
                                        double testX = (refToryZGwiazdka[jj].xp1 + wektorP2P1x) / refToryZGwiazdka[jj].xp1;
                                        if (testX >= 1) {
                                            rosnieX = true;
                                        } else malejeX = true;
                                    }
                                    if (waznyY) {
//debug                                double nieprzesunietyyp1 = refToryZGwiazdka[jj].yp1;
//debug                                double przesunietyYp1 = refToryZGwiazdka[jj].yp1 + wektorP2P1y;
                                        double testY = (refToryZGwiazdka[jj].yp1 + wektorP2P1y) / refToryZGwiazdka[jj].yp1;
                                        if (testY >= 1) {
                                            rosnieY = true;
                                        } else malejeY = true;
                                    }
                                    if (rosnieY) {
                                        int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                        int zaokraglonyXpuwg = Xpuwg / 1000;
                                        int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                        int zaokraglonyYpuwg = Ypuwg / 1000;
                                        int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                        int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                        if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                            doOdrzutu = true;
                                            break;
                                        }
                                    }
                                    if (malejeY) {
                                        int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                        int zaokraglonyXpuwg = Xpuwg / 1000;
                                        int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                        int zaokraglonyYpuwg = Ypuwg / 1000;
                                        int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                        int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                        if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                            doOdrzutu = true;
                                            break;
                                        }
                                    }
                                    if (rosnieX) {
                                        int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                        int zaokraglonyYpuwg = Ypuwg / 1000;
                                        int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                        int zaokraglonyXpuwg = Xpuwg / 1000;
                                        int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                        int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                        if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                            doOdrzutu = true;
                                            break;
                                        }
                                    }
                                    if (malejeX) {
                                        int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                        int zaokraglonyYpuwg = Ypuwg / 1000;
                                        int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                        int zaokraglonyXpuwg = Xpuwg / 1000;
                                        int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                        int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                        if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                            doOdrzutu = true;
                                            break;
                                        }
                                    }
                                }
                                if (!doOdrzutu) {
                                    refWierzcholki.push_back(wierzcholek{Ypuwg, Xpuwg, z});
                                    ++refNrId;
                                    ++licznik;
                                }
                            }
                        }
                    }
                }
            }
        }
        ++nrPunktu;
    }
    std::cout << "\nLiczba znalezionych wierzcholkow w pliku: " << licznik << "\nW sumie znalezionych punktow: " << refNrId << "\n\n";
}

void odczytPunktowDT2(std::vector<wierzcholek> &refWierzcholki, std::vector<std::vector<unsigned int> > &refTablica,
                      std::vector<std::vector<unsigned int> > &refTablicaBrakow, std::string nazwaPliku, unsigned int &id,
                      unsigned int &refNrPliku, unsigned int &refLiczbaPlikow, unsigned int szerokosc, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX,
                      unsigned int refKorektaY, unsigned int refKorektaXbraki, unsigned int refKorektaYbraki, unsigned int refWierszeTablicy,
                      unsigned int refKolumnyTablicy) {
    unsigned int licznik = 0, dlugoscNazwyPliku = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
    double minutaX = 0, minutaY = 0;
    const double sekunda = 1.0/3600.0;
    dlugoscNazwyPliku = nazwaPliku.length();
    std::cout << "nazwaPliku=" << nazwaPliku << "\n";
    std::string nrx = nazwaPliku.substr(dlugoscNazwyPliku-21,2);
    std::string nry = nazwaPliku.substr(dlugoscNazwyPliku-28,2);
    std::string mnX = nazwaPliku.substr(dlugoscNazwyPliku-19,2);
    std::string mnY = nazwaPliku.substr(dlugoscNazwyPliku-26,2);
    if (mnX == "15") minutaX = 15.0/60.0;
    if (mnX == "30") minutaX = 30.0/60.0;
    if (mnX == "45") minutaX = 45.0/60.0;
    if (mnY == "15") minutaY = 15.0/60.0;
    if (mnY == "30") minutaY = 30.0/60.0;
    if (mnY == "45") minutaY = 45.0/60.0;
    const double XwsgPoczatek = atof(nrx.c_str());
    const double YwsgPoczatek = atof(nry.c_str());
    const unsigned int SRTM_SIZE = 900;
    // Odczyt pliku DEM
    // unsigned char moze przechowywac 1 Byte (8bits) danych (0-255)
    typedef unsigned char BYTE;
    std::cout << "Otwieram plik " << nazwaPliku << " " << refNrPliku + 1 << " z " << refLiczbaPlikow <<" \n";
    BYTE *fileBuf;			// Pointer do danych
    FILE *file = NULL;		// File pointer
    if ((file = fopen(nazwaPliku.c_str(), "rb")) == NULL) {
        std::cout << "Nie mozna otworzyc pliku" << nazwaPliku << "\n";
        std::cin.get();
    }
    // Jaka jest wielkosc pliku?
    unsigned long fileSize = getFileSize(file);
    // Allokowanie miejsca na caly plik
    fileBuf = new BYTE[fileSize];
    // Odczyt pliku do bufora
    size_t sizeRead1 = fread(fileBuf, fileSize, 1, file);
    if (sizeRead1 != 1) {
        std::cout << "\nsizeRead ERROR!\n";
        std::cin.get();
    }
    std::vector<std::vector<unsigned int> > tablicaDEM;
    tablicaDEM.resize(901);
    for (unsigned int i = 0; i < 901; ++i) {
        tablicaDEM[i].resize(901);
    }
    for (unsigned int i = 3436, wiersze = 0, kolumny = 0, k = 0; i < fileSize; ++i) {
        if (k == 1) {
            tablicaDEM[wiersze][kolumny] = (fileBuf[i] | (fileBuf[i - 1] << 8));
            ++kolumny;
            if (kolumny > SRTM_SIZE) {
                kolumny = 0;
                ++wiersze;
                i = i + 12;
            }
        }
        ++k;
        if (k == 2) k = 0;
    }
    double geoidUndulation = 10*(fileBuf[491] - '0') + fileBuf[492] - '0';
    fclose(file);
    delete[]fileBuf;
//Odczyt pliku HEM
    nazwaPliku.replace(dlugoscNazwyPliku-34,3,"HEM");
    nazwaPliku.replace(dlugoscNazwyPliku-7,3,"HEM");
    std::cout << "Otwieram plik z korekta wysokosci" << nazwaPliku << "\n";
    if ((file = fopen(nazwaPliku.c_str(), "rb")) == NULL) {
        std::cout << "Nie mozna otworzyc pliku" << nazwaPliku << "\n";
        std::cin.get();
    }
    fileSize = getFileSize(file);
    fileBuf = new BYTE[fileSize];
    size_t sizeRead2 = fread(fileBuf, fileSize, 1, file);
    if (sizeRead2 != 1) {
        std::cout << "\nsizeRead ERROR!\n";
        std::cin.get();
    }
    std::vector<std::vector<unsigned int> > tablicaHEM;
    tablicaHEM.resize(901);
    for (unsigned int i = 0; i < 901; ++i) {
        tablicaHEM[i].resize(901);
    }
    for (unsigned int i = 3436, wiersze = 0, kolumny = 0, k = 0; i < fileSize; ++i) {
        if (k == 1) {
            tablicaHEM[wiersze][kolumny] = (fileBuf[i] | (fileBuf[i - 1] << 8));
            ++kolumny;
            if (kolumny > SRTM_SIZE) {
                kolumny = 0;
                ++wiersze;
                i = i + 12;
            }
        }
        ++k;
        if (k == 2) k = 0;
    }
    fclose(file);
// Kod przeksztalcenia formatu WGS84 do PUWG 1992 zostal zapozyczony i zoptymalizowany. Naglowek autora ponizej
    /*
    Autor: Zbigniew Szymanski
    E-mail: z.szymanski@szymanski-net.eu
    Wersja: 1.1
    Historia zmian:
    		1.1 dodano przeksztalcenie odwrotne PUWG 1992 ->WGS84
    		1.0 przeksztalcenie WGS84 -> PUWG 1992
    Data modyfikacji: 2012-11-27
    Uwagi: Oprogramowanie darmowe. Dozwolone jest wykorzystanie i modyfikacja
           niniejszego oprogramowania do wlasnych celow pod warunkiem
           pozostawienia wszystkich informacji z naglowka. W przypadku
           wykorzystania niniejszego oprogramowania we wszelkich projektach
           naukowo-badawczych, rozwojowych, wdrozeniowych i dydaktycznych prosze
           o zacytowanie nastepujacego artykulu:

           Zbigniew Szymanski, Stanislaw Jankowski, Jan Szczyrek,
           "Reconstruction of environment model by using radar vector field histograms.",
           Photonics Applications in Astronomy, Communications, Industry, and
           High-Energy Physics Experiments 2012, Proc. of SPIE Vol. 8454, pp. 845422 - 1-8,
           doi:10.1117/12.2001354

    Literatura:
           Uriasz, J., "Wybrane odwzorowania kartograficzne", Akademia Morska w Szczecinie,
           http://uriasz.am.szczecin.pl/naw_bezp/odwzorowania.html
    */
// Parametry elipsoidy GRS-80
    const double e = 0.0818191910428;  	//pierwszymimosrod elipsoidy
    const double R0 = 6367449.14577; 		//promien sfery Lagrange.a
    const double Snorm = 2.0E-6;   		//parametr normujacy
    const double xo = 5760000.0; 		//parametr centrujacy
//Wspolczynniki wielomianu
    const double a0 = 5765181.11148097;
    const double a1 = 499800.81713800;
    const double a2 = -63.81145283;
    const double a3 = 0.83537915;
    const double a4 = 0.13046891;
    const double a5 = -0.00111138;
    const double a6 = -0.00010504;
// Parametry odwzorowania Gaussa-Kruegera dla ukladu PUWG92
    const double L0_stopnie = 19.0; 		//Poczatek ukladu wsp. PUWG92 (dlugosc)
    const double m0 = 0.9993;
    const double x0 = -5300000.0;
    const double y0 = 500000.0;
    unsigned int nrPunktu = 0;
    for (int i = SRTM_SIZE - 1; i >= 0; --i) {
        double Xwsg = XwsgPoczatek + minutaX + (i * sekunda);
        double B = Xwsg * M_PI / 180.0;
        double U = 1.0 - e * sin(B);
        double V = 1.0 + e * sin(B);
        double K = pow((U / V),(e / 2.0));
        double C = K * tan(B / 2.0 + M_PI / 4.0);
        double fi = 2.0 * atan(C) - M_PI / 2.0;
        double p = sin(fi);
        double cosfi = cos(fi);
        for (unsigned int j = 0; j < SRTM_SIZE; ++j) {
            double Ywsg = YwsgPoczatek + minutaY + (j * sekunda);
            double z = tablicaDEM[j][i] - tablicaHEM[j][i] - geoidUndulation;
//            if ((z > 5.0) && (z < 3000.0)) {
            double dL_stopnie = Ywsg - L0_stopnie;
            double d_lambda = dL_stopnie * M_PI / 180.0;
// Etap I - elipsoida na kule
// Etap II - kula na walec
            double sindlambda = sin(d_lambda);
            double q = cosfi * cos(d_lambda);
            double r = 1.0 + cosfi * sindlambda;
            double s = 1.0 - cosfi * sindlambda;
            double XMERC = R0 * atan(p / q);
            double YMERC = 0.5 * R0 * log(r / s);
// Etap III - walec na plaszczyzne
            std::complex<double> Z((XMERC - xo) * Snorm,YMERC * Snorm);
            std::complex<double> Zgk;
            Zgk = a0 + Z *(a1 + Z * (a2 + Z * (a3 + Z * (a4 + Z * (a5 + Z * a6)))));
            double Xgk=Zgk.real();
            double Ygk=Zgk.imag();
// Przejscie do ukladu aplikacyjnego
            double Xpuwg = m0 * Xgk + x0;
            double Ypuwg = m0 * Ygk + y0;
            bool doOdrzutu = false, nieSprawdzaj = false;
            if (((Ypuwg - refKorektaX) / szerokosc < 1) || ((Ypuwg - refKorektaX) / szerokosc > refWierszeTablicy)) nieSprawdzaj = true;
            if (((Xpuwg - refKorektaY) / szerokosc < 1) || ((Xpuwg - refKorektaY) / szerokosc > refKolumnyTablicy)) nieSprawdzaj = true;
            if (!nieSprawdzaj) {
                if (refTablica[(Ypuwg - refKorektaX) / szerokosc][(Xpuwg - refKorektaY) / szerokosc] == 1) {
                    if ((z > 5.0) && (z < 3000.0) && (tablicaHEM[j][i] < 13.0)) {
                        for (unsigned int jj = 0; jj < liczbaTorowZGwiazdka; ++jj) {
                            bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
                            double wektorP2P1x = refToryZGwiazdka[jj].xp1 - refToryZGwiazdka[jj].xp2;
                            double wektorP2P1y = refToryZGwiazdka[jj].yp1 - refToryZGwiazdka[jj].yp2;
                            int resztax1toru = (int)refToryZGwiazdka[jj].xp1 % 1000;
                            int resztay1toru = (int)refToryZGwiazdka[jj].yp1 % 1000;
                            if (resztax1toru == 0) waznyX = true;
                            if (resztay1toru == 0) waznyY = true;
                            if (waznyX) {
                                double testX = (refToryZGwiazdka[jj].xp1 + wektorP2P1x) / refToryZGwiazdka[jj].xp1;
                                if (testX >= 1) {
                                    rosnieX = true;
                                } else malejeX = true;
                            }
                            if (waznyY) {
                                double testY = (refToryZGwiazdka[jj].yp1 + wektorP2P1y) / refToryZGwiazdka[jj].yp1;
                                if (testY >= 1) {
                                    rosnieY = true;
                                } else malejeY = true;
                            }
                            if (rosnieY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyXpuwg = Xpuwg / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyYpuwg = Ypuwg / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyXpuwg = Xpuwg / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyYpuwg = Ypuwg / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (rosnieX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyYpuwg = Ypuwg / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyXpuwg = Xpuwg / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyYpuwg = Ypuwg / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyXpuwg = Xpuwg / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                        }
                        if (!doOdrzutu) {
                            refWierzcholki.push_back(wierzcholek{Ypuwg, Xpuwg, z});
                            ++id;
                            ++licznik;
                        }
                    } else refTablicaBrakow[(Ypuwg - refKorektaXbraki) / 50][(Xpuwg - refKorektaYbraki) / 50] = 1;
                }
            }
//            }
        }
        ++nrPunktu;
    }
    delete[]fileBuf;
//    std::cout << "\nLiczba znalezionych wierzcholkow w pliku: " << licznik << "\nW sumie znalezionych punktow: " << id << "\n\n";
    printf ("\nLiczba znalezionych wierzcholkow w pliku: %u", licznik);
    printf ("\nW sumie znalezionych punktow: %u \n\n", id);
}

void odczytPunktowDT2zUwzglednieniemProfilu(std::vector<wierzcholek> &refWierzcholki, std::vector<wierzcholek> &refWierzcholkiProfilu,
        std::vector<std::vector<unsigned int> > &refTablica1, std::vector<std::vector<unsigned int> > &refTablica2,
        std::vector<std::vector<unsigned int> > &refTablicaBrakow, std::string nazwaPliku, unsigned int &refNrId,
        unsigned int NrPliku, unsigned int LiczbaPlikow, unsigned int szerokosc1, unsigned int szerokosc2, std::vector<punktyTorow> &refToryZGwiazdka,
        unsigned int refKorektaX1, unsigned int refKorektaY1, unsigned int refKorektaX2, unsigned int refKorektaY2, unsigned int refKorektaXbraki,
        unsigned int refKorektaYbraki, unsigned int refWierszeTablicy1, unsigned int refKolumnyTablicy1, unsigned int refWierszeTablicy2,
        unsigned int refKolumnyTablicy2) {
    unsigned int licznik = 0, dlugoscNazwyPliku = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
    double minutaX = 0, minutaY = 0;
    const double sekunda = 1.0/3600.0;
    dlugoscNazwyPliku = nazwaPliku.length(); // 30 dla .dt2
    std::cout << "nazwaPliku=" << nazwaPliku << "\n";
    std::string nrx = nazwaPliku.substr(dlugoscNazwyPliku-21,2);
    std::string nry = nazwaPliku.substr(dlugoscNazwyPliku-28,2);
    std::string mnX = nazwaPliku.substr(dlugoscNazwyPliku-19,2);
    std::string mnY = nazwaPliku.substr(dlugoscNazwyPliku-26,2);
    if (mnX == "15") minutaX = 15.0/60.0;
    if (mnX == "30") minutaX = 30.0/60.0;
    if (mnX == "45") minutaX = 45.0/60.0;
    if (mnY == "15") minutaY = 15.0/60.0;
    if (mnY == "30") minutaY = 30.0/60.0;
    if (mnY == "45") minutaY = 45.0/60.0;
    const double XwsgPoczatek = atof(nrx.c_str());
    const double YwsgPoczatek = atof(nry.c_str());
    const unsigned int SRTM_SIZE = 900;
    // Otwarcie pliku DEM
    // unsigned char moze przechowywac 1 Byte (8bits) danych (0-255)
    typedef unsigned char BYTE;
    std::cout << "Otwieram plik " << nazwaPliku << " " << NrPliku + 1 << " z " << LiczbaPlikow <<" \n";
    BYTE *fileBuf;			// Pointer do danych
    FILE *file = NULL;		// File pointer
    if ((file = fopen(nazwaPliku.c_str(), "rb")) == NULL) {
        std::cout << "Nie mozna otworzyc pliku" << nazwaPliku << "\n";
        std::cin.get();
    }
    // Jaka jest wielkosc pliku?
    unsigned long fileSize = getFileSize(file);
    // Allokowanie miejsca na caly plik
    fileBuf = new BYTE[fileSize];
    // Odczyt pliku do bufora
    size_t sizeRead1 = fread(fileBuf, fileSize, 1, file);
    if (sizeRead1 != 1) {
        std::cout << "\nsizeRead ERROR!\n";
        std::cin.get();
    }
    std::vector<std::vector<unsigned int> > tablicaDEM;
    tablicaDEM.resize(901);
    for (unsigned int i = 0; i < 901; ++i) {
        tablicaDEM[i].resize(901);
    }
    for (unsigned int i = 3436, wiersze = 0, kolumny = 0, k = 0; i < fileSize; ++i) {
        if (k == 1) {
            tablicaDEM[wiersze][kolumny] = (fileBuf[i] | (fileBuf[i - 1] << 8));
            ++kolumny;
            if (kolumny > SRTM_SIZE) {
                kolumny = 0;
                ++wiersze;
                i = i + 12;
            }
        }
        ++k;
        if (k == 2) k = 0;
    }
    double geoidUndulation = 10*(fileBuf[491] - '0') + fileBuf[492] - '0';
    fclose(file);
    delete[]fileBuf;
    nazwaPliku.replace(dlugoscNazwyPliku-34,3,"HEM");
    nazwaPliku.replace(dlugoscNazwyPliku-7,3,"HEM");
    std::cout << "Otwieram plik z korekta wysokosci" << nazwaPliku << "\n";
    if ((file = fopen(nazwaPliku.c_str(), "rb")) == NULL) {
        std::cout << "Nie mozna otworzyc pliku" << nazwaPliku << "\n";
        std::cin.get();
    }
    // Jaka jest wielkosc pliku?
    fileSize = getFileSize(file);
    // Allokowanie miejsca na caly plik
    fileBuf = new BYTE[fileSize];
    // Odczyt pliku do bufora
    size_t sizeRead2 = fread(fileBuf, fileSize, 1, file);
    if (sizeRead2 != 1) {
        std::cout << "\nsizeRead ERROR!\n";
        std::cin.get();
    }
    std::vector<std::vector<unsigned int> > tablicaHEM;
    tablicaHEM.resize(901);
    for (unsigned int i = 0; i < 901; ++i) {
        tablicaHEM[i].resize(901);
    }
    for (unsigned int i = 3436, wiersze = 0, kolumny = 0, k = 0; i < fileSize; ++i) {
        if (k == 1) {
            tablicaHEM[wiersze][kolumny] = (fileBuf[i] | (fileBuf[i - 1] << 8));
            ++kolumny;
            if (kolumny > SRTM_SIZE) {
                kolumny = 0;
                ++wiersze;
                i = i + 12;
            }
        }
        ++k;
        if (k == 2) k = 0;
    }
    fclose(file);
// Kod przeksztalcenia formatu WGS84 do PUWG 1992 zostal zapozyczony i zoptymalizowany. Naglowek autora ponizej
    /*
    Autor: Zbigniew Szymanski
    E-mail: z.szymanski@szymanski-net.eu
    Wersja: 1.1
    Historia zmian:
    		1.1 dodano przeksztalcenie odwrotne PUWG 1992 ->WGS84
    		1.0 przeksztalcenie WGS84 -> PUWG 1992
    Data modyfikacji: 2012-11-27
    Uwagi: Oprogramowanie darmowe. Dozwolone jest wykorzystanie i modyfikacja
           niniejszego oprogramowania do wlasnych celow pod warunkiem
           pozostawienia wszystkich informacji z naglowka. W przypadku
           wykorzystania niniejszego oprogramowania we wszelkich projektach
           naukowo-badawczych, rozwojowych, wdrozeniowych i dydaktycznych prosze
           o zacytowanie nastepujacego artykulu:

           Zbigniew Szymanski, Stanislaw Jankowski, Jan Szczyrek,
           "Reconstruction of environment model by using radar vector field histograms.",
           Photonics Applications in Astronomy, Communications, Industry, and
           High-Energy Physics Experiments 2012, Proc. of SPIE Vol. 8454, pp. 845422 - 1-8,
           doi:10.1117/12.2001354

    Literatura:
           Uriasz, J., "Wybrane odwzorowania kartograficzne", Akademia Morska w Szczecinie,
           http://uriasz.am.szczecin.pl/naw_bezp/odwzorowania.html
    */
// Parametry elipsoidididy GRS-80
    const double e = 0.0818191910428;  	//pierwszymimosrod elipsoidy
    const double R0 = 6367449.14577; 		//promien sfery Lagrange.a
    const double Snorm = 2.0E-6;   		//parametr normujacy
    const double xo = 5760000.0; 		//parametr centrujacy
//Wspolczynniki wielomianu
    const double a0 = 5765181.11148097;
    const double a1 = 499800.81713800;
    const double a2 = -63.81145283;
    const double a3 = 0.83537915;
    const double a4 = 0.13046891;
    const double a5 = -0.00111138;
    const double a6 = -0.00010504;
// Parametry odwzorowania Gaussa-Kruegera dla ukladu PUWG92
    const double L0_stopnie = 19.0; 		//Poczatek ukladu wsp. PUWG92 (dlugosc)
    const double m0 = 0.9993;
    const double x0 = -5300000.0;
    const double y0 = 500000.0;
    unsigned int nrPunktu = 0;
    for (int i = SRTM_SIZE; i >= 0; --i) {
        double Xwsg = XwsgPoczatek + minutaX + (i * sekunda);
        double B = Xwsg * M_PI / 180.0;
        double U = 1.0 - e * sin(B);
        double V = 1.0 + e * sin(B);
        double K = pow((U / V),(e / 2.0));
        double C = K * tan(B / 2.0 + M_PI / 4.0);
        double fi = 2.0 * atan(C) - M_PI / 2.0;
        double p = sin(fi);
        double cosfi = cos(fi);
        for (unsigned int j = 0; j <= SRTM_SIZE; ++j) {
            double Ywsg = YwsgPoczatek + minutaY + (j * sekunda);
            double z = tablicaDEM[j][i] - tablicaHEM[j][i] - geoidUndulation;
//            if ((z > 5.0) && ( z < 3000.0)) {
            double dL_stopnie = Ywsg - L0_stopnie;
            double d_lambda = dL_stopnie * M_PI / 180.0;
// Etap I - elipsoida na kule
// Etap II - kula na walec
            double sindlambda = sin(d_lambda);
            double q = cosfi * cos(d_lambda);
            double r = 1.0 + cosfi * sindlambda;
            double s = 1.0 - cosfi * sindlambda;
            double XMERC = R0 * atan(p / q);
            double YMERC = 0.5 * R0 * log(r / s);
// Etap III - walec na plaszczyzne
            std::complex<double> Z((XMERC - xo) * Snorm,YMERC * Snorm);
            std::complex<double> Zgk;
            Zgk = a0 + Z *(a1 + Z * (a2 + Z * (a3 + Z * (a4 + Z * (a5 + Z * a6)))));
            double Xgk=Zgk.real();
            double Ygk=Zgk.imag();
// Przejscie do ukladu aplikacyjnego
            double Xpuwg = m0 * Xgk + x0;
            double Ypuwg = m0 * Ygk + y0;
            bool zaBlisko = false, nieSprawdzaj = false;
            if (((Ypuwg - refKorektaX1) / szerokosc1 < 1) || ((Ypuwg - refKorektaX1) / szerokosc1 > refWierszeTablicy1)) nieSprawdzaj = true;
            if (((Xpuwg - refKorektaY1) / szerokosc1 < 1) || ((Xpuwg - refKorektaY1) / szerokosc1 > refKolumnyTablicy1)) nieSprawdzaj = true;
            if (!nieSprawdzaj) {
                if (refTablica1[(Ypuwg - refKorektaX1) / szerokosc1][(Xpuwg - refKorektaY1) / szerokosc1] == 1) {
                    if ((z > 5.0) && (z < 3000.0) && (tablicaHEM[j][i] < 13.0)) {
                        bool doOdrzutu = false;
                        if (((Ypuwg - refKorektaX2) / szerokosc2 < 1) || ((Ypuwg - refKorektaX2) / szerokosc2 > refWierszeTablicy2)) nieSprawdzaj = true;
                        if (((Xpuwg - refKorektaY2) / szerokosc2 < 1) || ((Xpuwg - refKorektaY2) / szerokosc2 > refKolumnyTablicy2)) nieSprawdzaj = true;
                        if (!nieSprawdzaj) {
                            if (refTablica2[(Ypuwg - refKorektaX2) / szerokosc2][(Xpuwg - refKorektaY2) / szerokosc2] == 1) zaBlisko = true;
                        }
                        if (!zaBlisko) {
                            for (unsigned int jj = 0; jj < liczbaTorowZGwiazdka; ++jj) {
                                bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
                                double wektorP2P1x = refToryZGwiazdka[jj].xp1 - refToryZGwiazdka[jj].xp2;
                                double wektorP2P1y = refToryZGwiazdka[jj].yp1 - refToryZGwiazdka[jj].yp2;
                                int resztax1toru = (int)refToryZGwiazdka[jj].xp1 % 1000;
                                int resztay1toru = (int)refToryZGwiazdka[jj].yp1 % 1000;

                                if (resztax1toru == 0) waznyX = true;
                                if (resztay1toru == 0) waznyY = true;

                                if (waznyX) {
//debug                                double nieprzesunietyxp1 = refToryZGwiazdka[jj].xp1;
//debug                                double przesunietyXp1 = refToryZGwiazdka[jj].xp1 + wektorP2P1x;
                                    double testX = (refToryZGwiazdka[jj].xp1 + wektorP2P1x) / refToryZGwiazdka[jj].xp1;
                                    if (testX >= 1) {
                                        rosnieX = true;
                                    } else malejeX = true;
                                }
                                if (waznyY) {
//debug                                double nieprzesunietyyp1 = refToryZGwiazdka[jj].yp1;
//debug                                double przesunietyYp1 = refToryZGwiazdka[jj].yp1 + wektorP2P1y;
                                    double testY = (refToryZGwiazdka[jj].yp1 + wektorP2P1y) / refToryZGwiazdka[jj].yp1;
                                    if (testY >= 1) {
                                        rosnieY = true;
                                    } else malejeY = true;
                                }
                                if (rosnieY) {
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (malejeY) {
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (rosnieX) {
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (malejeX) {
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyYpuwg = Ypuwg / 1000;
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyXpuwg = Xpuwg / 1000;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyYpuwg;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyXpuwg;
                                    if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                            }
                            if (!doOdrzutu) {
                                refWierzcholki.push_back(wierzcholek{Ypuwg, Xpuwg, z});
                                ++refNrId;
                                ++licznik;
                            }
                        }
                    } else refTablicaBrakow[(Ypuwg - refKorektaXbraki) / 50][(Xpuwg - refKorektaYbraki) / 50] = 1;
                }
            }
//            }
        }
        ++nrPunktu;
    }
    delete[]fileBuf;
    std::cout << "\nLiczba znalezionych wierzcholkow w pliku: " << licznik << "\nW sumie znalezionych punktow: " << refNrId << "\n\n";
}

void odczytPunktowTXT(std::vector<wierzcholek> &refWierzcholki, std::vector<std::vector<unsigned int> > &refTablica, std::string nazwaPliku,
                      unsigned int &id, unsigned int &refNrPliku, unsigned int &refLiczbaPlikow, unsigned int szerokosc,
                      std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX, unsigned int refKorektaY, unsigned int refWierszeTablicy,
                      unsigned int refKolumnyTablicy) {
    unsigned int licznik = 0, nrPunktu = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
    std::cout << "nazwaPliku=" << nazwaPliku << "\n";
    std::cout << "Otwieram plik " << nazwaPliku << " " << refNrPliku + 1 << " z " << refLiczbaPlikow <<" \n";
    std::ifstream plik1;
    plik1.open(nazwaPliku.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
    while(!plik1.eof()) {
        std::string linia ("");
        std::getline(plik1, linia);
        std::istringstream ln(linia);
        unsigned int wyraz = 0;
        double x = 0, y =0, z = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po " ")
        while (!ln.eof()) {
            std::string temp ("");
            std::getline(ln, temp, ' ');
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
            if (wyraz == 0) x = atof (temp.c_str()); // X
            if (wyraz == 1) y = atof (temp.c_str()); // Y
            if (wyraz == 2) {
                z = atof (temp.c_str()); //Z
                bool doOdrzutu = false, nieSprawdzaj = false;
                if (((x - refKorektaX) / szerokosc < 1) || ((x - refKorektaX) / szerokosc > refWierszeTablicy)) nieSprawdzaj = true;
                if (((y - refKorektaY) / szerokosc < 1) || ((y - refKorektaY) / szerokosc > refKolumnyTablicy)) nieSprawdzaj = true;
                if (!nieSprawdzaj) {
                    if (refTablica[(x - refKorektaX) / szerokosc][(y - refKorektaY) / szerokosc] == 1) {
                        for (unsigned int jj = 0; jj < liczbaTorowZGwiazdka; ++jj) {
                            bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
                            double wektorP2P1x = refToryZGwiazdka[jj].xp1 - refToryZGwiazdka[jj].xp2;
                            double wektorP2P1y = refToryZGwiazdka[jj].yp1 - refToryZGwiazdka[jj].yp2;
                            int resztax1toru = (int)refToryZGwiazdka[jj].xp1 % 1000;
                            int resztay1toru = (int)refToryZGwiazdka[jj].yp1 % 1000;
                            if (resztax1toru == 0) waznyX = true;
                            if (resztay1toru == 0) waznyY = true;
                            if (waznyX) {
                                double testX = (refToryZGwiazdka[jj].xp1 + wektorP2P1x) / refToryZGwiazdka[jj].xp1;
                                if (testX >= 1) {
                                    rosnieX = true;
                                } else malejeX = true;
                            }
                            if (waznyY) {
                                double testY = (refToryZGwiazdka[jj].yp1 + wektorP2P1y) / refToryZGwiazdka[jj].yp1;
                                if (testY >= 1) {
                                    rosnieY = true;
                                } else malejeY = true;
                            }
                            if (rosnieY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeY) {
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (rosnieX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                            if (malejeX) {
                                int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                int zaokraglonyX = x / 1000;
                                int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                int zaokraglonyY = y / 1000;
                                int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                    doOdrzutu = true;
                                    break;
                                }
                            }
                        }
                        if (!doOdrzutu) {
                            refWierzcholki.push_back(wierzcholek{x, y, z});
                            ++id;
                            ++licznik;
                        }
                    }
                }
            }
            ++wyraz;
        }
        ++nrPunktu;
//        std::cout << "Petla nr " << nrPunktu << " z 1201           \r";
        printf ("Petla nr %u         \r", nrPunktu);
    }
//    std::cout << "\nLiczba znalezionych wierzcholkow w pliku: " << licznik << "\nW sumie znalezionych punktow: " << id << "\n\n";
    printf ("\nLiczba znalezionych wierzcholkow w pliku: %u", licznik);
    printf ("\nW sumie znalezionych punktow: %u \n\n", id);
}

void odczytPunktowTXTzUwzglednieniemProfilu(std::vector<wierzcholek> &refWierzcholki, std::vector<wierzcholek> &refWierzcholkiProfilu,
        std::vector<std::vector<unsigned int> > &refTablica1, std::vector<std::vector<unsigned int> > &refTablica2, std::string nazwaPliku,
        unsigned int &refNrId, unsigned int NrPliku, unsigned int LiczbaPlikow, unsigned int szerokosc1,
        unsigned int szerokosc2, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX1, unsigned int refKorektaY1, unsigned int refKorektaX2,
        unsigned int refKorektaY2, unsigned int refWierszeTablicy1, unsigned int refKolumnyTablicy1, unsigned int refWierszeTablicy2,
        unsigned int refKolumnyTablicy2) {
    unsigned int nrLinii = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
    std::cout << "nazwaPliku=" << nazwaPliku << "\n";
    std::cout << "Otwieram plik " << nazwaPliku << " " << NrPliku + 1 << " z " << LiczbaPlikow <<" \n";
    std::ifstream plik1;
    plik1.open(nazwaPliku.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
    while(!plik1.eof()) {
        std::string linia ("");
        std::getline(plik1, linia);
        std::istringstream ln(linia);
        unsigned int wyraz = 0;
        double x = 0, y =0, z = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po " ")
        while (!ln.eof()) {
            std::string temp ("");
            std::getline(ln, temp, ' ');
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
            if (wyraz == 0) x = atof (temp.c_str()); // X
            if (wyraz == 1) y = atof (temp.c_str()); // Y
            if (wyraz == 2) {
                bool zaBlisko = false, nieSprawdzaj = false, doOdrzutu = false;
                z = atof (temp.c_str()); // Z
                if (((x - refKorektaX1) / szerokosc1 < 1) || ((x - refKorektaX1) / szerokosc1 > refWierszeTablicy1)) nieSprawdzaj = true;
                if (((y - refKorektaY1) / szerokosc1 < 1) || ((y - refKorektaY1) / szerokosc1 > refKolumnyTablicy1)) nieSprawdzaj = true;
                if (!nieSprawdzaj) {
                    if (refTablica1[(x - refKorektaX1) / szerokosc1][(y - refKorektaY1) / szerokosc1] == 1) {
                        if (((x - refKorektaX2) / szerokosc2 < 1) || ((x - refKorektaX2) / szerokosc2 > refWierszeTablicy2)) nieSprawdzaj = true;
                        if (((y - refKorektaY2) / szerokosc2 < 1) || ((y - refKorektaY2) / szerokosc2 > refKolumnyTablicy2)) nieSprawdzaj = true;
                        if (!nieSprawdzaj) {
                            if (refTablica2[(x - refKorektaX2) / szerokosc2][(y - refKorektaY2) / szerokosc2] == 1) zaBlisko = true;
                        }
                        if (!zaBlisko) {
                            for (unsigned int jj = 0; jj < liczbaTorowZGwiazdka; ++jj) {
                                bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;

                                double wektorP2P1x = refToryZGwiazdka[jj].xp1 - refToryZGwiazdka[jj].xp2;
                                double wektorP2P1y = refToryZGwiazdka[jj].yp1 - refToryZGwiazdka[jj].yp2;

                                int resztax1toru = (int)refToryZGwiazdka[jj].xp1 % 1000;
                                int resztay1toru = (int)refToryZGwiazdka[jj].yp1 % 1000;

                                if (resztax1toru == 0) waznyX = true;
                                if (resztay1toru == 0) waznyY = true;

                                if (waznyX) {
//debug                                double nieprzesunietyxp1 = refToryZGwiazdka[jj].xp1;
//debug                                double przesunietyXp1 = refToryZGwiazdka[jj].xp1 + wektorP2P1x;
                                    double testX = (refToryZGwiazdka[jj].xp1 + wektorP2P1x) / refToryZGwiazdka[jj].xp1;
                                    if (testX >= 1) {
                                        rosnieX = true;
                                    } else malejeX = true;
                                }
                                if (waznyY) {
//debug                                double nieprzesunietyyp1 = refToryZGwiazdka[jj].yp1;
//debug                                double przesunietyYp1 = refToryZGwiazdka[jj].yp1 + wektorP2P1y;
                                    double testY = (refToryZGwiazdka[jj].yp1 + wektorP2P1y) / refToryZGwiazdka[jj].yp1;
                                    if (testY >= 1) {
                                        rosnieY = true;
                                    } else malejeY = true;
                                }
                                if (rosnieY) {
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyY = y / 1000;
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyX = x / 1000;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                    if ((roznicaY < 1) && (roznicaY > -8) && (roznicaX < 4) && (roznicaX > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (malejeY) {
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyY = y / 1000;
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyX = x / 1000;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                    if ((roznicaY > 0) && (roznicaY < 8) && (roznicaX < 4) && (roznicaX > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (rosnieX) {
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyX = x / 1000;
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyY = y / 1000;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                    if ((roznicaX < 1) && (roznicaX > -8) && (roznicaY < 4) && (roznicaY > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                                if (malejeX) {
                                    int zaokraglonyxp1 = refToryZGwiazdka[jj].xp1 / 1000;
                                    int zaokraglonyX = x / 1000;
                                    int zaokraglonyyp1 = refToryZGwiazdka[jj].yp1 / 1000;
                                    int zaokraglonyY = y / 1000;
                                    int roznicaX = zaokraglonyxp1 - zaokraglonyX;
                                    int roznicaY = zaokraglonyyp1 - zaokraglonyY;
                                    if ((roznicaX > 0) && (roznicaX < 8) && (roznicaY < 4) && (roznicaY > -4)) {
                                        doOdrzutu = true;
                                        break;
                                    }
                                }
                            }
                            if (!doOdrzutu) {
                                refWierzcholki.push_back(wierzcholek{x, y, z});
                                ++refNrId;
                            }
                        }
                    }
                }
            }
            ++wyraz;
        }
        ++nrLinii;
        std::cout << "Linia nr " << nrLinii << "          \r";
    }
    std::cout << "\nLiczba znalezionych punktow NMT100 w pliku: " << nrLinii << "\nW sumie znalezionych punktow: " << refNrId << "\n\n";
}

void odczytPlikuPoTriangulacji(std::vector<triangle> &refTriangles, std::vector<wierzcholek> &refWierzcholki, std::string zarostek,
                               const double ograniczenieTrojkata, unsigned int &refLicznikTrojkatow, std::vector<wierzcholek> &refWierzcholkiTriangles, double ExportX,
                               double ExportY) {
    std::string linia ("");
    std::string ln ("");
    unsigned int nrLinii = 0;
// Odczytuje striangulowane programem Triangle wierzcholki
    std::ifstream plik1;
    std::string nazwaPliku = "wierzcholki.1.ele";
    nazwaPliku.insert(11,zarostek);
    std::cout << "Otwieram plik " << nazwaPliku << " z trojkatami.\n";
    plik1.open(nazwaPliku.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
    while(!plik1.eof()) {
        std::getline(plik1, linia);
        std::istringstream ln(linia);
        unsigned int wyraz = 0, temp2 = 0, temp3 = 0, temp4 = 0;
        bool flagPunkty = false;
        double AB = 0, BC = 0, CA = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po " ")
        if (nrLinii > 0) {
            while (!ln.eof()) {
                std::string temp ("");
                std::getline(ln, temp, ' ');
                bool znalazlem = false;
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
                if (temp == "") {
                    flagPunkty = true;
                } else {
                    flagPunkty = false;
                }
                if (temp == "#") break;
                if (!flagPunkty) {
                    if (wyraz == 1) temp2 = atoi(temp.c_str()); // X
                    if (wyraz == 2) temp3 = atoi(temp.c_str()); // Y
                    if (wyraz == 3) {
                        temp4 = atoi(temp.c_str()); // Z
                        if (!znalazlem) {
                            AB = hypot(refWierzcholki[temp3].x - refWierzcholki[temp2].x, refWierzcholki[temp3].y - refWierzcholki[temp2].y);
                            BC = hypot(refWierzcholki[temp4].x - refWierzcholki[temp3].x, refWierzcholki[temp4].y - refWierzcholki[temp3].y);
                            CA = hypot(refWierzcholki[temp2].x - refWierzcholki[temp4].x, refWierzcholki[temp2].y - refWierzcholki[temp4].y);
                            if ((AB < ograniczenieTrojkata) && (BC < ograniczenieTrojkata) && (CA < ograniczenieTrojkata)) {
                                bool doOdrzutu1 = false, doOdrzutu2 = false, doOdrzutu3 = false;
                                for (unsigned i = 0; i < refWierzcholkiTriangles.size(); ++i) {
                                    if (((refWierzcholki[temp2].x - refWierzcholkiTriangles[i].x > -0.01) && (refWierzcholki[temp2].x - refWierzcholkiTriangles[i].x < 0.01))
                                            && ((refWierzcholki[temp2].y - refWierzcholkiTriangles[i].y > -0.01)
                                                && (refWierzcholki[temp2].y - refWierzcholkiTriangles[i].y < 0.01))) doOdrzutu1 = true;
                                    if (((refWierzcholki[temp3].x - refWierzcholkiTriangles[i].x > -0.01) && (refWierzcholki[temp3].x - refWierzcholkiTriangles[i].x < 0.01))
                                            && ((refWierzcholki[temp3].y - refWierzcholkiTriangles[i].y > -0.01)
                                                && (refWierzcholki[temp3].y - refWierzcholkiTriangles[i].y < 0.01))) doOdrzutu2 = true;
                                    if (((refWierzcholki[temp4].x - refWierzcholkiTriangles[i].x > -0.01) && (refWierzcholki[temp4].x - refWierzcholkiTriangles[i].x < 0.01))
                                            && ((refWierzcholki[temp4].y - refWierzcholkiTriangles[i].y > -0.01)
                                                && (refWierzcholki[temp4].y - refWierzcholkiTriangles[i].y < 0.01))) doOdrzutu3 = true;
                                }
                                if ((!doOdrzutu1) || (!doOdrzutu2) || (!doOdrzutu3)) {
                                    double XwektorAB = ((refWierzcholki[temp2].x - ExportX) * -1.0) - ((refWierzcholki[temp3].x - ExportX) * -1.0);
                                    double YwektorAB = (refWierzcholki[temp2].y - ExportY) - (refWierzcholki[temp3].y - ExportY);
                                    double ZwektorAB = refWierzcholki[temp2].z - refWierzcholki[temp3].z;
                                    double XwektorCB = ((refWierzcholki[temp2].x - ExportX) * -1.0) - ((refWierzcholki[temp4].x - ExportX) * -1.0);
                                    double YwektorCB = (refWierzcholki[temp2].y - ExportY) - (refWierzcholki[temp4].y - ExportY);
                                    double ZwektorCB = refWierzcholki[temp2].z - refWierzcholki[temp4].z;
                                    double XiloczynABiCB = (YwektorAB * ZwektorCB) - (ZwektorAB * YwektorCB);
                                    double YiloczynABiCB = (ZwektorAB * XwektorCB) - (XwektorAB * ZwektorCB);
                                    double ZiloczynABiCB = (XwektorAB * YwektorCB) - (YwektorAB * XwektorCB);
                                    double dlugoscIloczynABiCB = hypot(XiloczynABiCB, hypot(YiloczynABiCB, ZiloczynABiCB));
                                    refTriangles.push_back(triangle{refWierzcholki[temp2].x, refWierzcholki[temp2].y, refWierzcholki[temp2].z, refWierzcholki[temp3].x, refWierzcholki[temp3].y, refWierzcholki[temp3].z, refWierzcholki[temp4].x, refWierzcholki[temp4].y, refWierzcholki[temp4].z, XiloczynABiCB / dlugoscIloczynABiCB, YiloczynABiCB / dlugoscIloczynABiCB, ZiloczynABiCB / dlugoscIloczynABiCB, ZiloczynABiCB, -1});
//                                    refTriangles.push_back(triangle());
//                                    refTriangles[refLicznikTrojkatow].x1 = refWierzcholki[temp2].x;
//                                    refTriangles[refLicznikTrojkatow].y1 = refWierzcholki[temp2].y;
//                                    refTriangles[refLicznikTrojkatow].z1 = refWierzcholki[temp2].z;
//                                    refTriangles[refLicznikTrojkatow].x2 = refWierzcholki[temp3].x;
//                                    refTriangles[refLicznikTrojkatow].y2 = refWierzcholki[temp3].y;
//                                    refTriangles[refLicznikTrojkatow].z2 = refWierzcholki[temp3].z;
//                                    refTriangles[refLicznikTrojkatow].x3 = refWierzcholki[temp4].x;
//                                    refTriangles[refLicznikTrojkatow].y3 = refWierzcholki[temp4].y;
//                                    refTriangles[refLicznikTrojkatow].z3 = refWierzcholki[temp4].z;
//                                    refTriangles[refLicznikTrojkatow].normalX = XiloczynABiCB / dlugoscIloczynABiCB;
//                                    refTriangles[refLicznikTrojkatow].normalY = YiloczynABiCB / dlugoscIloczynABiCB;
//                                    refTriangles[refLicznikTrojkatow].normalZ = ZiloczynABiCB / dlugoscIloczynABiCB;
//                                    refTriangles[refLicznikTrojkatow].Ziloczyn = ZiloczynABiCB;
//                                    refTriangles[refLicznikTrojkatow].nastepnyTrojkat = -1;
                                    ++refLicznikTrojkatow;
                                }
                            }
                            znalazlem = true;
                        }
                    }
                    ++wyraz;
                }
            }
        }
        ++nrLinii;
        std::cout << "Linia nr " << nrLinii << ", przyjety trojkat nr " << refLicznikTrojkatow << "        \r";
    }
    std::cout << "\nLiczba znalezionych trojkatow: " << refTriangles.size() << ". Odrzucono " << nrLinii - refLicznikTrojkatow << " trojkaty\n";
// Zamyka plik
    plik1.close();
}

void utworzDodatkowePunktySiatki(std::vector<triangle> &refTriangles, std::vector<wierzcholek> &refRefWierzcholki) {
    unsigned int liczbaTrojkatow = refTriangles.size();
    unsigned int dotychczasowaLiczbaWierzcholkow = refRefWierzcholki.size();
    std::cout << "Teraz czas utworzyc dodatkowe punkty zageszczajace siatke:\n";
    for (unsigned int i = 0, ii = dotychczasowaLiczbaWierzcholkow; i < liczbaTrojkatow; ++i, ++ii) {
        refRefWierzcholki.push_back(wierzcholek{(refTriangles[i].x1 + refTriangles[i].x2 + refTriangles[i].x3) / 3, (refTriangles[i].y1 + refTriangles[i].y2 + refTriangles[i].y3) / 3, (refTriangles[i].z1 + refTriangles[i].z2 + refTriangles[i].z3) / 3});
//        refRefWierzcholki.push_back(wierzcholek());
//        refRefWierzcholki[ii].x = (refTriangles[i].x1 + refTriangles[i].x2 + refTriangles[i].x3) / 3;
//        refRefWierzcholki[ii].y = (refTriangles[i].y1 + refTriangles[i].y2 + refTriangles[i].y3) / 3;
//        refRefWierzcholki[ii].z = (refTriangles[i].z1 + refTriangles[i].z2 + refTriangles[i].z3) / 3;
        std::cout << "Pierwotna liczba wierzcholkow: " << dotychczasowaLiczbaWierzcholkow << ". Nr dodatkowego wierzcholka" << i << "          \r";
    }
    std::cout << "\nKoniec tworzenia dodatkowych wierzcholkow\n";
}

void utworzDodatkowePunktySiatkiZUwzglednieniemProfilu(std::vector<triangle> &refTriangles, std::vector<wierzcholek> &refRefWierzcholki,
        std::vector<wierzcholek> &refRefWierzcholkiProfilu, unsigned int &refRefNrId, std::vector<std::vector<unsigned int> > &refRefTablica2,
        unsigned int szerokosc2, unsigned int refRefKorektaX2, unsigned int refRefKorektaY2, unsigned int refRefWierszeTablicy2,
        unsigned int refRefKolumnyTablicy2, std::vector<std::vector<unsigned int> > &refRefTablicaBrakow, unsigned int refRefKorektaXbraki,
        unsigned int refRefKorektaYbraki) {
    const unsigned int liczbaTrojkatow = refTriangles.size();
    unsigned int dotychczasowaLiczbaWierzcholkow = refRefNrId, noweWierzcholki = 0;
    const unsigned int liczbaWierzcholkowProfilu = refRefWierzcholkiProfilu.size();
    //W przypadku plikow SRTM 1 arc sec zageszczanie wierzcholkow nie daje zadnych korzysci
#ifdef HGT
    std::cout << "Teraz czas utowrzyc dodatkowe punkty zageszczajace siatke:\n";
    for (unsigned int z = 0; z < liczbaTrojkatow; ++z, ++refRefNrId) {
        bool zaBlisko = false, nieSprawdzaj = false;
        double nowyX = (refTriangles[z].x1 + refTriangles[z].x2 + refTriangles[z].x3) / 3.0;
        double nowyY = (refTriangles[z].y1 + refTriangles[z].y2 + refTriangles[z].y3) / 3.0;
        if (((nowyX - refRefKorektaX2) / szerokosc2 < 1) || ((nowyX - refRefKorektaX2) / szerokosc2 > refRefWierszeTablicy2)) nieSprawdzaj = true;
        if (((nowyY - refRefKorektaY2) / szerokosc2 < 1) || ((nowyY - refRefKorektaY2) / szerokosc2 > refRefKolumnyTablicy2)) nieSprawdzaj = true;
        if (refRefTablicaBrakow[(nowyX - refRefKorektaXbraki) / 50][(nowyY - refRefKorektaYbraki) / 50] < 1) nieSprawdzaj = true;
        if (!nieSprawdzaj) {
            // Aby nie tworzylo nowych wierzcholkow za blisko profilu z Rainsteda
            if (refRefTablica2[(nowyX - refRefKorektaX2) / szerokosc2][(nowyY - refRefKorektaY2) / szerokosc2] == 1) zaBlisko = true;
            // Bo tworzyly sie sporadycznie wierzcholki nad wierzcholkami
            for (unsigned int j = 0; j < dotychczasowaLiczbaWierzcholkow; ++j) {
                if (((refRefWierzcholki[j].x - nowyX > -0.01) && (refRefWierzcholki[j].x - nowyX < 0.01)) && ((refRefWierzcholki[j].y - nowyY > -0.01)
                        && (refRefWierzcholki[j].y - nowyY < 0.01))) zaBlisko = true;
            }
            if (!zaBlisko) {
                refRefWierzcholki.push_back(wierzcholek{nowyX, nowyY, (refTriangles[z].z1 + refTriangles[z].z2 + refTriangles[z].z3) / 3.0});
                ++noweWierzcholki;
                std::cout << "Pierwotna liczba wierz.: " << dotychczasowaLiczbaWierzcholkow << ". Liczba dodatkowych wierz.: " << noweWierzcholki << "          \r";
            }
        }
    }
#endif // HGT
    // A na koniec dodajemy wierzcholki profilu
    dotychczasowaLiczbaWierzcholkow = refRefWierzcholki.size();
    for (unsigned int i = 0, ii = dotychczasowaLiczbaWierzcholkow; i < liczbaWierzcholkowProfilu; ++i, ++ii) {
        refRefWierzcholki.push_back(wierzcholek{refRefWierzcholkiProfilu[i].x, refRefWierzcholkiProfilu[i].y, refRefWierzcholkiProfilu[i].z});
//        refRefWierzcholki.push_back(wierzcholek());
//        refRefWierzcholki[ii].x = refRefWierzcholkiProfilu[i].x;
//        refRefWierzcholki[ii].y = refRefWierzcholkiProfilu[i].y;
//        refRefWierzcholki[ii].z = refRefWierzcholkiProfilu[i].z;
    }
    std::cout << "\nKoniec tworzenia dodatkowych wierzcholkow\n";
}

void zapisSymkowychTrojkatow(std::vector<triangle> &refTriangles, double ExportX, double ExportY) {
    unsigned int licznik = 0, zmianaPliku1 = 2400000, zmianaPliku2 = 4800000, zmianaPliku3 = 7200000, zmianaPliku4 = 9600000, zmianaPliku5 = 12000000,
                 zmianaPliku6 = 14400000, zmianaPliku7 = 16800000, zmianaPliku8 = 19200000;
    std::string nazwaPliku = "teren.scm";
    unsigned int nrPliku = 1;
    unsigned int szerokosc = 300, testXmax = 0, testYmax = 0, testXmin = 900000, testYmin = 900000, wierszeTablicy = 0, kolumnyTablicy = 0;
    std::string numerPliku = to_string(nrPliku);
    std::string nowaNazwaPliku = nazwaPliku;
    nowaNazwaPliku.insert(5,numerPliku);
    unsigned int liczbaTrojkatow = refTriangles.size();

    std::cout << "Sprawdzenie jak duza ma byc tablica na trojkaty... ";
    for (unsigned int i = 0; i < liczbaTrojkatow; ++i) {
        if (testXmax < refTriangles[i].x1) testXmax = refTriangles[i].x1;
        if (testXmax < refTriangles[i].x2) testXmax = refTriangles[i].x2;
        if (testXmax < refTriangles[i].x3) testXmax = refTriangles[i].x3;

        if (testXmin > refTriangles[i].x1) testXmin = refTriangles[i].x1;
        if (testXmin > refTriangles[i].x2) testXmin = refTriangles[i].x2;
        if (testXmin > refTriangles[i].x3) testXmin = refTriangles[i].x3;

        if (testYmax < refTriangles[i].y1) testYmax = refTriangles[i].y1;
        if (testYmax < refTriangles[i].y2) testYmax = refTriangles[i].y2;
        if (testYmax < refTriangles[i].y3) testYmax = refTriangles[i].y3;

        if (testYmin > refTriangles[i].y1) testYmin = refTriangles[i].y1;
        if (testYmin > refTriangles[i].y2) testYmin = refTriangles[i].y2;
        if (testYmin > refTriangles[i].y3) testYmin = refTriangles[i].y3;
    }
    testXmin -= 2 * szerokosc;
    testXmax += 2 * szerokosc;
    testYmin -= 2 * szerokosc;
    testYmax += 2 * szerokosc;
    wierszeTablicy = (testXmax - testXmin) / szerokosc;
    kolumnyTablicy = (testYmax - testYmin) / szerokosc;
    std::vector<std::vector<int> > szachownica;
    szachownica.resize(wierszeTablicy);
    for (unsigned int i = 0; i < wierszeTablicy; ++i) {
        szachownica[i].resize(kolumnyTablicy);
    }
    for (unsigned int i = 0; i < wierszeTablicy; ++i) {
        for (unsigned int ii = 0; ii < kolumnyTablicy; ++ii) {
            szachownica[i][ii] = -1;
        }
    }
    std::cout << "Gotowe.\n";
    std::cout << "Oznaczanie trojkatow w poszczegolnych polach tablicy...\n";
    for (unsigned int i = 0; i < liczbaTrojkatow; ++i) {
        if (szachownica[(refTriangles[i].x1 - testXmin) / szerokosc][(refTriangles[i].y1 - testYmin) / szerokosc] == -1) {
            szachownica[(refTriangles[i].x1 - testXmin) / szerokosc][(refTriangles[i].y1 - testYmin) / szerokosc] = i;
        } else {
            int test = szachownica[(refTriangles[i].x1 - testXmin) / szerokosc][(refTriangles[i].y1 - testYmin) / szerokosc];
            int poprzedni = test;
            while (test != -1) {
                test = refTriangles[poprzedni].nastepnyTrojkat;
                if (test != -1) poprzedni = test;
            }
            refTriangles[poprzedni].nastepnyTrojkat = i;
        }
        std::cout << "Petla " << i << " z: " << liczbaTrojkatow << "                                   \r";
    }
    std::cout << "\nGotowe.\n\n";

// Czas zapisac w nowym pliku przetworzone dane.
    std::cout << "Otwieram plik " << nowaNazwaPliku << ", aby zapisac teren w formacie symulatora MaSzyna.\n";
    std::ofstream plik1;
// Otwiera plik
    plik1.open(nowaNazwaPliku.c_str());
    if (!plik1) {
        std::cout << "Brak pliku " << nowaNazwaPliku << "\n";
        std::cin.get();
    }
    plik1.setf( std::ios::fixed, std:: ios::floatfield );
// Zegarek
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
    std::string str(buffer);
    if (liczbaTrojkatow > 0) {
        plik1 << "// generated by TerenEU07.exe " << AutoVersion::FULLVERSION_STRING << AutoVersion::STATUS_SHORT << " on " << str << "\n";
        plik1 << "// Przesuniecie scenerii x = " << ExportX << ", y = " << ExportY << "\n\n";
    }
// W petli leci po wszystkich trojkatach
    for (unsigned int i = 0; i < liczbaTrojkatow; ++i) {
        double AnormalX = 0, AnormalY = 0, AnormalZ = 0, BnormalX = 0, BnormalY = 0, BnormalZ = 0, CnormalX = 0, CnormalY = 0, CnormalZ = 0;
        double AsumaNormal = 0, BsumaNormal = 0, CsumaNormal = 0;
        for (int przesuniecieX = -1; przesuniecieX < 2; ++przesuniecieX) {
            for (int przesuniecieY = -1; przesuniecieY < 2; ++przesuniecieY) {
                int test = szachownica[((refTriangles[i].x1 - testXmin) / szerokosc) + przesuniecieX][((refTriangles[i].y1 - testYmin) / szerokosc) + przesuniecieY];
                while (test != -1) {
                    if ((refTriangles[i].x1 > refTriangles[test].x1 - 0.01) && (refTriangles[i].x1 < refTriangles[test].x1 + 0.01)
                            && (refTriangles[i].y1 > refTriangles[test].y1 - 0.01) && (refTriangles[i].y1 < refTriangles[test].y1 + 0.01)) {
                        AnormalX += refTriangles[test].normalX;
                        AnormalY += refTriangles[test].normalY;
                        AnormalZ += refTriangles[test].normalZ;
                        ++AsumaNormal;
                    }
                    if ((refTriangles[i].x1 > refTriangles[test].x2 - 0.01) && (refTriangles[i].x1 < refTriangles[test].x2 + 0.01)
                            && (refTriangles[i].y1 > refTriangles[test].y2 - 0.01) && (refTriangles[i].y1 < refTriangles[test].y2 + 0.01)) {
                        AnormalX += refTriangles[test].normalX;
                        AnormalY += refTriangles[test].normalY;
                        AnormalZ += refTriangles[test].normalZ;
                        ++AsumaNormal;
                    }
                    if ((refTriangles[i].x1 > refTriangles[test].x3 - 0.01) && (refTriangles[i].x1 < refTriangles[test].x3 + 0.01)
                            && (refTriangles[i].y1 > refTriangles[test].y3 - 0.01) && (refTriangles[i].y1 < refTriangles[test].y3 + 0.01)) {
                        AnormalX += refTriangles[test].normalX;
                        AnormalY += refTriangles[test].normalY;
                        AnormalZ += refTriangles[test].normalZ;
                        ++AsumaNormal;
                    }
                    if ((refTriangles[i].x2 > refTriangles[test].x1 - 0.01) && (refTriangles[i].x2 < refTriangles[test].x1 + 0.01)
                            && (refTriangles[i].y2 > refTriangles[test].y1 - 0.01) && (refTriangles[i].y2 < refTriangles[test].y1 + 0.01)) {
                        BnormalX += refTriangles[test].normalX;
                        BnormalY += refTriangles[test].normalY;
                        BnormalZ += refTriangles[test].normalZ;
                        ++BsumaNormal;
                    }
                    if ((refTriangles[i].x2 > refTriangles[test].x2 - 0.01) && (refTriangles[i].x2 < refTriangles[test].x2 + 0.01)
                            && (refTriangles[i].y2 > refTriangles[test].y2 - 0.01) && (refTriangles[i].y2 < refTriangles[test].y2 + 0.01)) {
                        BnormalX += refTriangles[test].normalX;
                        BnormalY += refTriangles[test].normalY;
                        BnormalZ += refTriangles[test].normalZ;
                        ++BsumaNormal;
                    }
                    if ((refTriangles[i].x2 > refTriangles[test].x3 - 0.01) && (refTriangles[i].x2 < refTriangles[test].x3 + 0.01)
                            && (refTriangles[i].y2 > refTriangles[test].y3 - 0.01) && (refTriangles[i].y2 < refTriangles[test].y3 + 0.01)) {
                        BnormalX += refTriangles[test].normalX;
                        BnormalY += refTriangles[test].normalY;
                        BnormalZ += refTriangles[test].normalZ;
                        ++BsumaNormal;
                    }
                    if ((refTriangles[i].x3 > refTriangles[test].x1 - 0.01) && (refTriangles[i].x3 < refTriangles[test].x1 + 0.01)
                            && (refTriangles[i].y3 > refTriangles[test].y1 - 0.01) && (refTriangles[i].y3 < refTriangles[test].y1 + 0.01)) {
                        CnormalX += refTriangles[test].normalX;
                        CnormalY += refTriangles[test].normalY;
                        CnormalZ += refTriangles[test].normalZ;
                        ++CsumaNormal;
                    }
                    if ((refTriangles[i].x3 > refTriangles[test].x2 - 0.01) && (refTriangles[i].x3 < refTriangles[test].x2 + 0.01)
                            && (refTriangles[i].y3 > refTriangles[test].y2 - 0.01) && (refTriangles[i].y3 < refTriangles[test].y2 + 0.01)) {
                        CnormalX += refTriangles[test].normalX;
                        CnormalY += refTriangles[test].normalY;
                        CnormalZ += refTriangles[test].normalZ;
                        ++CsumaNormal;
                    }
                    if ((refTriangles[i].x3 > refTriangles[test].x3 - 0.01) && (refTriangles[i].x3 < refTriangles[test].x3 + 0.01)
                            && (refTriangles[i].y3 > refTriangles[test].y3 - 0.01) && (refTriangles[i].y3 < refTriangles[test].y3 + 0.01)) {
                        CnormalX += refTriangles[test].normalX;
                        CnormalY += refTriangles[test].normalY;
                        CnormalZ += refTriangles[test].normalZ;
                        ++CsumaNormal;
                    }
                    test = refTriangles[test].nastepnyTrojkat;
                }
            }
        }
        AnormalX += refTriangles[i].normalX;
        AnormalY += refTriangles[i].normalY;
        AnormalZ += refTriangles[i].normalZ;
        ++AsumaNormal;

        BnormalX += refTriangles[i].normalX;
        BnormalY += refTriangles[i].normalY;
        BnormalZ += refTriangles[i].normalZ;
        ++BsumaNormal;

        CnormalX += refTriangles[i].normalX;
        CnormalY += refTriangles[i].normalY;
        CnormalZ += refTriangles[i].normalZ;
        ++CsumaNormal;

        if (refTriangles[i].Ziloczyn > 0) {
            if ((refTriangles[i].z2 > 800.0) || (refTriangles[i].z1 > 800.0) || (refTriangles[i].z3 > 800.0)) {
                plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial snow\n";
            } else {
                plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial grass\n";
            }
//          plik1 << "// Trojkat 1 - Iloczyn wektora AB i CB =[" << XiloczynABiCB << ", " << YiloczynABiCB << ", " << ZiloczynABiCB << "] xyz. I dlugosc = " << dlugoscIloczynABiCB << "\n";
            plik1 << (refTriangles[i].x2 - ExportX) * -1.0 << " " << refTriangles[i].z2 << " " << refTriangles[i].y2 - ExportY << " " << AnormalX / AsumaNormal <<
                  " " << AnormalZ / AsumaNormal << " " << AnormalY / AsumaNormal << " " << ((refTriangles[i].x2 - ExportX) * -1.0) / 25.0 << " " <<
                  (refTriangles[i].y2 - ExportY) / 25.0 << " end\n";
            plik1 << (refTriangles[i].x1 - ExportX) * -1.0 << " " << refTriangles[i].z1 << " " << refTriangles[i].y1 - ExportY << " " << BnormalX / BsumaNormal <<
                  " " << BnormalZ / BsumaNormal << " " << BnormalY / BsumaNormal << " " << ((refTriangles[i].x1 - ExportX) * -1.0) / 25.0 << " " <<
                  (refTriangles[i].y1 - ExportY) / 25.0 << " end\n";
            plik1 << (refTriangles[i].x3 - ExportX) * -1.0 << " " << refTriangles[i].z3 << " " << refTriangles[i].y3 - ExportY << " " << CnormalX / CsumaNormal <<
                  " " << CnormalZ / CsumaNormal << " " << CnormalY / CsumaNormal << " " << ((refTriangles[i].x3 - ExportX) * -1.0) / 25.0 << " " <<
                  (refTriangles[i].y3 - ExportY) / 25.0 << "\n";
            plik1 << "endtri\n";

        } else {
            if ((refTriangles[i].z2 > 800.0) || (refTriangles[i].z1 > 800.0) || (refTriangles[i].z3 > 800.0)) {
                plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial snow\n";
            } else {
                plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial grass\n";
            }
//          plik1 << "// Trojkat 2 - Iloczyn wektora AB i CB =[" << XiloczynABiCB << ", " << YiloczynABiCB << ", " << ZiloczynABiCB << "] xyz. I dlugosc = " << dlugoscIloczynABiCB << "\n";
            plik1 << (refTriangles[i].x1 - ExportX) * -1.0 << " " << refTriangles[i].z1 << " " << refTriangles[i].y1 - ExportY << " " <<
                  (AnormalX * -1.0) / AsumaNormal << " " << (AnormalZ * -1.0) / AsumaNormal << " " << (AnormalY * -1.0) / AsumaNormal << " " << ((
                              refTriangles[i].x1 - ExportX) * -1.0) / 25.0 << " " << (refTriangles[i].y1 - ExportY) / 25.0 << " end\n";
            plik1 << (refTriangles[i].x2 - ExportX) * -1.0 << " " << refTriangles[i].z2 << " " << refTriangles[i].y2 - ExportY << " " <<
                  (BnormalX * -1.0) / BsumaNormal << " " << (BnormalZ * -1.0) / BsumaNormal << " " << (BnormalY * -1.0) / BsumaNormal << " " << ((
                              refTriangles[i].x2 - ExportX) * -1.0) / 25.0 << " " << (refTriangles[i].y2 - ExportY) / 25.0 << " end\n";
            plik1 << (refTriangles[i].x3 - ExportX) * -1.0 << " " << refTriangles[i].z3 << " " << refTriangles[i].y3 - ExportY << " " <<
                  (CnormalX * -1.0) / CsumaNormal << " " << (CnormalZ * -1.0) / CsumaNormal << " " << (CnormalY * -1.0) / CsumaNormal << " " << ((
                              refTriangles[i].x3 - ExportX) * -1.0) / 25.0 << " " << (refTriangles[i].y3 - ExportY) / 25.0 << "\n";
            plik1 << "endtri\n";
        }
        std::cout << "Petla " << i << " z: " << liczbaTrojkatow << "                                   \r";
        licznik += 6;
        if ((licznik == zmianaPliku1) || (licznik == zmianaPliku2) || (licznik == zmianaPliku3) || (licznik == zmianaPliku4) || (licznik == zmianaPliku5)
                || (licznik == zmianaPliku6) || (licznik == zmianaPliku7) || (licznik == zmianaPliku8)) {
            plik1.close();
            ++nrPliku;
            numerPliku = to_string(nrPliku);
            nowaNazwaPliku = nazwaPliku;
            nowaNazwaPliku.insert(5,numerPliku);
// Czas zapisac w nowym pliku przetworzone dane.
            std::cout << "Otwieram plik " << nowaNazwaPliku << "\n";
            plik1.open(nowaNazwaPliku.c_str());
            plik1 << "// generated by TerenEU07.exe " << AutoVersion::FULLVERSION_STRING << AutoVersion::STATUS_SHORT << " on " << str << "\n";
            plik1 << "// Przesuniecie scenerii x = " << ExportX << ", y = " << ExportY << "\n\n";
            if (!plik1) {
                std::cout << "Brak pliku " << nowaNazwaPliku << "\n";
                std::cin.get();
            }
            plik1.setf( std::ios::fixed, std:: ios::floatfield );
        }
    }
// Zamykamy nowo utworzony plik
    plik1.close();
}

void sadzenieDrzew(std::vector<triangle> &refTriangles, double ExportX, double ExportY) {
    unsigned int licznik = 0, zmianaPliku1 = 2400000, zmianaPliku2 = 4800000, zmianaPliku3 = 7200000, zmianaPliku4 = 9600000, zmianaPliku5 = 12000000,
                 zmianaPliku6 = 14400000, zmianaPliku7 = 16800000, zmianaPliku8 = 19200000;
    std::string nazwaPliku = "drzewa.scm";
    std::string wylosowaneDrzewo ("");
    unsigned int nrPliku = 1, z = 0, liczbaTrojkatow = refTriangles.size(), minWysokoscDrzewa = 0, maxWysokoscDrzewa = 0, minPromienKorona = 0,
                 maxPromienKorona = 0;
    std::vector<std::string> tabelaDrzew;
    tabelaDrzew.clear();

    std::string pathToTree ("." "\x5C" "l61_plants");
    std::string pathToTreeClosed ("." "\x5C" "l61_plants" "\x5C");
    std::string errorTreeString ("failed to open directory ." "\x5C" "l61_plants");
// Znalezny kod do listowania plikow z aktualnego katalogu
    DIR*     dir;
    dirent*  pdir;
    if (!(dir = opendir(pathToTree.c_str()))) {
        perror(errorTreeString.c_str());
        return;
    }
    dir = opendir(pathToTree.c_str());
    while ((pdir = readdir(dir)) != NULL) {
        if (!strcmp(pdir->d_name, "..") || !strcmp(pdir->d_name, ".")) {
            continue;
        }
// Drobne poprawki i znajduje tylko z takim rozszerzeniem, z jakim chcialem
        std::string nazwaDrzewa ("");
        nazwaDrzewa = pdir->d_name;
        int znaleziono = nazwaDrzewa.find("dds");
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            nazwaDrzewa = nazwaDrzewa.insert(0, pathToTreeClosed);
            tabelaDrzew.push_back(nazwaDrzewa);
            std::cout << nazwaDrzewa << "\n";
        }
    }
    closedir(dir);
    unsigned int liczbaDrzew = tabelaDrzew.size();
    if (liczbaDrzew > 0) {
        std::string numerPliku = to_string(nrPliku);
        std::string nowaNazwaPliku = nazwaPliku;
        nowaNazwaPliku.insert(6,numerPliku);
// Czas zapisac w nowym pliku przetworzone dane.
        std::cout << "Otwieram plik " << nowaNazwaPliku << ", aby zapisac drzewa w formacie symulatora MaSzyna.\n";
        std::ofstream plik1;
// Otwiera plik
        plik1.open(nowaNazwaPliku.c_str());
        if (!plik1) {
            std::cout << "Brak pliku " << nowaNazwaPliku << "\n";
            std::cin.get();
        }
        plik1.setf( std::ios::fixed, std:: ios::floatfield );
// Zegarek
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];
        time (&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
        std::string str(buffer);
        if (liczbaTrojkatow > 0) {
            plik1 << "// generated by TerenEU07.exe " << AutoVersion::FULLVERSION_STRING << AutoVersion::STATUS_SHORT << " on " << str << "\n";
            plik1 << "// Przesuniecie scenerii x = " << ExportX << ", y = " << ExportY << "\n\n";
        }
        srand( time( NULL ) );
// W petli leci po wszystkich punktach NMT
        for (unsigned int i = 0; i < liczbaTrojkatow; ++i) {
            if (z == 0) {
                wylosowaneDrzewo = tabelaDrzew[rand() % liczbaDrzew];
                z = 5000;
                int znaleziono1 = (wylosowaneDrzewo.find("drzewo"));
                int znaleziono2 = (wylosowaneDrzewo.find("sosna"));
// Zeby drzewa nie byly zbyt male
                if ((znaleziono1 != -1) || (znaleziono2 != -1)) {
                    minWysokoscDrzewa = 4;
                    maxWysokoscDrzewa = 31;
                    minPromienKorona = 4;
                    maxPromienKorona = 13;
// Zeby krzaki nie byly tak wysokie jak drzewa
                } else {
                    minWysokoscDrzewa = 1;
                    maxWysokoscDrzewa = 4;
                    minPromienKorona = 1;
                    maxPromienKorona = 4;
                }
// Zeby nie robic lasow martwych drzew
                int znaleziono3 = (wylosowaneDrzewo.find("dead"));
                if (znaleziono3 != -1) {
                    z = 2;
                }
            }
            --z;
// Wspolrzedne
// Jedno drzewo na trojkat
            double barycentrum1x = (refTriangles[i].x1 + refTriangles[i].x2 + refTriangles[i].x3) / 3.0;
            double barycentrum1y = (refTriangles[i].y1 + refTriangles[i].y2 + refTriangles[i].y3) / 3.0;
            double barycentrum1z = (refTriangles[i].z1 + refTriangles[i].z2 + refTriangles[i].z3) / 3.0;
// Dwa drzewa na trojkat
//        double barycentrum2x = (refTriangles[i].x1 + refTriangles[i].x2 + barycentrum1x) / 3.0;
//        double barycentrum2y = (refTriangles[i].y1 + refTriangles[i].y2 + barycentrum1y) / 3.0;
//        double barycentrum2z = (refTriangles[i].z1 + refTriangles[i].z2 + barycentrum1z) / 3.0;
// Trzy drzewa na trojkat
//        double barycentrum3x = (barycentrum1x + refTriangles[i].x2 + refTriangles[i].x3) / 3.0;
//        double barycentrum3y = (barycentrum1y + refTriangles[i].y2 + refTriangles[i].y3) / 3.0;
//        double barycentrum3z = (barycentrum1z + refTriangles[i].z2 + refTriangles[i].z3) / 3.0;
// Cztery drzewa na trojkat
//        double barycentrum4x = (refTriangles[i].x1 + barycentrum1x + refTriangles[i].x3) / 3.0;
//        double barycentrum4y = (refTriangles[i].y1 + barycentrum1y + refTriangles[i].y3) / 3.0;
//        double barycentrum4z = (refTriangles[i].z1 + barycentrum1z + refTriangles[i].z3) / 3.0;

// Zapis drzew do pliku
// Jedno drzewo na trojkat
            int plusMinus = 0;
            if (rand() % 2 == 0) {
                plusMinus = -1;
            } else {
                plusMinus = 1;
            }
            int randomizer = (rand() % 7) * plusMinus;
            plik1 << "include tree.inc " << wylosowaneDrzewo << " " << (barycentrum1x - ExportX) * -1.0 + randomizer << " " << barycentrum1z << " " <<
                  barycentrum1y - ExportY + randomizer << " " << rand() % 360 << " " << minWysokoscDrzewa + (rand() % maxWysokoscDrzewa) << " " << minPromienKorona +
                  (rand() % maxPromienKorona) << " end\n";
//        plik1 << "include tree.inc " << wylosowaneDrzewo << " " << (barycentrum1x - ExportX) * -1.0 << " " << barycentrum1z << " " << barycentrum1y - ExportY << " " << dd(g) << " " << ddd(g) << " " << dddd(g) << " end\n";
// Dwa drzewa na trojkat
//        wylosowaneDrzewo = tabelaDrzew[rand() % liczbaDrzew];
//        plik1 << "include tree.inc " << wylosowaneDrzewo << " " << (barycentrum2x - ExportX) * -1.0 << " " << barycentrum2z << " " << barycentrum2y - ExportY << " " << rand() % 360 << " " << 3 + (rand() % 30) << " " << 3 + (rand() % 12) << " end\n";
// Trzy drzewa na trojkat
//        wylosowaneDrzewo = tabelaDrzew[rand() % liczbaDrzew];
//        plik1 << "include tree.inc " << wylosowaneDrzewo << " " << (barycentrum3x - ExportX) * -1.0 << " " << barycentrum3z << " " << barycentrum3y - ExportY << " " << rand() % 360 << " " << 3 + (rand() % 30) << " " << 3 + (rand() % 12) << " end\n";
// Cztery drzewa na trojkat
//        wylosowaneDrzewo = tabelaDrzew[rand() % liczbaDrzew];
//        plik1 << "include tree.inc " << wylosowaneDrzewo << " " << (barycentrum4x - ExportX) * -1.0 << " " << barycentrum4z << " " << barycentrum4y - ExportY << " " << rand() % 360 << " " << 3 + (rand() % 30) << " " << 3 + (rand() % 12) << " end\n";
            std::cout << "Petla " << i << " z: " << liczbaTrojkatow << "                                   \r";
            licznik += 6;
            if ((licznik == zmianaPliku1) || (licznik == zmianaPliku2) || (licznik == zmianaPliku3) || (licznik == zmianaPliku4) || (licznik == zmianaPliku5)
                    || (licznik == zmianaPliku6) || (licznik == zmianaPliku7) || (licznik == zmianaPliku8)) {
                plik1.close();
                ++nrPliku;
                numerPliku = to_string(nrPliku);
                nowaNazwaPliku = nazwaPliku;
                nowaNazwaPliku.insert(5,numerPliku);
// Czas zapisac w nowym pliku przetworzone dane.
                std::cout << "Otwieram plik " << nowaNazwaPliku << "\n";
                plik1.open(nowaNazwaPliku.c_str());
                if (!plik1) {
                    std::cout << "Brak pliku " << nowaNazwaPliku << "\n";
                    std::cin.get();
                }
                plik1.setf( std::ios::fixed, std:: ios::floatfield );
            }
        }
// Zamykamy nowo utworzony plik
        plik1.close();
    }
}

void zrobOtoczke(std::vector<wierzcholek> &refWierzcholki, std::vector<wypukla> &refOtoczka, std::vector<wierzcholek> &refBezOtoczki) {
    unsigned int i = 2, j = 1, l = 0, k = 0, ileWierzcholkow = refWierzcholki.size();
//    double lowestX = refWierzcholki[0].x, lowestY = refWierzcholki[0].y;
// W petli leci po wszystkich punktach NMT
//    refOtoczka[0].x = lowestX;
//    refOtoczka[0].y = lowestY;
//    std::ofstream plik2;
//    plik2.open("error.log");
//    if (!plik2) {
//        std::cout << "Brak pliku error.log" << "\n";
//        std::cin.get();
//    }

    for (; i < ileWierzcholkow; ++i) {
        if (sprawdzKatPolarny(refWierzcholki[k].x, refWierzcholki[k].y, refWierzcholki[j].x, refWierzcholki[j].y, refWierzcholki[i].x,
                              refWierzcholki[i].y) > 0) {
            refBezOtoczki.push_back(wierzcholek{refWierzcholki[j].x, refWierzcholki[j].y, refWierzcholki[j].z});
//            refBezOtoczki.push_back(wierzcholek());
//            refBezOtoczki[l].x = refWierzcholki[j].x;
//            refBezOtoczki[l].y = refWierzcholki[j].y;
//            refBezOtoczki[l].z = refWierzcholki[j].z;
            ++l;
            ++j;
//            plik2 << "Bez otoczki - punkt x=" << refWierzcholki[j].x << " y=" << refWierzcholki[j].y << " z=" << refWierzcholki[j].z << "\n";
        }
        else if (i < ileWierzcholkow-1) {
            refOtoczka.push_back(wypukla{refWierzcholki[k].x, refWierzcholki[k].y, refWierzcholki[k].z, i});
//            refOtoczka.push_back(wypukla());
//            refOtoczka[k].x = refWierzcholki[k].x;
//            refOtoczka[k].y = refWierzcholki[k].y;
//            refOtoczka[k].z = refWierzcholki[k].z;
//            refOtoczka[k].id = i;
            ++j;
            ++k;
//           plik2 << "Jest otoczka - id =" << i << " punkt x=" << refWierzcholki[k].x << " y=" << refWierzcholki[k].y << " z=" << refWierzcholki[k].z << "\n";
        }
        else if (i == ileWierzcholkow) {
            refOtoczka.push_back(wypukla{refWierzcholki[k].x, refWierzcholki[k].y, refWierzcholki[k].z, i - 2});
//            refOtoczka.push_back(wypukla());
//            refOtoczka[k].x = refWierzcholki[k].x;
//            refOtoczka[k].y = refWierzcholki[k].y;
//            refOtoczka[k].z = refWierzcholki[k].z;
//            refOtoczka[k].id = i - 2;
            refOtoczka.push_back(wypukla{refWierzcholki[j].x, refWierzcholki[j].y, refWierzcholki[j].z, i - 1});
//            refOtoczka.push_back(wypukla());
//            refOtoczka[j].x = refWierzcholki[j].x;
//            refOtoczka[j].y = refWierzcholki[j].y;
//            refOtoczka[j].z = refWierzcholki[j].z;
//            refOtoczka[j].id = i - 1;
            refOtoczka.push_back(wypukla{refWierzcholki[i].x, refWierzcholki[i].y, refWierzcholki[i].z, i});
//            refOtoczka.push_back(wypukla());
//            refOtoczka[i].x = refWierzcholki[i].x;
//            refOtoczka[i].y = refWierzcholki[i].y;
//            refOtoczka[i].z = refWierzcholki[i].z;
//            refOtoczka[i].id = i;
//            plik2 << "Jest otoczka - id =" << i << " punkt x=" << refWierzcholki[k].x << " y=" << refWierzcholki[k].y << " z=" << refWierzcholki[k].z << "\n";
//            plik2 << "Jest otoczka - id =" << i << " punkt x=" << refWierzcholki[j].x << " y=" << refWierzcholki[j].y << " z=" << refWierzcholki[j].z << "\n";
//            plik2 << "Jest otoczka - id =" << i << " punkt x=" << refWierzcholki[i].x << " y=" << refWierzcholki[i].y << " z=" << refWierzcholki[i].z << "\n";
            ++j;
            ++k;
        }
        std::cout << "Obliczanie otoczki wypuklej algorytmem Grahama - Petla nr " << i << " z " << ileWierzcholkow << "        \r";
    }
//    plik2.close();
}

void zapisPunktowDoTriangulacji(std::vector<wierzcholek> &refWierzcholki, std::vector<wypukla> &refOtoczka, double ExportX, double ExportY,
                                std::string zarostek) {
    unsigned int i = 0, ileWierzcholkow = refWierzcholki.size(), poprawka = 1;
//    unsigned int ileOtoczki = refOtoczka.size(), j = 1, jestOtoczka = 0;
// Otwarcie pliku do zbierania punktow
    std::ofstream plik1;
    std::string nazwaPliku = "wierzcholki.node";
    if (zarostek == "zageszczone") {
        poprawka = 1;
    }
    nazwaPliku.insert(11,zarostek);
    std::cout << "\nZapis wierzcholkow do pliku " << nazwaPliku << "...";
    plik1.open(nazwaPliku.c_str());
    plik1 << ileWierzcholkow - poprawka << " 2 0 0\n";
    if (!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
    plik1.setf( std::ios::fixed, std:: ios::floatfield );
//    std::ofstream plik2;
//    plik2.open("error.log");
//    if (!plik2) {
//        std::cout << "Brak pliku error.log" << "\n";
//        std::cin.get();
//    }
// plik .node dla programu Triangle - A Two-Dimensional Quality Mesh Generator and Delaunay Triangulator
    for (i = 0 + poprawka; i < ileWierzcholkow; ++i) {
//        jestOtoczka = 0;
//        for (j = 0; j < refOtoczka.size(); ++j) {
//            if (refOtoczka[j].id == i) {
//                jestOtoczka = 1;
//            }
//        }
//        plik1 << i + 1 << " " << refWierzcholki[i].x << " " << refWierzcholki[i].y << " 0 " << jestOtoczka << "\n";
        plik1 << i + 1 - poprawka << " " << refWierzcholki[i].x << " " << refWierzcholki[i].y << " #" << refWierzcholki[i].z << "\n";
//        plik2 << i + 1 << " " << refWierzcholki[i].x - ExportX<< " " << refWierzcholki[i].z << " " << refWierzcholki[i].y - ExportY << " " << i + 1 << "\n";
    }
//    plik1 << ileOtoczki - 2 << " 1\n";
//    for (i = 1; i < ileOtoczki - 2; ++i) {
//        plik1 << i << " " << refOtoczka[i].id << " " << refOtoczka[i+1].id << "\n";
//    }
//    plik1 << ileOtoczki - 2 << " " << refOtoczka[ileOtoczki - 1].id << " " << refOtoczka[0].id << "\n";
    plik1.close();
    std::cout << " Gotowe.\n";
//    plik2.close();
}

void obrobkaDanychNodeDoZageszczeniaPoTriangulacjiZUwzglednieniemProfilu(std::string szerokosc, std::vector<wierzcholek> &refWierzcholki,
        std::vector<wierzcholek> &refWierzcholkiProfilu, unsigned int &refNrId, std::vector<std::vector<unsigned int> > &refTablica2, unsigned int szerokosc2,
        unsigned int refKorektaX2, unsigned int refKorektaY2, unsigned int refWierszeTablicy2, unsigned int refKolumnyTablicy2, double ExportX,
        double ExportY, std::vector<std::vector<unsigned int> > &refTablicaBrakow, unsigned int refKorektaXbraki, unsigned int refKorektaYbraki) {
// Niezbedne zmienne
    std::vector<wierzcholek> wierzcholkiTriangles;
    std::vector<wierzcholek> wierzcholkiPoTriangulacji;
    std::vector<triangle> triangles;
    unsigned int licznikWierzcholkow = 0, licznikTrojkatow = 0;
    const double oganiczenieDlugosciRamionTrojkata = 300.0;
    wierzcholkiPoTriangulacji.clear();
    wierzcholkiTriangles.clear();
    odczytPunktowNode(wierzcholkiPoTriangulacji, szerokosc, licznikWierzcholkow);
    odczytPlikuPoTriangulacji(triangles, wierzcholkiPoTriangulacji, szerokosc, oganiczenieDlugosciRamionTrojkata, licznikTrojkatow, wierzcholkiTriangles,
                              ExportX, ExportY);
    utworzDodatkowePunktySiatkiZUwzglednieniemProfilu(triangles, refWierzcholki, refWierzcholkiProfilu, refNrId, refTablica2, szerokosc2, refKorektaX2,
            refKorektaY2, refWierszeTablicy2, refKolumnyTablicy2, refTablicaBrakow, refKorektaXbraki, refKorektaYbraki);
}

void obrobkaDanychHGTPrzedTriangulacja(std::vector<std::string> &refTabelaNazwPlikowHGT, std::vector<std::string> &refTabelaNazwPlikowDT2,
                                       double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne zmienne
    unsigned int nrId = 0, wierszeTablicy = 0, kolumnyTablicy = 0, korektaX = 0, korektaY = 0, wierszeTablicyBrakow = 0, kolumnyTablicyBrakow = 0,
                 korektaXbraki = 0, korektaYbraki = 0;
    std::vector<wierzcholek> wierzcholki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<wypukla> otoczka;
//    std::vector<wierzcholek> bezOtoczki;
    std::vector<std::vector<unsigned int> > tablica;
    std::vector<std::vector<unsigned int> > tablicaBrakow;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikowHGT = refTabelaNazwPlikowHGT.size();
    unsigned int liczbaPlikowDT2 = refTabelaNazwPlikowDT2.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikowHGT << "\n";
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikowDT2 << "\n";
    wierzcholki.clear();
    toryZGwiazdka.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica, tablicaBrakow, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX, korektaY, korektaXbraki, korektaYbraki,
                       wierszeTablicy, kolumnyTablicy, wierszeTablicyBrakow, kolumnyTablicyBrakow);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
#ifdef DT2
    for (unsigned int i = 0; i < liczbaPlikowDT2; ++i) {
        odczytPunktowDT2(wierzcholki, tablica, tablicaBrakow, refTabelaNazwPlikowDT2[i], nrId, i, liczbaPlikowDT2,
                         atoi(szerokoscTablicy.c_str()), toryZGwiazdka, korektaX, korektaY, korektaXbraki, korektaYbraki, wierszeTablicy, kolumnyTablicy);
    }
#endif //DT2
#ifdef HGT
    for (unsigned int i = 0; i < liczbaPlikowHGT; ++i) {
        odczytPunktowHGT(wierzcholki, tablica, tablicaBrakow, refTabelaNazwPlikowHGT[i], nrId, i, liczbaPlikowHGT,
                         atoi(szerokoscTablicy.c_str()), toryZGwiazdka, korektaX, korektaY, korektaXbraki, korektaYbraki, wierszeTablicy, kolumnyTablicy);
    }
#endif // HGT
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
//    unsigned int podwojnaLiczbaWierzcholkow = wierzcholki.size() * 2;
//    unsigned int tablicaPunktow[podwojnaLiczbaWierzcholkow];
//    unsigned int *wskaznikTablicaPunktow;
//    wskaznikTablicaPunktow = tablicaPunktow;
//    unsigned int x = 0, jj = 0;
//    for (unsigned int j = 0; j < podwojnaLiczbaWierzcholkow; ++j) {
//        if (x == 0) {
//            tablicaPunktow[j] = wierzcholki[jj].x;
//        }
//        if (x == 1) {
//            tablicaPunktow[j] = wierzcholki[jj].y;
//            ++jj;
//            x = 0;
//        }
//    }
//    wskaznikTablicaPunktow = tablicaPunktow;
//    triangulate("zYV", wskaznikTablicaPunktow, NULL, NULL);
    std::cout << "\nProgram zakonczyl dzialanie. Nacisnij jakis klawisz.                           \n\n";
}

void obrobkaDanychTXTPrzedTriangulacja(std::vector<std::string> &refTabelaNazwPlikowTXT, double &refWspolrzednaX, double &refWspolrzednaY,
                                       std::string szerokoscTablicy) {
// Niezbedne zmienne
    unsigned int nrId = 0, wierszeTablicy = 0, kolumnyTablicy = 0, wierszeTablicyBrakow = 0, kolumnyTablicyBrakow = 0, korektaX = 0, korektaY = 0,
                 korektaXbraki = 0, korektaYbraki = 0;
    std::vector<wierzcholek> wierzcholki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<wypukla> otoczka;
//    std::vector<wierzcholek> bezOtoczki;
    std::vector<std::vector<unsigned int> > tablica;
    std::vector<std::vector<unsigned int> > tablicaBrakow;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowTXT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    wierzcholki.clear();
    toryZGwiazdka.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica, tablicaBrakow, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX, korektaY, korektaXbraki, korektaYbraki,
                       wierszeTablicy, kolumnyTablicy, wierszeTablicyBrakow, kolumnyTablicyBrakow);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowTXT(wierzcholki, tablica, refTabelaNazwPlikowTXT[i], nrId, i, liczbaPlikow, atoi(szerokoscTablicy.c_str()), toryZGwiazdka,
                         korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
    }
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
    std::cout << "Program zakonczyl dzialanie. Nacisnij jakis klawisz.                           \n\n";
}

void obrobkaDanychTXT(std::vector<std::string> &refTabelaNazwPlikowTXT, double &refWspolrzednaX, double &refWspolrzednaY,
                      std::string szerokoscTablicy) {
// Niezbedne zmienne
    unsigned int nrId = 0, wierszeTablicy = 0, kolumnyTablicy = 0, korektaX = 0, korektaY = 0, wierszeTablicyBrakow = 0, kolumnyTablicyBrakow = 0,
                 korektaXbraki = 0, korektaYbraki = 0;
    std::vector<wierzcholek> wierzcholki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<wypukla> otoczka;
//    std::vector<wierzcholek> bezOtoczki;
    std::vector<std::vector<unsigned int> > tablica;
    std::vector<std::vector<unsigned int> > tablicaBrakow;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowTXT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    wierzcholki.clear();
    toryZGwiazdka.clear();
    otoczka.clear();
//    bezOtoczki.clear();
// Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica, tablicaBrakow, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX, korektaY, korektaXbraki, korektaYbraki,
                       wierszeTablicy, kolumnyTablicy, wierszeTablicyBrakow, kolumnyTablicyBrakow);
//    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
// Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowTXT(wierzcholki, tablica, refTabelaNazwPlikowTXT[i], nrId, i, liczbaPlikow, atoi(szerokoscTablicy.c_str()), toryZGwiazdka,
                         korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
    }
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
    std::cout << "Program zakonczyl dzialanie. Nacisnij jakis klawisz.                           \n\n";
}

void obrobkaDanychTXTPrzedTriangulacjaZUwazglednieniemProfilu(std::vector<std::string> &refTabelaNazwPlikowTXT, double &refWspolrzednaX,
        double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne zmienne
    unsigned int nrId = 0, wierszeTablicy1 = 0, kolumnyTablicy1 = 0, wierszeTablicy2 = 0, kolumnyTablicy2 = 0, wierszeTablicyBrakow = 0,
                 kolumnyTablicyBrakow = 0, licznikWierzcholkow = 0, szerokosc2 = 36, korektaX1 = 0, korektaY1 = 0, korektaX2 = 0, korektaY2 = 0, korektaXbraki = 0,
                 korektaYbraki = 0;
    std::vector<wierzcholek> wierzcholki;
    std::vector<wierzcholek> wierzcholkiProfilu;
    std::vector<wypukla> otoczka;
//    std::vector<wierzcholek> bezOtoczki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<std::vector<unsigned int> > tablica1;
    std::vector<std::vector<unsigned int> > tablica2;
    std::vector<std::vector<unsigned int> > tablicaBrakow;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowTXT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    wierzcholki.clear();
    wierzcholkiProfilu.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    toryZGwiazdka.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica1, tablicaBrakow, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX1, korektaY1, korektaXbraki, korektaYbraki,
                       wierszeTablicy1, kolumnyTablicy1, wierszeTablicyBrakow, kolumnyTablicyBrakow);
    odczytPunktowNode(wierzcholkiProfilu, "profil", licznikWierzcholkow);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    tablicaWierzcholkowTriangles(tablica2, exportX, exportY, szerokosc2, toryZGwiazdka, korektaX2, korektaY2, wierszeTablicy2, kolumnyTablicy2);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowTXTzUwzglednieniemProfilu(wierzcholki, wierzcholkiProfilu, tablica1, tablica2, refTabelaNazwPlikowTXT[i], nrId, i,
                                               liczbaPlikow, atoi(szerokoscTablicy.c_str()), szerokosc2, toryZGwiazdka, korektaX1, korektaY1, korektaX2, korektaY2, wierszeTablicy1, kolumnyTablicy1,
                                               wierszeTablicy2, kolumnyTablicy2);
    }
//    if (szerokoscTablicy == "1000") {
    obrobkaDanychNodeDoZageszczeniaPoTriangulacjiZUwzglednieniemProfilu("150", wierzcholki, wierzcholkiProfilu, nrId, tablica2, szerokosc2, korektaX2,
            korektaY2, wierszeTablicy2, kolumnyTablicy2, exportX, exportY, tablicaBrakow, korektaXbraki, korektaYbraki);
//    }

    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
    std::cout << "Program zakonczyl dzialanie. Nacisnij jakis klawisz.                           \n\n";
}

void obrobkaDanychHGTPrzedTriangulacjaZUwazglednieniemProfilu(std::vector<std::string> &refTabelaNazwPlikowHGT,
        std::vector<std::string> &refTabelaNazwPlikowDT2, double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne zmienne
    unsigned int nrId = 0, wierszeTablicy1 = 0, kolumnyTablicy1 = 0, wierszeTablicy2 = 0, kolumnyTablicy2 = 0, wierszeTablicyBrakow = 0,
                 kolumnyTablicyBrakow = 0, licznikWierzcholkow = 0, szerokosc2 = 36, korektaX1 = 0, korektaY1 = 0, korektaX2 = 0, korektaY2 = 0, korektaXbraki = 0,
                 korektaYbraki = 0;
    std::vector<wierzcholek> wierzcholki;
    std::vector<wierzcholek> wierzcholkiProfilu;
    std::vector<wypukla> otoczka;
//    std::vector<wierzcholek> bezOtoczki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<std::vector<unsigned int> > tablica1;
    std::vector<std::vector<unsigned int> > tablica2;
    std::vector<std::vector<unsigned int> > tablicaBrakow;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikowHGT = refTabelaNazwPlikowHGT.size();
    std::cout << "Liczba plikow HGT przekazanych do obrobki: "<< liczbaPlikowHGT << "\n";
    unsigned int liczbaPlikowDT2 = refTabelaNazwPlikowDT2.size();
    std::cout << "Liczba plikow DT2 przekazanych do obrobki: "<< liczbaPlikowDT2 << "\n";
    wierzcholki.clear();
    wierzcholkiProfilu.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    toryZGwiazdka.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica1, tablicaBrakow, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX1, korektaY1, korektaXbraki, korektaYbraki,
                       wierszeTablicy1, kolumnyTablicy1, wierszeTablicyBrakow, kolumnyTablicyBrakow);
    odczytPunktowNode(wierzcholkiProfilu, "profil", licznikWierzcholkow);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    tablicaWierzcholkowTriangles(tablica2, exportX, exportY, szerokosc2, toryZGwiazdka, korektaX2, korektaY2, wierszeTablicy2, kolumnyTablicy2);
// Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
#ifdef DT2
    for (unsigned int i = 0; i < liczbaPlikowDT2; ++i) {
        odczytPunktowDT2zUwzglednieniemProfilu(wierzcholki, wierzcholkiProfilu, tablica1, tablica2, tablicaBrakow, refTabelaNazwPlikowDT2[i],
                                               nrId, i, liczbaPlikowDT2, atoi(szerokoscTablicy.c_str()), szerokosc2, toryZGwiazdka, korektaX1, korektaY1, korektaX2, korektaY2, korektaXbraki,
                                               korektaYbraki, wierszeTablicy1, kolumnyTablicy1, wierszeTablicy2, kolumnyTablicy2);
    }
#endif //DT2
#ifdef HGT
    for (unsigned int i = 0; i < liczbaPlikowHGT; ++i) {
        odczytPunktowHGTzUwzglednieniemProfilu(wierzcholki, wierzcholkiProfilu, tablica1, tablica2, tablicaBrakow, refTabelaNazwPlikowHGT[i],
                                               nrId, i, liczbaPlikowHGT, atoi(szerokoscTablicy.c_str()), szerokosc2, toryZGwiazdka, korektaX1, korektaY1, korektaX2, korektaY2, korektaXbraki,
                                               korektaYbraki, wierszeTablicy1, kolumnyTablicy1, wierszeTablicy2, kolumnyTablicy2);
    }
#endif //HGT
    obrobkaDanychNodeDoZageszczeniaPoTriangulacjiZUwzglednieniemProfilu("150", wierzcholki, wierzcholkiProfilu, nrId, tablica2, szerokosc2, korektaX2,
            korektaY2, wierszeTablicy2, kolumnyTablicy2, exportX, exportY, tablicaBrakow, korektaXbraki, korektaYbraki);
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
    std::cout << "Program zakonczyl dzialanie. Nacisnij jakis klawisz.                           \n\n";
}

void odczytWierzcholkowZTriangles(double &refWspolrzednaX, double &refWspolrzednaY) {
// Niezbedne zmienne
    std::vector<wierzcholek> wierzcholkiTriangles;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<wypukla> otoczka;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    wierzcholkiTriangles.clear();
    toryZGwiazdka.clear();
    otoczka.clear();
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    odczytWierzcholkowTriangles(wierzcholkiTriangles, exportX, exportY, toryZGwiazdka);
    sort(wierzcholkiTriangles.begin(), wierzcholkiTriangles.end(), by_yx());
    zapisPunktowDoTriangulacji(wierzcholkiTriangles, otoczka, exportX, exportY, "profil");
}

void obrobkaDanychNodePoTriangulacji(double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokosc) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
    std::vector<wierzcholek> wierzcholki;
    std::vector<triangle> triangles;
    std::vector<wierzcholek> wierzcholkiTriangles;
    std::vector<punktyTorow> toryZGwiazdka;
    unsigned int licznikWierzcholkow = 0, licznikTrojkatow = 0;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
// Odrzucamy wszystkie trojkaty, ktore maja jakiekolwiek ramie dluzsze o podanej wartosci
    const double oganiczenieDlugosciRamionTrojkata = 300.0;
    wierzcholki.clear();
    triangles.clear();
    wierzcholkiTriangles.clear();
    toryZGwiazdka.clear();
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    odczytWierzcholkowTriangles(wierzcholkiTriangles, exportX, exportY, toryZGwiazdka);
    sort(wierzcholkiTriangles.begin(), wierzcholkiTriangles.end(), by_yx());
    odczytPunktowNode(wierzcholki, szerokosc, licznikWierzcholkow);
    odczytPlikuPoTriangulacji(triangles, wierzcholki, szerokosc, oganiczenieDlugosciRamionTrojkata, licznikTrojkatow, wierzcholkiTriangles, exportX,
                              exportY);
    zapisSymkowychTrojkatow(triangles, exportX, exportY);
#ifdef zalesianie
    sadzenieDrzew(triangles, exportX, exportY);
#endif // zalesianie
    std::cout << "Program zakonczyl dzialanie. Nacisnij jakis klawisz.                         \n" << "\n";
}

void zrobListePlikow(std::vector<std::string> &refTabelaNazwPlikow, std::string rozsz) {
    std::string linia ("");
    std::string ln ("");
#ifdef _WIN32
    std::string pathToSRTM ("." "\x5C" "SRTM");
    std::string pathToSRTMclosed ("." "\x5C" "SRTM" "\x5C");
    std::string errorSRTMString ("failed to open directory ." "\x5C" "SRTM");
    std::string pathToNMT100 ("." "\x5C" "NMT100");
    std::string pathToNMT100closed ("." "\x5C" "NMT100" "\x5C");
    std::string errorNMT100String ("failed to open directory ." "\x5C" "NMT100");
    std::string pathToDTED ("." "\x5C" "DTED" "\x5C" "DEM");
    std::string pathToDTEDclosed ("." "\x5C" "DTED" "\x5C" "DEM" "\x5C");
    std::string errorDTEDString ("failed to open directory ." "\x5C" "DTED" "\x5C" "DEM");
#endif // _WIN32
#ifdef __unix__
    std::string pathToSRTM ("./SRTM");
    std::string pathToSRTMclosed ("./SRTM/");
    std::string errorSRTMString ("failed to open directory ./SRTM");
    std::string pathToNMT100 ("./NMT100");
    std::string pathToNMT100closed ("./NMT100/");
    std::string errorNMT100String ("failed to open directory ./NMT100");
    std::string pathToDTED ("./DTED/DEM");
    std::string pathToDTEDclosed ("./DTED/DEM/");
    std::string errorDTEDString ("failed to open directory ./DTED/DEM");
#endif // __unix__
// Znalezny kod do listowania plikow z aktualnego katalogu
    DIR*     dir;
    dirent*  pdir;
    if (rozsz == "hgt") {
        if (!(dir = opendir(pathToSRTM.c_str()))) {
            perror(errorSRTMString.c_str());
            return;
        }
        dir = opendir(pathToSRTM.c_str());
    }
    if (rozsz == "txt") {
        if (!(dir = opendir(pathToNMT100.c_str()))) {
            perror(errorNMT100String.c_str());
            return;
        }
        dir = opendir(pathToNMT100.c_str());
    }
    if (rozsz == "dt2") {
        if (!(dir = opendir(pathToDTED.c_str()))) {
            perror(errorDTEDString.c_str());
            return;
        }
        dir = opendir(pathToDTED.c_str());
    }
    while ((pdir = readdir(dir)) != NULL) {
        if (!strcmp(pdir->d_name, "..") || !strcmp(pdir->d_name, ".")) {
            continue;
        }
// Drobne poprawki i znajduje tylko z takim rozszerzeniem, z jakim chcialem
        std::string nazwaPliku ("");
        nazwaPliku = pdir->d_name;
        int znaleziono = nazwaPliku.find(rozsz);
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            if (rozsz == "hgt") {
                nazwaPliku = nazwaPliku.insert(0, pathToSRTMclosed);
            }
            if (rozsz == "txt") {
                nazwaPliku = nazwaPliku.insert(0, pathToNMT100closed);
            }
            if (rozsz == "dt2") {
                nazwaPliku = nazwaPliku.insert(0, pathToDTEDclosed);
            }
            refTabelaNazwPlikow.push_back(nazwaPliku);
            std::cout << nazwaPliku << "\n";
        }
    }
    closedir(dir);
}

void odczytWspolrzednychPrzesunieciaSCN(double &refWspolrzednaX, double &refWspolrzednaY) {
    std::string nazwaPlikuZTorami ("EXPORT.SCN");
    std::string linia ("");
    std::string ln ("");
    unsigned int wyraz = 0, liniaNr = 0;
    bool flagKoniec = false;

    std::ifstream plik1;
    plik1.open(nazwaPlikuZTorami.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPlikuZTorami << "\n";
        std::cin.get();
    }
// W petli szukamy plikow .SCN oraz wspolrzednych przesuniecia scenerii
    while(!plik1.eof()) {
        std::getline(plik1, linia);
        ++liniaNr;
        if (liniaNr == 2) {
            std::istringstream ln(linia);
            wyraz = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po spacji)
            while (!ln.eof()) {
                std::string temp ("");
                std::getline(ln, temp, ' ');
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
                if (temp == " ") {
                    flagKoniec = true;
                    break;
                }
                if (flagKoniec == true) break;
                if (wyraz == 2) refWspolrzednaX = atof(temp.c_str());
                if (wyraz == 3) {
                    refWspolrzednaY = atof(temp.c_str());
                    flagKoniec = true;
                    break;
                }
                ++wyraz;
            }
        }
    }
    plik1.close();
}

int main()
{
// Tworzymy niezbedne zmienne lokalne
    std::vector<std::string> tabelaNazwPlikowTXT;
    std::vector<std::string> tabelaNazwPlikowHGT;
    std::vector<std::string> tabelaNazwPlikowDT2;
    double wspolrzednaX = 0, wspolrzednaY = 0;
    std::string rozszerzenie1 ("txt");
    std::string rozszerzenie2 ("hgt");
    std::string rozszerzenie3 ("dt2");
    char c = 'c';
    tabelaNazwPlikowTXT.clear();
#ifdef HGT
    tabelaNazwPlikowHGT.clear();
#endif // HGT
#ifdef DT2
    tabelaNazwPlikowDT2.clear();
#endif // DT2

    std::cout << "Witaj w programie TerenEU07.exe - wersja " << AutoVersion::MAJOR << "." << AutoVersion::MINOR << "." << AutoVersion::BUILD << "." <<
              AutoVersion::REVISION << AutoVersion::STATUS_SHORT << "\n\n";
    std::cout << "Wybierz opcje:\n\n";
    std::cout << "1. Tworzenie pliku .node z wierzcholkami profilu.\n";
    std::cout << "2. Tworzenie pliku .node z wierzcholkami SRTM w promieniu 0.375 km od konca kazdego odcinka toru.\n";
    std::cout << "3. Tworzenie pliku .node z wierzcholkami SRTM w promieniu 4.5 km od konca kazdego odcinka toru.\n";
    std::cout << "4. Tworzenie pliku .scm z terenem w formacie symulatora MaSzyna EU07.\n";
    std::cout << "5. Tworzenie pliku .node z wierzcholkami NMT100 w promieniu 0.375 km od konca kazdego odcinka toru.\n";
    std::cout << "6. Tworzenie pliku .node z wierzcholkami NMT100 w promieniu 4.5 km od konca kazdego odcinka toru.\n";
//  std::cout << "7. Tworzenie pliku .node z wierzcholkami NMT100 w promieniu 4.5 km od konca kazdego odcinka toru bez dopasowanych profili pod torami.\n";
    std::cout << "q - Wyjscie z programu.\n\n";
    std::cout << "Wybieram: ";

// Petla menu
    while ( (c = getchar()) != '\n') {
        switch(c) {
        case 'q': {
            break;
        }
        case '1': {
            odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
            odczytWierzcholkowZTriangles(wspolrzednaX, wspolrzednaY);
            break;
        }
        case '2': {
#ifdef HGT
            zrobListePlikow(tabelaNazwPlikowHGT, rozszerzenie2);
#endif // HGT
#ifdef DT2
            zrobListePlikow(tabelaNazwPlikowDT2, rozszerzenie3);
#endif // DT2
            odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
            obrobkaDanychHGTPrzedTriangulacja(tabelaNazwPlikowHGT, tabelaNazwPlikowDT2, wspolrzednaX, wspolrzednaY, "150");
            break;
        }
        case '3': {
#ifdef HGT
            zrobListePlikow(tabelaNazwPlikowHGT, rozszerzenie2);
#endif // HGT
#ifdef DT2
            zrobListePlikow(tabelaNazwPlikowDT2, rozszerzenie3);
#endif // DT2
            odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
            obrobkaDanychHGTPrzedTriangulacjaZUwazglednieniemProfilu(tabelaNazwPlikowHGT, tabelaNazwPlikowDT2, wspolrzednaX, wspolrzednaY, "1000");
            break;
        }
        case '4': {
            odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
            obrobkaDanychNodePoTriangulacji(wspolrzednaX, wspolrzednaY, "1000");
            break;
        }
        case '5': {
            zrobListePlikow(tabelaNazwPlikowTXT, rozszerzenie1);
            odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
            obrobkaDanychTXTPrzedTriangulacja(tabelaNazwPlikowTXT, wspolrzednaX, wspolrzednaY, "150");
            break;
        }
        case '6': {
            zrobListePlikow(tabelaNazwPlikowTXT, rozszerzenie1);
            odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
            obrobkaDanychTXTPrzedTriangulacjaZUwazglednieniemProfilu(tabelaNazwPlikowTXT, wspolrzednaX, wspolrzednaY, "1000");
            break;
        }
        case '7': {
            zrobListePlikow(tabelaNazwPlikowTXT, rozszerzenie1);
            odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
            obrobkaDanychTXT(tabelaNazwPlikowTXT, wspolrzednaX, wspolrzednaY, "1000");
            break;
        }
        }
    }
// Pauza.
    std::cin.get();
    return 0;
}
