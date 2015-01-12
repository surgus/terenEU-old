# TerenEU07
Program tworzy, z danych SRTM, lub NMT100, oraz scenerii z edytora Rainsted,
teren dla realistycznych scenerii symulatora MaSzyna EU07,
o ile sceneria z edytora Rainsted posiada profil pionowy pod torami.

Krótka instrukcja obsługi programu:
W katalogu z programem musimy mieć:
1. Plik ze scenerią (aktualnie na stałe zaszyta nazwa "EXPORT.SCN",
bo taki plik domyślnie tworzy edytor Rainsted).
2. Katalog o nazwie SRTM, a w nim pliki .hgt, lub katalog
o nazwie NMT100, a w nim pliki .txt z obszarem pokrywającym teren scenerii.
3. W tym samym (najlepiej) lub innym katalogu możliwość skompilowania
i uruchomienia darmowego programu "Triangle", który z podanych punktów tworzy trójkąty.

Kompilacja programu TerenEU07:
g++ -std=c++11 -O3 -o terenEU07 main.cpp

Aby uzyskać teren w formacie symulatora MaSzyna EU07 należy
uruchomić program TerenEU07 i wybrać opcję nr 1.
To utworzy nam plik wierzcholkiprofil.node. Następnie 
uruchamiamy ponownie i wybieramy opcję nr 2 lub 5 w zależności
od tego, jakie dane modelu wysokościowego wykorzystujemy. Powstanie plik 
wierzcholki150.node, który trzeba potraktować programem Triangle:
./triangle -YV [sciezka_do_jesli_inny_katalog]wierzcholki150.node.
Utworzonych zostanie kilka plików, ale ważny jest wierzcholki150.1.ele.
Ponownie uruchamiamy program TerenEU7, wybieramy opcję nr 3, lub 6
w zależności od tego jakie dane modelu wysokościowego wykorzystujemy,
i otrzymamy plik wierzcholki1000.node. Ten też przetwarzamy programem Triangle,
podobnie jak poprzedni ./triangle -YV wierzcholki1000.node. Powstaną nowe pliki,
a w nim ten istotny dla nas wierzcholki1000.1.ele. Ostatni etap,
uruchamiamy program TerenEU07 i wybieramy opcję nr 4.
W ten sposób otrzymamy plik(i) terenX.scm (gdzie X to nr od 1),
który można już użyć w symulatorze jako teren.

Dwie uwagi:
1. Edytor Rainsted, pomimo moich sugestii, tworzy plik tekstowy
EXPORT.SCN z końcami linii w formacie Microsoft, a więc należy
go przekonwertować na format unixowy. Np. w programie vi
komendą ":set ff=unix".
2. W edytorze Rainsted na obecną chwilę nie da się w prosty sposób
wykonać rowow profilu pod torami przy użyciu danych modelu NMT100,
dlatego też generowanie całego terenu z danych NMT100 mija się z celem.

Symulator MaSzyna EU07 można pobrać ze strony: http://eu07.pl/.
Edytor Rainsted znajduje się w paczce z symulatorem. Strona projektu http://rainsted.com/pl/.
Program Triangle dostepny pod adresem: http://www.cs.cmu.edu/~quake/triangle.html.
Dane SRTM można pobrać pod adresem: http://dds.cr.usgs.gov/srtm/version2_1/SRTM3/Eurasia/
(nowsza wersja), lub: http://netgis.geo.uw.edu.pl/srtm/Poland/ (starsza wersja, i serwer
ostatnio nie działa).
Dane NMT100 można pobrać z tego adresu: http://www.codgik.gov.pl/index.php/darmowe-dane/nmt-100.html
