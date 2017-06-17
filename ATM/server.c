#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_CLIENTS			40
#define BUFLEN 				256
#define LOGIN				"login\0"
#define LOGOUT				"logout\0"
#define LISTSOLD			"listsold\0"
#define GETMONEY			"getmoney\0"
#define PUTMONEY			"putmoney\0"
#define UNLOCK				"unlock\0"
#define UNLOCKT				"UNLOCK> \0"
#define ATM					"ATM> \0"
#define ATMWELCOME 			"ATM> Welcome \0"
#define ATMBYE	   			"ATM> Deconectare de la bancomat!\0\n"
#define ERROR1				"-1 : Clientul nu este autentificat\0\n"
#define ERROR2				"-2 : Sesiune deja deschisa\0\n"
#define ERROR3				"-3 : Pin gresit\0\n"
#define ERROR4				"-4 : Numar card inexistent\0\n"
#define ERROR5				"-5 : Card blocat\0\n"
#define ERROR6				"-6 : Operatie esuata\0\n"
#define ERROR7				"-7 : Deblocare esuata\0\n"
#define ERROR8				"-8 : Fonduri insuficiente\0\n"
#define ERROR9				"-9 : Suma nu e multiplu de 10\0\n"
#define ERROR10				"-10 : Eroare la apel nume-functie\0\n"
#define ASKPASS				"UNLOCK> Trimite parola secreta\0\n"
#define WRONGPASS			"UNLOCK> -7 : Deblocare esuata\0\n"
#define UNLOCKED			"UNLOCK> Client deblocat\0\n"



/**
 * Structura ce reprezinta o intrare din fisierul de input
 *
 */
typedef struct {
	unsigned char nume[12];
	unsigned char prenume[12];
	int numar_card;
	int pin;
	unsigned char parola_secreta[16];
	float sold;
	int nOfWrongPin;
	bool blocked;
	bool logged;
	
} Person;

void error(char *msg) {
    perror(msg);
    exit(1);
}

/**
 *	Parseaza informatiile din fisierul de intrare si intoarce 
 *  un array de structuri de tipul Person,numarul de intrari
 *  este salvat la adresa lui n
 */
Person* readInput(char* filename, int* n) {
	Person* array;
	int nOfP;
	int i;

	FILE* fid = fopen(filename,"rw+");
	if (fid < 0) {
		error("Error opening the file");
	}
	fscanf(fid,"%d",&nOfP);
	//printf("n=%d\n",nOfP);
	*n = nOfP;
	array = malloc((nOfP + 1)*sizeof(Person));
	for (i = 0; i < nOfP; i++) {
		fscanf(fid,"%s %s %d %d %s %f",array[i].nume,array[i].prenume,&array[i].numar_card,&array[i].pin,
			array[i].parola_secreta,&array[i].sold);
		array[i].sold = round(array[i].sold * 100) / 100;
		array[i].nOfWrongPin = 0;
		array[i].logged = false;
		array[i].blocked = false;
	}
	fclose(fid);
	return array;
}
/**
 * Pentru debugging
 *
 */
void printData(Person* array,int n) {
	for (int i = 0; i < n; i++) {
		printf("%s %s %d %d %s %.2f\n",array[i].nume,array[i].prenume,array[i].numar_card,array[i].pin,
			array[i].parola_secreta,array[i].sold);
	}
}

/**
 * Obtine indexul dintr-un array de Person in functie de 
 * numarul cardului
 */
int getCardNumberIndex(Person* array, int card_number, int n) {
	for(int i = 0 ; i < n; i++) {
		if (array[i].numar_card == card_number)
			return i;
	}
	return -1; //there is no card with the given card number
}

bool wrongPin(Person* array, int card_number,int pin,int n) {
	bool wrong = false;
	int index = getCardNumberIndex(array,card_number,n);
	if (array[index].pin != pin)
		wrong = true;
	return wrong;

}
bool startsWithLogin(char* command) {
	return (strncmp(LOGIN,command,5) == 0) ? true : false;
}
bool startsWithLogout(char* command) {
	return (strncmp(LOGOUT,command,6) == 0) ? true : false;
}
bool startsWithUnlock(char* command) {
	return (strncmp(UNLOCK,command,6) == 0) ? true : false;
}

