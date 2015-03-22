Readme tema 3 PC - Sistem de partajare a fisierelor

@author: Jalba Cristian 323 CA

Implementarea temei a pornit de la scheletul de cod al laboratorului 8 , cel de
multiplexare. Spre deosebire de acel laborator , clientii actioneaza aici ca si
servere deoarece ei trebuie sa dea / primeasca informatii din fisiere direct 
catre alti clienti . 
In principiu am respectat cerintele temei, desi trebuie sa mentionez cateva 
defecte pe care le are momentan programul :

1. Atunci cand primeste getshare pe un client care nu a sharuit nimic , serverul 
crapa (isi da segfault) deoarece parcurge lista de fisiere, nu gaseste fisier 
activ si face strncmp sau o functie de stringuri pe un pointer NULL

2. Am lasat in bytes dimensiunea fisierelor 

3. Ora la care se conecteaza un client e defapt data + ora

Getfile l-am conceput astfel incat clientul sa dea un request la server , 
serverul sa caute in baza sa de date clientul care are fisierul si sa retina 
datele clientului care are nevoie de fisier . Apoi serverul ii trimite clientului
cu fisier datele (fie A in acest exemplu) clientului fara (il vom numi B) .
A se conecteaza apoi la B si ii trimite un chunk de informatii (adica 1024B).
Exista un flag care  "semnaleaza" daca mai este necesar sa fie transmise informatii
(sunt defapt 2 flaguri , acestea se afla in client_out respectiv client_in, 
le puteti gasi la inceputul while-ului principal din client.c).

Din pacate sunt niste probleme cu conectivitatea intre clienti , nu am reusit
sa ii fac sa comunice intre ei fara intermediul serverului . Din acest motiv  
getfile nu e terminat . 

Ar fi fost mult mai usor sa lucrez in C++ dar nu am reusit sa adaptez scheletul 
de cod pentru C++ . 
