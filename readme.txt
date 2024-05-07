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
    