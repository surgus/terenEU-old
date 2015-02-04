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
Or on IRCnet, nick surgeon
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
// #include <random>

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

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
    unsigned int odlegloscNMT;
};

struct punkty {
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
    unsigned int odlegloscNMT;
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

void odczytPunktowTorow(std::vector<std::vector<unsigned int> > &refTablica, double ExportX, double ExportY, unsigned int szerokosc, unsigned int &refKorektaX, unsigned int &refKorektaY, unsigned int &refWierszeTablicy, unsigned int &refKolumnyTablicy) {
    std::string linia ("");
    std::string ln ("");
    std::string szukanyString (" track ");
    std::string nazwaPlikuZTorami ("EXPORT.SCN");
    unsigned int liczbaLiniiTorow = 0, wyraz = 0, testXmax = 0, testYmax = 0, testXmin = 900000, testYmin = 900000;
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
        unsigned znaleziono = (linia.find(szukanyString));
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
    testXmin = testXmin - (5 * szerokosc);
    testXmax = testXmax + (5 * szerokosc);
    testYmin = testYmin - (5 * szerokosc);
    testYmax = testYmax + (5 * szerokosc);
// Ustalamy rozmiar tablicy i resetujemy zawartosc
    refWierszeTablicy = (testXmax / szerokosc) - (testXmin / szerokosc);
    refKolumnyTablicy = (testYmax / szerokosc) - (testYmin / szerokosc);
    refTablica.resize(refWierszeTablicy);
    for (unsigned int i = 0; i < refWierszeTablicy; ++i) {
        refTablica[i].resize(refKolumnyTablicy);
    }
// Te zmienne w dalszym dzialaniu programu beda kluczowe do prawidlowego przeszukiwania tablicy
    refKorektaX = testXmin;
    refKorektaY = testYmin;
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
        unsigned znaleziono = (linia.find(szukanyString));
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
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 2][((trackY - refKorektaY) / szerokosc) + 2] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 1][((trackY - refKorektaY) / szerokosc) + 2] = 1;
                                    refTablica[(trackX - refKorektaX) / szerokosc][((trackY - refKorektaY) / szerokosc) + 2] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 1][((trackY - refKorektaY) / szerokosc) + 2] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 2][((trackY - refKorektaY) / szerokosc) + 2] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 2][((trackY - refKorektaY) / szerokosc) + 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 1][((trackY - refKorektaY) / szerokosc) + 1] = 1;
                                    refTablica[(trackX - refKorektaX) / szerokosc][((trackY - refKorektaY) / szerokosc) + 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 1][((trackY - refKorektaY) / szerokosc) + 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 2][((trackY - refKorektaY) / szerokosc) + 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 2][(trackY - refKorektaY) / szerokosc] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 1][(trackY - refKorektaY) / szerokosc] = 1;
                                    refTablica[(trackX - refKorektaX) / szerokosc][(trackY - refKorektaY) / szerokosc] = 1; // srodek
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 1][(trackY - refKorektaY) / szerokosc] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 2][(trackY - refKorektaY) / szerokosc] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 2][((trackY - refKorektaY) / szerokosc) - 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 1][((trackY - refKorektaY) / szerokosc) - 1] = 1;
                                    refTablica[(trackX - refKorektaX) / szerokosc][((trackY - refKorektaY) / szerokosc) - 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 1][((trackY - refKorektaY) / szerokosc) - 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 2][((trackY - refKorektaY) / szerokosc) - 1] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 2][((trackY - refKorektaY) / szerokosc) - 2] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) - 1][((trackY - refKorektaY) / szerokosc) - 2] = 1;
                                    refTablica[(trackX - refKorektaX) / szerokosc][((trackY - refKorektaY) / szerokosc) - 2] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 1][((trackY - refKorektaY) / szerokosc) - 2] = 1;
                                    refTablica[((trackX - refKorektaX) / szerokosc) + 2][((trackY - refKorektaY) / szerokosc) - 2] = 1;
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
        unsigned znaleziono = (linia.find(szukanyString));
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
                        if (nazwaToru.substr(4,1) == "*"){
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

void odczytWierzcholkowTriangles(std::vector<wierzcholek> &refWierzcholki, double ExportX, double ExportY, std::vector<punktyTorow> &refToryZGwiazdka) {
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
        unsigned znaleziono = (linia.find(szukanyString));
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
                            refWierzcholki.push_back(wierzcholek{x, y, z, liczbaLiniiWierzcholkow});
                            ++liczbaLiniiWierzcholkow;
                        }
                    }
                    ++wyraz1;
                }
                if (wyraz > llw) llw = wyraz + 1;
            }
        }
    }
    std::cout << "Liczba znalezionych wierzcholkow w triangles: " << liczbaLiniiWierzcholkow << ". Najwieksza liczba linii w triangles: " << llw << "\n\n";
// Zamkniecie pliku z torami
    plik1.close();
}

void tablicaWierzcholkowTriangles(std::vector<std::vector<unsigned int> > &refTablica, double ExportX, double ExportY, unsigned int szerokosc, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int &refKorektaX, unsigned int &refKorektaY, unsigned int &refWierszeTablicy, unsigned int &refKolumnyTablicy) {
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
        unsigned znaleziono = (linia.find(szukanyString));
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
    testXmin = testXmin - (5 * szerokosc);
    testXmax = testXmax + (5 * szerokosc);
    testYmin = testYmin - (5 * szerokosc);
    testYmax = testYmax + (5 * szerokosc);
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
        unsigned znaleziono = (linia.find(szukanyString));
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
                            refTablica[((x - refKorektaX) / szerokosc) - 2][((y - refKorektaY) / szerokosc) + 2] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 1][((y - refKorektaY) / szerokosc) + 2] = 1;
                            refTablica[(x - refKorektaX) / szerokosc][((y - refKorektaY) / szerokosc) + 2] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 1][((y - refKorektaY) / szerokosc) + 2] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 2][((y - refKorektaY) / szerokosc) + 2] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 2][((y - refKorektaY) / szerokosc) + 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 1][((y - refKorektaY) / szerokosc) + 1] = 1;
                            refTablica[(x - refKorektaX) / szerokosc][((y - refKorektaY) / szerokosc) + 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 1][((y - refKorektaY) / szerokosc) + 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 2][((y - refKorektaY) / szerokosc) + 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 2][(y - refKorektaY) / szerokosc] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 1][(y - refKorektaY) / szerokosc] = 1;
                            refTablica[(x - refKorektaX) / szerokosc][(y - refKorektaY) / szerokosc] = 1; // srodek
                            refTablica[((x - refKorektaX) / szerokosc) + 1][(y - refKorektaY) / szerokosc] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 2][(y - refKorektaY) / szerokosc] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 2][((y - refKorektaY) / szerokosc) - 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 1][((y - refKorektaY) / szerokosc) - 1] = 1;
                            refTablica[(x - refKorektaX) / szerokosc][((y - refKorektaY) / szerokosc) - 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 1][((y - refKorektaY) / szerokosc) - 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 2][((y - refKorektaY) / szerokosc) - 1] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 2][((y - refKorektaY) / szerokosc) - 2] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) - 1][((y - refKorektaY) / szerokosc) - 2] = 1;
                            refTablica[(x - refKorektaX) / szerokosc][((y - refKorektaY) / szerokosc) - 2] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 1][((y - refKorektaY) / szerokosc) - 2] = 1;
                            refTablica[((x - refKorektaX) / szerokosc) + 2][((y - refKorektaY) / szerokosc) - 2] = 1;
                            ++liczbaLiniiWierzcholkow;
                        }
                    }
                    ++wyraz1;
                }
                if (wyraz > llw) llw = wyraz + 1;
            }
        }
    }
    std::cout << "Liczba znalezionych wierzcholkow w triangles: " << liczbaLiniiWierzcholkow << ". Najwieksza liczba linii w triangles: " << llw << "\nTablica zapisana.\n\n";
// Zamkniecie pliku z torami
    plik1.close();
}

void odczytPunktowNMT(std::vector<wierzcholek> &refWierzcholki, std::vector<std::vector<unsigned int> > &refTablica, std::string nazwaPliku, std::vector<double> &refOdlegloscNMT) {
    std::string linia ("");
    std::string ln ("");
    bool flagPunkty = false;
    unsigned int nrLinii = 0, id = 0;
// Otwarcie pliku tylko do odczytu z danymi NMT
    std::cout << "Otwieram plik z punktami NMT i wczytuje je do pamieci,\n" << "jesli znajduja sie w poblizu wczytanych wczesniej torow.\n";
    std::ifstream plik1;
    plik1.open(nazwaPliku.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
// Otwarcie pliku do zbierania punktow
    std::ofstream plik2;
    plik2.open("dane-NMT-miasto.txt");
    if (!plik2) {
        std::cout << "Brak pliku dane-NMT-miasto.txt" << "\n";
        std::cin.get();
    }
    plik2.precision(32);
    nrLinii = 0;
    while(!plik1.eof()) {
        std::getline(plik1, linia);
        std::istringstream ln(linia);
        unsigned int wyraz = 0, a = 0;
        std::string temp1 ("");
        std::string temp2 ("");
        std::string temp3 ("");
        double temp11 = 0, temp22 = 0, temp33 = 0;
        unsigned int temp111 = 0, temp222 = 0;
// Petla rozbijajaca odczytana linie na pojedyncze stringi (rozdzielanie po " ")
        while (!ln.eof()) {
            std::string temp ("");
            std::getline(ln, temp, ' ');
            bool znalazlem = false;
// Jesli ostatnia linijka jest pusta, przerywamy wszystkie petle oprocz pierwszej for
            if (temp == " ") {
                flagPunkty = true;
                break;
            }
            if (flagPunkty == true) break;
// Y
            if (wyraz == 0) temp1 = temp; // y.push_back(atof(temp.c_str()));
// -X
            if (wyraz == 1) temp2 = temp; // x.push_back(atof(temp.c_str()));
// Z
            if (wyraz == 2) {
                temp3 = temp; // z.push_back(atof(temp.c_str()));
                if (!znalazlem) {
                    temp11 = atof (temp1.c_str());
                    temp22 = atof (temp2.c_str());
                    temp33 = atof (temp3.c_str());
                    temp111 = temp11;
                    temp222 = temp22;
                    if (refTablica[temp111 / 2000][temp222 / 2000] == 1) {
                        refOdlegloscNMT.push_back(a);
                        refWierzcholki.push_back(wierzcholek());
                        refWierzcholki[id].x = temp11;
                        refWierzcholki[id].y = temp22;
                        refWierzcholki[id].z = temp33;
                        refWierzcholki[id].odlegloscNMT = id;
                        plik2 << temp11 << " " << temp22 << " " << temp33 << " #id=" << id << "\n";
                        znalazlem = true;
                        ++id;
                    }
                }
            }
            ++wyraz;
        }
        ++nrLinii;
        std::cout << "Linia nr " << nrLinii << "          \r";
    }
    std::cout << "\nLiczba znalezionych punktow NMT: " << refWierzcholki.size() << "\n";
// Zamyka plik
    plik1.close();
    plik2.close();
    std::cout << "Wszystkie punkty NMT wczytane. Zamykam ten plik" << "\n\n";
// Zamyka plik ze znalezionymi punktami NMT
}

void odczytPunktowNode(std::vector<wierzcholek> &refWierzcholki, std::vector<double> &refOdlegloscNMT, std::string zarostek, unsigned int &refLicznikWierzcholkow) {
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
        unsigned int wyraz = 0, a = 0;
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
// Y
                if (wyraz == 1) temp1 = temp; // y.push_back(atof(temp.c_str()));
// -X
                if (wyraz == 2) temp2 = temp; // x.push_back(atof(temp.c_str()));
// Z
                if (wyraz == 3) {
                    temp3 = temp.erase (0,1);
                    if (!znalazlem) {
                        refOdlegloscNMT.push_back(a);
                        refWierzcholki.push_back(wierzcholek());
                        refWierzcholki[refLicznikWierzcholkow].x = atof (temp1.c_str());
                        refWierzcholki[refLicznikWierzcholkow].y = atof (temp2.c_str());
                        refWierzcholki[refLicznikWierzcholkow].z = atof (temp3.c_str());
                        refWierzcholki[refLicznikWierzcholkow].odlegloscNMT = refLicznikWierzcholkow;
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

void odczytPunktowNodeZUwzglednieniemProfilu(std::vector<wierzcholek> &refWierzcholki, std::vector<wierzcholek> &refWierzcholkiProfilu, std::vector<double> &refOdlegloscNMT, std::string zarostek, unsigned int &refLicznikWierzcholkow) {
    std::string linia ("");
    std::string ln ("");
    std::string nazwaPliku ("wierzcholki.node");
    nazwaPliku.insert(11,zarostek);
    unsigned int nrLinii = 0;
    unsigned int liczbaWierzcholkowProfilu = refWierzcholkiProfilu.size();
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
        unsigned int wyraz = 0, a = 0;
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
            if (temp == " ") flagPunkty = true;
            if (!flagPunkty) {
// Y
                if (wyraz == 1) temp1 = temp; // y.push_back(atof(temp.c_str()));
// -X
                if (wyraz == 2) temp2 = temp; // x.push_back(atof(temp.c_str()));
// Z
                if (wyraz == 3) {
                    temp3 = temp.erase (0,1);
                    double x = atof(temp1.c_str());
                    double y = atof(temp2.c_str());
                    double z = atof(temp3.c_str());
                    for ( unsigned int i = 0; i < liczbaWierzcholkowProfilu; ++i) {
                        if (znalazlem) break;
                        double roznicaMiedzyPunktamiSRTMiProfiluX = hypot(refWierzcholkiProfilu[i].x - x, refWierzcholkiProfilu[i].y - y);
                        if (roznicaMiedzyPunktamiSRTMiProfiluX > 30) {
                            refOdlegloscNMT.push_back(a);
                            refWierzcholki.push_back(wierzcholek());
                            refWierzcholki[refLicznikWierzcholkow].x = x;
                            refWierzcholki[refLicznikWierzcholkow].y = y;
                            refWierzcholki[refLicznikWierzcholkow].z = z;
                            refWierzcholki[refLicznikWierzcholkow].odlegloscNMT = refLicznikWierzcholkow;
                            znalazlem = true;
                            ++refLicznikWierzcholkow;
                        }
                    }
                }
                ++wyraz;
            }
        }
        ++nrLinii;
        std::cout << "Linia nr " << nrLinii << "          \r";
    }
    std::cout << "\nLiczba znalezionych punktow node: " << refWierzcholki.size() << "\n";
// Zamyka plik
    plik1.close();
    std::cout << "Wszystkie punkty node wczytane. Zamykam ten plik" << "\n\n";
// Zamyka plik ze znalezionymi punktami NMT
}

void odczytPunktowHGT(std::vector<wierzcholek> &refWierzcholki, std::vector<std::vector<unsigned int> > &refTablica, std::string nazwaPliku, std::vector<double> &refOdlegloscHGT, unsigned int &id, unsigned int &refNrPliku, unsigned int &refLiczbaPlikow, unsigned int szerokosc, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX, unsigned int refKorektaY, unsigned int refWierszeTablicy, unsigned int refKolumnyTablicy) {
	unsigned char buffer[2];
	unsigned int licznik = 0;
	const int a = 0;
	unsigned int dlugoscNazwyPliku = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
//	double minutaX = 0, minutaY = 0;
	const double sekunda = 3.0/3600.0;
//	const double sekunda = 1.0/3600.0;
//Przykladowy plik: .\SRTM\E0101500N594500_SRTM_1_DEM.dt2
    dlugoscNazwyPliku = nazwaPliku.length();
	std::string nrx = nazwaPliku.substr(dlugoscNazwyPliku-10,2);
	std::string nry = nazwaPliku.substr(dlugoscNazwyPliku-6,2);
	std::cout << "nazwaPliku=" << nazwaPliku << "\n";
//	std::string nry = nazwaPliku.substr(9,2);
//	std::string nrx = nazwaPliku.substr(16,2);
//	std::string mnY = nazwaPliku.substr(11,2);
//	std::string mnX = nazwaPliku.substr(18,2);
//	if (mnX == "15") minutaX = 15.0/60.0;
//	if (mnX == "30") minutaX = 30.0/60.0;
//	if (mnX == "45") minutaX = 45.0/60.0;
//	if (mnY == "15") minutaY = 15.0/60.0;
//	if (mnY == "30") minutaY = 30.0/60.0;
//	if (mnY == "45") minutaY = 45.0/60.0;
	const double XwsgPoczatek = atof(nrx.c_str());
	const double YwsgPoczatek = atof(nry.c_str());
    const unsigned int SRTM_SIZE = 1201;
//    const unsigned int SRTM_SIZE = 901;
// Kod przekształcenia formatu WGS84 do PUWG 1992 zostal zapozyczony i zoptymalizowany. Naglowek autora ponizej
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
       Uriasz, J., “Wybrane odwzorowania kartograficzne”, Akademia Morska w Szczecinie,
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
//        double Xwsg = XwsgPoczatek + minutaX + (i * sekunda);
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
//            double Ywsg = YwsgPoczatek + minutaY + (j * sekunda);
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
//    		refPlik2 << "Xwsg=" << Xwsg << " Ywsg=" << Ywsg << " z=" << z << " || Xpuwg=" << Xpuwg << " Ypuwg=" << Ypuwg << "\n";
                bool doOdrzutu = false, nieSprawdzaj = false;
                if (((Ypuwg - refKorektaX) / szerokosc < 1) || ((Ypuwg - refKorektaX) / szerokosc > refWierszeTablicy)) nieSprawdzaj = true;
                if (((Xpuwg - refKorektaY) / szerokosc < 1) || ((Xpuwg - refKorektaY) / szerokosc > refKolumnyTablicy)) nieSprawdzaj = true;
                if (!nieSprawdzaj) {
                    if (refTablica[(Ypuwg - refKorektaX) / szerokosc][(Xpuwg - refKorektaY) / szerokosc] == 1) {
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
                            refOdlegloscHGT.push_back(a);
//                      refWierzcholki.push_back(wierzcholek());
//                      refWierzcholki.resize(id + 1);
                            refWierzcholki.push_back(wierzcholek{Ypuwg, Xpuwg, z, id});
//                      refWierzcholki[id].x = Ypuwg;
//                      refWierzcholki[id].y = Xpuwg;
//                      refWierzcholki[id].z = z;
//                      refWierzcholki[id].odlegloscNMT = id;
//                      std::cout.setf( std::ios::scientific, std:: ios::floatfield );
//                      std::cout.precision(32);
//                      std::cout << "refWierzcholki[" << id << "].x=" << refWierzcholki[id].x << " | A tymczasem Ypuwg=" << Ypuwg << "                \r";
                            ++id;
                            ++licznik;
                        }
                    }
                }
            }
        }
    	++nrPunktu;
