#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_CLIENTS 20
#define BUFLEN 1048
#define SHARE_ERROR	-2

void error(char *msg)
{
    perror(msg);
    exit(0);
}
/*
> sharefile picture.png
Succes
> sharefile enunt.pdf
-2 : Fisier inexistent
*/
int share_file	(char *nume_fisier , char *nume_folder) {
    char path[100];
	memset(path, 0 , 100);
	sprintf (path, "%s/%s", nume_folder, nume_fisier);
	struct stat sb;
	int s = stat (path , &sb);
	if (s < 0) {
		return -2;
	}
	int size = sb.st_size;
	memset(path, 0 , 100);
	return size;
}

struct client_info {
	int socket_id;		//trebuie sa retin socket-ul , altfel se pierde
	int port_client;
	int semnal; 	//semnal 1 inseamna ca mai sunt date de trimis , 0 inseamna ca are un canal liber
	char ip[16];
	char nume_fisier[BUFLEN];
	char nume_client[BUFLEN];
};
int main(int argc, char *argv[])
{
    int ssockfd, clsockfd, newsockfd, n, i;
    struct sockaddr_in serv_addr, cl_addr, new_cl_addr;
    struct hostent *server;
    struct client_info client_in, client_out;		//doar 1 client pe intrare date si 1 pe iesire date, no more
    char * token, *nume_fisier;
	char gf1[BUFLEN], gf2[BUFLEN], gf3[BUFLEN], gf4[BUFLEN];
    char buffer_server[BUFLEN], buffer_client[BUFLEN];	//ce va da , bufferul e limitat la 1048 bytes (adica un KB)
    
    if (argc != 6) {
       fprintf(stderr,"Usage: ./client <nume_client> <nume_director> <port_client> <ip_server> <port_server>\n");
       exit(0);
    }  
    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
		ssockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ssockfd < 0) 
        error("ERROR opening socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[5]));
    inet_aton(argv[4], &serv_addr.sin_addr);
    if (connect(ssockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting at server");
    memset(buffer_server, 0, BUFLEN);
    n = sprintf(buffer_server, "__%s %s %s", argv[1], argv[2], argv[3]);      
    n = send(ssockfd, buffer_server, n, 0);
    if (n<=0) error("Nu s-a putut trimite informatiile clientului la conectare\n");
    //deschidem un alt socket pe care vrem sa ascultam conexiuni de la alti clienti
    clsockfd = socket(AF_INET, SOCK_STREAM, 0);
    //cl_addr reprezinta informatiile clientului
    memset((char *) &cl_addr, 0, sizeof(cl_addr));
    cl_addr.sin_family = AF_INET;
    cl_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    cl_addr.sin_port = htons(atoi(argv[3]));
    //deoarece clientul va actiona ca server , dam bind pe socketul creat pt a asculta de la ceilalti clienti
    if (bind(clsockfd, (struct sockaddr *) &cl_addr, sizeof(struct sockaddr)) < 0) 
              error("ERROR on binding");
    listen(clsockfd, MAX_CLIENTS);
    FD_SET(ssockfd, &read_fds);
    FD_SET(0,&read_fds);
    int fdmax = ssockfd;
    int fd_out; //file-descriptorul fisierului din care da informatii
    int fd_in; 	//file-descriptorul fisierului in care baga informatii din exterior
    int sf_ret;
    while(1) {
  		//citesc de la tastatura o comanda pentru server
  		//momentan o sa trimit comanda inapoi la client, dupa aceea voi incepe sa implementezi comenzile din tema
    	memset(buffer_server, 0, BUFLEN);
		memset(buffer_client, 0, BUFLEN);
    	if (client_out.semnal == 1) {
			 n = read (fd_out, buffer_client, BUFLEN);
			 printf ("vreau sa dau: %s", buffer_client);
			if (n < 0) {
				printf ("Eroare citire din fisierul: %s", client_out.nume_fisier);
				client_out.semnal = 0;
			}
			else n = send(newsockfd, buffer_client, n, 0);
		}
    	if (client_in.semnal == 1) {
			n = recv (client_in.socket_id, buffer_client, BUFLEN, 0);
			printf ("am primit :%s", buffer_client);
			if (n <= 0) {
				printf ("Eroare scriere in fisierul: %s", client_in.nume_fisier);
				client_in.semnal = 0;
			}
			else n = write (fd_in, buffer_client, n);
		}
    	tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
		for (i=0; i<= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) {
					fgets(buffer_server, BUFLEN-1, stdin);
					if (strncmp (buffer_server, "sharefile", 9) == 0) {
						token = strtok(buffer_server, " ");
						token = strtok(NULL, "\n");
						nume_fisier = (char *) malloc (sizeof(char) * (strlen(token)+1));
						sprintf(nume_fisier, "%s", token);
						memset (buffer_server, 0 , BUFLEN);
						sf_ret = share_file(nume_fisier , argv[2]);
						if (sf_ret == -2)   printf("-2 : Fisier inexistent\n");
						else {
						    sprintf (buffer_server, "sharefile %s %d\n",nume_fisier ,share_file(nume_fisier , argv[2]));
						    n = send(ssockfd, buffer_server, strlen(buffer_server), 0);
						    if (n < 0) error("ERROR in sharefile");
						}
					}
					else if (strncmp (buffer_server, "getfile", 7) == 0) {
						n = send(ssockfd, buffer_server, strlen(buffer_server), 0);
						token = strtok (buffer_server, " ");
						token = strtok (NULL, " ");
						sprintf (client_in.nume_client, "%s", token);
						token = strtok (NULL, " \n");
					    sprintf (client_in.nume_fisier, "%s/%s", argv[2], token);
						fd_in = open (client_in.nume_fisier , O_RDWR|O_CREAT|O_APPEND);
						client_in.semnal = 0;	
					}
					else if (strncmp (buffer_server, "quit", 4) == 0) {
						n = send(ssockfd, buffer_server, strlen(buffer_server), 0);
						FD_ZERO(&read_fds);
						close(ssockfd);
						close(clsockfd);
						return 0;
					}
					else {	
						n = send(ssockfd, buffer_server, strlen(buffer_server), 0);
					}
				}
				else if (i == clsockfd) {	
				    printf ("salvez datele destinatiei\n");
					int clilen = sizeof(new_cl_addr);
					if ((newsockfd = accept(clsockfd, (struct sockaddr *)&new_cl_addr, &clilen)) == -1) {
						error("ERROR in accept client-client");
					} 
					else {
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
						client_in.semnal = 1;
						client_in.socket_id = newsockfd;
						client_in.port_client = new_cl_addr.sin_port;
						//sprintf (client_in.ip,"%s",new_cl_addr.sin_addr);
						inet_aton(client_in.ip, &new_cl_addr.sin_addr);
					}
				}
				else {
					memset(buffer_server, 0, BUFLEN);
					if ((n = recv(i, buffer_server, sizeof(buffer_server), 0)) <= 0) {
						if (n == 0) {
							printf("selectserver: socket %d hung up\n", i);
						} else {
							error("ERROR in recv");
						}
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
					}
					else {	//AICI ANALIZAM RASPUNSUL SERVERULUI
						//printf ("Am primit de la server mesajul: %s\n", buffer_server);
						//printf ("server mesajul2 :%s\n", buffer_server);
						if (buffer_server[0] == 'S'){
						    printf ("Succes\n");
						}
						else if (buffer_server[0] == '-'){
						    printf("-2 : Fisier inexistent\n");
						}
						else if (buffer_server[0] == 'i' && buffer_server[1] == 'f'){
						    token = strtok (buffer_server, " ");
						    while(token != NULL) {    
							    token = strtok (NULL, " \n");
							    if (token != NULL) printf ("%s ", token);
						    }
						    printf("\n");
						}
						else if (buffer_server[0] == 'g' && buffer_server[1] == 's'){
						    token = strtok (buffer_server, " ");
						    while(token != NULL) {    
							    token = strtok (NULL, " \n");
							    if (token != NULL) {
							        if (strncmp(token, "gs",2) ==0) printf ("\n");    
							        else printf ("%s ", token);
						        }
						    }
						    printf("\n");
						}
						else if (buffer_server[0] == 'i' && buffer_server[1] == 'c'){
						    token = strtok (buffer_server, " ");
						    while(token != NULL) {    
							    token = strtok (NULL, " \n");
							    if (token != NULL) printf ("%s ", token);
						    }
						    printf("\n");
						}
						else if (buffer_server[0] == 'l' && buffer_server[1] == 'c') {
						    token = strtok (buffer_server, " ");
							while(token != NULL) {    
							    token = strtok (NULL, " \n");
							    if (token != NULL) printf ("%s\n", token);
						    }	
						}
						else if (buffer_server[0] == 'g' && buffer_server[1] == 'f') {
						//gf <nume_recipient> <nume_file> <ip_recipient> <port_recipient>
						//fiecare gf reprezinta un parametru din raspuns
							memset (gf1, 0, BUFLEN);			
							memset (gf2, 0, BUFLEN);		
							memset (gf3, 0, BUFLEN);		
							memset (gf4, 0, BUFLEN);
							printf("dwajdja2\n");
							memset (buffer_client, 0, BUFLEN);
							token = strtok (buffer_server, " ");
							token = strtok (NULL, " ");							
							sprintf (gf1, "%s", token);
							token = strtok (NULL, " ");
							sprintf (gf2, "%s/%s", argv[2], token);
							token = strtok (NULL, " ");
							sprintf (gf3, "%s", token);
							token = strtok (NULL, " \n");
							sprintf (gf4, "%s", token);
							fd_out = open (gf2, O_RDONLY);
							printf("dwajdja3\n");
							newsockfd = socket(AF_INET, SOCK_STREAM, 0);
							printf("dwajdja4\n");
							if (newsockfd < 0) 
       						    error("ERROR opening socket");
							if (newsockfd > fdmax) { 
								fdmax = newsockfd;
							}
							printf("dwajdja\n");
							new_cl_addr.sin_family = AF_INET;
    						new_cl_addr.sin_port = atoi(gf4);
    						inet_aton(gf3, &new_cl_addr.sin_addr);
    						printf ("informatii :%s %s %d %d\n",gf2,gf3,ntohs(atoi(gf4)), newsockfd);
    					 	if (connect(newsockfd,(struct sockaddr*) &new_cl_addr,sizeof(new_cl_addr)) < 0) 
        							error("ERROR connectare la client\n");
							if (fd_out < 0) printf ("EROARE deschidere fisier");
							n = read (fd_out, buffer_client, BUFLEN);
							if (n < 0) {
								printf ("Eroare citire din fisierul: %s\n", gf2);
								client_out.semnal = 0;
							}
							else {
								n = send(newsockfd, buffer_client, n, 0);
								if (n < 0) printf ("Eroare1 trimitere: %s\n", gf2);
								client_out.semnal = 1;
								client_out.socket_id = newsockfd;
								client_out.port_client = atoi(gf4);
								sprintf (client_out.ip,"%s",gf3);
								sprintf (client_out.nume_fisier,"%s",gf2);		//fisierul dorit
								sprintf (client_out.nume_client,"%s",gf1);		//numele clinetului care primeste fisierul , nu care da
							}
						}
						//in caz ca primim quit de la server , ne inchidem
						else if (strncmp (buffer_server, "quit", 4) == 0) {
							FD_ZERO(&read_fds);
							close(ssockfd);
							close(clsockfd);
							return 0;
						}
						else {
						//	printf ("Inca nu cunosc comanda asta");
						}
					}
				}				
			}
		}
    }
	close(ssockfd);
	close(clsockfd);
	return 0;
}

