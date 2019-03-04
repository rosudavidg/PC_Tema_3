```
================================================================================
Nume:    Rosu
Prenume: Gabriel - David
Grupa:   321CD
Materia: Protocoale de comunicatii
Titlul:  Tema 3 - Client DNS
Data:    20 mai 2018
================================================================================
                                    Tema 3
                                  Client DNS
================================================================================
[0] Cuprins
--------------------------------------------------------------------------------

    1. Introducere
    2. Continutul arhivei
    3. Implementare 
    4. Rulare
    5. Feedback

================================================================================
[1] Introducere
--------------------------------------------------------------------------------

    Aceasta tema reprezinta dezvoltarea unui client DNS.

================================================================================
[2] Continului arhivei
--------------------------------------------------------------------------------
    
    +=================================================================+
    |        NUME        |               DESCRIERE                    |
    |=================================================================|
    | dnsclient.c        | Fisierul sursa pentru client               |
    | dnsclient.h        | Header folosit de dnsclient.c              |
    | Makefile           | Makefile pentru compilare / stergere       |
    | README             | Prezentul readme                           |
    +=================================================================+
    
================================================================================
[3] Implementare
--------------------------------------------------------------------------------

    Fisierul dnsclient.h contine structurile prezentate in textul temei.

    Fisierul dnsclient.c este singurul fisier sursa, care contine functia
main, care rezolva cerintele temei.
    
    Executabilul creat, dnsclient, primeste exact doua argumente.
    
    Initial, se verifica numarul de argumente primit.
    Daca numarul de argumente nu este 2, atunci programul va iesi cu codul de
eroare 1 si se va afisa mesajul respectiv. (vezi check_argc)

    Dupa, se citesc toate adresele DNS din fisierul dns_servers.conf.
Citesc fiecare linie din fisier. Daca aceasta incepe cu caracterul '#',
aceasta linie va fi ignorata. Altfel, se parseaza adresa primita
intr-un vector de caractere: dns_servers[MAX_DNS_SERVERS_NO][16] (fiecare
adresa are maxim 15 caractere: 4 * 3 + 3, plus NULL). Numarul de adrese
citite va fi salvat in dns_servers_no. (vezi read_dns_servers)

    Se verifica cel de-al doilea argument primit, adica tipul de interogare.
Daca acesta exista, variabila type va primi valoare respectiva (integer).
Daca nu exista, se va intoarce codul de eroare 1 si se va afisa mesajul
respectiv. (vezi find_type)

    Apoi, prelucrez primul argument primit ca parametru, adica numele de
domeniu. Spre exemplu, pentru un argument primit: "www.google.com", 
acesta fiind name, valoare lui qname va fi : "3www6google3com",
unde cifrele reprezinta numarul de caractere pana la urmatorul punct.
Acestea nu sunt trecute ca valoare char, adica '3', ci ca valoare int,
adica char x = 3.

    Folosesc un socket UDP pentru trimiterea datelor prin PORT-ul 53 (DNS).
    Pentru setarile socket-ului, am adaugat un timp limita de 2 secunde.
    In caz de timeout pe adresa curenta, se trece la adresa urmatoare.
    Daca nu se primeste / nu se trimite un mesaj, se trece la adresa urmatoare.

    Cand s-a realizat corect o conexiune si s-a primit raspuns, atunci
decodific raspunsul primit.
    
    Variabila globala message reprezinta o zona de meorie alocata atat pentru
trimiterea unui mesaj catre DNS, cat si pentru primirea acestuia.
    Pentru a "ma putea misca" prin acea zona de memorie folosesc un pointer,
"p".
    Odata cu mutarea pointerului, retin si o valoare de offset: total_offset,
pentru a stii unde ma aflu fata de inceputul zonei de memorie.
    
    Functia read_qname_in_name primeste ca argument un offset fata de inceputul
zonei de memorie si scoate o adresa de tipul qname (cu sau fara pointer)
si o converteste in-place la name (functie foarte utila, utilizata des).
    
    La primirea mesajului, extrag prima data numarul de raspunsuri de fiecare
tip, pentru a putea "aranja" fisierul de log conform cerintei.
    Pentru fiecare tip de raspuns, daca exista cel putin un raspuns de acel
tip, afisez mesajul si decodific fiecare mesaj. Pointerul p se actualizeaza
mereu. Astfel, folosesc o functie care imi decodifica fiecare entry
(read_entry), indiferent de tipul raspunsului (adica categorie - e redundant).
    Aici, intr-un switch tratez fiecare tip de raspuns (A, NS, SOA etc).

    Nu am tratat cazul pentru PTR.
    In rest, am testat programul (aprox 500 de rulari) si functioneaza
conform cerintei (rezultate similare cu utilitarul host).

    In realizarea temei am consultat:
    RFC 1034 https://www.ietf.org/rfc/rfc1034.txt
    RFC 1035 https://www.ietf.org/rfc/rfc1035.txt
    http://www.zytrax.com/books/dns/ch15/
    https://gist.github.com/fffaraz/9d9170b57791c28ccda9255b48315168
    utilitarul host

================================================================================
[4] Rulare
--------------------------------------------------------------------------------

(*) Makefile

    build: compileaza sursa
    
    clean: sterge executabilul

Exemplu rulare client
    ./dnsclient google.com A

================================================================================
[5] Feedback
--------------------------------------------------------------------------------
    
    + Tema isi atinge obiectivele.

================================================================================
                                    SFARSIT
================================================================================
```