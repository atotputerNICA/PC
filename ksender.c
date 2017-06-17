#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

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
 * Changes the seq number of a MINI-KERMIT pckg
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
 * Returns a File Header package for the file given
 * as parameter
 */
mini_kermit makeFileHeader(char* fileName,int seq) {
	mini_kermit pckg = newMiniKermitPckg();
	pckg.SEQ = (unsigned char) seq;
	//sprintf(pckg.DATA,"%s",fileName);
	memcpy(pckg.DATA,fileName,strlen(fileName));
	pckg.DATA[strlen(fileName)]='\0';
	int len = 7 + strlen(fileName);
	pckg.LEN = (char)(5 + strlen(fileName));
	pckg.TYPE = FILEH;
	char* buf = mini_kermit_to_char(pckg);
	unsigned short crc = crc16_ccitt(buf,len - 3);
	pckg.CHECK = crc;
	return pckg;
}

/*
 * Returns a Send-Init package 
 *
 */
mini_kermit makeSendInit(int seq) {
	mini_kermit pckg = newMiniKermitPckg();
	pckg.SEQ = (unsigned char) seq;
	s_pckg init = newSendInitPckg();
	char* init_char = init_pckg_to_char(init);
	//sprintf(pckg.DATA,"%s",init_pckg_to_char(init));
	memcpy(pckg.DATA,init_char,11);
	pckg.LEN  =(char)16;
	pckg.TYPE = SEND_INIT;
	char* buf = mini_kermit_to_char(pckg);
	unsigned short crc = crc16_ccitt(buf,15);
	pckg.CHECK = crc;
	return pckg;
}
/*
 * Returns a Data package that has in the DATA field
 * the data parameter
 */
mini_kermit makeData(char* data,int nofB,int seq) {
	mini_kermit pckg = newMiniKermitPckg();
	pckg.SEQ = (unsigned char) seq;
	memcpy(pckg.DATA,data,nofB);
	int len = 7 + nofB;
	pckg.LEN = (char)(5 + nofB);
	pckg.TYPE = Data;
	char* buf = mini_kermit_to_char(pckg);
	unsigned short crc = crc16_ccitt(buf,len-3);
	pckg.CHECK = crc;
	return pckg;
}

/*
 * Returns an EOF package
 *
 */
mini_kermit makeEOF(int seq){
	mini_kermit pckg =  newMiniKermitPckg();
	pckg.SEQ = (unsigned char) seq;
	sprintf(pckg.DATA,"%s","");
	int len = 7;
	pckg.LEN  = (char)5;
	pckg.TYPE = EOFP;
	char* buf = mini_kermit_to_char(pckg);
	unsigned short crc = crc16_ccitt(buf,len-3);
	pckg.CHECK = crc;
	return pckg;
}

/*
 * Returns an EOT package
 *
 */
