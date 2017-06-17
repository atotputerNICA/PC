#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"
//#include "package.h"
#define HOST "127.0.0.1"
#define PORT 10001

/*
 * Returns the new name for the input
 * file
 *
 */
char* nameOut(char* filein){
	char* buff = malloc((strlen(filein)+6)*sizeof(char));
	//buff[(strlen(filein)+5)] = '\0';
	strcpy(buff,"recv_");
	strcat(buff,filein);
	return buff;
}

/*
 * Returns the integer value of the LEN field
 * of a mini-kermit payload
 *
 */
int getLength(char* kermitPayload){
	unsigned char len;
	memcpy(&len,kermitPayload+1,1);
	return (int) len;
}

/*
 * Returns the integer value of the SEQ field
 * of a mini-kermit payload
 *
 */
int getSeq(char* kermitPayload){
	unsigned char seq;
	memcpy(&seq,kermitPayload+2,1);
	return (int) seq;
}

/*
 * Returns the char value of the TYPE field
 * of a mini-kermit payload
 *
 */
unsigned char getType(char* kermitPayload) {
	unsigned char type;
	memcpy(&type,kermitPayload+3,1);
	return type;
}

/*
 * Returns the DATA field of a mini-kermit
 * payload
 *
 */
char* getData(char* kermitPayload) {
	char* buffer = malloc((getLength(kermitPayload)-5)*sizeof(char));
	memcpy(buffer,kermitPayload+4,(getLength(kermitPayload)-5));
	return buffer;
}

/*
 * Returns the short value of the CHECK field
 * of a mini-kermit payload
 *
 */
unsigned short getCRC(char* kermitPayload) {
	unsigned short crc ;
	memcpy(&crc,kermitPayload+(getLength(kermitPayload)-1),2);
	return crc;

}

/*
 * Returns the char value of the MARK field
 * of a mini-kermit payload
 *
 */
unsigned char getMARK(char* kermitPayload) {
	unsigned getMARK;
	memcpy(&getMARK,kermitPayload+(getLength(kermitPayload)+1),1);
	return getMARK;

}

/*
 * Returns a Send-Init package with the settings of
 * the ksender.
 */
s_pckg newSendInitPckg() {
	s_pckg pckg;
	pckg.MAXL  = MAXL_BYTE;
	pckg.TIME = TIME_BYTE;
	pckg.NPAD = NUL;
	pckg.PADC = NUL;
	pckg.EOL  = CR;
	pckg.QCTL = NUL;
	pckg.QBIN = NUL;
	pckg.CHKT = NUL;
	pckg.REPT = NUL;
	pckg.CAPA = NUL;
	pckg.R    = NUL;
	return pckg;
}

/*
 * Returns a mini-kermit package with the basic fields
 * set
 */
mini_kermit newMiniKermitPckg() {
	mini_kermit pckg;
	pckg.SOH = SOH_B ;
	pckg.SEQ = NUL;
	pckg.MARK = CR;
	return pckg;
}


/*
 * Returns the payload of a Send-Init package
 *
 */
char* init_pckg_to_char (s_pckg pckg) {

	char* buf = malloc(11* sizeof(char));
	memcpy(buf,&(pckg.MAXL),1);
	memcpy(buf+1,&(pckg.TIME),1);
	memcpy(buf+2,&(pckg.NPAD),1);
	memcpy(buf+3,&(pckg.PADC),1);
	memcpy(buf+4,&(pckg.EOL),1);
	memcpy(buf+5,&(pckg.QCTL),1);
	memcpy(buf+6,&(pckg.QBIN),1);
	memcpy(buf+7,&(pckg.CHKT),1);
	memcpy(buf+8,&(pckg.REPT),1);
	memcpy(buf+9,&(pckg.CAPA),1);
	memcpy(buf+10,&(pckg.R),1);
	return buf;

}

/*
 * Returns the payload of a MINI-KERMIT package
 *
 */
char* mini_kermit_to_char (mini_kermit pckg) {
	char* buf = malloc((((int)pckg.LEN)+3)*sizeof(char));
	memcpy(buf,&(pckg.SOH),1);
	memcpy(buf+1,&(pckg.LEN),1);
	memcpy(buf+2,&(pckg.SEQ),1);
	memcpy(buf+3,&(pckg.TYPE),1);
	memcpy(buf+4,pckg.DATA,((unsigned int)pckg.LEN) - 5 );
	memcpy(buf+4+((unsigned int)pckg.LEN) - 5,&(pckg.CHECK),2);
	memcpy(buf+6+((unsigned int)pckg.LEN) - 5,&(pckg.MARK),1);
	return buf;
}

/*
 * Changes the seq number of a mini_kermit pckg
 * and recalculates the crc
 */
mini_kermit newSeqMiniKermit(mini_kermit pckg,int seq) {
	pckg.SEQ = (unsigned char) seq;
	char* buf = mini_kermit_to_char(pckg);
	int len  = 2 + getLength(buf);
	unsigned short crc = crc16_ccitt(buf,len - 3);
	pckg.CHECK = crc;
	return pckg;
}

