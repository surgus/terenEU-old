# TerenEU07
Program tworzy, z danych SRTM oraz scenerii z edytora Rainsted, teren dla realistycznych scenerii symulatora MaSzyna EU07,
o ile sceneria z edytora Rainsted posiada profil pionowy pod torami.

Krótka instrukcja obsługi programu:
W katalogu z programem musimy mieć:
1. Plik ze scenerią (aktualnie na stałe zaszyta nazwa "EXPORT.SCM", bo taki plik domyślnie tworzy edytor Rainsted).
2. Katalog o nazwie SRTM, a w nim pliki .hgt lub .dem z obszarem pokrywającym teren scenerii.
3. W tym samym (najlepiej) lub innym katalogu możliwość skompilowania i uruchomienia darmowego programu "Triangle",
który z podanych punktów tworzy trójkąty.

Kompilacja programu TerenEU07:
g++ -std=c++11 -O3 -o terenEU07 main.cpp

Aby uzyskać teren w formacie symulatora MaSzyna EU07 należy uruchomić program TerenEU07 i wybrać opcję nr 1.
To utworzy nam plik wierzcholkiprofil.node. Następnie uruchamiamy ponownie i wybieramy opcję nr 2.
Powstanie plik wierzcholki100.node, który trzeba potraktować programem Triangle:
./triangle -YV [sciezka_do_jesli_inny_katalog]wierzcholki100.node.
Utworzonych zostanie kilka plików, ale ważny jest wierzcholki100.1.ele. Ponownie uruchamiamy program TerenEU7,
wybieramy opcję nr 3 i otrzymujemy plik wierzcholki1000.node. Ten też przetwarzamy programem Triangle,
podobnie jak poprzedni ./triangle -YV wierzcholki1000.node. Powstają nowe pliki,
a w nim ten istotny dla nas wierzcholki1000.1.ele. Ostatni etap, uruchamiamy TerenEU07 i wybieramy opcję nr 4.
W ten sposób otrzymujemy plik(i) terenX.scm (gdzie X to nr od 1),  który można użyć w symulatorze.

Dwie uwagi:
1. Edytor Rainsted, pomimo moich sugestii, tworzy plik tekstowy EXPORT.SCN z końcami linii w formacie Microsoft,
a więc należy go przekonwertować na format unixowy. Np. w programie vi komendą ":set ff=unix".
2. Jeśli ktoś chce skompilować program pod sytemem Microsoftu i tam go używać, to powinien poprawić ścieżkę w funkcji:
zrobListePlikow().

Symulator MaSzyna EU07 można pobrać ze strony: http://eu07.pl/.
Edytor Rainsted znajduje się w paczce z symulatorem.
Program Triangle dostepny pod adresem: http://www.cs.cmu.edu/~quake/triangle.html.