// Fajne, ale zabiera duzo czasu
//        std::cout << "Petla nr " << nrPunktu << " z 1201           \r";
//        printf ("Petla nr %u", nrPunktu);
//        printf ("%s           \r", " z 1201        ");
    }
//    std::cout << "\nLiczba znalezionych punktow HGT w pliku: " << licznik << "\nW sumie znalezionych punktow: " << id << "\n\n";
    printf ("\nLiczba znalezionych punktow HGT w pliku: %u", licznik);
    printf ("\nW sumie znalezionych punktow: %u \n\n", id);
//    std::cout << "Wszystkie punkty HGT wczytane. Zamykam ten plik" << "\n\n";
}

void odczytPunktowHGTzUwzglednieniemProfilu(std::vector<wierzcholek> &refWierzcholki, std::vector<wierzcholek> &refWierzcholkiProfilu, std::vector<std::vector<unsigned int> > &refTablica1, std::vector<std::vector<unsigned int> > &refTablica2, std::string nazwaPliku, std::vector<double> &refOdlegloscHGT, unsigned int &refNrId, unsigned int NrPliku, unsigned int LiczbaPlikow, unsigned int szerokosc1, unsigned int szerokosc2, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX1, unsigned int refKorektaY1, unsigned int refKorektaX2, unsigned int refKorektaY2, unsigned int refWierszeTablicy1, unsigned int refKolumnyTablicy1, unsigned int refWierszeTablicy2, unsigned int refKolumnyTablicy2) {
	unsigned char buffer[2];
	unsigned int licznik = 0, dlugoscNazwyPliku = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
//	unsigned int liczbaWierzcholkowProfilu = refWierzcholkiProfilu.size();
	const int a = 0;
//	double minutaX = 0, minutaY = 0;
	const double sekunda = 3.0/3600.0;
//	const double sekunda = 1.0/3600.0;
//Przykladowy plik: .\SRTM\E0101500N594500_SRTM_1_DEM.dt2
    dlugoscNazwyPliku = nazwaPliku.length();
	std::string nrx = nazwaPliku.substr(dlugoscNazwyPliku-10,2);
	std::string nry = nazwaPliku.substr(dlugoscNazwyPliku-6,2);
	std::cout << "nazwaPliku=" << nazwaPliku << "\n";
//	std::string nry = nazwaPliku.substr(9,2);
//	std::string nrx = nazwaPliku.substr(16,2);
//	std::string mnY = nazwaPliku.substr(11,2);
//	std::string mnX = nazwaPliku.substr(18,2);
//	if (mnX == "15") minutaX = 15.0/60.0;
//	if (mnX == "30") minutaX = 30.0/60.0;
//	if (mnX == "45") minutaX = 45.0/60.0;
//	if (mnY == "15") minutaY = 15.0/60.0;
//	if (mnY == "30") minutaY = 30.0/60.0;
//	if (mnY == "45") minutaY = 45.0/60.0;
	const double XwsgPoczatek = atof(nrx.c_str());
	const double YwsgPoczatek = atof(nry.c_str());
    const unsigned int SRTM_SIZE = 1201;
//    const unsigned int SRTM_SIZE = 901;
// Kod przekształcenia formatu WGS84 do PUWG 1992 zostal zapozyczony i zoptymalizowany. Naglowek autora ponizej
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
       Uriasz, J., “Wybrane odwzorowania kartograficzne”, Akademia Morska w Szczecinie,
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
//        double Xwsg = XwsgPoczatek + minutaX + (i * sekunda);
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
//            double Ywsg = YwsgPoczatek + minutaY + (j * sekunda);
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
//                double roznicaMiedzyPunktamiSRTMiProfiluX = 0;
//                double roznicaMiedzyPunktamiSRTMiProfiluX1 = 91.0;
                bool zaBlisko = false, nieSprawdzaj = false;
//    		refPlik2 << "Xwsg=" << Xwsg << " Ywsg=" << Ywsg << " z=" << z << " || Xpuwg=" << Xpuwg << " Ypuwg=" << Ypuwg << "\n";
//                int test1 = (Ypuwg - refKorektaX1) / szerokosc1;
//                int test2 = (Xpuwg - refKorektaY1) / szerokosc1;
                if (((Ypuwg - refKorektaX1) / szerokosc1 < 1) || ((Ypuwg - refKorektaX1) / szerokosc1 > refWierszeTablicy1)) nieSprawdzaj = true;
                if (((Xpuwg - refKorektaY1) / szerokosc1 < 1) || ((Xpuwg - refKorektaY1) / szerokosc1 > refKolumnyTablicy1)) nieSprawdzaj = true;
                if (!nieSprawdzaj) {
                    if (refTablica1[(Ypuwg - refKorektaX1) / szerokosc1][(Xpuwg - refKorektaY1) / szerokosc1] == 1) {
                        bool doOdrzutu = false;
//                    for (unsigned int ii = 0; ii < liczbaWierzcholkowProfilu; ++ii) {
//                        if ((Ypuwg < 1) || (Xpuwg < 1)) {
//                            zaBlisko = true;
//                            break;
//                        }
//                        roznicaMiedzyPunktamiSRTMiProfiluX = hypot(refWierzcholkiProfilu[ii].x - Ypuwg, refWierzcholkiProfilu[ii].y - Xpuwg);
                        // Jesli punkt SRTM jest blizej punktu
//                        if (roznicaMiedzyPunktamiSRTMiProfiluX1 > roznicaMiedzyPunktamiSRTMiProfiluX) {
//                            roznicaMiedzyPunktamiSRTMiProfiluX1 = roznicaMiedzyPunktamiSRTMiProfiluX;
//                            zaBlisko = true;
//                            break;
//                        }
//                    }
                        if (((Ypuwg - refKorektaX2) / szerokosc2 < 1) || ((Ypuwg - refKorektaX2) / szerokosc2 > refWierszeTablicy2)) nieSprawdzaj = true;
                        if (((Xpuwg - refKorektaY2) / szerokosc2 < 1) || ((Xpuwg - refKorektaY2) / szerokosc2 > refKolumnyTablicy2)) nieSprawdzaj = true;
                        if (!nieSprawdzaj) {
                            if (refTablica2[(Ypuwg - refKorektaX2) / szerokosc2][(Xpuwg - refKorektaY2) / szerokosc2] == 1) zaBlisko = true;
                        }
                        if (!zaBlisko) {
                            for (unsigned int jj = 0; jj < liczbaTorowZGwiazdka; ++jj) {
                                    bool waznyX = false, waznyY = false, rosnieY = false, malejeY = false, rosnieX = false, malejeX = false;
//                            double testxp1 = refToryZGwiazdka[jj].xp1;
//                            double testyp1 = refToryZGwiazdka[jj].yp1;
//                            double testxp2 = refToryZGwiazdka[jj].xp2;
//                            double testyp2 = refToryZGwiazdka[jj].yp2;
                                double wektorP2P1x = refToryZGwiazdka[jj].xp1 - refToryZGwiazdka[jj].xp2;
                                double wektorP2P1y = refToryZGwiazdka[jj].yp1 - refToryZGwiazdka[jj].yp2;
                            // Przesuwamy pierwotne punkty P1 i P2 o wektor P2P1
//                            testxp1 = testxp1 + wektorP2P1x;
//                            testyp1 = testyp1 + wektorP2P1y;
//                            testxp2 = testxp2 + wektorP2P1x;
//                            testyp2 = testyp2 + wektorP2P1y;
//                            double odlegloscHGTodP1Toru = hypot(testxp1 - Ypuwg, testyp1 - Xpuwg);
//                            double odlegloscHGTodP2Toru = hypot(testxp2 - Ypuwg, testyp2 - Xpuwg);
                            // Jesli punkt SRTM jest lizej punktu P1 niz P2 toru z gwiazdka, oraz ta dleglosc jest mniejsza niz 8 km, wtedy punkt odrzucamy
//                            if ((odlegloscHGTodP1Toru < odlegloscHGTodP2Toru) && (odlegloscHGTodP1Toru < 8001.0)) {
//                                doOdrzutu = true;
//                            }
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
                                refOdlegloscHGT.push_back(a);
//                    refWierzcholki.push_back(wierzcholek());
//                    refWierzcholki.resize(id + 1);
                                refWierzcholki.push_back(wierzcholek{Ypuwg, Xpuwg, z, refNrId});
//                    refWierzcholki[id].x = Ypuwg;
//                    refWierzcholki[id].y = Xpuwg;
//                    refWierzcholki[id].z = z;
//                    refWierzcholki[id].odlegloscNMT = id;
//                    std::cout.setf( std::ios::scientific, std:: ios::floatfield );
//                    std::cout.precision(32);
//                    std::cout << "refWierzcholki[" << id << "].x=" << refWierzcholki[id].x << " | A tymczasem Ypuwg=" << Ypuwg << "                \r";
                                ++refNrId;
                                ++licznik;
                            }
                        }
                    }
                }
            }
        }
        ++nrPunktu;
// Fajne, ale zabiera za duzo czasu
//        std::cout << "Petla nr " << nrPunktu << " z 1201           \r";
    }
    std::cout << "\nLiczba znalezionych punktow HGT w pliku: " << licznik << "\nW sumie znalezionych punktow: " << refNrId << "\n\n";
