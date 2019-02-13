/* VINA NICOLETA 325CD */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <algorithm>

#define BUFLEN 256

using namespace std;

void error(char *msg) {
    
    perror(msg);
    exit(1);
}

/* functie ce returneaza un vector de char-uri, dupa ce face split pe sirul
primit ca parametru */
char **cuvinte(char *str) {

	if (str == NULL) {
		return NULL;
	}

	char **rez = (char **)calloc(10, sizeof(char *));
	char *p;
	int i = 0;
	p = strtok(str, " \n");
	while(p != NULL) {
		rez[i] = p;
		strcat(rez[i], "\0");
		i++;
		p = strtok(NULL, " \n");
	}

	return rez;
}

/* trateaza majoritatea codurilor de eroare */
string cod_eroare(char *cod) {

	char *msj = strtok(cod, " \n");
	std::string mesaj_eroare("");
	if (strcmp(msj, "-3") == 0) {
		return mesaj_eroare += "IBANK> -3 : Pin gresit\0";  
	}

	if (strcmp(msj, "-4") == 0) {
		return mesaj_eroare += "IBANK> -4 : Numar card inexistent\0";  
	}

	if (strcmp(msj, "-2") == 0) {
		return mesaj_eroare += "IBANK> -2 : Sesiune deja deschisa\0";  
	}

	if (strcmp(msj, "-5") == 0) {
		return mesaj_eroare += "IBANK> -5 : Card blocat\0";  
	}

	if (strcmp(msj, "-8") == 0) {
		return mesaj_eroare += "IBANK> -8 : Fonduri insuficiente\0";  
	}

	if (strcmp(msj, "0") == 0) {
		int trei = 3;
		mesaj_eroare += "IBANK> ";
		while (trei > 0) {
			msj = strtok(NULL, " \n");
			mesaj_eroare += std::string(msj) + " ";
			trei--;
		}
		return mesaj_eroare += "\0";
	}

	return "";
}

