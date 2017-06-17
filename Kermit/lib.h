#ifndef LIB
#define LIB

/*Campuri noi adaugate*/
/*******************************************************************************************************************************************/
#define SEND_INIT	'S'
#define FILEH		'F'
#define Data		'D'
#define EOFP		'Z'
#define EOT		'B'
#define ACK		'Y'
#define NACK		'N'
#define ERROR		'E'

#define MAXL_N		250
#define MAXL_BYTE	0xFA
#define TIME_BYTE	0x05
#define TIME_MS		5000
#define NUL		0x00
#define CR		0x0D
#define SOH_B		0x01

#define MAX_PKJsN	100

/*
 * MINI-KERMIT package  
 */
typedef struct {
	unsigned char SOH ;
	unsigned char LEN;
	unsigned char SEQ;
	unsigned char TYPE;
	char DATA[MAXL_N];
	short CHECK;
	unsigned char MARK;

} mini_kermit;

/**
 * Send-Init package
 */
typedef struct {
	unsigned char MAXL;
	unsigned char TIME;
	unsigned char NPAD;
	unsigned char PADC;
	unsigned char EOL;
	unsigned char QCTL;
	unsigned char QBIN;
	unsigned char CHKT;
	unsigned char REPT;
	unsigned char CAPA;
	unsigned char R;
} s_pckg;

/**
 * Represents the packages of a file
 * MAX_PKJsN maximum number of packages
 * nOfPckgs the actual capacity
 */
typedef struct {
	mini_kermit pckgs[MAX_PKJsN];
	int nOfPckgs;
	int nOfT[MAX_PKJsN];
}filePckgs;

typedef int bool;
#define true 1
#define false 0


/*******************************************************************************************************************************************/
typedef struct {
    int len;
    char payload[1400];
} msg;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif

