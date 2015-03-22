#include "header.h"
#include "dns_message.h"

#define BUFLEN 512

char* getType (char type) {
    if (type == 1) {
        return "A";
    }
    else if (type == 15) {
        return "MX";
    }
    else if (type == 2) {
        return "NS";
    }
    else if (type == 5) {
        return "CNAME";
    }
    else if (type == 6) {
        return "SOA";
    }
    else if (type == 16) {
        return "TXT";
    }
    else if (type == 12) {
        return "PTR";
    }
    return "Unknown";
}
char * getClass (char clas) {
    if (clas == 1) {
        return "IN";
    }
    else if (clas == 3) {
        return "CHAOS";
    }
    else if (clas == 4) {
        return "HESIOD";
    }
    return "OTHERS";
}
int main (int argc, char **argv) {

	if (argc!=3) {
		printf ("Usage: <numele de domeniu / adresa IP,> <interogare>\n");
		return 0;
	}
	FILE *hexalog = fopen ("message.log" , "w");
	struct sockaddr_in server, from_server;
    socklen_t server_size;
	dns_header_t header;
	dns_question_t question;
	char nume_adresa[BUFLEN];		    //numele adresei ce o vom verifica cu DNS-ul
	char comanda[BUFLEN];				//comanda , adica ce vrem sa interogam DNS-ul in legatura cu adresa data
	sprintf (nume_adresa,"%s", argv[1]);
	sprintf (comanda,"%s", argv[2]);
	char ip_matrix [BUFLEN][BUFLEN];	//matrice care va retine adresele ip ale tuturor serverelor DNS
										//noi vom parcurge prin mai multe servere pana cand se gaseste unul
										//care sa aiba adresa ce o cautam inregistrata
	char *token;
	struct timeval wait_time;           //cat vom astepta pana sa trecem la urmatorul server DNS din lista
	int RRtype , i, j;
	FILE *fp = fopen ("dns_servers.conf", "r");
	int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(53);
    wait_time.tv_sec = 3;
    wait_time.tv_usec = 0;
    char buffer[BUFLEN];
    int ok;
	  //incarc toate ip-urile din lista dns_servers.conf
	  int contor_ip_DNS = 0;

	  if ( fp != NULL ) {
			char line [ BUFLEN ];
			while ( fgets ( line, sizeof line, fp ) != NULL ) {	//citeste o linie
				//verificam daca line incepe cu # , daca nu si difera de null , atunci presupunem ca este ip
				if (line[0] == '\n' || line [0] == '\0' || line [0] == '#') continue;
				else {	//inseamna ca am gasit ip
					line[strlen (line) -1] = '\0';
					sprintf (ip_matrix[contor_ip_DNS],"%s",line);
					contor_ip_DNS ++ ;
				}
			}
		}
		else {
			printf ("Nu s-a putut deschide fisierul cu servere DNS\n");
			return 0;
		}
	  for (i = 0 ; i < contor_ip_DNS ; i++) {
	  	printf ("%s\n" , ip_matrix[i]);
	  }
		memset (&header, 0, sizeof(header));
		header.id = htons(0);
		header.rd = 1;
		header.tc = 0;
		header.aa = 0;
		header.opcode = 0;
		header.qr = 0;
		header.rcode = 0;
		header.z = 0;
		header.ra = 0;
		header.qdcount = htons(1);
		header.ancount = 0;
		header.nscount = 0;
		header.arcount = 0;

	//verificam ce tip are comanda primita
	if (strncmp (comanda, "A" , 1) == 0) {
		RRtype = 1;
	}
	else if (strncmp (comanda, "NS", 2) == 0) {
		RRtype = 2;
	}
	else if (strncmp (comanda, "CNAME", 5) == 0) {
		RRtype = 5;
	}
	else if (strncmp (comanda, "SOA", 3) == 0) {
		RRtype = 6;
	}
    else if (strncmp (comanda, "MX" , 2) == 0) {
        RRtype = 15;
    }
	else if (strncmp (comanda, "TXT", 3) == 0) {
		RRtype = 16;
	}
    else if (strncmp (comanda, "PTR", 3) == 0) {
        RRtype = 12;
    }
	else {
		printf ("ERROR: Unknown RRtype\n");
		return 0;
	}
	printf ("RRtype este : %d\n",RRtype);

	question.qtype = htons(RRtype);
	question.qclass = htons(1);
	//transform adresa ca in urmatorul exemplu :
	// 	  www.google.com in 3www6google3com

	char QNAME [BUFLEN];
	memset (QNAME , 0 , BUFLEN);
	sprintf (buffer , "%s" , nume_adresa);
	char x ;
	int lenght;
	if (RRtype == 12) {
        //daca comanda e PTR , atunci modificam QNAME
        //; 8.8.8.8 - com.yahoo.in-addr.arpa PTR
        char **aux_vector = (char**) malloc (100 * sizeof (char *));
        for (i = 0 ; i < 100 ; i++) {
            aux_vector [i] = (char *) malloc (100 * sizeof (char));
        }
        int poz=0;
        token = strtok (buffer, ".");
        sprintf (aux_vector[poz] , "%s" , token);
        poz++;
        while (token != NULL) {
            token = strtok (NULL, ".");
            if (token != NULL) {
                sprintf (aux_vector[poz] , "%s" , token);
                poz++;
            }
        }
        poz --;
        for (i = poz ; i>= 0 ; i--) {
            QNAME[strlen(QNAME)] = strlen (aux_vector[i]);
            QNAME[strlen(QNAME)+1] = '\0';
            strcat (QNAME , aux_vector[i]);
        }
        strcat (QNAME , "7in-addr4arpa");
    }
    else {
        token = strtok (buffer, ".");
        while (token != NULL) {
	      	i = strlen (token);
		    x = i;
            QNAME[strlen(QNAME)] = x;
            QNAME[strlen(QNAME)+1] = '\0';
		    strncat (QNAME , token , i);
		    token = strtok (NULL, ".");
	   }
    }
	QNAME[strlen(QNAME)] = 0;
    QNAME[strlen(QNAME)+1] = '\0';
	printf ("QNAME este :");

	for(i = 0; i < (strlen(QNAME) + 1) ;i++) {
		if(QNAME[i] < 20)
			printf("%d ", QNAME[i]);
		else printf("%c ", QNAME[i]);
	}
	printf ("\n");

	int name_len = strlen (nume_adresa) + 2;
	int msg_len = sizeof (header) + sizeof (dns_question_t) + name_len;

	char *message;
	message = malloc (sizeof (char) * msg_len);

	memcpy (message, &header, sizeof (header));
	memcpy (message + sizeof (header), QNAME, name_len);
	memcpy (message + sizeof (header) + name_len, &question, sizeof (dns_question_t));

 	memset (buffer , 0, BUFLEN);
	setsockopt (server_socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &wait_time, sizeof (struct timeval));
    j = 0;
    ok = 0;
    int returnat;
    for (i = 0 ; i < 512 ; i++)
        fprintf( hexalog, "%02X ",message[i]);
    //folosim un singur socket pentru a ne conecta la server , nu avem nevoie de mai multe canale
    while (j != contor_ip_DNS) {
        //setez adresa serverului DNS
        inet_aton(ip_matrix[j], (struct in_addr *) &server.sin_addr.s_addr);
        if (connect (server_socket, (struct sockaddr *) &server, sizeof (struct sockaddr_in)) < 0) {
            printf ("Eroare la conectare catre server DNS\n");
        }
        if (sendto(server_socket, message, BUFLEN, 0, (struct sockaddr* ) &server, sizeof(struct sockaddr_in)) < 0) {
            printf ("Eroare la trimitere request catre server DNS \n");
            continue;
        }
        returnat = recvfrom (server_socket, buffer, BUFLEN, 0, (struct sockaddr*) &from_server , &server_size);
        if (returnat < 0) {
            printf ("Eroare primire raspuns de la DNS");
        }
        else {  //daca ajunge aici inseamna ca s-a connectat cu succes la DNS
            printf ("M-am connectat cu succes la un server DNS si pot primi pachete\n");
            ok = 1;
            break;
        }
        j++;
    }
    if (ok == 0) {
    	printf ("Nu m-am connectat la nici un server DNS , opresc programul\n");
    	close (server_socket);
  	    fclose (fp);
    	return 0;
    }
    //scriu in message.log mesajul in hexa
    //buffer momentan contine raspunsul DNS-ului
    //Scriun in fisierul de log inregistrarea
  	FILE * LOG;
  	LOG = fopen ("logfile", "a+");
  	fprintf (LOG, "; %s - %s %s\n\n\n", ip_matrix[j], nume_adresa, argv[2]);

    //scriu informatiile RR primite de la DNS
    /*
    ignor primii 2 octeti care reprezinta id-ul
    ignor urmatorii 2 octeti ce reprezinta QR , OPCODE , etc
    ignore urmatorii 2 ce reprezinta QDCOUNT
    urmeaza 6 octeti cu : ANCOUNT , NSCOUNT , ARCOUNT

    urmeaza QUESTION care contine numele domeniului , type si class

    urmeaza un raspuns de la DNS :
    NAME        lungimea e data pas cu pas
    TYPE        2 octeti
    CLASS       2 octeti
    TTL         4 octeti
    RLENGTH
    RDATA       */

    dns_header_t h;
    char domain[BUFLEN];
    char question_type[12];
    char question_class[12];
    memcpy (&h, buffer, sizeof (dns_header_t));

    int position = 6;   //pozitia curenta in buffer, o incpe cu 5 deoarece ignor primele 6 pozitii

    //setez ancount , nscount , arcount ca sa vad cate raspunsuri si de ce tip voi avea
    unsigned short int ancount;
    unsigned short int nscount;
    unsigned short int arcount;
    unsigned short int aux;
    memcpy (&ancount , (buffer+position)  , 2);
    ancount = ntohs (ancount);
    position += 2;
    memcpy (&nscount , (buffer+position) , 2);
    nscount = ntohs (nscount);
    position += 2;
    memcpy (&arcount , (buffer+position) , 2);
    arcount = ntohs (arcount);
    position += 2;
	memset (domain , 0 , BUFLEN);
    //obtin QUESTION
    //functia care preia numele domeniului
    //position va fi setat dupa sfarsitul domeniului
    getdomain (buffer , position, domain);
    position += strlen (domain) + 1;
    sprintf (question_type , "%s" , getType(buffer[position +1]));
    // verific aux (adica type) cu o functie care verifica tipul ei
    position += 2;
    sprintf (question_class , "%s" , getClass(buffer[position+1]));  //acum aux este class
    position += 2;
    // verific aux cu o functie care verifica tipul clasei
    //formatele sunt aceleasi la toate, difera doar titlul acela frumos
    printf ("QUESTION: %s %s %s\n",domain, question_type, question_class);
    char line[BUFLEN];	//linia pe care o voi afisa la un moment dat in logfile
    char lungime_text;
    char rdata_type [12];
    char rdata_class [12];
    memset (line , 0 , BUFLEN);
    if (ancount != 0) {
        fprintf (LOG, ";;ANSWER SECTION\n\n");
         for (i=0 ; i<ancount ; i++) {
            memset (line , 0 , BUFLEN);
            memset (domain , 0 , BUFLEN);
            memset (rdata_type , 0 , 12);
            memset (rdata_class , 0 , 12);
            //NAME
            sprintf (line , "%s\t" ,  getdomain (buffer , position , domain));
            if (buffer[position] < 0) {
                position += 2;
            }
            else if (buffer[position] == 0) {
                position ++;
            }
            else {
                //sar pana la pointer :)
                while (buffer[position] > 0) {
                    position += buffer[position]+1;
                }
                position +=2;
            }
            printf ("position type %d\n", position);
            //TYPE
            sprintf (rdata_type , "%s" , getType (buffer [position + 1]));
            strcat (line ,rdata_type);
            strcat (line , "\t");
            position += 2;
            printf ("position class %d\n", position);
            //CLASS
            sprintf (rdata_class , "%s",  getClass(buffer [position + 1]));
            strcat (line , rdata_class);
            strcat (line , "\t");
            position += 2;
            //TTL
            position += 4;
            //RLENGTH
            lungime_text = buffer[position + 1];
            position += 2;
            //RDATA
            if (strcmp (rdata_type , "NS") == 0) {   //ma voi astepta la un nume
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                position += lungime_text;
            }
            else if (strcmp (rdata_type , "A") == 0) { //ma voi astepta la un ip
                //RLENGHT TREBUIE sa fie 4
                int ipaux[10];
                char ipaux2[10];
                if (buffer[position] < 0) ipaux[0] = buffer[position] + 256;
                else ipaux[0] = buffer[position];

                if (buffer[position + 1] < 0) ipaux[1] = buffer[position + 1] + 256;
                else ipaux[1] = buffer[position + 1];

                if (buffer[position + 2] < 0) ipaux[2] = buffer[position + 2] + 256;
                else ipaux[2] = buffer[position + 2];

                if (buffer[position + 3] < 0) ipaux[3] = buffer[position + 3] + 256;
                else ipaux[3] = buffer[position + 3];
                sprintf (ipaux2 , "%d.%d.%d.%d"
                            , ipaux[0]
                            , ipaux[1]
                            , ipaux[2]
                            , ipaux[3]);
                strcat (line , ipaux2);
                position += 4;
            }
            else if (strcmp (rdata_type , "MX") == 0) {
                //serverul 141.85.128.1 a fost down mereu ...
                //nu am putut testa MX , sper sa fie bine
                //short + adresa
                memset (domain , 0 , BUFLEN);
                unsigned short int mx_number;
                memcpy (&mx_number , buffer + position , 2);
                mx_number = ntohs (mx_number);
                memcpy (line + strlen (line), &mx_number , 2);
                strcat (line , "\t");
                sprintf (domain , "%s" , getdomain(buffer , position , domain));
                strcat (line , domain);
                position += 2;
            }
            else if (strcmp (rdata_type , "SOA") == 0) {
                //momentan iau codul de la NS
                //pe scurt sunt 2 adrese si 5 integere
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                if (buffer[position] < 0) {
                   position += 2;
                }
                else if (buffer[position] == 0) {
                 position ++;
                }
                else {
                    //tup-tup-tup pana la pointer :)
                    while (buffer[position] > 0) {
                        position += buffer[position]+1;
                    }
                    position +=2;
                }
                //iau a doua adresa
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                if (buffer[position] < 0) {
                   position += 2;
                }
                else if (buffer[position] == 0) {
                    position ++;
                }
                else {
                    //tup-tup-tup pana la pointer :)
                    while (buffer[position] > 0) {
                        position += buffer[position]+1;
                    }
                    position +=2;
                }
                //iau cele 5 inturi
                int int1 , int2 , int3 , int4 , int5;
                memcpy (&int1 , buffer + position , 4);
                position += 4;
                int1 = ntohl (int1);
                sprintf (line + strlen(line) , "%d\t" , int1);
                memcpy (&int2 , buffer + position , 4);
                position += 4;
                int2 = ntohl (int2);
                sprintf (line + strlen(line) , "%d\t" , int2);
                memcpy (&int3 , buffer + position , 4);
                position += 4;
                int3 = ntohl (int3);
                sprintf (line + strlen(line) , "%d\t" , int3);
                memcpy (&int4 , buffer + position , 4);
                position += 4;
                int4 = ntohl (int4);
                sprintf (line + strlen(line) , "%d\t" , int4);
                memcpy (&int5 , buffer + position , 4);
                position += 4;
                int5 = ntohl (int5);
                sprintf (line + strlen(line) , "%d\t" , int5);
            }
            else if (strcmp (rdata_type , "PTR") == 0) {
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                position += lungime_text+1;
            }
            else if (strcmp (rdata_type , "CNAME") == 0) {
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                position += lungime_text;
            }
            else if (strcmp (rdata_type , "TXT") == 0) {
                strncat (line , buffer , lungime_text);
                position += lungime_text;
            }
            fprintf (LOG, "%s\n", line);
        }
    }
    if (nscount != 0) {
        fprintf (LOG, ";;AUTHORITY SECTION\n\n");
            //tratez toate cazurile de optiuni
            //NAME , TYPE , CLASS , TTL , RLENGTH , RDATA
        for (i=0 ; i<nscount ; i++) {
            memset (line , 0 , BUFLEN);
            memset (domain , 0 , BUFLEN);
            memset (rdata_type , 0 , 12);
            memset (rdata_class , 0 , 12);
            //NAME
            sprintf (line , "%s\t" ,  getdomain (buffer , position , domain));
            if (buffer[position] < 0) {
                position += 2;
            }
            else if (buffer[position] == 0) {
                position ++;
            }
            else {
                //tup-tup-tup pana la pointer :)
                while (buffer[position] > 0) {
                    position += buffer[position]+1;
                }
                position +=2;
            }
            printf ("position type %d\n", position);
            //TYPE
            sprintf (rdata_type , "%s" , getType (buffer [position + 1]));
            strcat (line ,rdata_type);
            strcat (line , "\t");
            position += 2;
            printf ("position class %d\n", position);
            //CLASS
            sprintf (rdata_class , "%s",  getClass(buffer [position + 1]));
            strcat (line , rdata_class);
            strcat (line , "\t");
            position += 2;
            //TTL
            position += 4;
            //RLENGTH
            lungime_text = buffer[position + 1];
            position += 2;
            //RDATA , cu toate RR-tipurile
            if (strcmp (rdata_type , "NS") == 0) {    //ma voi astepta la un nume
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                position += lungime_text;
            }
            else if (strcmp (rdata_type , "A") == 0) {   //ma voi astepta la un ip
                //RLENGHT TREBUIE sa fie 4
                int ipaux[10];
                char ipaux2[10];
                if (buffer[position] < 0) ipaux[0] = buffer[position] + 256;
                else ipaux[0] = buffer[position];

                if (buffer[position + 1] < 0) ipaux[1] = buffer[position + 1] + 256;
                else ipaux[1] = buffer[position + 1];

                if (buffer[position + 2] < 0) ipaux[2] = buffer[position + 2] + 256;
                else ipaux[2] = buffer[position + 2];

                if (buffer[position + 3] < 0) ipaux[3] = buffer[position + 3] + 256;
                else ipaux[3] = buffer[position + 3];
                sprintf (ipaux2 , "%d.%d.%d.%d"
                            , ipaux[0]
                            , ipaux[1]
                            , ipaux[2]
                            , ipaux[3]);
                strcat (line , ipaux2);
                position += 4;
            }
            else if (strcmp (rdata_type , "MX") == 0) {
                memset (domain , 0 , BUFLEN);
                unsigned short int mx_number;
                memcpy (&mx_number , buffer + position , 2);
                mx_number = ntohs (mx_number);
                memcpy (line + strlen (line), &mx_number , 2);
                strcat (line , "\t");
                sprintf (domain , "%s" , getdomain(buffer , position , domain));
                strcat (line , domain);
                position += 2;
            }
            else if (strcmp (rdata_type , "SOA") == 0) {
                //momentan iau codul de la NS
                //pe scurt sunt 2 adrese si 5 integere
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                if (buffer[position] < 0) {
                   position += 2;
                }
                else if (buffer[position] == 0) {
                 position ++;
                }
                else {
                    //tup-tup-tup pana la pointer :)
                    while (buffer[position] > 0) {
                        position += buffer[position]+1;
                    }
                    position +=2;
                }
                //iau a doua adresa
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                if (buffer[position] < 0) {
                   position += 2;
                }
                else if (buffer[position] == 0) {
                    position ++;
                }
                else {
                    //tup-tup-tup pana la pointer :)
                    while (buffer[position] > 0) {
                        position += buffer[position]+1;
                    }
                    position +=2;
                }
                //iau cele 5 inturi
                int int1 , int2 , int3 , int4 , int5;
                memcpy (&int1 , buffer + position , 4);
                position += 4;
                int1 = ntohl (int1);
                sprintf (line + strlen(line) , "%d\t" , int1);
                memcpy (&int2 , buffer + position , 4);
                position += 4;
                int2 = ntohl (int2);
                sprintf (line + strlen(line) , "%d\t" , int2);
                memcpy (&int3 , buffer + position , 4);
                position += 4;
                int3 = ntohl (int3);
                sprintf (line + strlen(line) , "%d\t" , int3);
                memcpy (&int4 , buffer + position , 4);
                position += 4;
                int4 = ntohl (int4);
                sprintf (line + strlen(line) , "%d\t" , int4);
                memcpy (&int5 , buffer + position , 4);
                position += 4;
                int5 = ntohl (int5);
                sprintf (line + strlen(line) , "%d\t" , int5);
            }
            else if (strcmp (rdata_type , "PTR") == 0) {    //ma voi astepta la un nume
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , " ");
                position += lungime_text+1;
            }
            else if (strcmp (rdata_type , "CNAME") == 0) {    //ma voi astepta la un nume
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , " ");
                position += lungime_text;
            }
            else if (strcmp (rdata_type , "TXT") == 0) {
                strncat (line , buffer , lungime_text);
                position += lungime_text;
            }
            fprintf (LOG, "%s\n", line);
        }
    }
    if (arcount != 0) {
		fprintf (LOG, ";;ADDITIONAL SECTION\n\n");
         for (i=0 ; i<arcount ; i++) {
            memset (line , 0 , BUFLEN);
            memset (domain , 0 , BUFLEN);   //cine stie , poate voi folosi primul nume la ceva
            memset (rdata_type , 0 , 12);
            memset (rdata_class , 0 , 12);
            //NAME
            sprintf (line , "%s\t" ,  getdomain (buffer , position , domain));
            if (buffer[position] < 0) {
                position += 2;
            }
            else if (buffer[position] == 0) {
                position ++;
            }
            else {
                //tup-tup-tup pana la pointer :)
                while (buffer[position] > 0) {
                    position += buffer[position]+1;
                }
                position +=2;
            }
            printf ("position type %d\n", position);
            //TYPE
            sprintf (rdata_type , "%s" , getType (buffer [position + 1]));
            strcat (line ,rdata_type);
            strcat (line , "\t");
            position += 2;
            printf ("position class %d\n", position);
            //CLASS
            sprintf (rdata_class , "%s",  getClass(buffer [position + 1]));
            strcat (line , rdata_class);
            strcat (line , "\t");
            position += 2;
            //TTL
            position += 4;
            //RLENGTH
            lungime_text = buffer[position + 1];
            position += 2;
            //RDATA
            if (strcmp (rdata_type , "NS") == 0) {    //ma voi astepta la un nume
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                position += lungime_text;
            }
            else if (strcmp (rdata_type , "A") == 0) {   //ma voi astepta la un ip
                //RLENGHT TREBUIE sa fie 4
                int ipaux[10];
                char ipaux2[10];
                if (buffer[position] < 0) ipaux[0] = buffer[position] + 256;
                else ipaux[0] = buffer[position];

                if (buffer[position + 1] < 0) ipaux[1] = buffer[position + 1] + 256;
                else ipaux[1] = buffer[position + 1];

                if (buffer[position + 2] < 0) ipaux[2] = buffer[position + 2] + 256;
                else ipaux[2] = buffer[position + 2];

                if (buffer[position + 3] < 0) ipaux[3] = buffer[position + 3] + 256;
                else ipaux[3] = buffer[position + 3];
                sprintf (ipaux2 , "%d.%d.%d.%d"
                            , ipaux[0]
                            , ipaux[1]
                            , ipaux[2]
                            , ipaux[3]);
                strcat (line , ipaux2);
                position += 4;
            }
            else if (strcmp (rdata_type , "MX") == 0) {
                memset (domain , 0 , BUFLEN);
                unsigned short int mx_number;
                memcpy (&mx_number , buffer + position , 2);
                mx_number = ntohs (mx_number);
                memcpy (line + strlen (line), &mx_number , 2);
                strcat (line , "\t");
                sprintf (domain , "%s" , getdomain(buffer , position , domain));
                strcat (line , domain);
                position += 2;
            }
            else if (strcmp (rdata_type , "SOA") == 0) {
                //iau codul de la NS
                //pe scurt sunt 2 adrese si 5 integere
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                if (buffer[position] < 0) {
                   position += 2;
                }
                else if (buffer[position] == 0) {
                 position ++;
                }
                else {
                    while (buffer[position] > 0) {
                        position += buffer[position]+1;
                    }
                    position +=2;
                }
                //iau a doua adresa
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                if (buffer[position] < 0) {
                   position += 2;
                }
                else if (buffer[position] == 0) {
                    position ++;
                }
                else {
                    while (buffer[position] > 0) {
                        position += buffer[position]+1;
                    }
                    position +=2;
                }
                //iau cele 5 inturi
                int int1 , int2 , int3 , int4 , int5;
                memcpy (&int1 , buffer + position , 4);
                position += 4;
                int1 = ntohl (int1);
                sprintf (line + strlen(line) , "%d\t" , int1);
                memcpy (&int2 , buffer + position , 4);
                position += 4;
                int2 = ntohl (int2);
                sprintf (line + strlen(line) , "%d\t" , int2);
                memcpy (&int3 , buffer + position , 4);
                position += 4;
                int3 = ntohl (int3);
                sprintf (line + strlen(line) , "%d\t" , int3);
                memcpy (&int4 , buffer + position , 4);
                position += 4;
                int4 = ntohl (int4);
                sprintf (line + strlen(line) , "%d\t" , int4);
                memcpy (&int5 , buffer + position , 4);
                position += 4;
                int5 = ntohl (int5);
                sprintf (line + strlen(line) , "%d\t" , int5);
            }
            else if (strcmp (rdata_type , "PTR") == 0) {    //ma voi astepta la un nume
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                position += lungime_text+1;
            }
            else if (strcmp (rdata_type , "CNAME") == 0) {    //ma voi astepta la un nume
                memset (domain , 0 , BUFLEN);
                strcat (line , getdomain(buffer , position , domain));
                strcat (line , "\t");
                position += lungime_text;
            }
            else if (strcmp (rdata_type , "TXT") == 0) {
                strncat (line , buffer , lungime_text);
                position += lungime_text ;
            }
            fprintf (LOG, "%s\n", line);
        }
    }
    close (server_socket);
    fclose (fp);
	return 0;
}
