#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include "lib.h"

/* Global variables for start and end of cache */
httpNode *startOfCache;
httpNode *endOfCache;

/**
* Creates a new node in the cache.It's added
* at the end of the list.
*/
void createNode(char *url) {

  httpNode *newNode = (httpNode *) malloc(1 * sizeof(httpNode));
  memset(newNode->command, 0, MAX_LEN);
  memset(newNode->httpPage, 0, MEGA_SIZE);
  newNode->next = NULL;
  strcpy(newNode->command, url);

  if (endOfCache != NULL) {
    endOfCache->next = newNode;
    /* node added at the end of the list*/
  } else {
    /* endOfCache null => no node in cache the new node is the startOfCache */
    startOfCache = newNode;
  }
  endOfCache = newNode;
  /* update the new endOfCache */
}




int main(int argc, char *argv[]) {
  int sockTCP;
  int sockHTTP;
  int portNumber;
  int maxNOfConnections;

  int i;

  socklen_t client;

  struct sockaddr_in tcpStruct;
  struct sockaddr_in clientAddr;
  struct sockaddr_in httpStruct;

  char myBuffer[MAX_LEN];

  fd_set read_fds;
  fd_set tmp_fds;


  if (argc < 2) {
    fprintf(stderr, "Invalid input.Usage : %s port\n", argv[0]);
    exit(1);
  }

  sockTCP = socket(AF_INET, SOCK_STREAM, 0);

  if (sockTCP < 0) {
    error("ERROR opening socket");
  }

  memset((char *) &tcpStruct, 0, sizeof(tcpStruct));
  tcpStruct.sin_family = AF_INET;
  tcpStruct.sin_addr.s_addr = INADDR_ANY;
  tcpStruct.sin_port = htons(atoi(argv[1]));

  if (bind(sockTCP, (struct sockaddr *) &tcpStruct, sizeof(struct sockaddr)) < 0) {
    error("ERROR on binding");
  }

  listen(sockTCP, MAX_CLIENTS);
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  FD_SET(0, &read_fds);
  FD_SET(sockTCP, &read_fds);
  maxNOfConnections = MAX_CLIENTS;;


  while (TRUE) {
    tmp_fds = read_fds;

    if (select(maxNOfConnections + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
      error("ERROR in select");
    }

    for (i = 0; i <= maxNOfConnections; i++) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == 0) {

          memset(&myBuffer, 0, MAX_LEN);;

          scanf("%s", myBuffer);

          if (!strcmp(myBuffer, QUIT)) {
            printf("Serverul se inchide!\n");
            close(sockTCP);
            exit(0);
          }
        } else if (i == sockTCP) {
          /* New Connection */
          client = sizeof(clientAddr);
          int newSockfd;

          if ((newSockfd = accept(sockTCP, (struct sockaddr *) &clientAddr, &client)) == -1) {
            error("ERROR in accept!");
          } else {

            FD_SET(newSockfd, &read_fds);

            if (newSockfd > maxNOfConnections) {
              maxNOfConnections = newSockfd;
            }
          }

          printf("New client with IP: %s,port number %d, having the socket socket_client %d\n",
          inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), newSockfd);
        } else {

          memset(myBuffer, 0, MAX_LEN);

          if (recv(i, myBuffer, sizeof(myBuffer), 0) <= 0) {

            printf("Problems on receiving!\n");
            close(i);
            FD_CLR(i, &read_fds);

          } else {

            printf("Client %i wants: %s\n", i, myBuffer);

            if ((strncmp(myBuffer, GET_COMMAND, 4) == 0) || (strncmp(myBuffer, POST_COMMAND, 5) == 0)) {
              /* valid commands */
              if (strstr(myBuffer, FIREFOX_SUCCESS) == NULL) {
                /* If we need to confirm something we skip that part */
                int httpPort;
                int nOfBytes = 0;

                char *myBufferCopy = (char *) malloc(strlen(myBuffer) + 1 * sizeof(char));
                strcpy(myBufferCopy, myBuffer);

                char *hostName = parseURL(myBufferCopy, &httpPort);

                if (hostName == NULL) {
                  /* invalid url, no host name*/
                  send(i, BADPAGE, strlen(BADPAGE), 0);
                } else {
                  /* valid url */

                  int found = 0;
                  httpNode *currentNode = startOfCache;

                  /* searching command in cache */
                  while (currentNode != NULL && !found) {
                    if (strcmp(myBuffer, currentNode->command) == 0) {

                      /* command found */
                      found = 1;
                      break;
                      
                    } else {
                      currentNode = currentNode->next;
                    }
                  }


                  /* Verifying if we need to cache it or we have it */
                  if (found) {

                    int currentIndexInBuf = 0;
                    int totalLen = strlen(currentNode->httpPage);
                    while (currentIndexInBuf < totalLen) {
                      /* sending the httpPage from the cache */
                      memset(&myBuffer, 0, MAX_LEN);
                      strncpy(myBuffer,currentNode->httpPage + currentIndexInBuf, MAX_LEN - 1);
                      currentIndexInBuf = currentIndexInBuf + MAX_LEN - 1;

                      send(i, myBuffer, strlen(myBuffer), 0);
                    }

                    /*closing socket and clear it from the set */
                    close(i);
                    FD_CLR(i, &read_fds);

                  } else {
                    /* We need to cache it */
                    printf("Client needs %s\n", hostName);

                    if ((sockHTTP = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                      error("HTTP Socket Error!");
                    }

                    memset(&httpStruct, 0, sizeof(httpStruct));

                    /* resolve hostname */
                    struct hostent *he;
                    if ((he = gethostbyname(hostName)) == NULL) {
                      error("Bad Hostname!");
                      /* error */
                    }

                    /* copy the network address obtained from the parsed url to sockaddr_in structure */
                    memcpy(&httpStruct.sin_addr, he->h_addr_list[0], he->h_length);
                    httpStruct.sin_family = AF_INET;
                    httpStruct.sin_port = htons(httpPort);

                    /*  connecting to server */
                    if (connect(sockHTTP, (struct sockaddr *) &httpStruct, sizeof(httpStruct)) < 0) {
                      error("Can't connect to HTTP");
                    }
                    printf("\n");

                    /* creating entry in cache */
                    createNode(myBuffer);

                    /* sending the command to the http server */
                    send(sockHTTP, myBuffer, strlen(myBuffer), 0);

                    memset(&myBuffer, 0, MAX_LEN);

                    while (TRUE) {

                      /* receiving data from the http server */

                      nOfBytes = recv(sockHTTP, myBuffer, MAX_LEN - 1, 0);

                      if (nOfBytes == 0) {
                        printf("The socket is out!\n");
                        close(sockHTTP);
                        break;
                      } else if (nOfBytes > 0) {
                        /* updating cache */
                        strcat(endOfCache->httpPage, myBuffer);
                        send(i, myBuffer, nOfBytes, 0);
                        memset(&myBuffer, 0, MAX_LEN);;
                      } else {
                        error("Can't receive!");
                      }

                    }
                    /*closing socket and clear it from the set */
                    close(i);
                    FD_CLR(i, &read_fds);
                  }
                }
              } else {
                /*closing socket and clear it from the set */
                close(i);
                FD_CLR(i, &read_fds);
              }
            } else {
              /* error */
              send(i, BADPAGE, strlen(BADPAGE), 0);
            }
          }
        }
      }
    }
  }


  close(sockTCP);
  return 0;
}
