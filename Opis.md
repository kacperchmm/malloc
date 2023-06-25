# Opis projektu "Menadżer pamięci"

### Sposób działania

Mój menadżer pamięci wykorzystuje politykę "next-fit" do znajdowania wolnych bloków, która polega na tym, że szukanie wolnego bloku rozpoczynam od ostatnio przydzielonego bloku, po czym gdy dojdzie na koniec sterty, rozpoczyna szukać od początku sterty, aż do momentu trafienia z powrotem na ostatnio przydzielony blok. Wtedy rozszerzam stertę i nowy blok przydzielam na jego koniec.

### Jak wygląda struktura bloku?
-  Header - pierwsze 4B bloku, w którymch przechowywana jest liczba równa rozmiarowy tego bloku oraz flagi tego bloku
- Footer - ostatnie 4B bloku, których zawartość jest podobna do headera, bez flagi PREVFREE
- Payload - miejsce w którym przechowujemy dane, które chcemy przydzielić (tylko pełne bloki)

- Flagi:
    - FREE - oznacza, że blok jest wolny
    - USED - oznacza, że blok jest zajęty
    - PREVFREE - oznacza, że poprzedni blok jest wolny


Blok zajęty składa się z header'a oraz payload'u.

Blok wolny składa się z header'a, przestrzeni pustej (dawnego payload'u) footera.


### Zwalnianie bloków
Zwalniajając bloki odbywa się poprzez zmienienie flag bloku oraz sprawdzenie i ewentualne połączenie go z wolnymi blokami sąsiednimi.

### mm_checkheap
W funkcji mm_checkheap wypisuje obecny stan sterty:
- pierwszy blok sterty,
- ostatni blok sterty,
- wskaźnik na koniec sterty.

Wypisywane są również wszystkie bloki na naszej stercie razem, z jego flagami, rozmiarem oraz numerem w jakim miejscu na stercie leży aktualnie rozpatrywany blok.

Funkcja ta była przydatna w trakcie pisania programu, lecz w ostatecznej wersji usunąłem jego wszystkie wywołania, aby nie wpływały na jego wydajność.

### Funkcje pomocnicze

#### Wszystkie funkcji zaczynające się od bt_**
Większość tych funkcji została skopiowana z pliku mm-implicit.c załączonego do repozytorium w momencie jego tworzenia. 

#### bt_make
Ustawia flagi i rozmiar w header'ze oraz footer'ze

#### findfit
Funkcja, której zaimplementowanie zainspirowałem się korzystająć z książki CSAPP. Wyszukuje ona wolne bloki, które mają wystarczający rozmiar, aby przydzielić w nim pamięć. Jeśli znajdzie blok o wystarczającej wielkości zwraca wskaźnik na ten blok, a w przeciwnym przypadku zwraca NULL, który w funkcji malloc jest traktowany jako sygnał do przydzielenia bloku na końcu sterty.

#### split_block
Funkcja ta jest wywoływana gdy mając mniejszy blok do przydzielenia, przydzielamy go w takim bloku, którego rozmiar przekracza rozmiar jaki chcemy przydzielić. Wtedy musimy podzielić ten blok na zajęty blok o żądanym rozmiarze oraz blok wolny, którego rozmiar będzie różnicą rozmiaru jaki miał na początku, z rozmiarem przydzielonego bloku.

### merge_free_blocks
Funkcja ta w momencie zwolnienia jakiegoś bloku, sprawdza czy jego sąsiędzi również są blokami wolnymi. Jeśli tak, to łączy go razem z konkretnymi sąsiadami (poprzednim lub następnym, a czasami nawet z oboma) tak, aby żadne dwa bloki wolne nie sąsiadowały ze sobą