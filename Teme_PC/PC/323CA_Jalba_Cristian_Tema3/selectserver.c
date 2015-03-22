#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include<dirent.h>

#define MAX_CLIENTS	20
#define BUFLEN 1048		//numarul de bytes pe care pot sa ii trimit la un moment dat
#define MAX_FILES 100
void error(char *msg)
{
    perror(msg);
    exit(1);
}

struct pereche {
	char *nume_fisier;
	int dimensiune_fisier;
	int flag_fisier;
};
struct client_info {
	int flag;		//flag va arata daca un client e conectat la server sau nu
	int socket_id;		//trebuie sa retin socket-ul , altfel se pierde
	char nume[BUFLEN];
	char director[BUFLEN];
	int port_client;
	char ip[16];
	char *time;
	int numar_fisiere;
	struct pereche fisiere[MAX_FILES];		//LISTA DE FISIERE A FIECARUI CLIENT , poate avea maxim 100 fisiere
};

//functia returneaza prima pozitie libera in vectorul de clienti, altfel returneaza -1
int find_spot_clienti (struct client_info *clienti) {
	int k;
	for (k=0; k<MAX_CLIENTS ; k++) {
			if (clienti[k].flag == 0) return k;
	}
	return -1;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    char buffer[BUFLEN];
    struct client_info	clienti[MAX_CLIENTS + 1];	
    struct sockaddr_in serv_addr, cli_addr;
    time_t rawtime;
  	struct tm * timeinfo;
  	time (&rawtime);
    int n, i, j, poz_clienti;
	char *token;
	char dimensiune[14];
	char nume_client[100] , *nume_file, itoa_buff[20];
	for (i=0;i<MAX_CLIENTS;i++){
		clienti[i].flag=0;
	    clienti[i].numar_fisiere =0;
		for (j=0;j<MAX_FILES;j++){
			clienti[i].fisiere[j].flag_fisier = 0;
		}
	}
    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds
    if (argc < 2) {
        fprintf(stderr,"Usage : %s port\n", argv[0]);
        exit(1);
    }
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
    portno = atoi(argv[1]);
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
            error("ERROR on binding");
    listen(sockfd, MAX_CLIENTS);
    FD_SET(sockfd, &read_fds);
    FD_SET(0,&read_fds);
    fdmax = sockfd;
    while (1) {
        tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
    	for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) {
    				fgets(buffer, BUFLEN-1, stdin);
    				if (strncmp (buffer, "quit", 4) == 0) {
						for (j=0 ; j<MAX_CLIENTS; j++) {
							if (clienti[j].flag == 1) {
								n = send(clienti[j].socket_id, buffer, strlen(buffer), 0);
							}
						}
						FD_ZERO(&read_fds);
						close(sockfd);
    					return 0; 
					}
					else if (strncmp (buffer, "status", 6) == 0) {
						for (j=0 ; j<MAX_CLIENTS; j++) {
							if (clienti[j].flag == 1) {
								printf ("%s %s %d\n", clienti[j].nume, clienti[j].ip, ntohs(clienti[j].port_client));
							}
						}
					}
				}
				else if (i == sockfd) {
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
				}
				else {
    				memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
    						FD_CLR(i, &read_fds);		//cu asta opresti legatura la server 
						} else {
							error("ERROR in recv");
						}
					} 
					else { 
						//printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
				  		poz_clienti = find_spot_clienti(clienti);
				  		if (poz_clienti == -1) printf ("Clientul nu poate fi adaugat , s-a ajuns la capacitatea maxima!\n");
				  		if (buffer[0] == '_'&& buffer[1] == '_' && poz_clienti >= 0) {
							memcpy(clienti[poz_clienti].ip, inet_ntoa(cli_addr.sin_addr), strlen(inet_ntoa(cli_addr.sin_addr)));
					  		token = strtok(buffer, "_ \n");
            				memcpy(clienti[poz_clienti].nume, token, strlen(token));
							token = strtok (NULL, "_ \n");
							memcpy(clienti[poz_clienti].director, token, strlen(token));
							token = strtok (NULL, "_ \n");
							clienti[poz_clienti].port_client = htons(atoi(token));
							clienti[poz_clienti].flag = 1;
							clienti[poz_clienti].time = (char *) malloc (sizeof(char) * 50);
	    					timeinfo = localtime (&rawtime);
							clienti[poz_clienti].time = asctime(timeinfo);
							clienti[poz_clienti].socket_id = newsockfd;		
						}
				  		else if (strncmp(buffer, "listclients", 11) == 0) {
				  			memset(buffer, 0, BUFLEN);	
				  			sprintf (buffer,"lc ");
				  			for (j=0;j<MAX_CLIENTS;j++) {
				  		    	if (clienti[j].flag == 1) {
				  				    strcat(buffer, clienti[j].nume);
				  					strcat(buffer, " ");
				  				}
				  			}
				  			n = send(i, buffer, strlen(buffer), 0);
						  	if (n < 0) printf ("Imi bag picioarele...\n");
			  			}
				  		//infoclient client2 => client2 127.0.0.1 10012 09:51:57 AM
				  		//cautare client dupa nume, returnare 
				  		else if (strncmp(buffer, "infoclient", 10) == 0) {
				  		    token = strtok (buffer, " ");
				  			token = strtok (NULL, " ");
				  			for (j=0 ; j<MAX_CLIENTS; j++) {
							   if ((clienti[j].flag == 1 ) && strncmp(clienti[j].nume, token, strlen(token)-1) == 0) {
							        memset(buffer, 0, BUFLEN);
							        sprintf(buffer, "ic %s %s %d %s ", clienti[j].nume, clienti[j].ip, ntohs(clienti[j].port_client), clienti[j].time); 
								}
							}			  			
							n = send(i, buffer, strlen(buffer), 0);
						  	if (n < 0) printf ("Imi bag picioarele...reloaded\n");	
				  		}
				  		else if (strncmp(buffer, "sharefile", 9) == 0) {
				  			token = strtok (buffer, " ");
				  		    token = strtok (NULL, " ");
				  			nume_file = (char *) malloc (sizeof(char)*strlen(token) + 1);
    			  			sprintf(nume_file, "%s", token);
        		  			token = strtok (NULL, " \n");
        		  			sprintf(dimensiune, "%s", token);
				  			int ok = 0;
				  			int l=0;
				  			for (j=0 ; j<MAX_CLIENTS; j++) {
				  				if (clienti[j].socket_id == i) {
				  					for (l=0; l<MAX_FILES; l++) {
				  					    if (clienti[j].fisiere[l].flag_fisier == 1 && strncmp(clienti[j].fisiere[l].nume_fisier,nume_file ,strlen(nume_file)) == 0) {
				  					        clienti[j].fisiere[l].dimensiune_fisier = atoi (dimensiune);
				  					        ok =1; break;
				  					    }
				  					    
				    				}
								
								if (ok == 1) break;
								clienti[j].fisiere[clienti[j].numar_fisiere].flag_fisier = 1;
				  				clienti[j].fisiere[clienti[j].numar_fisiere].nume_fisier = (char *) malloc (sizeof(char)*strlen(nume_file) + 1);
				 	    		sprintf (clienti[j].fisiere[clienti[j].numar_fisiere].nume_fisier , "%s", nume_file);
				 		    	clienti[j].fisiere[clienti[j].numar_fisiere].dimensiune_fisier = atoi (dimensiune);
				    	    	clienti[j].numar_fisiere ++;
							    ok = 1; break;
							    }
							}
							if (ok == 1) n=send(i, "Succes", 6, 0);
							else  n=send(i, "Fail :(", 7, 0);
				  		}
				  		//unsharefile lab.pdf
				  		else if (strncmp(buffer, "unsharefile", 11) == 0) {
				  			token = strtok (buffer, " ");
				  			token = strtok (NULL, " ");
				  			nume_file = (char *) malloc (sizeof(char)*strlen(token) + 1);
				  			sprintf(nume_file, "%s", token);
				  			int k=0, ok = 0;
				  			for (j=0 ; j<MAX_CLIENTS; j++) {
				  				if (clienti[j].socket_id == i && clienti[j].flag == 1) {
				  					for (k=0; k<clienti[j].numar_fisiere; k++) {
				  						if (strncmp(clienti[j].fisiere[k].nume_fisier, nume_file, strlen(nume_file)-1) == 0) { 
				  							 clienti[j].fisiere[k].flag_fisier = 0;
				  							 ok = 1;
				  				        }
				  					} 
				  				}
				  			}
				  			if (ok == 1) n=send(i, "Succes", 6, 0);
				  			else n=send(i, "-2 : Fisier inexistent",22 , 0);
				  		}
				  		else if (strncmp(buffer, "getshare", 8) == 0) {
				  			token = strtok (buffer, " ");
				  			token = strtok (NULL, " \n");
				  			sprintf(nume_file, "%s", token);
				  			for (j=0 ; j<MAX_CLIENTS; j++) {
				  				if (clienti[j].flag == 1 && strncmp(clienti[j].nume, nume_file, strlen(nume_file)) == 0) {
				  					int k;
				  					memset(buffer, 0, BUFLEN);
				  					for (k=0 ; k<clienti[j].numar_fisiere; k++){
				  						if (clienti[j].fisiere[k].flag_fisier == 1) {
				  						    strcat (buffer, "gs ");
				  							strcat (buffer, clienti[j].fisiere[k].nume_fisier);
				  							strcat (buffer, " ");
				  						    sprintf (itoa_buff, "%d",clienti[j].fisiere[k].dimensiune_fisier);
				  						    strncat (buffer, itoa_buff, strlen(itoa_buff));
				  						    strcat (buffer, " ");
				  						}
				  					}
				  					n = send(i, buffer, strlen(buffer), 0);
				  					if (n < 0) printf ("ERROR getshare");
				  				}
				  			}
				  		}
				  		else if (strncmp(buffer, "infofile", 8) == 0) {
							token = strtok (buffer, " ");
				  			token = strtok (NULL, " \n");
				  			sprintf(nume_file, "%s", token);
				  			int k;
				  			memset(buffer, 0, BUFLEN);
				  			memset(itoa_buff, 0, BUFLEN);
				  			for (j=0 ; j<MAX_CLIENTS; j++) {
				  				if (clienti[j].flag == 1){
				  					for (k=0; k<clienti[j].numar_fisiere; k++){
				  						if (strncmp(clienti[j].fisiere[k].nume_fisier,nume_file ,strlen(nume_file)) == 0) {
				  							strcat (buffer, "if ");
				  							strcat (buffer, clienti[j].nume);
				  							strcat (buffer, " ");
				  							strcat (buffer, nume_file);
				  							strcat (buffer, " ");
				  							sprintf (itoa_buff, "%d",clienti[j].fisiere[k].dimensiune_fisier);
				  							strncat (buffer, itoa_buff, strlen(itoa_buff));
				  							strcat (buffer, "\n");
				  						}
				  					}
				  				}
				  			}
				  			n = send(i, buffer, strlen(buffer), 0);
				  			if (n < 0) printf ("ERROR infofile");
						}
						else if (strncmp(buffer, "getfile", 7) == 0) {
							memset (nume_client, 0 , 100);
							token = strtok (buffer, " ");
			  				token = strtok (NULL, " \n");
			  				sprintf(nume_client, "%s", token);
			  				token = strtok (NULL, " \n");
			  				nume_file = (char *) malloc (sizeof(char)*strlen(token) + 1);
			  				sprintf(nume_file, "%s", token);
			  				memset(itoa_buff, 0, 20);		
				  			//voi trimite clientului 2 un mesaj de forma :
				  			//<nume_recipient> <nume_file> <ip_recipient> <port_recipient>
				  			int ok = i;
				  			for (j=0 ; j<MAX_CLIENTS; j++) {
				  				if (clienti[j].flag == 1 && clienti[j].socket_id == i) {
				  					sprintf (buffer, "gf %s %s %s %d",clienti[j].nume ,nume_file ,clienti[j].ip ,clienti[j].port_client);
				  				}
				  			}
				  			//acum caut clientul cu numele nume_client si ii dau buffer-ul facut mai sus :)
				  			int l=0;
				  			for (l=0 ; l<MAX_CLIENTS; l++) {
				  				if ((clienti[l].flag == 1) && (strncmp(clienti[l].nume, nume_client, strlen(nume_client)) ==0)) {
				  					ok = clienti[l].socket_id;		
				  				}
				  			}
				  			n = send(ok, buffer, strlen(buffer), 0);
				  			if (n < 0) printf ("Eroare 1 getfile\n");
				  			else printf ("Success getfile\n");
						}
						else if (n = send(i, buffer, strlen(buffer), 0) <=0 ){
							printf ("Eroare trimitere inapoi mesaj la client");
						}
					}
				}
			} 
		}
	}
    close(sockfd);
    return 0; 
}

