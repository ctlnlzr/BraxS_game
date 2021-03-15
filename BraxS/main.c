#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>
#include <stdbool.h>
#include <sqlite3.h>
/* portul folosit */
#define PORT 2025


/* codul de eroare returnat de anumite apeluri */
extern int errno;

void create_ranking() {
    sqlite3 *data_b;
    char *create = "CREATE TABLE IF NOT EXISTS Ranking ("
                   "ID INTEGER,"
                   "Username TEXT PRIMARY KEY,"
                   "Victories INTEGER,"
                   "Defeats INTEGER)";

    int error = sqlite3_open("/home/lazarcatalina/CLionProjects/BraxS/Brax.db", &data_b);

    if (error != SQLITE_OK) {
        perror("Eroare la deschiderea bazei de date!\n");
        sqlite3_close(data_b);
    }
    char *err_msg = 0;

    error = sqlite3_exec(data_b, create, 0, 0, &err_msg);

    if (error != SQLITE_OK) {
        fprintf(stderr, "Failed to exec step: %s\n", sqlite3_errmsg(data_b));
    }

    sqlite3_close(data_b);
}

int counter = 0;

/*Cere ambilor clienti username-ul si daca nu exista, introduce in baza de date un tuplu pentru acest username.*/
void login(int players[2], char username[2][100]) {
/*cer username pentru fiecare*/
    sqlite3 *data_b;
    char msg[100];
    int error = sqlite3_open("/home/lazarcatalina/CLionProjects/BraxS/Brax.db", &data_b);

    if (error != SQLITE_OK) {
        perror("Eroare la deschiderea bazei de date!\n");
        sqlite3_close(data_b);
    }
    for (int i = 0; i < 2; i++) {
        int ok = 0;
        bzero(msg, 100);
        printf("[server]Asteptam mesajul...\n");
        fflush(stdout);

        /* citirea mesajului */
        if (read(players[i], msg, 100) <= 0) {
            perror("[server]Eroare la read( la citirea username-ului ) de la clientul.\n");
            continue;        /* continuam sa ascultam */
        }
        strcpy(username[i], msg);
        printf("Am primit numele: %s\n", username[i]);
        sqlite3_stmt *que;

        char *verify = "SELECT 1 FROM Ranking WHERE Username= ?;";
        error = sqlite3_prepare_v2(data_b, verify, -1, &que, 0);

        if (error == SQLITE_OK) {
            sqlite3_bind_text(que, 1, msg, -1, 0);
        } else { perror("eroare la prepare verify"); }

        int step = sqlite3_step(que);
        if (step == SQLITE_ROW) {
            char rez[1];
            sprintf(rez, "%s", sqlite3_column_text(que, 0));
            if (strcmp(rez, "1") == 0) {
                printf("Exista username-ul!\n");
                ok = 1;
            }
        }

        if (ok == 0) {
            char *insert = "INSERT INTO Ranking(ID, Username, Victories, Defeats) VALUES (?, ?, 0, 0);";
            sqlite3_stmt *ins;
            error = sqlite3_prepare_v2(data_b, insert, -1, &ins, 0);
            counter++;
            if (error == SQLITE_OK) {
                sqlite3_bind_int(ins, 1, counter);
                sqlite3_bind_text(ins, 2, msg, -1, 0);
            } else { perror("eroare la prepare insert"); }

            sqlite3_step(ins);
        }
    }
    //inserarea in baza de date a informatiilor
}

/*Dupa incheierea unui joc va fi actualizata baza de date cu nr de jocuri jucate, pierdute, castigate pt fiecare jucator, in functie de rezultat*/
void update_db(int winner, char username[2][100]) {
    sqlite3 *data_b;
    int error = sqlite3_open("/home/lazarcatalina/CLionProjects/BraxS/Brax.db", &data_b);
    if (error != SQLITE_OK) {
        perror("Eroare la deschiderea bazei de date!\n");
        sqlite3_close(data_b);
    }

    sqlite3_stmt *que;

    char *update_winner = "UPDATE Ranking SET Victories=Victories+1 WHERE Username= ?;";//pregatirea instructiunii update pentru castigator
    error = sqlite3_prepare_v2(data_b, update_winner, -1, &que, 0);

    if (error == SQLITE_OK) {
        sqlite3_bind_text(que, 1, username[winner], -1, 0); //introducerea numelui castigatorului
    } else { perror("eroare la prepare"); }

    sqlite3_step(que); //executarea instructiunii
    char *update_loser = "UPDATE Ranking SET Defeats=Defeats+1 WHERE Username= ?;";////pregatirea instructiunii update pentru invins
    error = sqlite3_prepare_v2(data_b, update_loser, -1, &que, 0);

    if (error == SQLITE_OK) {
        sqlite3_bind_text(que, 1, username[(winner * (-1)) + 1], -1, 0);//introducerea numelui invinsului
    } else { perror("eroare la prepare"); }

    sqlite3_step(que);
    sqlite3_close(data_b);
}

/*Ofera fiecarui utilizator posibilitatea de a vedea clasamentul*/
void visualize_ranking(int players[2]) {
    char ans[3], rank[200];
    for (int i = 0; i < 2; i++) {
        bzero(ans, 3);
        if (read(players[i], ans, 3) <= 0) {
            perror("[server]Eroare la read( la citirea vizualizare clasament ) de la client.\n");
            continue;        /* continuam sa ascultam */
        }
        if (strncmp(ans, "da", strlen("da")) == 0) {
            //acceseaza baza de date si fa un mesaj cu primii 5

            bzero(rank, 200);

            //realizarea interogarii si crearea mesajului
            sqlite3 *data_b;
            int error = sqlite3_open("/home/lazarcatalina/CLionProjects/BraxS/Brax.db", &data_b);
            if (error != SQLITE_OK)
            {
                perror("Eroare la deschiderea bazei de date!\n");
                sqlite3_close(data_b);
            }
            sqlite3_stmt *viz;
            char *vis_rank = "SELECT * FROM Ranking ORDER BY Victories DESC LIMIT 5;";
            error = sqlite3_prepare_v2(data_b, vis_rank, -1, &viz, 0);
            if (error != SQLITE_OK)
            {
                perror("Eroare!\n");
            }
            bzero(rank,200);
            bool done = false;

            while(!done)
            {
                switch (sqlite3_step(viz)) {
                    case SQLITE_ROW:
                        strcat(rank, (const char *)sqlite3_column_text(viz, 0));
                        strcat(rank,"  ");
                        strcat(rank, (const char *)sqlite3_column_text(viz, 1));
                        strcat(rank,"  ");
                        strcat(rank, (const char *)sqlite3_column_text(viz, 2));
                        strcat(rank,"  ");
                        strcat(rank, (const char *)sqlite3_column_text(viz, 3));
                        strcat(rank,"\n");

                        break;
                    case SQLITE_DONE:
                        done=true;
                        break;
                    default:
                        printf("Eroare");
                }
            }
            sqlite3_finalize(viz);
            sqlite3_close(data_b);
            //transmiterea informatiilor
            if (write(players[i], rank, sizeof(rank)) <= 0) {
                perror("[server]Eroare la write( trimiterea primilor 5) catre client.\n");
            }
        } else continue;
    }
}

/*Mutarile sunt primite in umatorul fortmat "1109901|22|0" => trebuie prelucrat astfel poz_in=1109901; poz_fin=22; brax=0 */
//posibil
void input_parsing(char msg[18], int *poz_in, int *poz_fin, int *brax) {
    if (strlen(msg) < 10) {
        *poz_fin = -1;
        *poz_in = -1;
        *brax = -1;
        return;
    }
    *poz_in = 0;
    *poz_fin = 0;
    char num[8];
    strncpy(num, msg, 7);
    num[7] = '\0';
    *poz_in = (int) strtol(num, NULL, 10);
    strcpy(msg, msg + 7);
    strncpy(num, msg, strlen(msg) - 1);
    num[strlen(msg) - 1] = '\0';
    *poz_fin = (int) strtol(num, NULL, 10);
    strcpy(msg, msg + (strlen(msg) - 1));
    *brax = (int) strtol(msg, NULL, 10);
}

/*Initializeaza matricea care contine tabla de joc si piesele pe pozitiile initiale.
 * Matricea contine elementele={drum_rosu=88, drum_negru=99, diagonale=-1, 1108801 pozitie pe tabla cu piesa, 12 poizitie pe tabla fara piesa)}*/