mini_kermit makeEOT(int seq){
	mini_kermit pckg = newMiniKermitPckg();
	pckg.SEQ = (unsigned char) seq;
	sprintf(pckg.DATA,"%s","");
	int len = 7;
	pckg.LEN  = (char)5;
	pckg.TYPE = EOT;
	char* buf = mini_kermit_to_char(pckg);
	unsigned short crc = crc16_ccitt(buf,len-3);
	pckg.CHECK = crc;
	return pckg;
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

/*
 * for debugging(to print all the pckgs to be sent)
 */
void printFilesP(filePckgs* array,int nOfF) {
	for(int cont = 0; cont < nOfF; cont++) {
		for (int i = 0 ;i < array[cont].nOfPckgs;i++){
		printChar((unsigned char *)mini_kermit_to_char(array[cont].pckgs[i]));
		printf("%hu crc \n",getCRC(mini_kermit_to_char(array[cont].pckgs[i])));
		fflush(stdout);}
	}

}

/*
 * @filename the files to be divided into packages 
 * @nOfFiles the number of files
 * Returns a filePckgs array, an element of the array  
 * represents the packages of a file(an array of 
 * MINI-KERMIT packages and the number of packages).
 * The first file will contain in its pckgs on the first
 * position the Send-Init file of the sender,and the last
 * file will contain on tha last position the End of 
 * Transmission package.
 * PS: the nOfT(the number of transmissions of every package 
 * of a given file) field was not used 
 */
filePckgs* makePckgs(char** filename,int nOfFiles) {
	//int seq_number = 0;
	filePckgs* pckgs_array=malloc((nOfFiles + 1)*sizeof(filePckgs));
	pckgs_array[0].pckgs[0] = makeSendInit(0);
	char buffer[251];
	buffer[250] = '\0';
	int i = 0;
	int fd;
	int nOfB;
	for(int contor = 0; contor < nOfFiles ; contor++) {
			if (contor == 0) {
				i = 1;
			} else {
				i = 0;
			}
			pckgs_array[contor].pckgs[i] = makeFileHeader(filename[contor + 1],0);
			i++;
			fd = open(filename[contor+1],O_RDONLY);
			while ((nOfB = read(fd,buffer,250)) > 0) {
				buffer[nOfB] ='\0'; 	
				//printf("\n%d\n",nOfB);
				pckgs_array[contor].pckgs[i] = makeData(buffer,nOfB,0);
				i++;
			}
			pckgs_array[contor].pckgs[i] = makeEOF(0);
			i++;
			pckgs_array[contor].nOfPckgs = i;
			close(fd);
			for(int j = 0 ; j < i ; j++) {
				pckgs_array[contor].nOfT[j] = 0;
				/*
				* initializare numar de retransmiteri
				* pentru fiecare pachet la 0
				*/
			}
	}
	pckgs_array[nOfFiles - 1].pckgs[i]= makeEOT(0);
	pckgs_array[nOfFiles - 1].nOfPckgs += 1;
	pckgs_array[nOfFiles - 1].nOfT[i] = 0;
	return pckgs_array;
}

int main(int argc, char** argv) {
    msg t;


    init(HOST, PORT);
    
    int seqN = 0;
    int nOfT = 0;
    filePckgs* array = makePckgs(argv,argc-1);
    //printf("%s\n",argv[1]);
    //printFilesP(array,argc - 1);
    for(int i = 0; i < (argc - 1) ; i++) {
	for(int contor = 0; contor < array[i].nOfPckgs; contor++) {
		nOfT = 0;
		char* buff = mini_kermit_to_char(newSeqMiniKermit(array[i].pckgs[contor],seqN));
		int len = getLength(buff);
		memcpy(t.payload,buff,len + 2);
		t.len = len + 2;
		send_message(&t);
		nOfT += 1;
		printf("[%s] Sending [%c] [%d]\n",argv[0],getType(buff),seqN);
		//printChar((unsigned char*) t.payload);
		//printf("\n");
		//msg *y = receive_message_timeout(5000);
		bool resend = true;
		while (resend && (nOfT <= 3)) {
			
			msg *y = receive_message_timeout(5000);
			if (y == NULL) {
				send_message(&t);
				nOfT++;
				printf("[%s] Resendig : %d\n", argv[0], seqN);
				resend = true;
			} else {
				if(getType(y->payload) == ACK) {
					seqN = getSeq(y->payload) + 1;
					printf("[%s] Got ACK reply with seq number : [%d]\n", argv[0], getSeq(y->payload));
					resend = false;	
					break;	
				} else {
					seqN = getSeq(y->payload) + 1;
					buff = mini_kermit_to_char(newSeqMiniKermit(array[i].pckgs[contor],seqN));
					//len = getLength(buff);
					memcpy(t.payload,buff,len + 2);
					send_message(&t);
					printf("[%s] Got NAK reply with seq number : [%d]. Resending pckg with seq number [%d].\n"
						, argv[0], getSeq(y->payload),seqN);
					nOfT++;
					resend = true;
				}	
			}
			if (nOfT == 3) {
				printf("[%s]Received error ending transmission\n",argv[0]);
				//break;
				exit(0);
			}
		}
	}
    }
    return 0;
}