int charToInt(unsigned char c) {
	char buf[2];
	sprintf(buf,"%d",c);
	int number = atoi(buf);
	return number;

}

/*
 * Returns an ACK package. The type parameter is used
 * to determine whether or not the package for which
 * is sent the ACK is Send-Init(if so the data field
 * of the package contains the receiver's settings).
 */
mini_kermit makeACK(unsigned char type,int seq){
	mini_kermit pckg = newMiniKermitPckg();
	pckg.SEQ = (unsigned char) seq;
	int len;
	if (type == SEND_INIT) {
		s_pckg receptor_init = newSendInitPckg();
		//sprintf(pckg.DATA,"%s",init_pckg_to_char(receptor_init));
		char* init_char = init_pckg_to_char(receptor_init);
		memcpy(pckg.DATA,init_char,11);
		pckg.LEN = (char) 16;
		len = 18; // 7 + 11 (sendinit)

	} else {
		sprintf(pckg.DATA,"%s","");
		pckg.LEN  = (char) 5;
		len = 7;
	}
	pckg.TYPE = ACK;
	char* buf = mini_kermit_to_char(pckg);
	unsigned short crc = crc16_ccitt(buf,len-3);
	pckg.CHECK = crc;
	return pckg;

}

/*
 * Returns a NAK package
 *
 */
mini_kermit makeNAK(int seq){
	mini_kermit pckg = newMiniKermitPckg();
	pckg.SEQ = (unsigned char) seq;
	sprintf(pckg.DATA,"%s","");
	int len = 7;
	pckg.LEN  = (char )5;
	pckg.TYPE = NACK;
	char* buf = mini_kermit_to_char(pckg);
	unsigned short crc = crc16_ccitt(buf,len-3);
	pckg.CHECK = crc;
	return pckg;
}

/*
 * for debugging
 */
void printChar (unsigned char* kermitPayload){
	int len = 2 + getLength((char*)kermitPayload);
	for (int i =0; i < len; i++)
		printf("0x%02X ",kermitPayload[i]);
	printf("\n");

}

int main(int argc, char** argv) {
	msg t;


	init(HOST, PORT);
	int seqN = 0;
	int nOfT = 0;
	int fd;
	//int time = 5000;
	//msg* y = receive_message_timeout(time);
	bool resend = true;
	//bool notEOT = true;
	char* buff;
	while (1) {
		//printf("DA\n");
		resend = true;
		nOfT = 0;
		while (resend && (nOfT <= 3)) {
			msg *y = receive_message_timeout(5000);

			if (y == NULL) {
				//printf("Amprimit \n");
				mini_kermit nak = makeNAK(seqN+1);
				buff = mini_kermit_to_char(nak);
				int len = getLength(buff);
				memcpy(t.payload,buff,len + 2);
				t.len = len + 2;
				seqN++;
				send_message(&t);
				nOfT++;
				printf("[%s] Sending NAK : %d\n", argv[0], seqN);
				resend = true;
			} else {
				//printf("Amprimit \n");
				if(getCRC(y->payload) != crc16_ccitt(y->payload,y->len - 3)) {
					mini_kermit nak = makeNAK(seqN+1);
					buff = mini_kermit_to_char(nak);
					int len = getLength(buff);
					memcpy(t.payload,buff,len + 2);
					t.len = len + 2;
					seqN++;
					send_message(&t);
					printf("[%s] Sending NAK for corrupt crc : %d\n", argv[0], seqN);
					resend = false;
				} else {
					seqN = getSeq(y->payload) + 1;
					printf("[%s] Sending ACK reply with seq number : [%d]\n", argv[0], getSeq(y->payload)+1);
					mini_kermit ack;
					if(getType(y->payload) == SEND_INIT) {
						ack = makeACK(SEND_INIT,seqN);
					} else {
						ack = makeACK(ACK,seqN);
					}
					buff = mini_kermit_to_char(ack);
					//printf("%c\n",getType(buff));
					int len = getLength(buff);
					memcpy(t.payload,buff,len + 2);
					t.len = len + 2;
					send_message(&t);
					resend = false;
					if(getType(y->payload) == FILEH){
						fd = open(nameOut(getData(y->payload)),O_RDWR | O_CREAT,S_IRWXU);
						//printf("%s %s \n",nameOut(getData(t.payload)),getData(t.payload));
					}
					if(getType(y->payload) == EOT) {
						printf("[%s] Transmission complete.\n", argv[0]);
						exit(0);
					}
					if(getType(y->payload) == EOFP){
						printf("[%s] Transmission of file complete.Starting next file if any.\n", argv[0]);
						close(fd);
					}
					if(getType(y->payload) == Data){
						write(fd,getData(y->payload),getLength(y->payload)-5);
						//printf("%s\n",y->payload);
						printf("[%s] Writing in file.\n", argv[0]);
					}
				}
			}
			if (nOfT == 3) {
				printf("[%s]Received error ending transmission\n",argv[0]);
				//break;
				exit(0);
			}
		}
	}



	return 0;
}
