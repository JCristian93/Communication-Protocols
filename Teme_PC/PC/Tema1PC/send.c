#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"
#include <stdlib.h>
#include <time.h>

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc,char** argv) {
	init(HOST,PORT);
  	int g = 0;
	srand(time(NULL));
	msg message , received ;	//mesajul impachetat
	msg * ping;
	send_msg smsg;	//mesajul neimpachetat , adica frame
	smsg.seq = 0;	//next frame to send
	int IN = open( argv[1], O_RDONLY);
	int OUT = open("log.txt", O_CREAT|O_APPEND|O_WRONLY|O_TRUNC , S_IRUSR|S_IWUSR);
	int buffer[60];		//bufferul cu care citim chunks
	int reader = read(IN, buffer, rand() % 60 + 1);
	
	while(reader > 0) {
		smsg.info = (char*)malloc((reader + 1)*sizeof(char));		
		memcpy(smsg.info, &buffer, reader);		//s.payload = buffer;
		smsg.checksum = checksum(smsg.seq, smsg.info, reader);		//calculare checksum
		message.len = reader + 2;			//stabilire lungime mesaj impachetat (ce trimit e de tipul msg)
		message.payload[0] = smsg.seq;		//s.seq = next_frame_to_send
		memcpy(&message.payload[1], smsg.info, message.len - 2);		//impachetam mesajul
		message.payload[message.len-1] = smsg.checksum;					//impachetam mesajul
		send_message(&message);						//trimitere la nivel fizic
		// LOG sender cand trimite un pachet:		
 		time_t timer;
    	char buffer1_AFIS[23] , buffer2_AFIS [14] , buffer3_AFIS [reader + 11] ;
    	struct tm* tm_info;
		time(&timer);
		tm_info = localtime(&timer);
		strftime(buffer1_AFIS, 23, "[%d-%m-%Y %H:%M:%S] ", tm_info);
    	write(OUT, buffer1_AFIS, 22);
		write(OUT, " [sender] Am trimis urmatorul pachet:\n" , 38);
		sprintf ( buffer2_AFIS ,  "Seq No: %0.3d\n" , smsg.seq);
		write(OUT, buffer2_AFIS, 12);
		smsg.info [reader] = '\0';
		sprintf ( buffer3_AFIS ,  "Payload: %s\n" , smsg.info);
		write(OUT, buffer3_AFIS, strlen(buffer3_AFIS));
		sprintf ( buffer1_AFIS ,  "Checksum: %d\n" , smsg.checksum);
		write(OUT, buffer1_AFIS, strlen(buffer1_AFIS));
		write(OUT , "-----------------------------------------------------------------------------\n" , 78);
		free(smsg.info);
		//end LOG sender
		
		//if (timeout < durata)		
		//durata e random
		int TIMEOUT = rand () % 51;
		ping = receive_message_timeout(TIMEOUT);		// 50 reprezinta milisecunde
		if (ping == NULL){		//verific daca timpul de transmitere depaseste timpul de timeout
			write (OUT , "Confirmarea e intaziata , mai trimit o data\n" , 44 );
			write(OUT , "-----------------------------------------------------------------------------\n" , 78);
			continue;
		}
		received = converter_to_msg(ping);	//primim ACK , adica acel event 
		if (received.payload[0] == received.payload[1]) {	//daca ACK == SEQ_NUMBER , inseamna ca nu e un ACK fals
			if (received.payload[0] == smsg.seq) {		//if ACK == next_frame_to_send
				reader = read(IN, buffer, rand() % 60 + 1);		//luam un alt chunk de buffer
				smsg.seq++;		//s.seq = next_frame_to_send
			}
			else {		//toate else din send se bazeaza pe timeout si nu prea ma pricep cu ala .. :(
					continue;	
			}
		}
		else {
				continue;
		}
		
	}
	//ajunge aici cand nu mai are pachete de trimis , semnaland ca trebuie sa se opreasc programul
	//condtie daca primeste pachet cu dimensiune intre - infinit si 2
	message.len = 0; 
	send_message (&message);
	close(IN);
	close(OUT);

	return 0;
}