/**
 * Intoarce un array de dimensiunea n, initializat la -1
 *
 */
int* initializeSockfd(int n) {
	int* array = malloc(n * sizeof(int)) ;
	for (int i = 0; i < n; i++) {
		array[i] = -1;
	}
	return array;
}

/**
 * imparte un string in tokenuri care sunt salvate
 * la adresa unui char** si intoarce numarul de
 * tokens
 * Sursa : stackoverflow.com
 */
int split (char *str, char c, char ***arr) {
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0') {
		if (*p == c)
		  count++;
		p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
		exit(1);

    p = str;
    while (*p != '\0') {
		if (*p == c) {
			(*arr)[i] = (char*) malloc( sizeof(char) * token_len );
			if ((*arr)[i] == NULL)
				  exit(1);

			token_len = 0;
			i++;
		}
		p++;
		token_len++;
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*arr)[i] == NULL)
			exit(1);

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0') {
		if (*p != c && *(p+1) != '\0') {
			*t = *p;
			t++;
		}
		else {
			if (*(p+1) == '\0') {
				*t = *p;
				t++;
			}
			*t = '\0';
			i++;
			t = ((*arr)[i]);
		}
		p++;
    }

    return count;
}



void printArray(int* array, int n) {
	for (int i = 0; i < n; i++) {
		printf("%d\n",array[i]);
	}

}
void printCharArray(char** array, int n) {
	for (int i = 0; i < n; i++) {
		printf("%s\n",array[i]);
	}

}

/**
 * conversia char* - int
 * intoarce 0 daca stringul contine caracte ce nu sunt cifre
 */
int myatoi(char *str) {

    int res =0;
    int i;

    for (i = 0; str[i]!= '\0';i++) {
			if (!isdigit(str[i])) {
				return 0;
			}
      res = res*10 + str[i] - '0';
    }
    return res;
}

/**
 *
 * Concateneaza 2 stringuri
 */
