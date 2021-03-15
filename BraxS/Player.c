
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
    int sd;			// descriptorul de socket
    struct sockaddr_in server;	// structura folosita pentru conectare


    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons (port);

    /* ne conectam la server */
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("[client]Eroare la connect().\n");
        return errno;
    }

    /* trimitere username */
    char nume[100];
    bzero (nume, 100);
    printf ("Pentru a incepe jocul trebuie sa introduceti un username: ");
    fflush (stdout);
    read (0, nume, 100);

    if (write (sd, nume, strlen(nume)) <= 0)
    {
        perror ("[client]Eroare la write( trimiterea username-ului ) spre server.\n");
        return errno;
    }
    bool over=0;
    // dupa ce a intrat, mic regulament, formatul mutarii, dc poti primi o alta culoare
    printf("Reguli brax:\n"
           "- mutarile pot fi facute in directiile sus-jos / stanga -dreapta;\n"
           "- o mutare poate fi peste 2 casute daca sunt de aceeasi culoare cu pionii alesi, altfel este permisa o singura casuta;\n"
           "- poti spune brax atunci cand ameninti cu piesa ta o piesa a adversarului si acesta va fi nevoit sa o mute, astfel incat sa nu mai fie amenintata;\n"
           "- poti captura o piesa daca te asezi deasupra ei;\n"
           "- nu pot exista 2 piese in aceeasi casuta, chiar daca au aceeasi culoare.\n"
           "Iti poti alege culoarea pionilor. In cazul in care ambii jucatori aleg aceeasi culoare vor fi atribuite dupa ordinea conectarii.\n"
           "Tabla contine pozitiile casutelor, pozitiile pieselor si culorile drumurilor : 78 este o casuta, 88 drum de culoare rosie, 99 drum de culoare neagra\n"
           "1408801 - piesa rosie cu numarul 1 este pe casuta 14.\n"
           "Mutarile transmise trebuie sa aiba urmatorul format: 1408801-22-0 => primul numar piesa pe care doresti sa o muti, al doilea numar reprezinta unde doresti sa o muti si\n"
           "ultimul numar 0 - daca nu spui brax / 1 - daca spui brax.\n "
    );
    // bucla pentru mai multe jocuri
    while(!over) {
        /*trimitere culoare*/
        char color[6];
        printf("Incepe jocul!\n");
        while (1) {
            printf("Alege culoarea cu care vrei sa joci [negru/rosu]: ");
            fflush(stdout);
            bzero(color, 6);
            read(0, color, 6);
            if ((strncmp(color, "negru", 5) == 0) || (strncmp(color, "rosu", 4) == 0)) break;
            else {
                printf("Ati introdus o culoare gresita!\n");
            }
        }
        if (write(sd, color, strlen(color)) <= 0) {
            perror("[client]Eroare la write( trimiterea culorii ) spre server.\n");
            return errno;
        }
        // primesc culoarea mea + afisare
        bzero(color,6);
        if (read(sd, color, 6) < 0) {
            perror("[client]Eroare la read( rezultat culoare ) de la server.\n");
            return errno;
        }
        printf("Culoarea pieselor va fi: %s. \n", color);
        fflush(stdout);

        // bucla pentru joc'
        bool joc = 0;
        char tabla[1024];
        while (!joc) {
            // primeste tabla de joc
            bzero(tabla, 1024);
            if (read(sd, tabla, 1024) < 0) {
                perror("[client]Eroare la read( tabla de joc ) de la server.\n");
                return errno;
            }
            if(strncmp(tabla,"0", strlen("0"))==0) {printf("Jocul s-a incheiat. Din pacate, ai pierdut!\n"); break; }
            /* afisam tabla */
            printf("Tabla de joc:\n %s\n", tabla);

            /*trimitere mutare*/
            char mutare[18];
            while (1) {
                printf("Introduceti mutarea: ");
                fflush(stdout);
                bzero(mutare, 10);

                read(0, mutare, 10);

                /* trimiterea mutarii la server */
                if (write(sd, mutare, strlen(mutare)) <= 0) {
                    perror("[client]Eroare la write( trimiterea mutarii ) spre server.\n");
                    return errno;
                }

                //verific daca mutarea a fost corecta sau nu
                printf("[client] Sunt inainte de read!\n");
                fflush(stdout);
                char correct_move[100];
                bzero(correct_move, 100);
                if (read(sd, correct_move, 100) < 0) {
                    perror("[client]Eroare la read( corectitudine mutare ) de la server.\n");
                    return errno;
                }
                //afisez daca raspunsul este corect sau nu
                printf("[client]Mesajul primit este: %s\n", correct_move);
                fflush(stdout);

                if (strncmp(correct_move, "Mutarea a fost corecta.\n", strlen("Mutarea a fost corecta.\n")) == 0)
                    break;
            }



            // final / continuare joc
            char game_state[100];
            bzero(game_state, 100);
            if (read(sd, game_state, 100) < 0) {
                perror("[client]Eroare la read( final/continuare joc ) de la server.\n");
                return errno;
            }
            if (strncmp(game_state, "1", strlen("1")) == 0) {
                printf("Jocul s-a incheiat. Felicitari, ai castigat!\n");
                fflush(stdout);

                joc = 1;
            } else if (strncmp(game_state, "0", strlen("0")) == 0) {
                printf("Jocul s-a incheiat. Din pacate, ai pierdut!\n");
                fflush(stdout);

                joc = 1;
            } else if (strncmp(game_state, "2", strlen("2")) == 0) {
                printf("Jocul continua! Este randul oponentului!\n");
                fflush(stdout);

            }
        }

        //vizualizare clasament
        printf("Doresti sa vezi clasamentul?[da/nu]\n");
        fflush(stdout);
        char vis_rank[3];
        bzero(vis_rank, 3);
        read(0, vis_rank, 3);
        if (write(sd, vis_rank, strlen(vis_rank)) <= 0) {
            perror("[client]Eroare la write( viz clasament ) spre server.\n");
            return errno;
        }
        if (strncmp(vis_rank, "da", strlen("da")) == 0) { // daca vrea sa primeasca, asteptam raspunsul de la server
            char rank[200];
            bzero(rank, 200);
            if (read(sd, rank, 200) < 0) {
                perror("[client]Eroare la read( clasament ) de la server.\n");
                return errno;
            }
        }

        // inca o runda
        printf("Doresti sa mai joci o data?[da/nu]\n");
        fflush(stdout);
        char next_round[3];
        bzero(next_round, 3);
        read(0, next_round, 3);
        if (write(sd, next_round, 3) <= 0) {
            perror("[client]Eroare la write( inca o runda ) spre server.\n");
            return errno;
        }
        if (strncmp(next_round, "da", strlen("da")) ==
            0) { // daca vrea sa continue, asteptam sa vedem daca si celalalt jucator vrea sa continue
            char answer_opp[3];
            bzero(answer_opp, 3);
            if (read(sd, answer_opp, 3) < 0) {
                perror("[client]Eroare la read( se va mai juca o tura ) de la server.\n");
                return errno;
            }
            if (strncmp(answer_opp, "0", strlen("0")) == 0)
            { over = 1; printf("Oponentul nu doreste sa joace o noua runda.\n"); fflush(stdout);}
        } else over = 1;
    }
    //cand nu mai doreste sa joace se incheie conexiunea cu severul
    close (sd);
}