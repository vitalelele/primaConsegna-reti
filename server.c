/* Sviluppato con cura da Antonio Vitale [754740], Angelo Sciarra[758256], Antonio Troncellito[754736] */

#if defined WIN32 // se è win32 includiamo la libreria <winsock.h>
    #include <winsock.h>
#else // se quindi l'OS è macOS
    #define closesocket close
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif // questo if defined controlla che l'OS sia win32 piuttosto che macOS 

// librerie standard per ambo gli OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORTA_SERVER 7346  // definiamo la porta server da utilizzare 
#define MAX_CONNESSIONI 5 // definiamo un limite massimo di connessioni
#define BUFFERSIZE 512   // definiamo la grandezza del buffer come costante (BUFFERSIZE)
#define MAXSTR 255	    // definiamo la lunghezza MAX di una stringa che si può mandare (MAXSTR) abbiamo scelto 255 perché è
					   // più o meno la metà del buffersize (512)


void ClearWinSock(){
    #ifdef WIN32
        WSACleanup;
    #endif 
}


// funzione che trasforma una stringa in lowercase
// tolower() trasforma un singolo carattere
char* stringLower(char* stringa){
	int i; // bisogna dichiararlo fuori altrimenti bisogna settare il compiler con lo std=c99 per dichiarare nel for()
    for(i=0; i < strlen(stringa); i++){
        stringa[i] = tolower(stringa[i]);
    }
    return stringa;
}