char* concat(char *s1, char *s2) {
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
int main(int argc, char *argv[]) {
     int nOfClients;
     int *sockfdClients;
	 char *err;
	 int *activeSockfd = initializeSockfd(100);
	 int contorSockets = 0;
     Person* data = readInput(argv[2],&nOfClients);
     sockfdClients = initializeSockfd(nOfClients);
     printData(data,nOfClients);
     int sockfd, newsockfd, portno, clilen;
     char buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     int n, i, j;

     fd_set read_fds;	//multimea de citire folosita in select()
     fd_set tmp_fds;	//multime folosita temporar 
     int fdmax;			//valoare maxima file descriptor din multimea read_fds

     if (argc < 2) {
         fprintf(stderr,"Usage : %s port\n", argv[0]);
         exit(1);
     }

	//-------------------- UDP ---------------------------------------------------
	 struct sockaddr_in my_sockaddr,from_station ;
  	 socklen_t adr_len = sizeof(my_sockaddr);
	 int sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
	 if (sockfdUDP < 0) 
     	error("ERROR opening socket");
	 my_sockaddr.sin_family = AF_INET;
  	 my_sockaddr.sin_port = htons(atoi(argv[1]));
  	 my_sockaddr.sin_addr.s_addr = INADDR_ANY;
	 bind(sockfdUDP, (struct sockaddr *) &my_sockaddr, sizeof(my_sockaddr));
    //-------------------- UDP ---------------------------------------------------

     //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     
     portno = atoi(argv[1]);

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd, MAX_CLIENTS);

     //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
     FD_SET(sockfd, &read_fds);
     FD_SET(STDIN_FILENO, &read_fds); //socketul pentru standard input (citire de la tastatura)S
     FD_SET(sockfdUDP, &read_fds); //socketul UDP
     fdmax = (sockfd > sockfdUDP) ? sockfd : sockfdUDP;
	 int index = -1;
     char buf3[10];
     	// main loop
		while (1) {

			tmp_fds = read_fds; 
			if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
				error("ERROR in select");
			
			for (i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &tmp_fds)) {

					if (i == STDIN_FILENO) {
						
						//QUIT	din server
						fgets(buffer, BUFLEN, stdin);
						sprintf(buf3,"quit");
						buf3[4]='\n';
						buf3[5]='\0';
						if (strcmp(buffer, buf3) == 0) {
							for( j = 0; j < contorSockets; j++) {
								// trimite tutoror clietilor conectati mesajul "quit" dupa care se inchide
								if (activeSockfd[j] != -1) {
									send(activeSockfd[j],buf3,strlen(buf3)+1,0);
								}	
							}
							exit(0);
						}
					} else if (i == sockfdUDP) {
						// se primeste mesaj pe socketudp
						int r = recvfrom(sockfdUDP, buffer, BUFLEN, 0,(struct sockaddr *) &from_station, &adr_len) ;
						if ( r > 0) {
							char ** unlockC ;
							int utokens = split(buffer,' ',&unlockC); 
							int ind;

							//UNLOCK COMMAND
							if (startsWithUnlock(buffer)) {
								int cardnumber = myatoi(unlockC[1]);
								ind = getCardNumberIndex(data,cardnumber,nOfClients);
								if (data[ind].logged == true) {
									err = concat(UNLOCKT,ERROR6);
									int nu = sendto(sockfdUDP,err,strlen(err) + 1 , 0,(struct sockaddr *)&from_station, 
											 sizeof(struct sockaddr));
								} else {
									sendto(sockfdUDP, ASKPASS,strlen(ASKPASS) + 1 , 0, 
										  (struct sockaddr *)&from_station, sizeof(struct sockaddr));
								}								
							} else {
								buffer[strlen(buffer) - 1] = '\0';
								if (strcmp(buffer, data[ind].parola_secreta) == 0) {
									data[ind].blocked = false;
									data[ind].nOfWrongPin = 0;
									sendto(sockfdUDP, UNLOCKED,strlen(UNLOCKED) + 1 , 0, 
										  (struct sockaddr *)&from_station, sizeof(struct sockaddr));
								} else {
									sendto(sockfdUDP, WRONGPASS,strlen(WRONGPASS) + 1 , 0, 
										  (struct sockaddr *)&from_station, sizeof(struct sockaddr));
								}
							}
						}
						
						printf("Am primit de la clientul de pe socketul UDP %d, mesajul: %s\n", i, buffer);

					} else if (i == sockfd) {

						// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
						// actiunea serverului: accept()
						clilen = sizeof(cli_addr);

						if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
							error("ERROR in accept");
						} 
						else {
							//adaug noul socket intors de accept() la multimea descriptorilor de citire
							FD_SET(newsockfd, &read_fds);
							if (newsockfd > fdmax) { 
								fdmax = newsockfd;
							}
						}
						printf("Noua conexiune de la %s, port %d, socket_client %d\n", inet_ntoa(cli_addr.sin_addr),
							   ntohs(cli_addr.sin_port), newsockfd);
						activeSockfd[contorSockets] = newsockfd; // adaug socketul in arrayul cu socketi activi
						contorSockets +=1;
					
					} else {
						// am primit date pe unul din socketii cu care vorbesc cu clientii
						//actiunea serverului: recv()
						memset(buffer, 0, BUFLEN);
						if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
							if (n == 0) {
								//conexiunea s-a inchis
								printf("selectserver: socket %d hung up\n", i);
							} else {
								error("ERROR in recv");
							}
							close(i); 
							FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
						} 
					
						else { //recv intoarce >0
							char ** command = NULL;
							int tokens = 0;
							printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
							sprintf(buf3,"quit");
							buf3[4]='\n';
							if (strcmp(buffer, buf3) == 0) {
								printf("selectserver: socket %d hung up\n", i);
								FD_CLR(i, &read_fds);
							}
							
							buffer[strlen(buffer)-1] = '\0';
							tokens = split(buffer,' ',&command);
							char* welcome;
							

							//LOGIN
							if (startsWithLogin(buffer)) {
								if(tokens == 3) {
									if(index != -1 && data[index].blocked == true) {
										err = concat(ATM,ERROR5);
										send(i,err,strlen(err)+1,0);
									} else {
										int cardn = myatoi(command[1]);
										if (cardn == 0) {
											// ERROR4 - numar card contine caractere alfanumerice
											err = concat(ATM,ERROR4);
											send(i,err,strlen(err)+1,0);
										} else {
											index = getCardNumberIndex(data,cardn,nOfClients);
											if (index == -1) {
												//ERROR4 - numar card nu exista
												err = concat(ATM,ERROR4);
												send(i,err,strlen(err)+1,0);
											} else {
												int pin = myatoi(command[2]);
												if (data[index].logged == true) {
													// sesiune deja deschisa pentru numarul de card
													err = concat(ATM,ERROR2);
													send(i,err,strlen(err)+1,0);
												} else if ( pin != data[index].pin) {
													// pin gresit
													err = concat(ATM,ERROR3);
													if (data[index].nOfWrongPin < 2) {
														send(i,err,strlen(err)+1,0);
														data[index].nOfWrongPin += 1;
													} else if (data[index].nOfWrongPin == 2) {
														//card blocat

														//err = concat(ATM,ERROR3);
														//send(i,err,strlen(err)+1,0);
														err = concat(ATM,ERROR5);
														send(i,err,strlen(err)+1,0);
														data[index].blocked = true;

													} else if (data[index].blocked == true) {
														// card deja blocat
														err = concat(ATM,ERROR5);
														send(i,err,strlen(err)+1,0);
													}
												} else {
													if (data[index].logged == false) {
														// pin corect si client nelogat 
														if ( data[index].blocked != true) {
															// cardul nu este blocat
															data[index].logged = true;
															data[index].nOfWrongPin = 0;
															sockfdClients[index] = i;
															welcome = concat(ATMWELCOME,data[index].nume);
															welcome = concat(welcome," \0");
															welcome = concat(welcome,data[index].prenume);
															send(i,welcome,strlen(welcome)+1,0);
														} else {
															// card blocat
															err = concat(ATM,ERROR5);
															send(i,err,strlen(err)+1,0);
														}
													
													} else {
														// pin corect dar clientul este deja logat
														err = concat(ATM,ERROR2);
														send(i,err,strlen(err)+1,0);
													}
												
												}
											}
										}
									}
								} else {
									err = concat(ATM,ERROR10);
									send(i,err,strlen(err)+1,0);
								}
							} else if (data[index].blocked == false) {

							  // LOGOUT
								if (strcmp(command[0],LOGOUT) == 0) {
									data[index].logged = false;
									sockfdClients[index] = -1;
									send(i,ATMBYE,strlen(ATMBYE) + 1,0);
								}
							
								//LISTSOLD
								if (strcmp(command[0],LISTSOLD) == 0) {
									char soldf[20] ;
									sprintf(soldf,"%.02f",data[index].sold);
									char *sold = concat(ATM,soldf);
									send(i,sold,strlen(sold) + 1,0);
								}


								//GETMONEY
								if (strcmp(command[0],GETMONEY) == 0) {	
									int sum = myatoi(command[1]);
									if ( sum != 0) {
										if (sum % 10 == 0) {
											if (data[index].sold - ((double) sum) < 0) {
												err = concat(ATM,ERROR8);
												send(i,err,strlen(err) + 1,0);
											} else {
												char charsum[100];
												sprintf(charsum,"%d",sum);
												data[index].sold -= (double) sum;
												char *msg = concat(ATM,"Suma \0");
												msg = concat (msg,charsum);
												msg = concat (msg," retrasa cu succes\0");
												send(i,msg,strlen(msg) + 1,0);
											
											}
										} else {
											err = concat(ATM,ERROR9);
											send(i,err,strlen(err) + 1,0);
										}
									} else {
										err = concat(ATM,ERROR10);
										send(i,err,strlen(err) + 1,0);
									}
								}
							

								//PUTMONEY
								if (strcmp(command[0],PUTMONEY) == 0) {
									double sumdep = atof(command[1]);
									data[index].sold += sumdep;
									char charsum[100];
									sprintf(charsum,"%.02f",sumdep);
									char *msg = concat(ATM,"Suma \0");
									msg = concat (msg,charsum);
									msg = concat (msg," depusa cu succes\0");
									send(i,msg,strlen(msg) + 1,0);
								

								}
							} else {
								// card blocat
								err = concat(ATM,ERROR5);
								send(i,err,strlen(err)+1,0);
							}
			
						}
					} 
				}
			}
		   }


		   close(sockfd);
		 
		   return 0; 
	}


