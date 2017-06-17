#ifndef LIB
#define LIB

#define MAX_CLIENTS     25
#define MAX_LEN         500
#define HTTP_PORT       80
#define TRUE            1
#define FALSE           0
#define MEGA_SIZE       10240
#define BADPAGE         "400 Bad page"
#define QUIT            "QUIT"
#define GET_COMMAND     "GET "
#define POST_COMMAND    "POST "
#define FIREFOX_SUCCESS "GET http://detectportal.firefox.com/success.txt"


/**
* Structure that defines the node of a linked listen
* representing an entry in the cache
*/

typedef struct node {
  char httpPage[MEGA_SIZE];
  char command[MAX_LEN];
  struct node *next;
} httpNode;


/**
 * Parses the url and returns the parsed host.
 * Parameters the url to be parsed and a pointer
 * to an integer for the port number.
 * Returns the the name of the host.
 */
char *parseURL(char *urlToParse, int *port) {

  int startH = -1; /* start index of the hostname */
  int endH = -1;   /* end index of the hostname */
  int i;
  int endPort = -1; /* end index of the port number */

  int len = strlen(urlToParse);

  for (i = 0; i < (len - 1); i++) {
    if ((startH < 0) && (urlToParse[i] == '/' && urlToParse[i + 1] == '/')) {
      startH = i + 2; /* url contains // */
    }

    if ((startH >= 0)  && (i > startH) && (endH < 0)) {
      endH = (urlToParse[i] == ':') ? (i + 1) : endH;

      if (urlToParse[i] == '/') {
        endH = (urlToParse[i] == ':') ? (i + 2) : (i + 1);
      }
    }

    if ((endPort < 0) && ((endH > 0) && (urlToParse[i] == ' '))) {
      endPort = i - 1;
    }
  }


  if (startH == endH && endH == endPort) {
    /* invalid url */
    return NULL;

  } else if ((endPort > 0) && (endPort > endH)) {
    /* port specified */
    int lenPort = endPort - endH + 1;
    char *charPort = (char *) calloc(lenPort, sizeof(char));
    strncpy(charPort, urlToParse + endH, lenPort - 1);
    *port = atoi(charPort);

    if (*port == 0) {
      /* invalid port */
      *port = HTTP_PORT;
    }
  } else {
    /* unspecified port */
    *port = HTTP_PORT;
  }

  printf("The port is: %i\n", *port);
  /* getting host name */
  int lenHost = endH - startH + 1;
  char *parsedURL = (char *) calloc(lenHost, sizeof(char));
  strncpy(parsedURL, urlToParse + startH, lenHost - 2);

  char endURL = parsedURL[strlen(parsedURL) - 1];
  parsedURL[strlen(parsedURL) - 1] = (endURL == '\\') ? '\0' : endURL;
  return parsedURL;
}


void error(char *msg) {
  perror(msg);
  exit(1);
}


#endif