void initialize_board(int tabla[17][17]) {
    int k = 0;
    int nr_piesa = 1;
    for (int i = 0; i < 17; i++) {
        if (i % 2 == 0)k++;
        int l = 1;
        for (int j = 0; j < 17; j++) {
            if ((i % 2 == 1) && (j % 2 == 1)) tabla[i][j] = -1; //diagonalele dintre pozitii
            if (i % 2 == 0 && j % 2 == 0) { // pozitii

                tabla[i][j] = k * 10 + l;
            }
            if (i == 0 && j % 2 == 0 && j > 1 && j < 16)//piesele puse la pozitia initiala
            {
                tabla[0][j] = tabla[0][j] * 100000 + 9900 + nr_piesa;
                nr_piesa++;
            }
            if (i == 16 && j % 2 == 0 && j > 1 && j < 16)//piesele puse la pozitia initiala
            {
                tabla[16][j] = tabla[16][j] * 100000 + 8800 + 9 - nr_piesa;
                nr_piesa--;
            }
            if (j % 2 == 0) l++;
        }
    }
//drumuri orizontale
    for (int i = 0; i <= 16; i = i + 4)
        for (int j = 1; j < 17; j = j + 4)
            if (i % 8 == 0) {
                if (j == 1 || j == 9) {
                    tabla[i][j] = 88;
                    tabla[i][j + 2] = 88;
                } else {
                    tabla[i][j] = 99;
                    tabla[i][j + 2] = 99;
                }
            } else {
                if (j == 1 || j == 9) {
                    tabla[i][j] = 99;
                    tabla[i][j + 2] = 99;
                } else {
                    tabla[i][j] = 88;
                    tabla[i][j + 2] = 88;
                }
            }
    for (int i = 2; i <= 14; i = i + 4) {
        for (int j = 1; j <= 15; j = j + 4)
            tabla[i][j] = 99;
        for (int j = 3; j <= 15; j = j + 4)
            tabla[i][j] = 88;
    }

// drumuri verticale
    for (int i = 13; i >= 1; i = i - 4)
        for (int j = 16; j >= 0; j = j - 4)
            if (j % 8 == 0) {
                if (i == 1 || i == 9) {
                    tabla[i][j] = 88;
                    tabla[i + 2][j] = 88;
                } else {
                    tabla[i][j] = 99;
                    tabla[i + 2][j] = 99;
                }
            } else {
                if (i == 1 || i == 9) {
                    tabla[i][j] = 99;
                    tabla[i + 2][j] = 99;
                } else {
                    tabla[i][j] = 88;
                    tabla[i + 2][j] = 88;
                }
            }
    for (int j = 2; j <= 14; j = j + 4) {
        for (int i = 1; i <= 15; i = i + 4)
            tabla[i][j] = 88;

        for (int i = 3; i <= 15; i = i + 4)
            tabla[i][j] = 99;
    }

}

/*Jucatorii isi pot alege culoarea. In cazul care aleg aceeasi culoare, se pastreaza culorile dupa ordinea data de conectare
 * 0 - negru
 * 1 - rosu*/
void pick_color(int players[2]) {
    char ans[6]; //mesajul pe care il va primi de la clienti
    int color[2]; // retine culorile alese
    color[0] = 1;
    color[1] = 1;
    for (int i = 0; i < 2; i++) {
        bzero(ans, 6);

        if (read(players[i], ans, 6) <= 0) {
            perror("[server]Eroare la read( culoare ) de la player.\n");
        }
        if (strncmp(ans, "negru", 5) == 0) color[0] = color[0] * 10 + i; else color[1] = color[1] * 10 + i; //setarea culorii
    }
    if (!(color[0] > 12 || color[1] > 12 || (color[0] == 10 && color[1] == 11))) { //daca trebuie facuta schimbarea
        int a = players[0];
        players[0] = players[1];
        players[1] = a;
    }

    if (write(players[0], "negru", strlen("negru")) <= 0) {
        perror("[server]Eroare la write( culoare pentru joc ) catre client.\n");
    }
    if (write(players[1], "rosu", strlen("rosu")) <= 0) {
        perror("[server]Eroare la write( culoare pentru joc ) catre client.\n");
    }
}

void generate_coordonates(const int board[17][17], int *poz_in_i, int *poz_in_j, int *poz_fin_i, int *poz_fin_j, int poz_in,
                     int poz_fin) {
    int ok = 0;
    for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 17; j++) {
            if (board[i][j] == poz_in) {
                *poz_in_i = i;
                *poz_in_j = j;
                ok++;
            }
            if (board[i][j] == poz_fin) {
                *poz_fin_i = i;
                *poz_fin_j = j;
                ok++;
            }
            if (ok == 2) break;
        }
        if (ok == 2) break;
    }
    if (poz_fin == 88) {
        *poz_fin_i = 14;
        *poz_fin_j = 14;
    } else if (poz_fin == 99) {
        *poz_fin_i = 16;
        *poz_fin_j = 16;
    }
}

