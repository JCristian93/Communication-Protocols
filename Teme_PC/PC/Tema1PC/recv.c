#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"
#include <stdlib.h>
#include <time.h>


#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv){
	init(HOST,PORT);
	int TEXT = open ("output" , O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	int OUT = open("log.txt", O_APPEND|O_WRONLY, S_IRUSR|S_IWUSR);
	msg received , ACKsender ;
	send_msg ACK;
	ACK.seq = 0;
	char check;
	while (1) {
		recv_message(&received);
		if (received.len == 0) break;
		//calculam din nou checksum sa verificam daca ajunge intreg pachetul
		check = checksum(received.payload[0], (received.payload + 1), received.len - 2);
		if (check == received.payload[received.len-1]){		//verificam daca e corupt sau nu
			if (received.payload[0] == ACK.seq)	{	//verificam daca e pachetul corect
				//scriu mesajul in fisier fiindca nu e corupt
				write(TEXT, &received.payload[1], received.len - 2);		//scriere in FISIER OUTPUT
				//LOG receiver in cazul in care totul e in regula
				time_t timer;
    			char buffer1_AFIS[23] , buffer2_AFIS [100] ;
    			struct tm* tm_info;
				time(&timer);
				tm_info = localtime(&timer);
				strftime(buffer1_AFIS, 23, "[%d-%m-%Y %H:%M:%S] ", tm_info);
    			write(OUT, buffer1_AFIS, 22);
				sprintf ( buffer2_AFIS ,  " [receiver] Trimit ACK pentru secventa:\nSeq No: %0.3d\n" , received.payload[0]);
				write(OUT, buffer2_AFIS, strlen(buffer2_AFIS));
				sprintf ( buffer1_AFIS ,  "Checksum: %d\n" , received.payload[received.len - 1]);
				write(OUT, buffer1_AFIS, strlen(buffer1_AFIS));
				write(OUT , "-----------------------------------------------------------------------------\n" , 78);
				//TODO
				ACK.checksum = ACK.seq;				//metoda de protectie, sa nu se corupa ACK
				ACKsender.len = 2;
				ACKsender.payload[0] = ACK.seq;
				ACKsender.payload[1] = ACK.checksum;
				send_message(&ACKsender);		//trimitem ACK ca e bine
				ACK.seq++;						//vrem urmtorul pachet
			}
			//inseamna ca e copie si nu o scriu
			else {	
				ACKsender.payload[0] = ACK.seq - 1;
				ACKsender.payload[1] = ACK.seq - 1;
				send_message(&ACKsender);		//daca ACK nu e bun , cerem sa se mai trimita pachetul inca o data
			}
		}
		else {	//inseamna ca e corupt si ii cerem sa retransmita
				
				time_t timer;
    			char buffer1_AFIS[23] , buffer2_AFIS [100]  ;
    			struct tm* tm_info;
				//LOG pentru cazul de corrupt
				time(&timer);
				tm_info = localtime(&timer);
				strftime(buffer1_AFIS, 23, "[%d-%m-%Y %H:%M:%S] ", tm_info);
    			write(OUT, buffer1_AFIS, 22);
				sprintf ( buffer2_AFIS ,  " [receiver] Am primit urmatorul pachet:\nSeq No: %0.3d\n" , ACK.seq);
				write(OUT, buffer2_AFIS, strlen(buffer2_AFIS));
				char txt_checksum [100] ;
				sprintf (txt_checksum , "Am calculat checksum si am detectat eroare. Voi trimite ACK pentru Seq No %d (ultimul cadru corect pe care l-am primit)\n" , ACK.seq - 1);
				write(OUT, txt_checksum , strlen(txt_checksum));
				write(OUT , "-----------------------------------------------------------------------------\n" , 78);
				
			ACKsender.payload[0] = ACK.seq - 1;
			ACKsender.payload[1] = ACK.seq - 1;
			send_message(&ACKsender);
		}
	}
	close (OUT);
	return 0;
}
