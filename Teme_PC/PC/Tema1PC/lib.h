#ifndef LIB
#define LIB

#define TYPE1		1
#define TYPE2		2
#define TYPE3		3
#define TYPE4		4
#define ACK_T1		"ACK(TYPE1)"
#define ACK_T2		"ACK(TYPE2)"
#define ACK_T3		"ACK(TYPE3)"

#define MSGSIZE		1400
#define PKTSIZE		1396
#include <string.h>

typedef unsigned char byte;
byte calc_parity(byte d) {
  byte mask = 1;
  byte par = 0;
  int i;
  for(i=1; i <= 7; i++) {
    if( (d & mask) != 0 )
      par = par ^ 1;
    mask = mask << 1;
  }
  return par;
}
//aceasta functie am vrut sa o folosesc dar ma complicam cu afisarea string-ului

const char *byte_to_binary(int x)
{	//transforma un int in forma sa binara
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}
char checksum(char seq, char* info, int n) {	//n e lungimea lui info
	char chk = seq;
	int i;
	for(i = 0; i < n; i++)
		chk = chk ^ info[i];
	return chk;
}

typedef struct {
	int len;
	char payload[1400];
} msg;

typedef struct {
	char seq;
	char* info;
	char checksum;
} send_msg;

msg converter_to_msg (msg * pointer) {
	msg mesaj;
	mesaj.len = pointer->len;
	int i ;
	for (i = 0 ; i < mesaj.len ; i++){
		mesaj.payload[i] = pointer->payload[i];
	}
	return mesaj;
}

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout);
msg* receive_message();
#endif