int main(int argc, char *argv[]) {
    
    int sockfd, n;
    struct sockaddr_in serv_addr;

    char buffer[BUFLEN];
    if (argc < 3) {

       fprintf(stderr,"Utilizare %s adresa_server port_server\n", argv[0]);
       exit(1);
    }  
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) {
        error((char *)"EROARE deschidere socket");
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);    
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        error((char *)"EROARE la conectare");   
    }

	fd_set read_fds, tmp_fds;	
    
	FD_ZERO(&read_fds);     
	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
 
	int fdmax = sockfd, i;
	int elogat = 0;

	/* creare nume fisier de log pt fiecare client */
	std::string filename("client-");
	filename += std::to_string(getpid()) + ".log\0";

	FILE *log;

	while (1) {

		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
			error((char *)"EROARE in select");
		}

		for (i = 0; i <= fdmax; i++) {

			if (FD_ISSET(i, &tmp_fds)) {

				/* se citeste de la tastatura */
				if (i == 0) {
					
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					char buffer2[BUFLEN];
					memset(buffer2, '\0', sizeof(buffer2));
					strcpy(buffer2, buffer);

					char **splitted = cuvinte(buffer2);
					char *word = splitted[0];

					if (strcmp(word, "unlock") != 0) {
						if (strcmp(word, "login") == 0) {

							log = fopen(filename.c_str(), "at");
							if (log == NULL) {
								error((char *)"EROARE deschidere log");
							}

							/* daca e deja logat, da eroare -2 */
							if (elogat == 1) {
								
								char aux[50];
								strcpy(aux, "IBANK> -2 : Sesiune deja deschisa");
								fprintf(log, "%s", buffer);
								fprintf(log, "%s\n", aux);
								printf("%s\n", aux);
								fclose(log);
								break;
							}
						}

						if (strcmp(word, "logout") == 0) {

							log = fopen(filename.c_str(), "at");
							if (log == NULL) {
								error((char *)"EROARE deschidere log");
							}

							memset(buffer2, '\0', sizeof(buffer2));
							strcpy(buffer2, buffer);

							/* daca nu e logat deja, da eroare -1 */
							if (elogat == 0) {
								
								fprintf(log, "%s", buffer2);
								char aux[50];
								strcpy(aux, "IBANK> -1 : Clientul nu este autentificat");
								fprintf(log, "%s\n", aux);
								printf("%s\n", aux);
								fclose(log);
								continue;
							}
						}

						if (strcmp(word, "listsold") == 0) {

							log = fopen(filename.c_str(), "at");
							if (log == NULL) {
								error((char *)"EROARE deschidere log");
							}

							memset(buffer2, '\0', sizeof(buffer2));
							strcpy(buffer2, buffer);

							/* daca nu e logat deja, da eroare -1 */
							if (elogat == 0) {
								
								fprintf(log, "%s", buffer2);
								char aux[50];
								strcpy(aux, "IBANK> -1 : Clientul nu este autentificat");								
								fprintf(log, "%s\n", aux);
								printf("%s\n", aux);
								fclose(log);
								continue;
							}
						}

						if (strcmp(word, "transfer") == 0) {

							log = fopen(filename.c_str(), "at");
							if (log == NULL) {
								error((char *)"EROARE deschidere log");
							}

							memset(buffer2, '\0', sizeof(buffer2));
							strcpy(buffer2, buffer);

							/* daca nu e logat deja, da eroare -1 */
							if (elogat == 0) {
								
								fprintf(log, "%s", buffer2);
								char aux[50];
								strcpy(aux, "IBANK> -1 : Clientul nu este autentificat");
								fprintf(log, "%s\n", aux);
								printf("%s\n", aux);
								fclose(log);
								continue;
							}
						}

						if (strcmp(word, "quit") == 0) {

							log = fopen(filename.c_str(), "at");
							if (log == NULL) {
								error((char *)"EROARE deschidere log");
							}

							/* scrie comanda in fisier */
							fprintf(log, "%s\n", "IBANK> quit");
							fclose(log);

							n = send(sockfd, "quit\0", strlen("quit\0"), 0); //trimit msj la server
							if (n < 0) {
								error((char *)"EROARE scriere socket");
							}
							close(sockfd);
							return 0;
						}

						/* trimite serverului ce comanda a primit de la tastatura */
						n = send(sockfd, buffer, strlen(buffer), 0); //trimit msj la server
						if (n < 0) {
							error((char *)"EROARE scriere socket");
						}
					}

				} else {

					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					if (n < 0) {
						error((char *)"EROARE in recv");
					}

					if (n == 0) {
						close(i);
						FD_CLR(i, &read_fds);
						return -1;
					}

					char buffer2[BUFLEN];
					memset(buffer2, '\0', sizeof(buffer2));
					strcpy(buffer2, buffer);

					char **splitted = cuvinte(buffer2);
					char *cod = splitted[0];

					/* acest tip de mesaj este de instiintare cand serverul se
					va inchide */
					if (strcmp(cod, "Serverul") == 0) {
						elogat = 0;
						printf("%s\n", buffer);
						close(sockfd);
    					return 0;
					}

					if (strcmp(cod, "-3") == 0 || strcmp(cod, "-4") == 0 ||
						strcmp(cod, "0") == 0 || strcmp(cod, "-2") == 0 ||
						strcmp(cod, "-5") == 0) {

						log = fopen(filename.c_str(), "at");
						if (log == NULL) {
							error((char *)"EROARE deschidere log");
						}

						string buf = std::string(buffer);
						std::string s = cod_eroare(buffer);
						printf("%s\n", s.c_str());

						size_t pos = buf.find("login");
						string comanda = buf.substr(pos);

						char char_comanda[100];
						memset(char_comanda, '\0', sizeof(char_comanda));
						strcpy(char_comanda, comanda.c_str());

						/* scrie comanda (refacuta) si rezultatul ei in fisier 
						si la stdout */
						fprintf(log, "%s", char_comanda);
						fprintf(log, "%s\n", s.c_str());

						fclose(log);

						if (strcmp(cod, "0") == 0) {
							elogat = 1;							
						} else {
							elogat = 0;
						}

					} else if (strcmp(cod, "-1") == 0 || strcmp(cod, "1") == 0) {
						
						log = fopen(filename.c_str(), "at");
						if (log == NULL) {
							error((char *)"EROARE deschidere log");
						}
						fprintf(log, "%s\n", "logout\0");

						if (strcmp(cod, "-1") == 0) {

							char aux[50];
							strcpy(aux, "IBANK> -1 : Clientul nu este autentificat");
							fprintf(log, "%s\n", aux);
							printf("%s\n", aux);
						} else {

							char aux[50];
							strcpy(aux, "IBANK> Clientul a fost deconectat");
							fprintf(log, "%s\n", aux);
							printf("%s\n", aux);
							elogat = 0;
						}
						
						fclose(log);

					} else if (strcmp(cod, "-11") == 0 || strcmp(cod, "2") == 0) {

						log = fopen(filename.c_str(), "at");
						if (log == NULL) {
							error((char *)"EROARE deschidere log");
						}
						fprintf(log, "%s\n", "listsold\0");

						if (strcmp(cod, "-11") == 0) {

							fprintf(log, "%s\n", "IBANK> -6 : Operatie esuata");
							printf("%s\n", "IBANK> -6 : Operatie esuata");
						} else {

							char rez[BUFLEN];
							memset(rez, '\0', sizeof(rez));
							sprintf(rez, "IBANK> %.2lf", atof(splitted[1]));
							fprintf(log, "%s\n", rez);
							printf("%s\n", rez);
						}

						fclose(log);

					} else if (strcmp(cod, "4") == 0 || strcmp(cod, "-8") == 0) {

						log = fopen(filename.c_str(), "at");
						if (log == NULL) {
							error((char *)"EROARE deschidere log");
						}

						char err[5];

						if (strcmp(cod, "4") == 0) {
							strcpy(err, "-4\0");
						} else {
							strcpy(err, "-8\0");
						}

						string msg = cod_eroare(err);
						printf("%s\n", msg.c_str());

						string buf = std::string(buffer);
						size_t pos = buf.find("transfer");
						string comanda = buf.substr(pos);
						fprintf(log, "%s\n", comanda.c_str());
						fprintf(log, "%s\n", msg.c_str());

						fclose(log);

					} else if (strcmp(cod, "y") == 0) {

						log = fopen(filename.c_str(), "at");
						if (log == NULL) {
							error((char *)"EROARE deschidere log");
						}

						char decimals[BUFLEN];
						memset(decimals, '\0', sizeof(decimals));
						sprintf(decimals, "%.2lf", atof(splitted[2]));
						string comanda = "";
						comanda += "transfer " + std::string(splitted[1]) + " "
								+ std::string(decimals) +"\0";

						fprintf(log, "%s\n", comanda.c_str());

						string msg = "IBANK> Transfer " + std::string(decimals)
									+ " " +	"catre " + std::string(splitted[3])
									+ " " +	std::string(splitted[4]) + "? [y/n]\0";
						fprintf(log, "%s\n", msg.c_str());
						printf("%s\n", msg.c_str());

						fclose(log);

						char buffer3[BUFLEN];
						memset(buffer3, 0, BUFLEN);
						fgets(buffer3, BUFLEN - 1, stdin);

						n = send(sockfd, buffer3, strlen(buffer3), 0);
						if (n < 0) {
							error((char *)"EROARE scriere socket");
						}

						memset(buffer3, 0, BUFLEN);
						n = recv(i, buffer3, sizeof(buffer3), 0);
						if (n < 0) {
							error((char *)"EROARE in recv");
						}

						char **spl = cuvinte(buffer3);

						log = fopen(filename.c_str(), "at");
						if (log == NULL) {
							error((char *)"EROARE deschidere log");
						}

						fprintf(log, "%s\n", spl[1]);
						
						if (strcmp(spl[0], "-9") == 0) {

							fprintf(log, "%s\n", "IBANK> -9 : Operatie anulata");
							printf("%s\n", "IBANK> -9 : Operatie anulata");
						} else {

							fprintf(log, "%s\n", "IBANK> Transfer realizat cu succes");
							printf("%s\n", "IBANK> Transfer realizat cu succes");
						}
						fclose(log);
					}
				}
			}
		}
    }

	fclose(log);
	close(sockfd);
    return 0;
}