bool critic_position(const int board[17][17], const int poz_i, const int poz_j, const int color) {
    //amenintari la distanta de o mutare
    if ((poz_i + 2) < 17 && board[poz_i + 2][poz_j] > 100 && (board[poz_i + 2][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_i - 2) > 0 && board[poz_i - 2][poz_j] > 100 && (board[poz_i - 2][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_j + 2) < 17 && board[poz_i][poz_j + 2] > 100 && (board[poz_i][poz_j + 2] % 10000 / 100) != color)
        return true;
    if ((poz_j - 2) > 0 && board[poz_i][poz_j - 2] > 100 && (board[poz_i][poz_j - 2] % 10000 / 100) != color)
        return true;
    //la distanta de doua mutari directe
    if ((poz_i + 4) < 17 && board[poz_i + 1][poz_j] == board[poz_i + 3][poz_j] && board[poz_i + 3][poz_j] != color &&
        board[poz_i + 4][poz_j] > 100 && (board[poz_i + 4][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_i - 4) > 0 && board[poz_i - 1][poz_j] == board[poz_i - 3][poz_j] && board[poz_i - 3][poz_j] != color &&
        board[poz_i - 4][poz_j] > 100 && (board[poz_i - 4][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_j + 4) < 17 && board[poz_i + 1][poz_j] == board[poz_i][poz_j + 3] && board[poz_i][poz_j + 3] != color &&
        board[poz_i][poz_j + 4] > 100 && (board[poz_i][poz_j + 4] % 10000 / 100) != color)
        return true;
    if ((poz_j - 4) > 0 && board[poz_i + 1][poz_j] == board[poz_i][poz_j - 3] && board[poz_i][poz_j - 3] != color &&
        board[poz_i][poz_j - 4] > 100 && (board[poz_i][poz_j - 4] % 10000 / 100) != color)
        return true;
    // la distanta de doua mutari
    if ((poz_i + 2) < 17 && (poz_j + 2) < 17 && board[poz_i + 1][poz_j] == board[poz_i + 2][poz_j + 1] &&
        board[poz_i + 1][poz_j] != color && board[poz_i + 2][poz_j + 2] > 100 &&
        (board[poz_i + 2][poz_j + 2] % 10000 / 100) != color)
        return true;
    if ((poz_i + 2) < 17 && (poz_j + 2) < 17 && board[poz_i][poz_j + 1] == board[poz_i + 1][poz_j + 2] &&
        board[poz_i][poz_j + 1] != color && board[poz_i + 2][poz_j + 2] > 100 &&
        (board[poz_i + 2][poz_j + 2] % 10000 / 100) != color)
        return true;

    if ((poz_i + 2) < 17 && (poz_j - 2) > 0 && board[poz_i + 1][poz_j] == board[poz_i + 2][poz_j - 1] &&
        board[poz_i + 1][poz_j] != color && board[poz_i + 2][poz_j - 2] > 100 &&
        (board[poz_i + 2][poz_j - 2] % 10000 / 100) != color)
        return true;
    if ((poz_i + 2) < 17 && (poz_j - 2) > 0 && board[poz_i][poz_j - 1] == board[poz_i + 1][poz_j - 2] &&
        board[poz_i][poz_j + 1] != color && board[poz_i + 2][poz_j + 2] > 100 &&
        (board[poz_i + 2][poz_j - 2] % 10000 / 100) != color)
        return true;

    if ((poz_i - 2) > 0 && (poz_j + 2) < 17 && board[poz_i - 1][poz_j] == board[poz_i - 2][poz_j + 1] &&
        board[poz_i - 1][poz_j] != color && board[poz_i - 2][poz_j + 2] > 100 &&
        (board[poz_i - 2][poz_j + 2] % 10000 / 100) != color)
        return true;
    if ((poz_i - 2) > 0 && (poz_j + 2) < 17 && board[poz_i][poz_j + 1] == board[poz_i - 1][poz_j + 2] &&
        board[poz_i][poz_j + 1] != color && board[poz_i - 2][poz_j + 2] > 100 &&
        (board[poz_i - 2][poz_j + 2] % 10000 / 100) != color)
        return true;

    if ((poz_i - 2) > 0 && (poz_j - 2) > 0 && board[poz_i - 1][poz_j] == board[poz_i - 2][poz_j - 1] &&
        board[poz_i - 1][poz_j] != color && board[poz_i - 2][poz_j - 2] > 100 &&
        (board[poz_i - 2][poz_j - 2] % 10000 / 100) != color)
        return true;
    if ((poz_i - 2) > 0 && (poz_j - 2) > 0 && board[poz_i][poz_j - 1] == board[poz_i - 1][poz_j - 2] &&
        board[poz_i][poz_j - 1] != color && board[poz_i - 2][poz_j - 2] > 100 &&
        (board[poz_i - 2][poz_j - 2] % 10000 / 100) != color)
        return true;

    return false;
}

bool threaten(const int board[17][17], const int poz_i, const int poz_j, const int color) {
    //amenintari la distanta de o mutare
    if ((poz_i + 2) < 17 && board[poz_i + 2][poz_j] > 100 && (board[poz_i + 2][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_i - 2) > 0 && board[poz_i - 2][poz_j] > 100 && (board[poz_i - 2][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_j + 2) < 17 && board[poz_i][poz_j + 2] > 100 && (board[poz_i][poz_j + 2] % 10000 / 100) != color)
        return true;
    if ((poz_j - 2) > 0 && board[poz_i][poz_j - 2] > 100 && (board[poz_i][poz_j - 2] % 10000 / 100) != color)
        return true;
    //la distanta de doua mutari directe
    if ((poz_i + 4) < 17 && board[poz_i + 1][poz_j] == board[poz_i + 3][poz_j] && board[poz_i + 3][poz_j] == color &&
        board[poz_i + 4][poz_j] > 100 && (board[poz_i + 4][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_i - 4) > 0 && board[poz_i - 1][poz_j] == board[poz_i - 3][poz_j] && board[poz_i - 3][poz_j] == color &&
        board[poz_i - 4][poz_j] > 100 && (board[poz_i - 4][poz_j] % 10000 / 100) != color)
        return true;
    if ((poz_j + 4) < 17 && board[poz_i + 1][poz_j] == board[poz_i][poz_j + 3] && board[poz_i][poz_j + 3] == color &&
        board[poz_i][poz_j + 4] > 100 && (board[poz_i][poz_j + 4] % 10000 / 100) != color)
        return true;
    if ((poz_j - 4) > 0 && board[poz_i + 1][poz_j] == board[poz_i][poz_j - 3] && board[poz_i][poz_j - 3] == color &&
        board[poz_i][poz_j - 4] > 100 && (board[poz_i][poz_j - 4] % 10000 / 100) != color)
        return true;
    // la distanta de doua mutari
    if ((poz_i + 2) < 17 && (poz_j + 2) < 17 && board[poz_i + 1][poz_j] == board[poz_i + 2][poz_j + 1] &&
        board[poz_i + 1][poz_j] == color && board[poz_i + 2][poz_j + 2] > 100 &&
        (board[poz_i + 2][poz_j + 2] % 10000 / 100) != color)
        return true;
    if ((poz_i + 2) < 17 && (poz_j + 2) < 17 && board[poz_i][poz_j + 1] == board[poz_i + 1][poz_j + 2] &&
        board[poz_i][poz_j + 1] == color && board[poz_i + 2][poz_j + 2] > 100 &&
        (board[poz_i + 2][poz_j + 2] % 10000 / 100) != color)
        return true;

    if ((poz_i + 2) < 17 && (poz_j - 2) > 0 && board[poz_i + 1][poz_j] == board[poz_i + 2][poz_j - 1] &&
        board[poz_i + 1][poz_j] == color && board[poz_i + 2][poz_j - 2] > 100 &&
        (board[poz_i + 2][poz_j - 2] % 10000 / 100) != color)
        return true;
    if ((poz_i + 2) < 17 && (poz_j - 2) > 0 && board[poz_i][poz_j - 1] == board[poz_i + 1][poz_j - 2] &&
        board[poz_i][poz_j + 1] == color && board[poz_i + 2][poz_j + 2] > 100 &&
        (board[poz_i + 2][poz_j - 2] % 10000 / 100) != color)
        return true;

    if ((poz_i - 2) > 0 && (poz_j + 2) < 17 && board[poz_i - 1][poz_j] == board[poz_i - 2][poz_j + 1] &&
        board[poz_i - 1][poz_j] == color && board[poz_i - 2][poz_j + 2] > 100 &&
        (board[poz_i - 2][poz_j + 2] % 10000 / 100) != color)
        return true;
    if ((poz_i - 2) > 0 && (poz_j + 2) < 17 && board[poz_i][poz_j + 1] == board[poz_i - 1][poz_j + 2] &&
        board[poz_i][poz_j + 1] == color && board[poz_i - 2][poz_j + 2] > 100 &&
        (board[poz_i - 2][poz_j + 2] % 10000 / 100) != color)
        return true;

    if ((poz_i - 2) > 0 && (poz_j - 2) > 0 && board[poz_i - 1][poz_j] == board[poz_i - 2][poz_j - 1] &&
        board[poz_i - 1][poz_j] == color && board[poz_i - 2][poz_j - 2] > 100 &&
        (board[poz_i - 2][poz_j - 2] % 10000 / 100) != color)
        return true;
    if ((poz_i - 2) > 0 && (poz_j - 2) > 0 && board[poz_i][poz_j - 1] == board[poz_i - 1][poz_j - 2] &&
        board[poz_i][poz_j - 1] == color && board[poz_i - 2][poz_j - 2] > 100 &&
        (board[poz_i - 2][poz_j - 2] % 10000 / 100) != color)
        return true;

    return false;
}

void update_board(int poz_in_i, int poz_in_j, int poz_fin_i, int poz_fin_j, int board[17][17]) {
    if (board[poz_fin_i][poz_fin_j] < 100)
        board[poz_fin_i][poz_fin_j] = board[poz_fin_i][poz_fin_j] * 100000 + board[poz_in_i][poz_in_j] % 10000;
    else
        board[poz_fin_i][poz_fin_j] =
                (board[poz_fin_i][poz_fin_j] / 100000) * 100000 + board[poz_in_i][poz_in_j] % 10000;
    board[poz_in_i][poz_in_j] = board[poz_in_i][poz_in_j] / 100000;

}

/*Mutarea trebuie validata: daca se poate ajunge pe pozitia ceruta, daca poate fi aplicat brax, daca a primit brax tb verificat daca mutarea facuta este
*/
void check_move(int i, int poz_in, int poz_fin, int brax, int brax_move[2], int tabla[17][17], int pieces_counter[2],
                int *correct_move) {
    *correct_move = 0;
    // i=0 -> jucator maro 99 \ i=1 -> jucator rosu 88
    int color, poz_final, ok = 0;
    int poz_fin_i, poz_fin_j, poz_in_i, poz_in_j;

    generate_coordonates(tabla, &poz_in_i, &poz_in_j, &poz_fin_i, &poz_fin_j, poz_in, poz_fin);
    if (i == 0)
        color = 99;
    else
        color = 88;
    int poz_init = poz_in / 100000;
    if (poz_fin > 100) poz_final = poz_fin / 100000; else poz_final = poz_fin;
    if (brax > 2) return;
    if (brax_move[i] ==
        1) //daca am primit brax dar mut o piesa care nu e amenintata sau locul in care mut piesa este amenintat
        if (!((poz_in > 100 && poz_in % 10000 / 100 == color) && critic_position(tabla, poz_in_i, poz_in_j, color) &&
              !critic_position(tabla, poz_fin_i, poz_fin_j, color)))
            return;
    if (poz_in < 100 || poz_in % 10000 / 100 != color) return; // daca nu am piesa sau nu e culoarea mea
    if (brax == 1 && brax_move[i] == 1) return;// daca zic brax dar am de rezolvat un brax
    if (brax == 1 && !threaten(tabla, poz_fin_i, poz_fin_j, color))
        return; // daca spun brax si piesa mea nu ameninta nimic
    if (poz_fin > 100 && (poz_fin % 10000 / 100) == color) return; // daca pozitia finala e pe o piesa de culoarea mea

    if (poz_final == poz_init + 10 || poz_final == poz_init - 10 || poz_final == poz_init - 1 ||
        poz_final == poz_init + 1) {//daca este la distanta de un drum
        ok = 1;
    } else {
        if (poz_init + 20 == poz_final) {//poz_in_i==poz_in_f+4 poz_in_j==poz_fin_j
            if (poz_in_i + 4 < 17 && tabla[poz_in_i + 1][poz_in_j] == tabla[poz_in_i + 3][poz_in_j] &&
                tabla[poz_in_i + 3][poz_in_j] == color)
                ok = 1;
        } else if (poz_init + 2 == poz_final) {
            if (poz_in_j + 4 < 17 && tabla[poz_in_i][poz_in_j + 1] == tabla[poz_in_i][poz_in_j + 3] &&
                tabla[poz_in_i][poz_in_j + 3] == color)
                ok = 1;
        } else if (poz_init + 11 == poz_final) {
            if (poz_in_i + 2 < 17 && poz_in_j + 2 < 17) {
                if (tabla[poz_in_i + 1][poz_in_j] == tabla[poz_in_i + 2][poz_in_j + 1] &&
                    tabla[poz_in_i + 2][poz_in_j + 1] == color)
                    ok = 1;
                if (tabla[poz_in_i][poz_in_j + 1] == tabla[poz_in_i + 1][poz_in_j + 2] &&
                    tabla[poz_in_i + 1][poz_in_j + 2] == color)
                    ok = 1;
            }
        } else if (poz_init + 9 == poz_final) {
            if (poz_in_i + 2 < 17 && poz_in_j - 2 > 0) {
                if (tabla[poz_in_i + 1][poz_in_j] == tabla[poz_in_i + 2][poz_in_j - 1] &&
                    tabla[poz_in_i + 2][poz_in_j - 1] == color)
                    ok = 1;
                if (tabla[poz_in_i][poz_in_j - 1] == tabla[poz_in_i + 1][poz_in_j - 2] &&
                    tabla[poz_in_i + 1][poz_in_j - 2] == color)
                    ok = 1;
            }

        } else if (poz_init - 20 == poz_final) {
            if (poz_in_i - 4 > 0 && tabla[poz_in_i - 1][poz_in_j] == tabla[poz_in_i - 3][poz_in_j] &&
                tabla[poz_in_i - 3][poz_in_j] == color)
                ok = 1;
        } else if (poz_init - 11 == poz_final) {
            if (poz_in_i - 2 > 0 && poz_in_j - 2 > 0) {
                if (tabla[poz_in_i - 1][poz_in_j] == tabla[poz_in_i - 2][poz_in_j - 1] &&
                    tabla[poz_in_i - 2][poz_in_j - 1] == color)
                    ok = 1;
                if (tabla[poz_in_i][poz_in_j - 1] == tabla[poz_in_i - 1][poz_in_j - 2] &&
                    tabla[poz_in_i - 1][poz_in_j - 2] == color)
                    ok = 1;
            }
        } else if (poz_init - 9 == poz_final) {
            if (poz_in_i - 2 > 0 && poz_in_j + 2 < 17) {
                if (tabla[poz_in_i - 1][poz_in_j] == tabla[poz_in_i - 2][poz_in_j + 1] &&
                    tabla[poz_in_i - 2][poz_in_j + 1] == color)
                    ok = 1;
                if (tabla[poz_in_i][poz_in_j + 1] == tabla[poz_in_i - 1][poz_in_j + 2] &&
                    tabla[poz_in_i - 1][poz_in_j + 2] == color)
                    ok = 1;
            }
        } else if ((poz_init - 2) == poz_final) {
            if (poz_in_j - 4 > 0 && tabla[poz_in_i][poz_in_j - 1] == tabla[poz_in_i][poz_in_j - 3] &&
                tabla[poz_in_i][poz_in_j - 3] == color)
                ok = 1;
        }
    }

    if (ok == 1) {
        if (poz_fin > 100) {
            pieces_counter[(i * (-1)) + 1]--;
            printf("piese ramase oponent: %d", pieces_counter[(i * (-1)) + 1]);
        } // daca pot lua o piesa
        *correct_move = 1;
        brax_move[i] = 0;
        if (brax == 1) brax_move[(i * (-1)) + 1] = 1;
        update_board(poz_in_i, poz_in_j, poz_fin_i, poz_fin_j, tabla);
        return;
    }
}

/*Trimit matricea urmatorului jucator, sa vada mutarea celuilalt jucator si daca a primit brax*/
void send_board(const int board[17][17], int players[2], const int i, const int brax_move[2]) {
    char brax[3];
    char *msg;
    char *tabla_str;
    msg = malloc(1024);
    tabla_str = malloc(1024);
    char element[10];
    for (int k = 0; k < 17; k++) {
        for (int j = 0; j < 17; j++) {
            sprintf(element, "%d", board[k][j]);
            strcat(tabla_str, element);
            strcat(tabla_str, " ");
        }
        strcat(tabla_str, "\n");
    }
    if (brax_move[(i * (-1)) + 1] == 0)
        strcpy(brax, "nu");
    else
        strcpy(brax, "da");
    sprintf(msg, "%s\n%s", tabla_str, brax);
    if (write(players[(i * (-1)) + 1], msg, 1024) <= 0) {
        perror("[server]Eroare la write() catre client.\n");
    }
    //printf("%s\n", msg);
    printf("Fd-ul primului jucator: %d, al doilea jucator: %d \n", players[0], players[1]);

    if (msg != NULL) free(msg);
}

void send_end_of_game_msg(int players[2], int winner) {
    if (write(players[winner], "1", strlen("1")) <= 0) {
        perror("[server]Eroare la write(final joc - castigator) catre client.\n");
    } else
        printf("[server]Mesajul final joc - ok a fost transmis cu succes.\n");
    if (write(players[(winner * (-1)) + 1], "0", strlen("0")) <= 0) {
        perror("[server]Eroare la write(final joc - invins) catre client.\n");
    } else
        printf("[server]Mesajul final joc - ko a fost transmis cu succes.\n");
    printf("Fd-ul primului jucator: %d, al doilea jucator: %d \n", players[0], players[1]);

}

void send_not_end_of_game_msg(int players[2], int i) {
    if (write(players[i], "2", strlen("1")) <= 0) {
        perror("[server]Eroare la write(final joc - castigator) catre client.\n");
    }
    printf("Fd-ul primului jucator: %d, al doilea jucator: %d \n", players[0], players[1]);

}
/*intreaba ambii jucatori daca vor sa mai joace, trebuie sa aiba acceptul amandurora, altfel se incheie*/
bool another_round(int players[2]) {
    char ans[3];
    int ans_player[2] = {0};
    int yes = 0;
    for (int i = 0; i < 2; i++) {
        bzero(ans, 3);
        if (read(players[i], ans, 3) <= 0) {
            perror("[server]Eroare la read() de la player1.\n");
        }
        if (strncmp(ans, "da", strlen("da")) == 0) {
            yes++;
            ans_player[i] = 1;
        }
    }
    for (int i = 0; i < 2; i++)
        if (ans_player[i] ==
            1) { // 1 - continua jocul | 0 - nu continua jocul trimitem raspuns doar celui care vrea sa continue
            if (yes == 2) {
                if (write(players[i], "1", strlen("1")) <= 0) {
                    perror("[server]Eroare la write( another round ) catre client.\n");
                    continue;        /* continuam sa ascultam */
                }
            } else if (write(players[i], "0", strlen("0")) <= 0) {
                perror("[server]Eroare la write( another round ) catre client.\n");
                continue;        /* continuam sa ascultam */
            }
        }
    if (yes == 2) return true;
    return false;
}


void joc(int players[2], char username[2][100]) {
    for (int i = 0; i < 2; i++) {
        printf("FD-ul jucatorului %d este %d", i, players[i]);
    }
    while (1) { //contine un joc cap-coada | 2 jucatori pot avea optiunea de a juca de mai multe ori impreuna fara sa se deconecteze

        int board[17][17];
        initialize_board(board);

        pick_color(
                players); //isi aleg culoarea, acum se seteaza in players adevarata ordine, pana acum a fost cea de la conectare
        printf("Trec cu stil de pick_color!\n");
        int winner, brax = 0;
        int poz_in, poz_fin;
        int pieces_counter[2], brax_move[2] = {0};
        pieces_counter[0] = 7;
        pieces_counter[1] = 7;
        char mutare[18];
        int i = 0;
        //trimit tabla initiala primului jucator
        send_board(board, players, 1, brax_move);

        while (1) { // secventa de mutari pana la castig
            printf("Este randul jucatorlui: %d, cu fd %d\n", i, players[i]);
            int correct_move = 0;
            while (!correct_move) { // se preiau mutarile unui jucator pana cand face o mutare buna
                bzero(mutare, 10);
                if (read(players[i], mutare, 10) <= 0) {
                    perror("[server] Eroare la read() - primire mutare.\n");
                    continue;
                }
                printf("Am citit mutarea lui %d, cu fd %d si este : %s\n", i, players[i], mutare);
                input_parsing(mutare, &poz_in, &poz_fin, &brax);
                printf("Am parsat mutarea lui %d cu fd= %d!\n", i, players[i]);
                printf("Dupa bzero 490: %d cu fd= %d si este %d\n", i, players[i], correct_move);

                check_move(i, poz_in, poz_fin, brax, brax_move, board, pieces_counter, &correct_move); //validare mutare
                printf("Am validat mutarea lui %d cu fd= %d si este %d\n", i, players[i], correct_move);
                if (correct_move == 0) {
                    if (write(players[i], "Mutarea a fost incorecta, introduceti o alta mutare.\n",
                              strlen("Mutarea a fost incorecta, introduceti o alta mutare.\n")) <= 0) {
                        perror("[server]Eroare la write( mutare incorecta ) catre client.\n");
                        continue;        /* continuam sa ascultam */
                    } else
                        printf("[server]Mesajul-mutare gresita a fost transmis cu succes.\n");
                } else {
                    printf("FD-ul jucatorului %d este %d", i, players[i]);
                    if (write(players[i], "Mutarea a fost corecta.\n", strlen("Mutarea a fost corecta.\n")) <= 0) {
                        perror("[server]Eroare la write(mutare corecta) catre client.\n");
                        continue;        /* continuam sa ascultam */
                    } else
                        printf("[server]Mesajul-mutare corecta a fost transmis cu succes.\n");
                }
            }

            printf("Mai sunt %d piese pentru adversar\n", pieces_counter[(i * (-1)) + 1]);
            if (pieces_counter[(i * (-1)) + 1] == 0) { //daca a reusit sa ia toate piesele adversarului, a castigat
                printf("Intru pe ramura de final a jocului!\n");
                winner = i;
                send_end_of_game_msg(players, winner);
                break; //daca exista castigator se termina secventa de mutari
            } else {
                printf("Intru pe ramura de continuare a jocului!\n");
                send_not_end_of_game_msg(players, i);
                printf("Trimit mesajul pentru continuare!\n");
                send_board(board, players, i, brax_move);
                printf("Trimit tabla!\n");
                i = i * (-1) + 1; // alterneaza randul jucatorilor
                printf("Urmatorul jucator este: %d", i);
            }

        }
        update_db(winner, username);
        visualize_ranking(players);
        if (!another_round(players)) break; //daca nu vor sa joace inca o runda, se incheie jocul
    }
}

int main() {
    struct sockaddr_in server;    // structura folosita de server
    struct sockaddr_in player; // structura folosita pt acceptarea jucatorului
    int sd;            //descriptorul de socket

    /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Eroare la socket().\n");
        return errno;
    }
    /* utilizarea optiunii SO_REUSEADDR */
    //int on=1;
    //setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

    /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&player, sizeof(player));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if (bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, 10) == -1) {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    //creaza tabel in baza de date, daca nu exista
    create_ranking();

    int client[2] = {-1};
    /* servim in mod concurent clientii... */
    while (1) {

        int length = sizeof(player);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);
        //acceptarea grupata a clientilor
        int j = 0;
        while (j < 2) {
            client[j] = accept(sd, (struct sockaddr *) &player, (unsigned int *) &length);
            if (client[0] < 0) {
                perror("[server]Eroare la accept().\n");
                continue;
            } else j++;
        }
        int pid;

        if ((pid = fork()) == -1) {
            close(client[0]);
            close(client[1]);
            client[0] = client[1] = -1;
            continue;
        } else if (pid > 0) {
            // parinte
            close(client[0]);
            close(client[1]);
            client[0] = client[1] = -1;
            while (waitpid(-1, NULL, WNOHANG));
            continue;
        } else if (pid == 0) { //in client[0/1] am descriptorul socketului spre clientul respectiv
            // copil; close(sd) inchidem descriptorul pt server, nu vor mai vb cu el, ci cu procesul
            close(sd);
            char username[2][100];
            printf("Intru aici!\n");
            login(client, username); //cere username-ul si face update BD-ului
            printf("Trec de login!\n");
            joc(client, username);
            printf("Trec de joc!\n");
            /*dupa incheierea jocului, se inchide conexiunea cu clientii si este terminat procesul*/
            close(client[0]);
            close(client[1]);
            printf("Am incheiat convorbirea cu clientii!\n");
            exit(0);
        }
    }
}                /* while */
/* main */
