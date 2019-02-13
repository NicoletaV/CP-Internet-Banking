/* VINA NICOLETA 325CD */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <algorithm>

#define MAX_CLIENTS	10
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

/* structura ce retine datele unui client */
typedef struct info_client {
	char nume[13], prenume[13];
	int numar_card, pin;
	char parola_secreta[9];
	double sold;
	int blocat, i;
} info_client;

int main(int argc, char *argv[]) {

	int sockfd, newsockfd, portno; // clilen;
	socklen_t clilen;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, j;

	/* multimile pt select */
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;	

	if (argc < 2) {

		fprintf(stderr,"Utilizare %s port fisier\n", argv[0]);
		exit(1);
	}

	/* se golesc multile de descriptori */ 
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
     
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error((char *)"EROARE deschidere socket");		
	}

	portno = atoi(argv[1]);

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
		error((char *)"EROARE la bind");
	}

	listen(sockfd, MAX_CLIENTS);

	/* se adauga noul file descriptor in read_fds */
	FD_SET(sockfd, &read_fds);

	FD_SET(0, &read_fds);
	fdmax = sockfd;

	FILE *in = fopen(argv[2], "r");
	if (in == NULL) {
		fprintf(stderr, "Eroare deschidere fisier %s\n", argv[2]);
		exit(1);
	}

	fscanf(in, "%d", &n);

	/* se citesc si stocheaza informatiile din fisier intr-un vector de structuri */
	info_client *informatii = (info_client *) malloc (n * sizeof(info_client));

	for (i = 0; i < n; i++) {

		fscanf(in, "%s", informatii[i].nume);
		fscanf(in, "%s", informatii[i].prenume);
		fscanf(in, "%d", &informatii[i].numar_card);
		fscanf(in, "%d", &informatii[i].pin);
		fscanf(in, "%s", informatii[i].parola_secreta);
		fscanf(in, "%lf", &informatii[i].sold);
		informatii[i].blocat = 2;
		informatii[i].i = -1;
	}

	fclose(in);

	std::vector<int> sesiuni_active;

	while (1) {

		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
			error((char *)"EROARE in select");
		}

		for(i = 0; i <= fdmax; i++) {
	
			if (FD_ISSET(i, &tmp_fds)) {

				if (i == sockfd) {
					/* a venit ceva pe socketul inactiv (cel cu listen) = o noua
					conexiune, serverul va da accept */
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(i, (struct sockaddr *)&cli_addr,
						&clilen)) == -1) {
						error((char *)"EROARE in accept");
					}
											
					/* se adauga noul socket */
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					printf("Noua conexiune de la %s, port %d, socket_client %d\n ",
						inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
						newsockfd);

				} else if (i == 0) {

					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN, stdin);

					/* de la stdin se poate citi doar quit, caz in care se inchid
					toti clientii deschisi */
					if(strncmp(buffer, "quit", 4) == 0) {

						for (int k = 4; k <= fdmax; k++) {
							if (FD_ISSET(k, &read_fds)) {
								char special[25];
								strcpy(special, "Serverul se va inchide\0");
								int nq = send(k, special, strlen(special), 0);
								if (nq < 0) {
									error((char*)"Raspuns netransmis");
								}
								close(k); 
								FD_CLR(k, &read_fds);
							}
						}

						close(sockfd);
						return 0;
					}
				} else {

					/* se primesc date pe socketii cu care se comunica cu clientii
					si serverul va face recv() */
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("Socket %d a inchis\n", i);
						} else {
							error((char *)"ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds);
					} else {
						
						char buffer2[BUFLEN];
						memset(buffer2, '\0', sizeof(buffer2));
						strcpy(buffer2, buffer);

						char **splitted = cuvinte(buffer2);
						char *word = splitted[0];
						if (strcmp(word, "unlock") != 0) {
							/* daca a primit comanda login */
							if (strcmp(word, "login") == 0) {

								int numar_card, pin;
								numar_card = atoi(splitted[1]);
								pin = atoi(splitted[2]);

								std::string detrimis("");

								/* daca e deja in sesiuni_active, da eroarea -2 */
								if (std::find(sesiuni_active.begin(),
									sesiuni_active.end(), numar_card)
									!= sesiuni_active.end()) {
									
									detrimis += "-2 " + std::string(buffer) + "\0";
									int n2 = send(i, detrimis.c_str(),
											strlen(detrimis.c_str()), 0);
									if (n2 < 0) {
										perror((char*)"Raspuns netransmis");
									}
								} else {
									
									/* daca il gaseste si e blocat, da eroarea -5 */
									bool found = false;
									for (j = 0; j < n; j++) {

										if (informatii[j].numar_card == numar_card
											&& informatii[j].blocat == 0) {
											found = true;
											detrimis += "-5 " + std::string(buffer)
														+ "\0";
											int n5 = send(i, detrimis.c_str(),
													strlen(detrimis.c_str()), 0);
											if (n5 < 0) {
												error((char*)"Raspuns netransmis");
											}
											break;
										}

										/* daca sunt corecte datele primite, se
										face login cu succes si se adauga nr_card
										in sesiuni_active, resetandu-se nr de 
										incercari pt card blocat (blocat = 2) si
										actualizandu-se campul i cu socketul actual */
										if (informatii[j].numar_card == numar_card) {
											found = true;
											if (informatii[j].pin == pin) {
												if (FD_ISSET(i, &tmp_fds)
													&& (i > 3)) {
													
													sesiuni_active.push_back(numar_card);
													informatii[j].i = i;
													informatii[j].blocat = 2;
													detrimis += "0 Welcome "
													+ std::string(informatii[j].nume)
													+ " "
													+ std::string(informatii[j].prenume)
													+ " " + std::string(buffer)
													+ "\0";

													int n0 = send(i, detrimis.c_str(),
														strlen(detrimis.c_str()),
														0); //e corect
													if (n0 < 0) {
														perror((char*)"Raspuns netransmis");
													}
												}
												break;
											} else {
												if (FD_ISSET(i, &tmp_fds)
													&& (i > 3)) { 
													
													/* scade nr de incercari de
													logare si se trimite eroarea
													-3 in cazul pinului gresit */
													informatii[j].blocat--;
													detrimis += "-3 " + std::string(buffer)
															+ "\0";
													int n3 = send(i, detrimis.c_str(),
														strlen(detrimis.c_str()),
														0);

													if (n3 < 0) {
														error((char*)"Raspuns netransmis");
													}
												}
												break;
											}
										}
									} 
									if (!found) {
										/* daca nu s-a gasit printre cleintii
										disponibili, inseamna ca nr_card e gresit */
										if (FD_ISSET(i, &tmp_fds) && (i > 3)) { 

											detrimis += "-4 " + std::string(buffer)
													+ "\0";
											int n4 = send(i, detrimis.c_str(),
													strlen(detrimis.c_str()), 0);
											if (n4 < 0) {
												error((char*)"Raspuns netransmis");
											}
										}
										break;
									}
									
								}
							} else if (strcmp(word, "logout") == 0) {
								if (FD_ISSET(i, &tmp_fds) && (i > 3)) {
									std::string detrimis("");

									/* la logout se face din nou campul i = -1
									si cardul se scoate din sesiuni_active */
									for (j = 0; j < n; j++) {
										if (informatii[j].i == i) {
											informatii[j].i = -1;

											std::vector<int>::iterator pos = 
											std::find(sesiuni_active.begin(),
												sesiuni_active.end(),
												informatii[j].numar_card);

											if (pos != sesiuni_active.end()) {
												sesiuni_active.erase(pos);
											}

											detrimis += "1\0";
											int n11 = send(i, detrimis.c_str(),
												strlen(detrimis.c_str()), 0);

											if (n11 < 0) {
												error((char*)"Raspuns netransmis");
											}
											break;
										}
									}

									if (detrimis.empty()) {
										detrimis += "-1\0";
										int n11 = send(i, detrimis.c_str(),
											strlen(detrimis.c_str()), 0);

										if (n11 < 0) {
											error((char*)"Raspuns netransmis");
										}
									}
									break;
								}
							} else if (strcmp(word, "listsold") == 0) {
								/* se cauta cardul cu socketul i primit si se
								trimite soldul acestuia */
								std::string detrimis("");
								for (j = 0; j < n; j++) {
									if (informatii[j].i == i) {
										if (informatii[j].i > 0
											&& std::find(sesiuni_active.begin(),
											sesiuni_active.end(),
											informatii[j].numar_card) !=
											sesiuni_active.end()) {

											detrimis += "2 "
												+ std::to_string(informatii[j].sold)
												+ "\0";
											break;
										}
									}
								}

								if (detrimis.empty()) {
									detrimis += "-11\0";
								}

								int n22 = send(i, detrimis.c_str(),
									strlen(detrimis.c_str()), 0);
								if (n22 < 0) {
									error((char*)"Raspuns netransmis");
								}
								break;
							} else if (strcmp(word, "transfer") == 0) {
								std::string detrimis("");
								int numar_card = atoi(splitted[1]); 
								bool found = false;
								for (j = 0; j < n; j++) {
									if (informatii[j].numar_card == numar_card) {
										found = true;
										break;
									}
								}

								if (!found) {
									detrimis += "4 " + std::string(splitted[0])
											+ " " +	std::string(splitted[1])
											+ " " +	std::string(splitted[2])
											+ "\0";

									int n44 = send(i, detrimis.c_str(),
										strlen(detrimis.c_str()), 0);

									if (n44 < 0) {
										error((char*)"Raspuns netransmis");
									}
									break;
								} else {

									double suma_sursa;
									for (j = 0; j < n; j++) {
										if (informatii[j].i == i) {
											suma_sursa = informatii[j].sold;

											/* se transmite -8 daca nu sunt fonduri
											suficiente pt transferul propus */
											char decimals[BUFLEN];
											memset(decimals, '\0', sizeof(decimals));
											sprintf(decimals, "%.2lf", atof(splitted[2]));
											if (atof(decimals) > suma_sursa) {
												detrimis += "-8 "
													+ std::string(splitted[0])
													+ " " +	std::string(splitted[1])
													+ " " +	std::string(splitted[2])
													+ "\0";

												int n8 = send(i, detrimis.c_str(),
													strlen(detrimis.c_str()), 0);

												if (n8 < 0) {
													error((char*)"Raspuns netransmis");
												}

											} else {

												/* se trimit inapoi clientului
												datele necesare */
												for (int k = 0; k < n; k++) {
													if (informatii[k].numar_card
														== numar_card) {

														detrimis += "y "
														+ std::string(splitted[1])
														+ " "
														+ std::to_string(atof(decimals))
														+ " "
														+ std::string(informatii[k].nume)
														+ " "
														+ std::string(informatii[k].prenume)
														+ "\0";
														break;
													}
												}

												
												int ny = send(i, detrimis.c_str(),
													strlen(detrimis.c_str()), 0);

												if (ny < 0) {
													error((char*)"Raspuns netransmis");
												}

												char buffer3[BUFLEN];
												memset(buffer3, 0, BUFLEN);
												int np = recv(i, buffer3,
													sizeof(buffer3), 0);
												if (np < 0) {
													error((char*)"Raspuns neprimit");
												}

												/* daca nu se primeste 'y', se
												trimite -9 */
												std::string trimite("");
												if (buffer3[0] != 'y') {
													trimite += "-9 "
														+ std::string(buffer3)
														+ "\0";

												} else {

													/* daca transferul se face,
													se scade soldul clientului cu
													suma din comanda si se creste
													soldul destinatarului */
													informatii[j].sold -=
													atof(splitted[2]);

													for (int l = 0; l < n; l++) {
														if (informatii[l].numar_card
															== numar_card) {

															informatii[l].sold +=
															atof(splitted[2]);

															break;
														}
													}

													trimite += "9 "
														+ std::string(buffer3)
														+ "\0";
												}

												int n9 = send(i, trimite.c_str(),
													strlen(trimite.c_str()), 0);

												if (n9 < 0) {
													error((char*)"Raspuns netransmis");
												}
											}

											break;
										}
									}
									break;
								}
							} else if (strcmp(word, "quit") == 0) {

								/* daca un client da quit, e scos din sesiuni_active
								daca exista si din multimea read_fds */
								for (j = 0; j < n; j++) {
									if (informatii[j].i == i) {
										informatii[j].i = -1;
										std::vector<int>::iterator pos
											= std::find(sesiuni_active.begin(),
											sesiuni_active.end(),
											informatii[j].numar_card);

										if (pos != sesiuni_active.end()) {
											sesiuni_active.erase(pos);
										}
									}
								}

								close(i);
								FD_CLR(i, &read_fds);
								break;
							}
						}
					}
				}
			} 
		}
	}

	close(sockfd);
	return 0;
}