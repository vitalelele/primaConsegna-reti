/* Sviluppato con cura da Antonio Vitale [754740], Angelo Sciarra[758256], Antonio Troncellito[754736] */

#if defined WIN32 // se l'OS è win32 includiamo la libreria <winsock.h>
    #include <winsock.h>
#else // se l'OS fa parte della famiglia UNIX
    #define closesocket close
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif // questo if defined controlla che l'OS sia win32 piuttosto che UNIX 

// librerie standard per ambo gli OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERSIZE 512 // definiamo la grandezza del buffer come costante (BUFFERSIZE)
#define MAXSTR 255	  // definiamo la lunghezza MAX di una stringa che si può mandare (MAXSTR) abbiamo scelto 255 perché è
					 // più o meno la metà del buffersize (512)


void ClearWinSock(){
    #if defined WIN32
        WSACleanup;
    #endif 
}

// main() 
int main(){
    #ifdef WIN32
        WSADATA wsaData; // inizializziamo un elemento WSADATA per essere sicuri che le socket windows siano supportate dal sistema
        // MAKEWORD(2,2) specifica il numero di winsock sul sistema
        int risultatoWSA = WSAStartup(MAKEWORD(2,2), &wsaData);

        if(risultatoWSA != 0){
            printf("%s", "Errore nell'inizializzazione di WSAStartup()\n");
            return 0;
        }
    #endif // defined

    // clientSocket è la variabile che contiene il descrittore della socket del client
    // int socket (int famigliaProtocolli, int tipoDiSocket, int portocolloDaUsare)
	// nel nostro caso PF_INET = Internet Protocol Family, SOCK_STREAM poichè è una socket TCP, IPPROTO_TCP)	
    int clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(clientSocket < 0){
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
	// chiediamo all'utente di inserire la porta del server da contattare
	// se ne inserisce una errata utilizziamo una di default
    int numeroPortaServer;
    printf("%s", "Inserire numero porta del server da contattare: ");
    scanf("%d", &numeroPortaServer);
    while(getchar() != '\n');
    // porta di default = 7346
    if(numeroPortaServer != 7346){
        printf("%s", "La porta inserita non e' quella del server.\nVerra automaticamenta settata la porta 7346 (porta server).\n");
        numeroPortaServer = 7346;
    }
    serverAddress.sin_port = htons(numeroPortaServer);

    // connessione al server
    // connect() stabilisce una connessione ad una socket specificata
    // int connect(int socket, constr struct sockaddr* addr, int addrlen)
    if(connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
      	// se la connect() non va a buon fine avvisiamo
	    printf("%s\n", "Connessione fallita.");
        system("pause");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }

    int bytesRcvd;
    char buf[BUFFERSIZE];
    // azzero il buffer
	memset(buf, '\0', strlen(buf));
	
	// riceviamo i dati dal server (dati di connession avvenuta)
    if((bytesRcvd = recv(clientSocket, buf, BUFFERSIZE - 1, 0)) < 0){
        printf("%s", "Il metodo recv() ha fallito.\n");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }
    buf[bytesRcvd] = '\0';
    printf("%s\n", buf); // stampiamo contenuto buffer ("connessione avvenuta")
	
	// ora chiediamo all'utente le due stringhe da concatenare
	int chiusura_connessione = 0;	// settiamo il flag del while a 0 in modo la iterare fino a quando una stringa non è "quit"
	while(chiusura_connessione == 0){
		char stringaA[MAXSTR];

		printf("Inserisci la stringa A: ");
		// utilizziamo fgets() e non scanf() poiché abbiamo riscontrato dei problemi per acquisire gli spazi 
		// con questa funzione (fgets()) gli spazi vengono risolti
		fgets(stringaA, MAXSTR, stdin);
		stringaA[strlen(stringaA) - 1] = '\0'; // elimino il newline aggiunto in coda

		int stringLen = strlen(stringaA);
		
		// mandiamo al server la stringaA e controlliamo vada a buon fine
		if(send(clientSocket, stringaA, stringLen, 0) != stringLen){
	            printf("%s", "Il metodo send() ha inviato un numero diverso di byte rispetto a quelli aspettati.\n");
	        	system("pause");
	        	// chiudiamo solo quella del client poiché il server resta in ascolto
	            closesocket(clientSocket);
	            ClearWinSock();
	            break;
	            }

		// stringaB
		char stringaB[MAXSTR];
		printf("Inserisci la stringa B: ");
		// utilizziamo fgets() e non scanf() poiché abbiamo riscontrato dei problemi per acquisire gli spazi 
		// con questa funzione (fgets()) gli spazi vengono risolti
		// doc fgets() https://www.tutorialspoint.com/c_standard_library/c_function_fgets.htm
		fgets(stringaB, MAXSTR, stdin);
		stringaB[strlen(stringaB) - 1] = '\0'; // elimino il newline aggiunto in coda
		stringLen = strlen(stringaB);
		
		// mandiamo al server la stringaB e controlliamo vada a buon fine
		if(send(clientSocket, stringaB, stringLen, 0) != stringLen){
	        printf("%s", "Il metodo send() ha inviato un numero diverso di byte rispetto a quelli aspettati.\n");
	        system("pause");
	        // chiudiamo solo quella del client poiché il server resta in ascolto
	        closesocket(clientSocket);
	        ClearWinSock();
	        break;
	        }


		// con questa recv() il client riceverà o la stringa concatenata oppure "bye"
		memset(buf , '\0' , BUFFERSIZE);
	    if((bytesRcvd = recv(clientSocket, buf, BUFFERSIZE - 1, 0)) < 0){
	        printf("%s", "Il metodo recv() ha fallito.\n");
	        closesocket(clientSocket);
	        ClearWinSock();
			break;
	    }
	    // aggiungo fine stringa a buf
	    buf[bytesRcvd] = '\0';
		
		// dichiaro una variabile che conterrà la stringa concatenata
	    char stringaC[BUFFERSIZE];
	    strcpy(stringaC, buf);
	    // controllo se questa stringa è "bye" vuol dire che il server ha riconosciuto
	    // che stringaA o stringaB era uguale a "quit" e ci rimanda "bye"
		if(strcmp(stringaC, "bye") == 0){
            printf("%s\n",stringaC);
            // se è "bye" chiudiamo la connessione del client settando il flag = 1
			chiusura_connessione = 1;
		}
		// altrimenti stampiamo la stringa concatenata
		else{
            printf("Stringa concatenata: %s\n", stringaC);
		}

	} // end while

    closesocket(clientSocket);
    ClearWinSock();
    printf("\n");
    #ifdef WIN32
        system("pause");
    #endif // defined
    return 0;
} // end main()

