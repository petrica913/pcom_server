Petrica Gheorghe-Vladimir 323CA

Tema2 - PCOM, Aplicatie client-server TCP si UDP pentru gestionarea mesajelor


Mod de utilizare:
    Dupa compilarea codurilor, se porneste serverul cu ajutorul comenzii
./server <PORT_DORIT>, iar apoi pornim un client TCP prin intermediul
subscriberului astfel ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>.
Clientul accepta de la STDIN comenzile subscribe, unsubscribe, exit. Dupa
o comanda de subscribe/unsubscribe putem porni un client UDP cu ajutorul
comenzii python3 udp_client.py --source-port <PORT_SURSA> --input_file
<INPUT_FILE> --delay <DELAY_DORIT> <IP_SERVER> <PORT_SERVER>.

    Comanda exit rulata pe subscriber va face sa actualizeze starea cli-
entului pe offline, inchizand socket-ul corespunzator acestuia. Tot comanda
exit, dar rulata in server va inchide sererul si toti socketii corespun-
zatori tuturor clientilor indiferent de starea lor.

    Subscriberul va trimite catre server mesaje tcp, iar clientul UDP va
trimite catre server mesaje udp care le interpreteaza si le trimite mai
departe catre clientii tcp, astfel actualizand topicurile la care clientii
sunt abonati.

Structuri de date folosite in implementarea serverului si a subscriberului:
    * udp_msg: O structura care se ocupa de gestonarea mesajelor venite de
la clientii UDP (retinem portul clientului, ip-ul acestuia, topicul pe care
il actualizeaza, tipul de data pe care il transmite, un buffer in care este
retinuta informatia transmisa, iar in result vom avea mesajul care va fi
afisat clientilor TCP)
    * tcp_msg: O structura care se va ocupa de gestionarea mesajelor trimise
de clientii TCP catre server. In principiu se vor trimite informatii legate
de comenziile date de client
    * Client: care va tine evidenta pentru fiecare client TCP care este sau
a fost vreodata conectat la server. Vom retine socket-ul clientului, starea
sa, daca este activa sau nu pe server, id-ul clientului, topicurile la care
acesta este abonat.

Mod de implementare a serverului:
    Initializez socket-urile pentru atat pentru clientii UDP cat si TCP si
le leg la portul specificat in linia de comanda. Dezactivez algoritmul Nagle
prin setarea optiunii TCP_NODELAY (se poate obtine o performanta mai buna in
anumite cazuri). Initializez setul de descriptori de fisiere pentru multi-
plexarea conexiunilor, iar mai apoi deshidem o bucla infinita pentru a as-
culta mesajele venite de pe socketi cu ajutorul functiei select. In functie
de mesajul venit de pe socket apelam urmatoarele functii: handle_stdin_input,
handle_tcp_client_connection, handle_udp_messages.
    * handle_stdin_input: se ocupa cu gestionarea mesajului "exit" venit pe
CLI-ul serverului (va inchide serverul si toate conexiunile deschise).
    * handle_tcp_client_connection: gestioneaza mesajele provenite de la
clientii TCP (adauga un nou client sau inchide conexiunea dintre un client si
server).
    * handle_udp_messages: gestioneaza mesajele provenite pe socket-ul udp
(va itera printre toti clientii activi ai serverului si va trimite update-uri
acestora in functie de topicul la care sunt abonati). Atunci cand un client
este abonat la un topic de tip wildcard, vom apela o functie regex(matchTopic)
care va verifica daca topicul primit de pe socket-ul udp se potriveste cu
topicul la care este abonat clientul


    