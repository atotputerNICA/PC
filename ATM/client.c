#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdbool.h>
#include <unistd.h>

#define BUFLEN 256

//------------------------------------------------------------
#define ATMWELCOME  "ATM> Welcome \0"
#define LOGIN		"login\0"
#define LOGOUT		"logout\n"
#define LISTSOLD	"listsold\n"
#define GETMONEY	"getmoney\n"
#define QUIT		"quit\n"
#define UNLOCK		"unlock\n"
#define UNLOCK1		"unlock \0"
#define ERROR2		"-2 : Sesiune deja deschisa\0\n"
#define ERROR1		"-1 : Clientul nu este autentificat\0\n"
#define ERROR5		"ATM> -5 : Card blocat\0\n"
#define WRONGPASS	"UNLOCK> -7 : Deblocare esuata\0\n"
#define UNLOCKED	"UNLOCK> Client deblocat\0\n"
#define ASKPASS		"UNLOCK> Trimite parola secreta\0\n"
//------------------------------------------------------------


void error(char *msg) {
	perror(msg);
	exit(0);
}
/**
 * Verifica daca o comada incepe cu mesajul Welcome
 * 
 */
bool startsWithWelcome(char* command) {
	return (strncmp(ATMWELCOME,command,12) == 0) ? true : false;
}

/**
 * Verifica daca o comanda incepe cu mesajul login
 *
 */
bool startsWithLogin(char* command) {
	return (strncmp(LOGIN,command,5) == 0) ? true : false;
}

/**
 * 
 * Concateneaza 2 stringuri
 */
char* concat(char *s1, char *s2) {
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
/**
 * Obtine numarul cardului dintr-o comanda
 * de login
 */
int getCardN(char *loginCommand){
	char buf[7];
	int i;
  int j = 0;
	for (i = 6; i <= 11; i++) {
		buf[j] = loginCommand[i];
		j++;
	}
	buf[j] = '\0';
	return atoi(buf);

}

int main(int argc, char *argv[]) {

	bool blocked = false;
    bool logged = false;
	int lastBNC = 0;
	bool wrongUdpPass = false;
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFLEN];
    char buf2[BUFLEN];
	int pid = getpid();
	char* filename;
	char pidtochar[30];
	sprintf(pidtochar,"%d",pid);
	filename = concat("client-\0",pidtochar);
	filename = concat(filename,".log\0");
	FILE *fp = fopen(filename, "wb");
	//printf("%s\n\n",filename);
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
   //----------------- UDP-------------------------------------------------

    struct sockaddr_in to_station,recvstr;
	socklen_t adr_len = 0;
    /*Deschidere socket1*/
    int sockfdUDP = socket(PF_INET, SOCK_DGRAM, 0);

    if(sockfdUDP < 0)
		printf("can't open socket \n");

	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1],&(to_station.sin_addr.s_addr));
    
	//----------------- UDP------------------------------------------------
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    
    
    fd_set read_fs, temp_fs;
    FD_SET(sockfd, &read_fs);
    FD_SET(0, &read_fs);
	FD_SET(sockfdUDP, &read_fs);

    int fdmax = (sockfd > sockfdUDP) ? sockfd : sockfdUDP;
    char buf3[10];

    while(1){

		temp_fs = read_fs;
        select(fdmax + 1, &temp_fs, NULL, NULL, NULL);
    	memset(buffer, 0 , BUFLEN);

		for (int i = 0 ; i <= fdmax; i++) {
			if (FD_ISSET(i, &temp_fs)) {

				if ( i == 0) {

					// socketul pentru standard input
					//citesc mesaj de la tastatura
					fgets(buffer, BUFLEN - 1, stdin);
					
					fprintf(fp,"%s",buffer);
					
						
					if (startsWithLogin(buffer) && logged == true) {
						printf("%s\n\n",ERROR2);
						fprintf(fp,"%s\n",ERROR2);
					} else {
						if (strcmp(buffer,LOGOUT) == 0) {
							if (logged == false) {
								if (!blocked) {
									printf("%s\n\n",ERROR1); // client neautentificat
									fprintf(fp,"%s\n",ERROR1);
								} else {
									n = send(sockfd,buffer,strlen(buffer), 0);
								}
							} else {
								n = send(sockfd,buffer,strlen(buffer), 0);
								logged = false;
							}
						} else if (!(startsWithLogin(buffer) || (strcmp(buffer,QUIT) == 0) || (strcmp(buffer,UNLOCK)==0))) {

							if (wrongUdpPass == true) {
									//parola secreta ppentru deblocare card
									n = sendto(sockfdUDP, buffer,strlen(buffer) + 1 , 0,  (struct sockaddr *)&to_station,
									    sizeof(struct sockaddr));
							} else if (logged == false) {
								if (blocked == false) {
									if ( strcmp(buffer,"\n") != 0) {
										printf("%s\n\n",ERROR1); //client neautentificat
										fprintf(fp,"%s\n",ERROR1);
									}
								} else 
									n = send(sockfd,buffer,strlen(buffer), 0); // card blocat
							} else {
								n = send(sockfd,buffer,strlen(buffer), 0);
							}

						} else {
							// comanda de unlock trimisa pe socketul udp
							if (strcmp(buffer,UNLOCK) == 0) {
								/*if (logged == false) {
									printf("%s\n\n",ERROR1); // client neautentificat
									fprintf(fp,"%s\n",ERROR1);
								} else {*/
								char cardN[20];
								sprintf(cardN,"%d",lastBNC);
								char *msg;
								msg = concat(UNLOCK1,cardN);
								msg = concat(msg,"\0");
								n = sendto(sockfdUDP, msg,strlen(msg) + 1 , 0,  (struct sockaddr *)&to_station, 
									sizeof(struct sockaddr));
							} else {

								if (startsWithLogin(buffer)) {
									if (!blocked) {
										lastBNC = getCardN(buffer);							
									}
								}
								
								if (strcmp(buffer, QUIT) == 0) {
									//close(sockfd);
									fclose(fp);
									exit(0);
								}
								if ( strcmp(buffer,"\n") != 0) {
									n = send(sockfd,buffer,strlen(buffer), 0);
									if (n < 0) 
										error("ERROR writing to socket");
								}
							}
						}
					}
				} else if ( i == sockfd) {
					// primeste mesaj de pe serverul tcp
					memset(buffer, 0 , BUFLEN);
					int n =recv(sockfd, buf2, BUFLEN, 0);
					printf("%s\n\n",buf2);
					fprintf(fp,"%s\n",buf2);
					if (strcmp(buf2, QUIT) == 0) {
						//close(sockfd);
						fclose(fp);
						exit(0);
					}
					if (startsWithWelcome(buf2)) {
						logged = true; // client autentificat
					}
					if (strcmp(buf2,ERROR5) == 0) {
						blocked = true;	// card blocat
					}
				
				} else if (i == sockfdUDP) {
					// primeste mesaj de pe socketul udp
					recvfrom(sockfdUDP,buffer,BUFLEN,0,  (struct sockaddr *)&to_station, &adr_len);
					printf("%s\n\n",buffer);
					fprintf(fp,"%s\n",buffer);
					if (strcmp(buffer,ASKPASS) == 0) {
						wrongUdpPass = true;
					} else if (strcmp(buffer,UNLOCKED) == 0) {
						wrongUdpPass = false;
						blocked = false;
					}
				}

			}
		}

    }
    return 0;
}