// main()
int main(int argc,char *argv[]){
	
    int port;
    if(argc > 1){
        port = atoi(argv[1]);
    }else{
        port = PORTA_SERVER;
    }
    if(port < 0){
        printf("Port number errato. %s\n", argv[1]);
        return 0;
    }

    #ifdef WIN32
        WSADATA wsaData; // inizializziamo un elemento WSADATA per essere sicuri che le socket windows siano supportate dal sistema
        // MAKEWORD(2,2) specifica il numero di winsock sul sistema
        int risultatoWSA = WSAStartup(MAKEWORD(2,2), &wsaData);

        if(risultatoWSA != 0){
            printf("%s","Errore nell'inizializzazione di WSAStartup()\n");
            return 0;
        }
    #endif // defined

    // Creazione della socket 
    
    // MySocket è la variabile che contiene il descrittore della socket del server
    // int socket (int famigliaProtocolli, int tipoDiSocket, int portocolloDaUsare)
	// nel nostro caso PF_INET = Internet Protocol Family, SOCK_STREAM poichè è una socket TCP, IPPROTO_TCP)
    int MySocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(MySocket < 0){
        printf("%s", "Errore nella creazione della socket.\n");
        ClearWinSock();
        return -1;
    }
    
    // Assegnazione dell'indirizzo della socket
    // utilizziamo la struttura dati sockaddr_in e costruiamo un elemento di tipo sockaddr_in (serverAddress)
    struct sockaddr_in serverAddress;
    // void * memset( void *buffer, int c, size_t count )
	// copia il valore di "c" nell'area di memoria (puntata da una variabile di tipo Puntatore ) 
	// per una quantità stabilita nell'argomento count.
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // indirizzo di default (localhost) poichè il server è la nostra stessa macchina
    serverAddress.sin_port = htons(PORTA_SERVER);  // poiché è il server che deve ricevere
 
 	// assegnamo la porta e l'ip alla socket e verifichiamo se non ci sono errori
	// int bind (int socket, struct sockaddr* localaddress, int addressLength)
	// bind ritorna 0 in caso di successo e -1 altrimenti
    if(bind(MySocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){
        printf("%s","Operazione di bind() fallita.\n");
        system("pause"); // evitiamo di far chiudere la socket senza premere un tasto
        closesocket(MySocket);
        ClearWinSock();
        return -1;
    }
	
	// Ora settiamo la socket in ascolto
	// controlliamo se il listen() NON ha avuto successo, eventualmente avvisiamo l'utente con una stampa.
    if(listen(MySocket, MAX_CONNESSIONI) < 0){
        printf("%s", "Operazione di listen() fallita.\n");
        system("pause");
        closesocket(MySocket);
        ClearWinSock();
        return -1;
    }
	
	
	// Accettiamo una nuova connessione 
    struct sockaddr_in clientAddress;
    int clientSocket;
    int clientLen;
    printf("%s\n", "Attendo connesione di un nuovo client...");
	// itero all'infinito fin quando un client non si connette
    while(1){
        clientLen = sizeof(clientAddress);
        // Se l'accept() non va a buon fine mandiamo un messaggio
        if((clientSocket = accept(MySocket, (struct sockaddr*) &clientAddress, &clientLen)) < 0){
            printf("%s", "Operazione di accept() fallita.\n");
            system("pause");
            ClearWinSock();
            continue;
        }
        // accept() andata a buon fine, stampiamo con chi si è connesso il server
        printf("Connessione stabilita con il client: %s \n", inet_ntoa(clientAddress.sin_addr));
		
		// creiamo il messaggio "connession avvenuta" da mandare al client
        char* msgConnessioneAvvenuta = "Connessione avvenuta.";
        int stringLen = strlen(msgConnessioneAvvenuta); // calcoliamo la lunghezza della stringa (serve alla send())

        // inviamo i dati, se send() non va a buon fine avvisiamo l'utente
        if(send(clientSocket, msgConnessioneAvvenuta, stringLen, 0) != stringLen){
            printf("%s", "Il metodo send() ha inviato un numero diverso di byte rispetto a quelli aspettati.\n");
            system("pause");
            // chiudiamo solo quella del client così da poter riconnettersi un altro client
            closesocket(clientSocket);
            ClearWinSock();
            break;
        }
	
		// utilizziamo un altro while per controllare quando "muore" la connessione con il client
		// questo ci permette di ritornare in ascolto qual'ora la connessione con il client venisse interrotta
		// e di conseguenza un secondo client si potrà connettere al server
        int chiusura_connessione = 0;
        while(chiusura_connessione == 0){
            // variabili per ricevere le stringhe
            char stringaA[MAXSTR];
            char stringaB[MAXSTR];
		
            int bytesRcvd;
            char buf[BUFFERSIZE];
            memset(buf , '\0' , BUFFERSIZE);
            // controlliamo che la recv() vada a buon fine, eventualmente avvisiamo l'utente del problema
            if((bytesRcvd = recv(clientSocket, buf, BUFFERSIZE - 1, 0)) < 0){
                printf("%s", "Il metodo recv() ha fallito.\n");
                closesocket(clientSocket);
                ClearWinSock();
                break;
            }
            // aggiungiamo il fine stringa al buffer
            buf[bytesRcvd] = '\0';
            // strcpy() copia il contenuto di buf nella stringaA
            strcpy(stringaA, buf);
			// azzeriamo il buffer
            memset(buf , '\0' , BUFFERSIZE);
            // ora facciamo la stessa cosa per la seconda stringa
            if((bytesRcvd = recv(clientSocket, buf, BUFFERSIZE - 1, 0)) < 0){
                printf("%s", "Il metodo recv() ha fallito.\n");
                closesocket(clientSocket);
                ClearWinSock();
                break;
            }
            buf[bytesRcvd] = '\0';
            strcpy(stringaB, buf);

            // Controlliamo che la stringa ricevuta non sia "quit", per una maggior precisione abbiamo trasformato la stringa
            // in lowercase in modo tale che in qualsiasi modo l'utente scriva qui quest'ultimo sarà considerato come stringa di uscita
            if(strcmp(stringLower(stringaA), "quit") != 0 && strcmp(stringLower(stringaB), "quit") != 0){
            	// dichiariamo una variabile stringaC che conterrà la concatenazione delle due stringhe
                char* stringaC;
                stringaC = strcat(stringaA, stringaB); // funzione che server per concatenare due stringhe strcat()
                int stringLen = strlen(stringaC);
				
				// facciamo la send() della stringa concatenata
                if(send(clientSocket, stringaC, stringLen, 0) != stringLen){
                    printf("%s", "Il metodo send() ha inviato un numero diverso di byte rispetto a quelli aspettati.\n");
                    system("pause");
                    closesocket(clientSocket);
                    ClearWinSock();
                    break;
                    }
            // fine invio stringa concatenata
            }
            // se una delle due è uguale a quit invio "bye"
            else{
                char* bye = "bye";
                int stringLen = strlen(bye);

                // inviare dati al server
                if(send(clientSocket, bye, stringLen, 0) != stringLen){
                    printf("%s", "Il metodo send() ha inviato un numero diverso di byte rispetto a quelli aspettati.\n");
                    system("pause");
                    // chiudiamo solo quella del client poiché
                    closesocket(clientSocket);
                    ClearWinSock();
                    break;
                    }
                // poichè è stata decisa la chiusura con il client settiamo il flag=1 in modo che esca dal secondo while dichiarato
                chiusura_connessione = 1;
            }

    } // end secondo while
    // avvisiamo che la connessione con quel client è caduta
    printf("Connessione terminata con il client: %s\n\n", inet_ntoa(clientAddress.sin_addr));
    closesocket(clientSocket);
 } // end primo while
} // end main()