//    std::cout << "Wszystkie punkty HGT wczytane. Zamykam ten plik" << "\n\n";
}

void odczytPunktowTXT(std::vector<wierzcholek> &refWierzcholki, std::vector<std::vector<unsigned int> > &refTablica, std::string nazwaPliku, std::vector<double> &refOdlegloscTXT, unsigned int &id, unsigned int &refNrPliku, unsigned int &refLiczbaPlikow, unsigned int szerokosc, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX, unsigned int refKorektaY, unsigned int refWierszeTablicy, unsigned int refKolumnyTablicy) {
	unsigned int licznik = 0, nrPunktu = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
	const unsigned int a = 0;
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
// Y
            if (wyraz == 0) x = atof (temp.c_str()); // y.push_back(atof(temp.c_str()));
// X
            if (wyraz == 1) y = atof (temp.c_str()); // x.push_back(atof(temp.c_str()));
// Z
            if (wyraz == 2) {
                z = atof (temp.c_str());
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
                            refOdlegloscTXT.push_back(a);
                            refWierzcholki.push_back(wierzcholek{x, y, z, id});
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
//    std::cout << "\nLiczba znalezionych punktow HGT w pliku: " << licznik << "\nW sumie znalezionych punktow: " << id << "\n\n";
    printf ("\nLiczba znalezionych punktow NMT100 w pliku: %u", licznik);
    printf ("\nW sumie znalezionych punktow: %u \n\n", id);
}

void odczytPunktowTXTzUwzglednieniemProfilu(std::vector<wierzcholek> &refWierzcholki, std::vector<wierzcholek> &refWierzcholkiProfilu, std::vector<std::vector<unsigned int> > &refTablica1, std::vector<std::vector<unsigned int> > &refTablica2, std::string nazwaPliku, std::vector<double> &refOdlegloscTXT, unsigned int &refNrId, unsigned int NrPliku, unsigned int LiczbaPlikow, unsigned int szerokosc1, unsigned int szerokosc2, std::vector<punktyTorow> &refToryZGwiazdka, unsigned int refKorektaX1, unsigned int refKorektaY1, unsigned int refKorektaX2, unsigned int refKorektaY2, unsigned int refWierszeTablicy1, unsigned int refKolumnyTablicy1, unsigned int refWierszeTablicy2, unsigned int refKolumnyTablicy2) {
	unsigned int nrLinii = 0, liczbaTorowZGwiazdka = refToryZGwiazdka.size();
	const unsigned int a = 0;
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
// Y
            if (wyraz == 0) x = atof (temp.c_str()); // y.push_back(atof(temp.c_str()));
// X
            if (wyraz == 1) y = atof (temp.c_str()); // x.push_back(atof(temp.c_str()));
// Z
            if (wyraz == 2) {
                bool zaBlisko = false, nieSprawdzaj = false, doOdrzutu = false;
                z = atof (temp.c_str()); // z.push_back(atof(temp.c_str()));
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
                                refOdlegloscTXT.push_back(a);
                                refWierzcholki.push_back(wierzcholek{x, y, z, refNrId});
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

void odczytPlikuPoTriangulacji(std::vector<triangle> &refTriangles, std::vector<wierzcholek> &refWierzcholki, std::string zarostek, const double ograniczenieTrojkata, unsigned int &refLicznikTrojkatow, std::vector<wierzcholek> &refWierzcholkiTriangles) {
    std::string linia ("");
    std::string ln ("");
    unsigned int nrLinii = 0;
// Orczytuje przerobione innym programem wierzcholki na trianglesy
    std::ifstream plik1;
    std::string nazwaPliku = "wierzcholki.1.ele";
    nazwaPliku.insert(11,zarostek);
    std::cout << "Otwieram plik " << nazwaPliku << " z trojkatami.\n";
    plik1.open(nazwaPliku.c_str());
    if(!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
// Orczytuje przerobione innym programem wierzcholki na trianglesy
    #ifdef _DEBUG
    std::ofstream plik2;
    plik2.setf( std::ios::fixed, std:: ios::floatfield );
    plik2.open("debug.ele");
    if(!plik2) {
        std::cout << "Brak pliku debug.ele\n";
        std::cin.get();
    }
    for (unsigned i = 0; i < refWierzcholkiTriangles.size(); ++i) {
        plik2 << "x = " << refWierzcholkiTriangles[i].x << "     y = " << refWierzcholkiTriangles[i].y << "     z = " << refWierzcholkiTriangles[i].z << "\n";
    }
    plik2 << "\n\n";
    #endif // _DEBUG
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
                    if (wyraz == 1) temp2 = atoi(temp.c_str()); // x.push_back(atof(temp.c_str()));
                    if (wyraz == 2) temp3 = atoi(temp.c_str());
                    if (wyraz == 3) {
                        temp4 = atoi(temp.c_str()); // z.push_back(atof(temp.c_str()));
                        if (!znalazlem) {
                            AB = hypot(refWierzcholki[temp3].x - refWierzcholki[temp2].x, refWierzcholki[temp3].y - refWierzcholki[temp2].y);
                            BC = hypot(refWierzcholki[temp4].x - refWierzcholki[temp3].x, refWierzcholki[temp4].y - refWierzcholki[temp3].y);
                            CA = hypot(refWierzcholki[temp2].x - refWierzcholki[temp4].x, refWierzcholki[temp2].y - refWierzcholki[temp4].y);
                            if ((AB < ograniczenieTrojkata) && (BC < ograniczenieTrojkata) && (CA < ograniczenieTrojkata)) {
                                bool doOdrzutu1 = false, doOdrzutu2 = false, doOdrzutu3 = false;
                                for (unsigned i = 0; i < refWierzcholkiTriangles.size(); ++i) {
                                    if (((refWierzcholki[temp2].x - refWierzcholkiTriangles[i].x > -0.01) && (refWierzcholki[temp2].x - refWierzcholkiTriangles[i].x < 0.01)) && ((refWierzcholki[temp2].y - refWierzcholkiTriangles[i].y > -0.01) && (refWierzcholki[temp2].y - refWierzcholkiTriangles[i].y < 0.01))) doOdrzutu1 = true;
                                    if (((refWierzcholki[temp3].x - refWierzcholkiTriangles[i].x > -0.01) && (refWierzcholki[temp3].x - refWierzcholkiTriangles[i].x < 0.01)) && ((refWierzcholki[temp3].y - refWierzcholkiTriangles[i].y > -0.01) && (refWierzcholki[temp3].y - refWierzcholkiTriangles[i].y < 0.01))) doOdrzutu2 = true;
                                    if (((refWierzcholki[temp4].x - refWierzcholkiTriangles[i].x > -0.01) && (refWierzcholki[temp4].x - refWierzcholkiTriangles[i].x < 0.01)) && ((refWierzcholki[temp4].y - refWierzcholkiTriangles[i].y > -0.01) && (refWierzcholki[temp4].y - refWierzcholkiTriangles[i].y < 0.01))) doOdrzutu3 = true;
                                }
                                if ((!doOdrzutu1) || (!doOdrzutu2) || (!doOdrzutu3)) {
                                    refTriangles.push_back(triangle());
                                    refTriangles[refLicznikTrojkatow].x1 = refWierzcholki[temp2].x;
                                    refTriangles[refLicznikTrojkatow].y1 = refWierzcholki[temp2].y;
                                    refTriangles[refLicznikTrojkatow].z1 = refWierzcholki[temp2].z;
                                    refTriangles[refLicznikTrojkatow].x2 = refWierzcholki[temp3].x;
                                    refTriangles[refLicznikTrojkatow].y2 = refWierzcholki[temp3].y;
                                    refTriangles[refLicznikTrojkatow].z2 = refWierzcholki[temp3].z;
                                    refTriangles[refLicznikTrojkatow].x3 = refWierzcholki[temp4].x;
                                    refTriangles[refLicznikTrojkatow].y3 = refWierzcholki[temp4].y;
                                    refTriangles[refLicznikTrojkatow].z3 = refWierzcholki[temp4].z;
                                    ++refLicznikTrojkatow;
                                } else {
                                    #ifdef _DEBUG
                                    plik2 << "PROFIL xa = " << refWierzcholki[temp2].x << "     ya = " << refWierzcholki[temp2].y << "\n";
                                    plik2 << "PROFIL xb = " << refWierzcholki[temp3].x << "     yb = " << refWierzcholki[temp3].y << "\n";
                                    plik2 << "PROFIL xc = " << refWierzcholki[temp4].x << "     yc = " << refWierzcholki[temp4].y << "\n";
                                    #endif
                                }
                            } else {
                                #ifdef _DEBUG
                                plik2 << "ZA DLUGI x = " << refWierzcholki[temp2].x << "     y = " << refWierzcholki[temp2].y << "     AB= " << AB << "\n";
                                plik2 << "ZA DLUGI x = " << refWierzcholki[temp3].x << "     y = " << refWierzcholki[temp3].y << "     BC= " << BC << "\n";
                                plik2 << "ZA DLUGI x = " << refWierzcholki[temp4].x << "     y = " << refWierzcholki[temp4].y << "     CA= " << CA << "\n";
                                #endif
                            }
                            znalazlem = true;
//                        plik2 << "nrLinii=" << nrLinii << " refWierzcholki[" << temp22 << "].x=" << refWierzcholki[temp22].x << " refWierzcholki[" << temp22 << "].y=" << refWierzcholki[temp22].y << " refWierzcholki[" << temp22 << "].z=" << refWierzcholki[temp22].z << " refWierzcholki[" << temp33 << "].x=" << refWierzcholki[temp33].x << " refWierzcholki[" << temp33 << "].y=" << refWierzcholki[temp33].y << " refWierzcholki[" << temp33 << "].z=" << refWierzcholki[temp33].z << " refWierzcholki[" << temp44 << "].x=" << refWierzcholki[temp44].x << " refWierzcholki[" << temp44 << "].y=" << refWierzcholki[temp44].y << " refWierzcholki[" << temp44 << "].z=" << refWierzcholki[temp44].z << "\n";
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
    #ifdef _DEBUG
    plik2.close();
    #endif // _DEBUG
}

void utworzDodatkowePunktySiatki(std::vector<triangle> &refTriangles, std::vector<wierzcholek> &refRefWierzcholki) {
    unsigned int liczbaTrojkatow = refTriangles.size();
    unsigned int dotychczasowaLiczbaWierzcholkow = refRefWierzcholki.size();
    std::cout << "Teraz czas utowrzyc dodatkowe punkty zageszczajace siatke:\n";
    for (unsigned int i = 0, ii = dotychczasowaLiczbaWierzcholkow; i < liczbaTrojkatow; ++i, ++ii) {
        refRefWierzcholki.push_back(wierzcholek());
        refRefWierzcholki[ii].x = (refTriangles[i].x1 + refTriangles[i].x2 + refTriangles[i].x3) / 3;
        refRefWierzcholki[ii].y = (refTriangles[i].y1 + refTriangles[i].y2 + refTriangles[i].y3) / 3;
        refRefWierzcholki[ii].z = (refTriangles[i].z1 + refTriangles[i].z2 + refTriangles[i].z3) / 3;
        std::cout << "Pierwotna liczba wierzcholkow: " << dotychczasowaLiczbaWierzcholkow << ". Nr dodatkowego wierzcholka" << i << "          \r";
    }
    std::cout << "\nKoniec tworzenia dodatkowych wierzcholkow\n";
}

void utworzDodatkowePunktySiatkiZUwzglednieniemProfilu(std::vector<triangle> &refTriangles, std::vector<wierzcholek> &refRefWierzcholki, std::vector<wierzcholek> &refRefWierzcholkiProfilu, unsigned int &refRefNrId, std::vector<std::vector<unsigned int> > &refRefTablica2, unsigned int szerokosc2, unsigned int refRefKorektaX2, unsigned int refRefKorektaY2, unsigned int refRefWierszeTablicy2, unsigned int refRefKolumnyTablicy2) {
    const unsigned int liczbaTrojkatow = refTriangles.size();
    unsigned int dotychczasowaLiczbaWierzcholkow = refRefNrId;
    const unsigned int liczbaWierzcholkowProfilu = refRefWierzcholkiProfilu.size();
    std::cout << "Teraz czas utowrzyc dodatkowe punkty zageszczajace siatke:\n";
    for (unsigned int z = 0; z < liczbaTrojkatow; ++z, ++refRefNrId) {
//        double roznicaMiedzyPunktamiZageszczonymiIProfilu1 = 91.0;
        bool zaBlisko = false, nieSprawdzaj = false;
        double nowyX = (refTriangles[z].x1 + refTriangles[z].x2 + refTriangles[z].x3) / 3.0;
        double nowyY = (refTriangles[z].y1 + refTriangles[z].y2 + refTriangles[z].y3) / 3.0;
        if (((nowyX - refRefKorektaX2) / szerokosc2 < 1) || ((nowyX - refRefKorektaX2) / szerokosc2 > refRefWierszeTablicy2)) nieSprawdzaj = true;
        if (((nowyY - refRefKorektaY2) / szerokosc2 < 1) || ((nowyY - refRefKorektaY2) / szerokosc2 > refRefKolumnyTablicy2)) nieSprawdzaj = true;
        if (!nieSprawdzaj) {
            // Aby nie tworzylo nowych wierzcholkow za blisko profilu z Rainsteda
            if (refRefTablica2[(nowyX - refRefKorektaX2) / szerokosc2][(nowyY - refRefKorektaY2) / szerokosc2] == 1) zaBlisko = true;
            // Bo tworzyly sie sporadycznie wierzcholki nad wierzcholkami
            for (unsigned int j = 0; j < dotychczasowaLiczbaWierzcholkow; ++j) {
                if (((refRefWierzcholki[j].x - nowyX > -0.01) && (refRefWierzcholki[j].x - nowyX < 0.01)) && ((refRefWierzcholki[j].y - nowyY > -0.01) && (refRefWierzcholki[j].y - nowyY < 0.01))) zaBlisko = true;
            }
            if (!zaBlisko) {
                refRefWierzcholki.push_back(wierzcholek{nowyX, nowyY, (refTriangles[z].z1 + refTriangles[z].z2 + refTriangles[z].z3) / 3.0, refRefNrId});
//            refRefWierzcholki.push_back(wierzcholek());
//            refRefWierzcholki[refRefNrId].x = nowyX;
//            refRefWierzcholki[refRefNrId].y = nowyY;
//            refRefWierzcholki[refRefNrId].z = (refTriangles[z].z1 + refTriangles[z].z2 + refTriangles[z].z3) / 3;
//            refRefWierzcholki[refRefNrId].odlegloscNMT = refRefNrId;
                std::cout << "Pierwotna liczba wierz.: " << dotychczasowaLiczbaWierzcholkow << ". Liczba dodatkowych wierz.: " << z << "          \r";
            }
        }
    }
    // A na koniec dodajemy wierzcholki profilu :)
    dotychczasowaLiczbaWierzcholkow = refRefWierzcholki.size();
    for (unsigned int i = 0, ii = dotychczasowaLiczbaWierzcholkow; i < liczbaWierzcholkowProfilu; ++i, ++ii) {
        refRefWierzcholki.push_back(wierzcholek());
        refRefWierzcholki[ii].x = refRefWierzcholkiProfilu[i].x;
        refRefWierzcholki[ii].y = refRefWierzcholkiProfilu[i].y;
        refRefWierzcholki[ii].z = refRefWierzcholkiProfilu[i].z;
    }
    std::cout << "\nKoniec tworzenia dodatkowych wierzcholkow\n";
}

void zapisSymkowychTrojkatow(std::vector<triangle> &refTriangles, double ExportX, double ExportY) {
    unsigned int licznik = 0, zmianaPliku1 = 2400000, zmianaPliku2 = 4800000, zmianaPliku3 = 7200000, zmianaPliku4 = 9600000, zmianaPliku5 = 12000000, zmianaPliku6 = 14400000, zmianaPliku7 = 16800000, zmianaPliku8 = 19200000;
    std::string nazwaPliku = "teren.scm";
    unsigned int nrPliku = 1;
    std::string numerPliku = to_string(nrPliku);
    std::string nowaNazwaPliku = nazwaPliku;
    nowaNazwaPliku.insert(5,numerPliku);
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
//    plik1.precision(32);
// Zegarek
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
    std::string str(buffer);
    unsigned int liczbaTrojkatow = refTriangles.size();

    if (liczbaTrojkatow > 0) {
        plik1 << "// generated by TerenEU07.exe " << AutoVersion::FULLVERSION_STRING << AutoVersion::STATUS_SHORT << " on " << str << "\n";
        plik1 << "// Przesuniecie scenerii x = " << ExportX << ", y = " << ExportY << "\n\n";
    }
// W petli leci po wszystkich punktach NMT
    for (unsigned int i = 0; i < liczbaTrojkatow; ++i) {
            double XwektorAB = ((refTriangles[i].x1 - ExportX) * -1.0) - ((refTriangles[i].x2 - ExportX) * -1.0);
            double YwektorAB = (refTriangles[i].y1 - ExportY) - (refTriangles[i].y2 - ExportY);
            double ZwektorAB = refTriangles[i].z1 - refTriangles[i].z2;

            double XwektorCB = ((refTriangles[i].x1 - ExportX) * -1.0) - ((refTriangles[i].x3 - ExportX) * -1.0);
            double YwektorCB = (refTriangles[i].y1 - ExportY) - (refTriangles[i].y3 - ExportY);
            double ZwektorCB = refTriangles[i].z1 - refTriangles[i].z3;

            double XiloczynABiCB = (YwektorAB * ZwektorCB) - (ZwektorAB * YwektorCB);
            double YiloczynABiCB = (ZwektorAB * XwektorCB) - (XwektorAB * ZwektorCB);
            double ZiloczynABiCB = (XwektorAB * YwektorCB) - (YwektorAB * XwektorCB);

            double dlugoscIloczynABiCB = hypot(XiloczynABiCB, hypot(YiloczynABiCB, ZiloczynABiCB));

            if (ZiloczynABiCB > 0) {
                if ((refTriangles[i].z2 > 800) || (refTriangles[i].z1 > 800) || (refTriangles[i].z3 > 800)) {
//                    plik1 << "// Trojkat 1 - Iloczyn wektora AB i CB =[" << XiloczynABiCB << ", " << YiloczynABiCB << ", " << ZiloczynABiCB << "] xyz. I dlugosc = " << dlugoscIloczynABiCB << "\n";
                    plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial snow\n";
                    plik1 << (refTriangles[i].x2 - ExportX) * -1.0 << " " << refTriangles[i].z2 << " " << refTriangles[i].y2 - ExportY << " " << XiloczynABiCB / dlugoscIloczynABiCB << " " << ZiloczynABiCB / dlugoscIloczynABiCB << " " << YiloczynABiCB / dlugoscIloczynABiCB << " " << ((refTriangles[i].x2 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y2 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x1 - ExportX) * -1.0 << " " << refTriangles[i].z1 << " " << refTriangles[i].y1 - ExportY << " " << XiloczynABiCB / dlugoscIloczynABiCB << " " << ZiloczynABiCB / dlugoscIloczynABiCB << " " << YiloczynABiCB / dlugoscIloczynABiCB << " " << ((refTriangles[i].x1 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y1 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x3 - ExportX) * -1.0 << " " << refTriangles[i].z3 << " " << refTriangles[i].y3 - ExportY << " " << XiloczynABiCB / dlugoscIloczynABiCB << " " << ZiloczynABiCB / dlugoscIloczynABiCB << " " << YiloczynABiCB / dlugoscIloczynABiCB << " " << ((refTriangles[i].x3 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y3 - ExportY) / 30.0 << "\n";
                    plik1 << "endtri\n";
                } else {
//                    plik1 << "// Trojkat 1 - Iloczyn wektora AB i CB =[" << XiloczynABiCB << ", " << YiloczynABiCB << ", " << ZiloczynABiCB << "] xyz. I dlugosc = " << dlugoscIloczynABiCB << "\n";
                    plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial grass\n";
                    plik1 << (refTriangles[i].x2 - ExportX) * -1.0 << " " << refTriangles[i].z2 << " " << refTriangles[i].y2 - ExportY << " " << XiloczynABiCB / dlugoscIloczynABiCB << " " << ZiloczynABiCB / dlugoscIloczynABiCB << " " << YiloczynABiCB / dlugoscIloczynABiCB << " " << ((refTriangles[i].x2 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y2 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x1 - ExportX) * -1.0 << " " << refTriangles[i].z1 << " " << refTriangles[i].y1 - ExportY << " " << XiloczynABiCB / dlugoscIloczynABiCB << " " << ZiloczynABiCB / dlugoscIloczynABiCB << " " << YiloczynABiCB / dlugoscIloczynABiCB << " " << ((refTriangles[i].x1 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y1 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x3 - ExportX) * -1.0 << " " << refTriangles[i].z3 << " " << refTriangles[i].y3 - ExportY << " " << XiloczynABiCB / dlugoscIloczynABiCB << " " << ZiloczynABiCB / dlugoscIloczynABiCB << " " << YiloczynABiCB / dlugoscIloczynABiCB << " " << ((refTriangles[i].x3 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y3 - ExportY) / 30.0 << "\n";
                    plik1 << "endtri\n";
                }

            } else {
                if ((refTriangles[i].z2 > 800) || (refTriangles[i].z1 > 800) || (refTriangles[i].z3 > 800)) {
//                  plik1 << "// Trojkat 1 - Iloczyn wektora AB i CB =[" << XiloczynABiCB << ", " << YiloczynABiCB << ", " << ZiloczynABiCB << "] xyz. I dlugosc = " << dlugoscIloczynABiCB << "\n";
                    plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial snow\n";
                    plik1 << (refTriangles[i].x1 - ExportX) * -1.0 << " " << refTriangles[i].z1 << " " << refTriangles[i].y1 - ExportY << " " << (XiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (ZiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (YiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << ((refTriangles[i].x1 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y1 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x2 - ExportX) * -1.0 << " " << refTriangles[i].z2 << " " << refTriangles[i].y2 - ExportY << " " << (XiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (ZiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (YiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << ((refTriangles[i].x2 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y2 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x3 - ExportX) * -1.0 << " " << refTriangles[i].z3 << " " << refTriangles[i].y3 - ExportY << " " << (XiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (ZiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (YiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << ((refTriangles[i].x3 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y3 - ExportY) / 30.0 << "\n";
                    plik1 << "endtri\n";
                } else {
//                  plik1 << "// Trojkat 1 - Iloczyn wektora AB i CB =[" << XiloczynABiCB << ", " << YiloczynABiCB << ", " << ZiloczynABiCB << "] xyz. I dlugosc = " << dlugoscIloczynABiCB << "\n";
                    plik1 << "node -1 0 none triangles material ambient: 104 104 104 diffuse: 208 208 208 specular: 146 146 146 endmaterial grass\n";
                    plik1 << (refTriangles[i].x1 - ExportX) * -1.0 << " " << refTriangles[i].z1 << " " << refTriangles[i].y1 - ExportY << " " << (XiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (ZiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (YiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << ((refTriangles[i].x1 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y1 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x2 - ExportX) * -1.0 << " " << refTriangles[i].z2 << " " << refTriangles[i].y2 - ExportY << " " << (XiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (ZiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (YiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << ((refTriangles[i].x2 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y2 - ExportY) / 30.0 << " end\n";
                    plik1 << (refTriangles[i].x3 - ExportX) * -1.0 << " " << refTriangles[i].z3 << " " << refTriangles[i].y3 - ExportY << " " << (XiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (ZiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << (YiloczynABiCB * -1.0) / dlugoscIloczynABiCB << " " << ((refTriangles[i].x3 - ExportX) * -1.0) / 30.0 << " " << (refTriangles[i].y3 - ExportY) / 30.0 << "\n";
                    plik1 << "endtri\n";
                }
            }
            std::cout << "Petla " << i << " z: " << liczbaTrojkatow << "                                   \r";
            licznik = licznik + 6;
            if ((licznik == zmianaPliku1) || (licznik == zmianaPliku2) || (licznik == zmianaPliku3) || (licznik == zmianaPliku4) || (licznik == zmianaPliku5) || (licznik == zmianaPliku6) || (licznik == zmianaPliku7) || (licznik == zmianaPliku8)) {
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

void sadzenieDrzew(std::vector<triangle> &refTriangles, double ExportX, double ExportY) {
    unsigned int licznik = 0, zmianaPliku1 = 2400000, zmianaPliku2 = 4800000, zmianaPliku3 = 7200000, zmianaPliku4 = 9600000, zmianaPliku5 = 12000000, zmianaPliku6 = 14400000, zmianaPliku7 = 16800000, zmianaPliku8 = 19200000;
    std::string nazwaPliku = "drzewa.scm";
    std::string wylosowaneDrzewo ("");
    unsigned int nrPliku = 1, z = 0, liczbaTrojkatow = refTriangles.size(), minWysokoscDrzewa = 0, maxWysokoscDrzewa = 0, minPromienKorona = 0, maxPromienKorona = 0;
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
        unsigned znaleziono = nazwaDrzewa.find("dds");
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
//    plik1.precision(32);
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
//    typedef std::minstd_rand G;
//    typedef std::uniform_int_distribution<> D;

//    G g( time( NULL ) );
//    D d(0,liczbaDrzew);
//    D dd(0,360);
//    D ddd(3,30);
//    D dddd(3,12);
    	srand( time( NULL ) );
// W petli leci po wszystkich punktach NMT
    	for (unsigned int i = 0; i < liczbaTrojkatow; ++i) {
            if (z == 0) {
                wylosowaneDrzewo = tabelaDrzew[rand() % liczbaDrzew];
//              wylosowaneDrzewo = tabelaDrzew[d(g)];
                z = 10000;
                unsigned znaleziono1 = (wylosowaneDrzewo.find("drzewo"));
                unsigned znaleziono2 = (wylosowaneDrzewo.find("sosna"));
                if ((znaleziono1 != -1) || (znaleziono2 != -1)) {
                    minWysokoscDrzewa = 4;
                    maxWysokoscDrzewa = 31;
                    minPromienKorona = 4;
                    maxPromienKorona = 13;
                } else {
                    minWysokoscDrzewa = 1;
                    maxWysokoscDrzewa = 4;
                    minPromienKorona = 1;
                    maxPromienKorona = 4;
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
            plik1 << "include tree.inc " << wylosowaneDrzewo << " " << (barycentrum1x - ExportX) * -1.0 + randomizer << " " << barycentrum1z << " " << barycentrum1y - ExportY + randomizer << " " << rand() % 360 << " " << minWysokoscDrzewa + (rand() % maxWysokoscDrzewa) << " " << minPromienKorona + (rand() % maxPromienKorona) << " end\n";
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
            licznik = licznik + 6;
            if ((licznik == zmianaPliku1) || (licznik == zmianaPliku2) || (licznik == zmianaPliku3) || (licznik == zmianaPliku4) || (licznik == zmianaPliku5) || (licznik == zmianaPliku6) || (licznik == zmianaPliku7) || (licznik == zmianaPliku8)) {
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

void zrobOtoczke(std::vector<wierzcholek> &refWierzcholki, std::vector<wypukla> &refOtoczka, std::vector<punkty> &refBezOtoczki) {
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
        if (sprawdzKatPolarny(refWierzcholki[k].x, refWierzcholki[k].y, refWierzcholki[j].x, refWierzcholki[j].y, refWierzcholki[i].x, refWierzcholki[i].y) > 0) {
            refBezOtoczki.push_back(punkty());
            refBezOtoczki[l].x = refWierzcholki[j].x;
            refBezOtoczki[l].y = refWierzcholki[j].y;
            refBezOtoczki[l].z = refWierzcholki[j].z;
            ++l;
            ++j;
//            plik2 << "Bez otoczki - punkt x=" << refWierzcholki[j].x << " y=" << refWierzcholki[j].y << " z=" << refWierzcholki[j].z << "\n";
        }
        else if (i < ileWierzcholkow-1) {
            refOtoczka.push_back(wypukla());
            refOtoczka[k].x = refWierzcholki[k].x;
            refOtoczka[k].y = refWierzcholki[k].y;
            refOtoczka[k].z = refWierzcholki[k].z;
            refOtoczka[k].id = i;
            ++j;
            ++k;
 //           plik2 << "Jest otoczka - id =" << i << " punkt x=" << refWierzcholki[k].x << " y=" << refWierzcholki[k].y << " z=" << refWierzcholki[k].z << "\n";
        }
       else if (i == ileWierzcholkow) {
       	refOtoczka.push_back(wypukla());
            refOtoczka[k].x = refWierzcholki[k].x;
            refOtoczka[k].y = refWierzcholki[k].y;
            refOtoczka[k].z = refWierzcholki[k].z;
            refOtoczka[k].id = i - 2.0;
            refOtoczka.push_back(wypukla());
            refOtoczka[j].x = refWierzcholki[j].x;
            refOtoczka[j].y = refWierzcholki[j].y;
            refOtoczka[j].z = refWierzcholki[j].z;
            refOtoczka[j].id = i - 1.0;
            refOtoczka.push_back(wypukla());
            refOtoczka[i].x = refWierzcholki[i].x;
            refOtoczka[i].y = refWierzcholki[i].y;
            refOtoczka[i].z = refWierzcholki[i].z;
            refOtoczka[i].id = i;
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

void zapisPunktowDoTriangulacji(std::vector<wierzcholek> &refWierzcholki, std::vector<wypukla> &refOtoczka, double ExportX, double ExportY, std::string zarostek) {
    unsigned int i = 0, ileWierzcholkow = refWierzcholki.size(), poprawka = 1;
//    unsigned int ileOtoczki = refOtoczka.size(), j = 1, jestOtoczka = 0;
// Otwarcie pliku do zbierania punktow
    std::ofstream plik1;
    std::string nazwaPliku = "wierzcholki.node";
    if (zarostek == "zageszczone") {
        poprawka = 1;
    }
    nazwaPliku.insert(11,zarostek);
    plik1.open(nazwaPliku.c_str());
    plik1 << ileWierzcholkow - poprawka << " 2 0 0\n";
    if (!plik1) {
        std::cout << "Brak pliku " << nazwaPliku << "\n";
        std::cin.get();
    }
    plik1.setf( std::ios::fixed, std:: ios::floatfield );
//    plik1.precision(32);
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
//    plik2.close();
}

void obrobkaDanychNodeDoZageszczeniaPoTriangulacjiZUwzglednieniemProfilu(std::string szerokosc, std::vector<wierzcholek> &refWierzcholki, std::vector<wierzcholek> &refWierzcholkiProfilu, unsigned int &refNrId, std::vector<std::vector<unsigned int> > &refTablica2, unsigned int szerokosc2, unsigned int refKorektaX2, unsigned int refKorektaY2, unsigned int refWierszeTablicy2, unsigned int refKolumnyTablicy2) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
    std::vector<double> odlegloscNMT;
    std::vector<wierzcholek> wierzcholkiTriangles;
    std::vector<wierzcholek> wierzcholkiPoTriangulacji;
    std::vector<triangle> triangles;
    unsigned int licznikWierzcholkow = 0, licznikTrojkatow = 0;
    const double oganiczenieDlugosciRamionTrojkata = 300.0;
    odlegloscNMT.clear();
    wierzcholkiPoTriangulacji.clear();
    wierzcholkiTriangles.clear();
    odczytPunktowNode(wierzcholkiPoTriangulacji, odlegloscNMT, szerokosc, licznikWierzcholkow);
    odczytPlikuPoTriangulacji(triangles, wierzcholkiPoTriangulacji, szerokosc, oganiczenieDlugosciRamionTrojkata, licznikTrojkatow, wierzcholkiTriangles);
    utworzDodatkowePunktySiatkiZUwzglednieniemProfilu(triangles, refWierzcholki, refWierzcholkiProfilu, refNrId, refTablica2, szerokosc2, refKorektaX2, refKorektaY2, refWierszeTablicy2, refKolumnyTablicy2);
    std::cout << "Program zakonczyl dzialanie. Nacisnij jakis klawisz.                           \n" << "\n";
}

void obrobkaDanychHGTPrzedTriangulacja(std::vector<std::string> &refTabelaNazwPlikowHGT, double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
    unsigned int nrId = 0, wierszeTablicy = 0, kolumnyTablicy = 0, korektaX = 0, korektaY = 0;
    std::vector<double> odlegloscHGT1;
    std::vector<wierzcholek> wierzcholki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<wypukla> otoczka;
//    std::vector<punkty> bezOtoczki;
    std::vector<std::vector<unsigned int> > tablica;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowHGT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    odlegloscHGT1.clear();
    wierzcholki.clear();
    toryZGwiazdka.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowHGT(wierzcholki, tablica, refTabelaNazwPlikowHGT[i], odlegloscHGT1, nrId, i, liczbaPlikow, atoi(szerokoscTablicy.c_str()), toryZGwiazdka, korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
    }
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
}

void obrobkaDanychTXTPrzedTriangulacja(std::vector<std::string> &refTabelaNazwPlikowTXT, double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
    unsigned int nrId = 0, wierszeTablicy = 0, kolumnyTablicy = 0, korektaX = 0, korektaY = 0;
    std::vector<double> odlegloscTXT1;
    std::vector<wierzcholek> wierzcholki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<wypukla> otoczka;
//    std::vector<punkty> bezOtoczki;
    std::vector<std::vector<unsigned int> > tablica;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowTXT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    odlegloscTXT1.clear();
    wierzcholki.clear();
    toryZGwiazdka.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowTXT(wierzcholki, tablica, refTabelaNazwPlikowTXT[i], odlegloscTXT1, nrId, i, liczbaPlikow, atoi(szerokoscTablicy.c_str()), toryZGwiazdka, korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
    }
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
}

void obrobkaDanychTXT(std::vector<std::string> &refTabelaNazwPlikowTXT, double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
    unsigned int nrId = 0, wierszeTablicy = 0, kolumnyTablicy = 0, korektaX = 0, korektaY = 0;
    std::vector<double> odlegloscTXT1;
    std::vector<wierzcholek> wierzcholki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<wypukla> otoczka;
//    std::vector<punkty> bezOtoczki;
    std::vector<std::vector<unsigned int> > tablica;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowTXT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    odlegloscTXT1.clear();
    wierzcholki.clear();
    toryZGwiazdka.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
//    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowTXT(wierzcholki, tablica, refTabelaNazwPlikowTXT[i], odlegloscTXT1, nrId, i, liczbaPlikow, atoi(szerokoscTablicy.c_str()), toryZGwiazdka, korektaX, korektaY, wierszeTablicy, kolumnyTablicy);
    }
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
}

void obrobkaDanychTXTPrzedTriangulacjaZUwazglednieniemProfilu(std::vector<std::string> &refTabelaNazwPlikowTXT, double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
    unsigned int nrId = 0, wierszeTablicy1 = 0, kolumnyTablicy1 = 0, wierszeTablicy2 = 0, kolumnyTablicy2 = 0, licznikWierzcholkow = 0, szerokosc2 = 36, korektaX1 = 0, korektaY1 = 0, korektaX2 = 0, korektaY2 = 0;
//    unsigned int wierszeTablicyDoUsuniecia = 22000, kolumnyTablicyDoUsuniecia = 22000;
    std::vector<double> odlegloscTXT1;
    std::vector<double> odlegloscTXT2;
    std::vector<wierzcholek> wierzcholki;
    std::vector<wierzcholek> wierzcholkiProfilu;
    std::vector<wypukla> otoczka;
//    std::vector<punkty> bezOtoczki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<std::vector<unsigned int> > tablica1;
    std::vector<std::vector<unsigned int> > tablica2;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowTXT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    odlegloscTXT1.clear();
    odlegloscTXT2.clear();
    wierzcholki.clear();
    wierzcholkiProfilu.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    toryZGwiazdka.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica1, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX1, korektaY1, wierszeTablicy1, kolumnyTablicy1);
    odczytPunktowNode(wierzcholkiProfilu, odlegloscTXT2, "profil", licznikWierzcholkow);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    tablicaWierzcholkowTriangles(tablica2, exportX, exportY, szerokosc2, toryZGwiazdka, korektaX2, korektaY2, wierszeTablicy2, kolumnyTablicy2);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowTXTzUwzglednieniemProfilu(wierzcholki, wierzcholkiProfilu, tablica1, tablica2, refTabelaNazwPlikowTXT[i], odlegloscTXT1, nrId, i, liczbaPlikow, atoi(szerokoscTablicy.c_str()), szerokosc2, toryZGwiazdka, korektaX1, korektaY1, korektaX2, korektaY2, wierszeTablicy1, kolumnyTablicy1, wierszeTablicy2, kolumnyTablicy2);
    }
//    if (szerokoscTablicy == "1000") {
        obrobkaDanychNodeDoZageszczeniaPoTriangulacjiZUwzglednieniemProfilu("150", wierzcholki, wierzcholkiProfilu, nrId, tablica2, szerokosc2, korektaX2, korektaY2, wierszeTablicy2, kolumnyTablicy2);
//    }

    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
}

void obrobkaDanychHGTPrzedTriangulacjaZUwazglednieniemProfilu(std::vector<std::string> &refTabelaNazwPlikowHGT, double &refWspolrzednaX, double &refWspolrzednaY, std::string szerokoscTablicy) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
    unsigned int nrId = 0, wierszeTablicy1 = 0, kolumnyTablicy1 = 0, wierszeTablicy2 = 0, kolumnyTablicy2 = 0, licznikWierzcholkow = 0, szerokosc2 = 36, korektaX1 = 0, korektaY1 = 0, korektaX2 = 0, korektaY2 = 0;
//    unsigned int wierszeTablicyDoUsuniecia = 22000, kolumnyTablicyDoUsuniecia = 22000;
    std::vector<double> odlegloscHGT1;
    std::vector<double> odlegloscHGT2;
    std::vector<wierzcholek> wierzcholki;
    std::vector<wierzcholek> wierzcholkiProfilu;
    std::vector<wypukla> otoczka;
//    std::vector<punkty> bezOtoczki;
    std::vector<punktyTorow> toryZGwiazdka;
    std::vector<std::vector<unsigned int> > tablica1;
    std::vector<std::vector<unsigned int> > tablica2;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
    unsigned int liczbaPlikow = refTabelaNazwPlikowHGT.size();
    std::cout << "Liczba plikow przekazanych do obrobki: "<< liczbaPlikow << "\n";
    odlegloscHGT1.clear();
    odlegloscHGT2.clear();
    wierzcholki.clear();
    wierzcholkiProfilu.clear();
    otoczka.clear();
//    bezOtoczki.clear();
    toryZGwiazdka.clear();
    // Zapis do tablicy wspolrzednych w odleglosci 5 km od torow (
    odczytPunktowTorow(tablica1, exportX, exportY, atoi(szerokoscTablicy.c_str()), korektaX1, korektaY1, wierszeTablicy1, kolumnyTablicy1);
    odczytPunktowNode(wierzcholkiProfilu, odlegloscHGT2, "profil", licznikWierzcholkow);
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    tablicaWierzcholkowTriangles(tablica2, exportX, exportY, szerokosc2, toryZGwiazdka, korektaX2, korektaY2, wierszeTablicy2, kolumnyTablicy2);
    // Odczyt danych HGT pokrywajacych sie z powierzchnia tablicy ( (2000 / 2) + 2000 + 2000 = 5 km )
    for (unsigned int i = 0; i < liczbaPlikow; ++i) {
        odczytPunktowHGTzUwzglednieniemProfilu(wierzcholki, wierzcholkiProfilu, tablica1, tablica2, refTabelaNazwPlikowHGT[i], odlegloscHGT1, nrId, i, liczbaPlikow, atoi(szerokoscTablicy.c_str()), szerokosc2, toryZGwiazdka, korektaX1, korektaY1, korektaX2, korektaY2, wierszeTablicy1, kolumnyTablicy1, wierszeTablicy2, kolumnyTablicy2);
    }
    obrobkaDanychNodeDoZageszczeniaPoTriangulacjiZUwzglednieniemProfilu("150", wierzcholki, wierzcholkiProfilu, nrId, tablica2, szerokosc2, korektaX2, korektaY2, wierszeTablicy2, kolumnyTablicy2);
    sort(wierzcholki.begin(), wierzcholki.end(), by_yx());
//    zrobOtoczke(wierzcholki, otoczka, bezOtoczki);
//    sort(wierzcholki.begin(), wierzcholki.end(), by_xy());
    zapisPunktowDoTriangulacji(wierzcholki, otoczka, exportX, exportY, szerokoscTablicy);
}

void odczytWierzcholkowZTriangles(std::vector<std::string> &refTabelaNazwPlikowHGT, double &refWspolrzednaX, double &refWspolrzednaY) {
// Niezbedne (a moze i zbedne, ale tak wyszlo) zmienne
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
    std::vector<double> odlegloscNMT;
    std::vector<wierzcholek> wierzcholki;
    std::vector<triangle> triangles;
    std::vector<wierzcholek> wierzcholkiTriangles;
    std::vector<punktyTorow> toryZGwiazdka;
    unsigned int licznikWierzcholkow = 0, licznikTrojkatow = 0;
    double exportX = refWspolrzednaX * 1000.0;
    double exportY = refWspolrzednaY * 1000.0;
// Odrzucamy wszystkie trojkaty, ktore maja jakiekolwiek ramie dluzsze o podanej wartosci
    const double oganiczenieDlugosciRamionTrojkata = 300.0;
    odlegloscNMT.clear();
    wierzcholki.clear();
    triangles.clear();
    wierzcholkiTriangles.clear();
    toryZGwiazdka.clear();
    odczytPunktowTorowZGwiazdka(toryZGwiazdka, exportX, exportY);
    odczytWierzcholkowTriangles(wierzcholkiTriangles, exportX, exportY, toryZGwiazdka);
    sort(wierzcholkiTriangles.begin(), wierzcholkiTriangles.end(), by_yx());
    odczytPunktowNode(wierzcholki, odlegloscNMT, szerokosc, licznikWierzcholkow);
    odczytPlikuPoTriangulacji(triangles, wierzcholki, szerokosc, oganiczenieDlugosciRamionTrojkata, licznikTrojkatow, wierzcholkiTriangles);
    zapisSymkowychTrojkatow(triangles, exportX, exportY);
    sadzenieDrzew(triangles, exportX, exportY);
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
    #endif // _WIN32
    #ifdef __unix__
    std::string pathToSRTM ("./SRTM");
    std::string pathToSRTMclosed ("./SRTM/");
    std::string errorSRTMString ("failed to open directory ./SRTM");
    std::string pathToNMT100 ("./NMT100");
    std::string pathToNMT100closed ("./NMT100/");
    std::string errorNMT100String ("failed to open directory ./NMT100");
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
    else {
        if (!(dir = opendir(pathToNMT100.c_str()))) {
        	perror(errorNMT100String.c_str());
        	return;
    	}
        dir = opendir(pathToNMT100.c_str());
    }
    while ((pdir = readdir(dir)) != NULL) {
        if (!strcmp(pdir->d_name, "..") || !strcmp(pdir->d_name, ".")) {
            continue;
        }
// Drobne poprawki i znajduje tylko z takim rozszerzeniem, z jakim chcialem
        std::string nazwaPliku ("");
        nazwaPliku = pdir->d_name;
        unsigned znaleziono = nazwaPliku.find(rozsz);
//npos zaowdzi pod linuksem        if (znaleziono!=std::string::npos) {
        if (znaleziono != -1) {
            if (rozsz == "hgt") {
                nazwaPliku = nazwaPliku.insert(0, pathToSRTMclosed);
            }
            else {
                nazwaPliku = nazwaPliku.insert(0, pathToNMT100closed);
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
    std::vector<std::string> tabelaNazwPlikow;
    double wspolrzednaX = 0, wspolrzednaY = 0;
    std::string rozszerzenie1 ("txt");
    std::string rozszerzenie2 ("hgt");
//    std::string rozszerzenie2 ("dt2");
    char c = 'c';
    tabelaNazwPlikow.clear();

    std::cout << "Witaj w programie TerenEU07.exe - wersja " << AutoVersion::MAJOR << "." << AutoVersion::MINOR << "." << AutoVersion::BUILD << "." << AutoVersion::REVISION << AutoVersion::STATUS_SHORT << "\n\n";
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
            case 'q': { break; }
            case '1': {
                odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
                odczytWierzcholkowZTriangles(tabelaNazwPlikow, wspolrzednaX, wspolrzednaY);
                break;
            }
            case '2': {
                zrobListePlikow(tabelaNazwPlikow, rozszerzenie2);
                odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
                obrobkaDanychHGTPrzedTriangulacja(tabelaNazwPlikow, wspolrzednaX, wspolrzednaY, "150");
                break;
            }
            case '3': {
                zrobListePlikow(tabelaNazwPlikow, rozszerzenie2);
                odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
                obrobkaDanychHGTPrzedTriangulacjaZUwazglednieniemProfilu(tabelaNazwPlikow, wspolrzednaX, wspolrzednaY, "1000");
                break;
            }
            case '4': {
                odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
                obrobkaDanychNodePoTriangulacji(wspolrzednaX, wspolrzednaY, "1000");
                break;
            }
            case '5': {
                zrobListePlikow(tabelaNazwPlikow, rozszerzenie1);
                odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
                obrobkaDanychTXTPrzedTriangulacja(tabelaNazwPlikow, wspolrzednaX, wspolrzednaY, "150");
                break;
            }
            case '6': {
                zrobListePlikow(tabelaNazwPlikow, rozszerzenie1);
                odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
                obrobkaDanychTXTPrzedTriangulacjaZUwazglednieniemProfilu(tabelaNazwPlikow, wspolrzednaX, wspolrzednaY, "1000");
                break;
            }
            case '7': {
                zrobListePlikow(tabelaNazwPlikow, rozszerzenie1);
                odczytWspolrzednychPrzesunieciaSCN(wspolrzednaX, wspolrzednaY);
                obrobkaDanychTXT(tabelaNazwPlikow, wspolrzednaX, wspolrzednaY, "1000");
                break;
            }
        }
    }
// Pauza.
    std::cin.get();
    return 0;
}
