#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>

//numele domeniului se afla in buffer la pozitia position
char * getdomain (char * buffer , int position, char * domain) {
    int i, contor = strlen (domain);
    unsigned short int offset;
    if (buffer[position] == 0) {
    	domain[contor] = '.';
    	domain[contor+1] = '\0';
    	return ".";
    }
    else if (buffer[position] < 0) {
        memcpy (&offset , (buffer+position) , 2);
        offset = ntohs (offset) - 49152;
        printf ("%02X la poz %d\n", buffer[offset], offset);
    	return getdomain (buffer , offset , domain);
    }
    else while (buffer[position] > 0) {
        for (i = 1 ; i <= buffer[position] ; i++) {
            domain [contor] = buffer[i + position];
            contor ++;
        }
        position += buffer[position] + 1;
        domain [contor] = '.';
        contor++;
    }
    if (buffer[position] < 0) {
        memcpy (&offset , (buffer+position) , 2);
        offset = ntohs (offset) - 49152;
        printf ("%02X la poz %d\n", buffer[offset], offset);
        return getdomain (buffer , offset , domain);
    }
    domain[contor] = '\0';
	return domain;
}

