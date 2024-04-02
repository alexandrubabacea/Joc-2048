#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/select.h>

#define COLOR_LIGHT_YELLOW	250
#define COLOR_LIGHT_GREEN	251
#define COLOR_ORANGE	252
#define COLOR_DARK_RED	253
#define COLOR_PINK	254
#define COLOR_GREY	255

#define N	4
#define FOREVER	1
#define KEYBOARD	0
#define S_TO_WAIT	4
#define MILIS_TO_WAIT	0
#define SELECT_ERROR	-1
#define SELECT_EVENT	1
#define SELECT_NO_EVENT	0
#define MENU_H	15
#define MENU_W	40
#define GAME_H	20
#define GAME_W	40
#define PANEL_W	26
#define SAVE	"save.txt"

#define ASCII_FILE	"ascii_art.txt"
#define ASCII_LEN	42
#define ASCII_ROWS	8
#define SP	2

/***************************************
 Memoreaza numerele de pe tabla de joc
 si o copie a lor (pentru optiunea undo)
***************************************/
typedef struct {
	int nr[N][N];
	int copy[N][N];
} mat;

/***************************************
 Memoreaza pozitiile pe de tabla de joc
 care sunt libere (pentru generarea
 noului numar pe una din aceste pozitii)
***************************************/
typedef struct {
	int x[N * N];
	int y[N * N];
} zero;

/***************************************
 Memoreaza informatii despre joc
***************************************/
typedef struct {
	/* Numarul de pozitii libere */
	int zero_cnt;

	/* Numarul de mutari de la inceputul jocului */
	int moves;

	/* Scorul */
	int score;

	/* Copia scorului (pentru undo) */
	int copy_scr;

	/* Retine daca matricea curenta este copiata */
	int copied;

	/* Retine cel mai bun scor */
	int h_score;
} data;

void menu(WINDOW *win, mat *board, zero *vect, data *info);

/**********************************************
		Genereaza 2 numere alese aleator din
	multimea {2, 4}, pe pozitii aleatoare.
**********************************************/
void new_number(mat *board, zero *vect, data *info)
{
	int i, j, random, number;

	/*********************************************************
			Pentru a genera 2 sau 4 aleator, se genereaza un
		numar aleator si se imparte la 2. Apoi, la rezultat
		se aduna 1 si se inmulteste cu 2.
	*********************************************************/
	number = rand() % 2;
	number += 1;
	number *= 2;

	/*********************************************************
			Se genereaza o pozitie intre 0 si numarul de
		elemente din campurile structurii 'vect'. Se obtin
		coordonate de la acea pozitie si se plaseaza numarul
		generat la pozitia determinata de 'i' si 'j' obtinuti
		din cei 2 vectori.
	*********************************************************/
	random = rand() % info->zero_cnt;
	i = vect->x[random];
	j = vect->y[random];
	board->nr[i][j] = number;

	/* Se elimina din 'vect' pozitia la care am plasat numarul */
	info->zero_cnt--;
	for (i = random; i < info->zero_cnt; i++) {
		vect->x[i] = vect->x[i + 1];
		vect->y[i] = vect->y[i + 1];
	}
}

/* Initializeaza tabla de joc si celelalte variabile */
void init_game(mat *board, zero *vect, data *info)
{
	int i, j, cnt = 0;

	/* Numarul de mutari si scorul sunt setate pe  0 */
	info->moves = 0;
	info->score = 0;

	/******************************************************************
			Toate casutele din tabla de joc sunt golite (setate pe 0).
			Vectorii din structura 'vect' vor retine toate pozitiile,
		deoarece toate sunt libere.
			Se actualizeaza si campul din 'info' care retine numarul
		de casute goale.
	******************************************************************/
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++) {
			board->nr[i][j] = 0;
			vect->x[cnt] = i;
			vect->y[cnt] = j;
			cnt++;
		}
	info->zero_cnt = cnt;

	/* Se genereaza aleator 2 numere din multimea {2, 4} */
	new_number(board, vect, info);
	new_number(board, vect, info);
}

/* Realizeaza mutarea la dreapta a casutelor */
int right(int mat[][N], int *modified)
{
	int score = 0;
	int i, j, k;
	*modified = 0;

	/******************************************************************
			Parcurge tabla de joc pe linii, de la dreapta la stanga,
		incepand cu penultima coloana.
			Cat timp casuta din dreapta casutei curenta este libera,
		acesta ia valoarea celei din stanga ei, urmand ca cea
		din stanga sa fie eliberata.
			Daca celula din dreapta are valoare egala cu celula
		curenta, cea din dreapta se dubleaza, iar cea din stanga este
		eliberata.
			Casutele create in mutarea curenta vor fi inmultite
		cu -1 pentru a nu se uni cu casute create in alte mutari
		ce au valoarea egala cu a lor.
			Daca o celula de joc s-a modificat, atunci se va
		retine acest lucru in variabila 'modified'.
	******************************************************************/
	for (i = 0; i < N; i++)
		for (j = 2; j >= 0; j--) {
			if (mat[i][j] != 0) {
				k = j;
				while (mat[i][k + 1] == 0 && k <= 2) {
					*modified = 1;
					mat[i][k + 1] = mat[i][k];
					mat[i][k] = 0;
					k++;
				}
				if (mat[i][k] == mat[i][k + 1] && k <= 2) {
					*modified = 1;
					score = score + mat[i][k + 1] * 2;
					mat[i][k + 1] *= (-2);
					mat[i][k] = 0;
				}
			}
		}

	/* Casutele cu valori negative se inmultesc cu -1 */
	if (*modified) {
		for (i = 0; i < N; i++)
			for (j = 0; j < N; j++)
				if (mat[i][j] < 0)
					mat[i][j] *= (-1);
	}

	return score;
}

/* Realizeaza mutarea la stanga a casutelor */
int left(int mat[][N], int *modified)
{
	int score = 0;
	int i, j, k;
	*modified = 0;

	/******************************************************************
			Parcurge tabla de joc pe linii, de la stanga la dreapta,
		incepand cu a doua coloana.
			Cat timp casuta din stanga casutei curenta este libera,
		acesta ia valoarea celei din dreapta ei, urmand ca cea
		din dreapta sa fie eliberata.
			Daca celula din stanga are valoare egala cu celula
		curenta, cea din stanga se dubleaza, iar cea din dreapta este
		eliberata.
			Casutele create in mutarea curenta vor fi inmultite
		cu -1 pentru a nu se uni cu casute create in alte mutari
		ce au valoarea egala cu a lor.
			Daca o celula de joc s-a modificat, atunci se va
		retine acest lucru in variabila 'modified'.
	******************************************************************/
	for (i = 0; i < N; i++)
		for (j = 1; j < N; j++) {
			if (mat[i][j] != 0) {
				k = j;
				while (mat[i][k - 1] == 0 && k >= 1) {
					*modified = 1;
					mat[i][k - 1] = mat[i][k];
					mat[i][k] = 0;
					k--;
				}
				if (mat[i][k - 1] == mat[i][k] && k >= 1) {
					*modified = 1;
					score = score + mat[i][k - 1] * 2;
					mat[i][k - 1] *= (-2);
					mat[i][k] = 0;
				}
			}
		}

	/* Casutele cu valori negative se inmultesc cu -1 */
	if (*modified)
		for (i = 0; i < N; i++)
			for (j = 0; j < N; j++)
				if (mat[i][j] < 0)
					mat[i][j] *= (-1);

	return score;
}

/* Realizeaza mutarea in sus a casutelor */
int up(int mat[][N], int *modified)
{
	int score = 0;
	int i, j, k;
	*modified = 0;

	/******************************************************************
			Parcurge tabla de joc pe coloane, de sus in jos,
		incepand cu al doilea rand.
			Cat timp casuta de deasupra casutei curenta este libera,
		acesta ia valoarea celei de sub ea, urmand ca cea
		de jos sa fie eliberata.
			Daca celula de deasupra are valoare egala cu celula
		curenta, cea de sus se dubleaza, iar cea de jos este
		eliberata.
			Casutele create in mutarea curenta vor fi inmultite
		cu -1 pentru a nu se uni cu casute create in alte mutari
		ce au valoarea egala cu a lor.
			Daca o celula de joc s-a modificat, atunci se va
		retine acest lucru in variabila 'modified'.
	******************************************************************/
	for (j = 0; j < N; j++)
		for (i = 1; i < N; i++) {
			if (mat[i][j] != 0) {
				k = i;
				while (mat[k - 1][j] == 0 && k >= 1) {
					*modified = 1;
					mat[k - 1][j] = mat[k][j];
					mat[k][j] = 0;
					k--;
				}
				if (mat[k - 1][j] == mat[k][j] && k >= 1) {
					*modified = 1;
					score = score + mat[k - 1][j] * 2;
					mat[k - 1][j] *= (-2);
					mat[k][j] = 0;
				}
			}
		}

	/* Casutele cu valori negative se inmultesc cu -1 */
	if (*modified)
		for (i = 0; i < N; i++)
			for (j = 0; j < N; j++)
				if (mat[i][j] < 0)
					mat[i][j] *= (-1);

	return score;
}

/* Realizeaza mutarea in jos a casutelor */
int down(int mat[][N], int *modified)
{
	int score = 0;
	int i, j, k;
	*modified = 0;

	/******************************************************************
			Parcurge tabla de joc pe coloane, de jos in sus,
		incepand cu penultimul rand.
			Cat timp casuta de sub casuta curenta este libera,
		acesta ia valoarea celei de deasupra ei, urmand ca cea
		de sus sa fie eliberata.
			Daca celula de sub ea are valoare egala cu celula
		curenta, cea de jos se dubleaza, iar cea de sus este
		eliberata.
			Casutele create in mutarea curenta vor fi inmultite
		cu -1 pentru a nu se uni cu casute create in alte mutari
		ce au valoarea egala cu a lor.
			Daca o celula de joc s-a modificat, atunci se va
		retine acest lucru in variabila 'modified'.
	******************************************************************/
	for (j = 0; j < N; j++)
		for (i = 2; i >= 0; i--) {
			if (mat[i][j] != 0) {
				k = i;
				while (mat[k + 1][j] == 0 && k <= 2) {
					*modified = 1;
					mat[k + 1][j] = mat[k][j];
					mat[k][j] = 0;
					k++;
				}
				if (mat[k][j] == mat[k + 1][j] && k <= 2) {
					*modified = 1;
					score = score + mat[k + 1][j] * 2;
					mat[k + 1][j] *= (-2);
					mat[k][j] = 0;
				}
			}
		}

	/* Casutele cu valori negative se inmultesc cu -1 */
	if (*modified)
		for (i = 0; i < N; i++)
			for (j = 0; j < N; j++)
				if (mat[i][j] < 0)
					mat[i][j] *= (-1);

	return score;
}

/* Afiseaza mesajul care semnifica terminarea jocului */
void final_msg(WINDOW *game, int end)
{
	int game_x, game_y;
	int poz_y, poz_x;
	int msg_h = 4, msg_w = 18;
	char key;

	/* Se calculeaza pozitia mesajului (in centru tablei de joc) */
	getmaxyx(game, game_y, game_x);
	poz_y = (game_y - msg_h) / 2;
	poz_x = (game_x - msg_w) / 2;

	/* Initializeaza o fereastra la acea pozitie si sterge continutul */
	WINDOW *msg = subwin(game, msg_h, msg_w, poz_y, poz_x);
	wclear(msg);
	box(msg, 0, 0);

	/* Afiseaza daca jocul a fost catigat sau pierdut */
	if (end == 1)
		mvwprintw(msg, 1, 5, "You  won");
	if (end == -1)
		mvwprintw(msg, 1, 5, "You lost");
	mvwprintw(msg, 2, 3, "Press  enter");
	wrefresh(game);

	/****************************************************************
			Se asteapta apasarea tastei Enter pentru a se reveni in
		functia check unde se reintra in meniu.
	****************************************************************/
	keypad(stdscr, TRUE);
	while (FOREVER) {
		key = getch();
		if (key == '\n')
			break;
	}
	keypad(stdscr, FALSE);
}

/* Verifica daca jocul s-a terminat */
void check(WINDOW *win, WINDOW *game, mat *board, zero *vect, data *info)
{
	int i, j, end = 0;
	int test[N][N];
	int test_up = 1, test_down = 1, test_left = 1, test_right = 1;

	/* Parcurge tabla de joc si verifica daca exista casuta '2048' */
	for (i = 0; i < N; i++) {
		for (j = 0; j < N; j++) {
			test[i][j] = board->nr[i][j];
			if (test[i][j] == 2048) {
				end = 1;
				break;
			}
		}
	}

	/**********************************************************
			Cand nu mai sunt casute goale si jocul nu a fost
		castigat se verifica daca mai exista mutari posibile.
	**********************************************************/
	if (!info->zero_cnt && !end) {
		up(test, &test_up);

		/******************************************************
				Daca se poate face mutarea in sus nu are sens
			verificarea celorlalte mutari ramase.
		******************************************************/
		if (!test_up) {
			down(test, &test_down);

			/******************************************************
					Daca se poate face mutarea in jos nu are sens
				verificarea celorlalte mutari ramase.
			******************************************************/
			if (!test_down) {
				left(test, &test_left);

				/********************************************************
						Daca se poate face mutarea la stanga nu are sens
					verificarea mutarii la dreapta.
				********************************************************/
				if (!test_left) {
					right(test, &test_right);

					/*****************************************
							Nu se poate muta nici la drepata,
						deci jocul este pierdut.
					********************************************************/
					if (!test_right)
						end = -1;
				}
			}
		}
	}

	/****************************
		end = 1 (joc castigat)
		end = -1 (joc pierdut)
		end = 0 (jocul continua)
	****************************/
	if (end) {
		/**********************************************************
				Se afiseaza mesajul de sfarsit si se face numarul
			de mutari 0 pentru a dezactiva optiunea de resume.
				Se revine la meniu.
		**********************************************************/
		final_msg(game, end);
		info->moves = 0;
		menu(win, board, vect, info);
	}
}

/* Copiaza o tabla de joc in alta */
void copy(int start[][N], int final[N][N])
{
	int i, j;
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			final[i][j] = start[i][j];
}

/* Actualizeaza pozitiile casutelor libere retinute de vectorul update_v */
void update_v(mat *board, zero *vect, data *info)
{
	int i, j, cnt = 0;
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			if (board->nr[i][j] == 0) {
				vect->x[cnt] = i;
				vect->y[cnt] = j;
				cnt++;
			}
	info->zero_cnt = cnt;
}

/* Numara casutele goale dintr-o tabla de joc */
int count_zero(int mat[][N])
{
	int i, j, cnt = 0;
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			if (!mat[i][j])
				cnt++;
	return cnt;
}

/* Calculeaza mutarea care elbereaza cele mai multe casute. */
char best_option(mat *board)
{
	int test[N][N];
	int test_up = 1, test_down = 1, test_left = 1, test_right = 1;
	int cnt = 0, maxim = 0;
	char ch = 'q';

	/* Face mutarea in sus pe copia matricei curente */
	copy(board->nr, test);
	up(test, &test_up);

	/* Numara casutele goale */
	cnt = count_zero(test);

	/****************************************************
			Daca mutarea se poate face, compara maximul
		de casute eliberate de celelalte mutari cu cele
		eliberate de mutarea in sus.
	****************************************************/
	if (test_up && maxim < cnt) {
		maxim = cnt;
		ch = 'w';
	}

	/* Face mutarea spre stanga pe copia matricei curente */
	copy(board->nr, test);
	left(test, &test_left);

	/* Numara casutele goale */
	cnt = count_zero(test);

	/****************************************************
			Daca mutarea se poate face, compara maximul
		de casute eliberate de celelalte mutari cu cele
		eliberate de mutarea la stanga.
	****************************************************/
	if (test_left && maxim < cnt) {
		maxim = cnt;
		ch = 'a';
	}

	/* Face mutarea in jos pe copia matricei curente */
	copy(board->nr, test);
	down(test, &test_down);

	/* Numara casutele goale */
	cnt = count_zero(test);

	/****************************************************
			Daca mutarea se poate face, compara maximul
		de casute eliberate de celelalte mutari cu cele
		eliberate de mutarea in jos.
	****************************************************/
	if (test_down && maxim < cnt) {
		maxim = cnt;
		ch = 's';
	}

	/* Face mutarea spre dreapta pe copia matricei curente */
	copy(board->nr, test);
	right(test, &test_right);

	/* Numara casutele goale */
	cnt = count_zero(test);

	/****************************************************
			Daca mutarea se poate face, compara maximul
		de casute eliberate de celelalte mutari cu cele
		eliberate de mutarea spre dreapta.
	****************************************************/
	if (test_right && maxim < cnt) {
		maxim = cnt;
		ch = 'd';
	}

	return ch;
}

/* Afiseaza panoul lateral. */
void panel_display(WINDOW *panel, data *info, mat *board)
{
	/* Initializeaza structura pentru timp */
	time_t t;
	struct tm *area;
	t = time(NULL);
	area = localtime(&t);

	int test_up = 1, test_down = 1, test_left = 1, test_right = 1;
	int test[N][N];
	int row = 1, col = 1;

	/* Afiseaza chenarul panoului */
	box(panel, 0, 0);

	mvwaddstr(panel, row++, PANEL_W / 2 - 2, "2048");
	mvwhline(panel, row++, col, ACS_HLINE, PANEL_W - 2);

	/****************************************************
			Afiseaza scorul curent, cel mai bun scor si
		numarul de mutari, pe linii consecutive.
	****************************************************/
	mvwprintw(panel, row++, col, "Highest Score: %d", info->h_score);
	mvwprintw(panel, row++, col, "Score: %d", info->score);
	mvwprintw(panel, row, col, "Moves: %d", info->moves);

	/* Afiseaza comenzile valide, lasand o linie libera */
	row += 2;
	mvwaddstr(panel, row++, col, "Valid commands:");
	mvwaddstr(panel, row++, col, "q + Enter - quit to menu");

	/* Testeaza daca mutarea in sus este valida */
	copy(board->nr, test);
	up(test, &test_up);
	if (test_up)
		mvwaddstr(panel, row++, col, "w + Enter - move up");

	/* Testeaza daca mutarea la stanga este valida */
	copy(board->nr, test);
	left(test, &test_left);
	if (test_left)
		mvwaddstr(panel, row++, col, "a + Enter - move left");

	/* Testeaza daca mutarea in jos este valida */
	copy(board->nr, test);
	down(test, &test_down);
	if (test_down)
		mvwaddstr(panel, row++, col, "s + Enter - move down");

	/* Testeaza daca mutarea la dreapta este valida */
	copy(board->nr, test);
	right(test, &test_right);
	if (test_right)
		mvwaddstr(panel, row++, col, "d + Enter - move right");

	/* Daca jocul curent are o copie se poate face undo */
	if (info->copied)
		mvwaddstr(panel, row++, col, "r + Enter - undo");

	/* Se afiseaza data si timpul curent */
	row = 15;
	mvwprintw(panel, row++, col, "Date: %d/%d/%d", area->tm_mday,
			  area->tm_mon + 1, area->tm_year + 1900);
	mvwprintw(panel, row, col, "Time: %.2d:%.2d", area->tm_hour, area->tm_min);

	wrefresh(panel);
}

/* Afiseaza jocul */
void game_display(WINDOW *win, WINDOW *panel, WINDOW *cell[],
				  mat *board, data *info)
{
	start_color();

	/* Definirea unor culori care nu sunt predefinite */
	init_color(COLOR_LIGHT_YELLOW, 1000, 1000, 500);
	init_color(COLOR_LIGHT_GREEN, 400, 650, 400);
	init_color(COLOR_ORANGE, 1000, 600, 0);
	init_color(COLOR_DARK_RED, 360, 0, 0);
	init_color(COLOR_PINK, 1000, 770, 800);
	init_color(COLOR_GREY, 500, 500, 500);

	/******************************************************
			Se definesc perechile de culori pentru casute
		si au id-ul corespunzator logaritmului in baza 2
		a valorii din casuta.
	******************************************************/
	init_pair(1, COLOR_BLACK, COLOR_LIGHT_YELLOW); /* pentru 2 */
	init_pair(2, COLOR_BLACK, COLOR_YELLOW); /* pentru 4 */
	init_pair(3, COLOR_BLACK, COLOR_LIGHT_GREEN); /* pentru 8 */
	init_pair(4, COLOR_BLACK, COLOR_GREEN); /* pentru 16 */
	init_pair(5, COLOR_BLACK, COLOR_CYAN); /* pentru 32 */
	init_pair(6, COLOR_BLACK, COLOR_ORANGE); /* pentru 64 */
	init_pair(7, COLOR_BLACK, COLOR_BLUE); /* pentru 128 */
	init_pair(8, COLOR_BLACK, COLOR_MAGENTA); /* pentru 256 */
	init_pair(9, COLOR_BLACK, COLOR_PINK); /* pentru 512 */
	init_pair(10, COLOR_BLACK, COLOR_RED); /* pentru 1024 */
	init_pair(11, COLOR_BLACK, COLOR_DARK_RED); /* pentru 2048 */
	init_pair(12, COLOR_BLACK, COLOR_GREY);

	wclear(win);
	int i, j, k = 0, log, center;
	char str[5];
	int cell_h, cell_w, poz_y, poz_x;

	/* Se calculeaza pozitia la care va fi plasata cifra in casuta*/
	getmaxyx(cell[0], cell_h, cell_w);
	poz_y = cell_h / 2;
	poz_x = cell_w / 2;

	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++) {
			/* Culoarea casutelor goale este gri */
			wbkgd(cell[k], COLOR_PAIR(0));
			wbkgd(cell[k], COLOR_PAIR(12));

			if (board->nr[i][j] != 0) {
				/* Calculeaza ce culoarea corespunde valorii */
				log = (int)log2((double)board->nr[i][j]);

				/* Face conversia din int in sir de caractere */
				sprintf(str, "%d", board->nr[i][j]);

				/* Afiseaza valoarea la pozitia calculata */
				wbkgd(cell[k], COLOR_PAIR(log));
				center = strlen(str) / 2;
				mvwaddstr(cell[k], poz_y, poz_x - center, str);
			}
			wrefresh(cell[k]);
			k++;
		}

	/* Odata cu afisarea jocului se afiseaza si panoul lateral */
	panel_display(panel, info, board);
	wrefresh(win);
}

/* Realizeaza mutarea celulelor in functie de alegerea facuta in functia game */
void game_option(WINDOW *win, WINDOW *game, WINDOW *cell[], WINDOW *panel,
				 mat *board, zero *vect, data *info, char key)
{
	int test[N][N];
	int test_up = 1, test_down = 1, test_left = 1, test_right = 1;

	switch (tolower(key)) {
	case 'q':
		/* Se intoarce la meniu */
		menu(win, board, vect, info);
		break;

	case 'a':
		/**********************************************
				Se copiaza scorul si matricea jocului
			pentru undo, inainte de modificare.
		**********************************************/
		info->copy_scr = info->score;
		copy(board->nr, board->copy);
		info->copied = 1;

		/********************************************
				Se calculeaza scorul dupa mutare si
			se testeaza daca mutarea este valida.
		********************************************/
		copy(board->nr, test);
		info->score += left(board->nr, &test_left);

		/* Se actualizeaza 'highest score' */
		if (info->h_score < info->score)
			info->h_score = info->score;

		/* Se actualizeaza vectorul care retine pozitiile libere din joc */
		update_v(board, vect, info);

		/**************************************************
				Daca miscarea este valida se adauga un nou
			numar si creste numarul de mutari.
		**************************************************/
		if (test_left) {
			new_number(board, vect, info);
			info->moves++;
		}

		/* Se afiseaza jocul si se verifica daca s-a terminat */
		game_display(win, panel, cell, board, info);
		check(win, game, board, vect, info);
		break;

	case 's':
		/*********************************************
				Se copiaza scorul si matricea jocului
			pentru undo, inainte de modificare.
		*********************************************/
		info->copy_scr = info->score;
		copy(board->nr, board->copy);
		info->copied = 1;

		/********************************************
				Se calculeaza scorul dupa mutare si
			se testeaza daca mutarea este valida.
		********************************************/
		copy(board->nr, test);
		info->score += down(board->nr, &test_down);

		/* Se actualizeaza 'highest score' */
		if (info->h_score < info->score)
			info->h_score = info->score;

		/* Se actualizeaza vectorul care retine pozitiile libere din joc */
		update_v(board, vect, info);

		/**************************************************
				Daca miscarea este valida se adauga un nou
			numar si creste numarul de mutari.
		**************************************************/
		if (test_down) {
			new_number(board, vect, info);
			info->moves++;
		}

		/* Se afiseaza jocul si se verifica daca s-a terminat */
		game_display(win, panel, cell, board, info);
		check(win, game, board, vect, info);
		break;

	case 'd':
		/**********************************************
				Se copiaza scorul si matricea jocului
			pentru undo, inainte de modificare.
		**********************************************/
		info->copy_scr = info->score;
		copy(board->nr, board->copy);
		info->copied = 1;

		/********************************************
				Se calculeaza scorul dupa mutare si
			se testeaza daca mutarea este valida.
		********************************************/
		copy(board->nr, test);
		info->score += right(board->nr, &test_right);

		/* Se actualizeaza 'highest score' */
		if (info->h_score < info->score)
			info->h_score = info->score;

		/* Se actualizeaza vectorul care retine pozitiile libere din joc */
		update_v(board, vect, info);

		/**************************************************
				Daca miscarea este valida se adauga un nou
			numar si creste numarul de mutari.
		**************************************************/
		if (test_right) {
			new_number(board, vect, info);
			info->moves++;
		}

		/* Se afiseaza jocul si se verifica daca s-a terminat */
		game_display(win, panel, cell, board, info);
		check(win, game, board, vect, info);
		break;

	case 'w':
		/*********************************************
				Se copiaza scorul si matricea jocului
			pentru undo, inainte de modificare.
		*********************************************/
		info->copy_scr = info->score;
		copy(board->nr, board->copy);
		info->copied = 1;

		/*******************************************
				Se calculeaza scorul dupa mutare si
			se testeaza daca mutarea este valida
		*******************************************/
		copy(board->nr, test);
		info->score += up(board->nr, &test_up);

		/* Se actualizeaza 'highest score' */
		if (info->h_score < info->score)
			info->h_score = info->score;

		/*Se actualizeaza vectorul care retine pozitiile libere din joc */
		update_v(board, vect, info);

		/**************************************************
				Daca miscarea este valida se adauga un nou
			numar si creste numarul de mutari.
		**************************************************/
		if (test_up) {
			new_number(board, vect, info);
			info->moves++;
		}

		/* Se afiseaza jocul si se verifica daca s-a terminat */
		game_display(win, panel, cell, board, info);
		check(win, game, board, vect, info);
		break;

	case 'r':
		/************************************************
				Optiunea de undo este posibila doar daca
			anterior s-au facut mutari.
		************************************************/
		if (info->moves > 0) {
			/******************************************************
					Scade numarul de mutari.
					Scorul si tabla de joc iau valoarea anterioara
				ultimei mutari, apoi se afiseaza jocul.
			******************************************************/
			copy(board->copy, board->nr);
			info->copied = 0;
			info->score = info->copy_scr;
			info->moves--;
			game_display(win, panel, cell, board, info);
		}
		break;
	}
}

void game(WINDOW *win, mat *board, zero *vect, data *info)
{
	char key, auto_key, buffer[1];
	int i, j, k = 0;
	int nfds = 1, sel;
	fd_set read_descriptors;
	struct timeval timeout;
	FD_ZERO(&read_descriptors);
	FD_SET(KEYBOARD, &read_descriptors);
	timeout.tv_sec = S_TO_WAIT;
	timeout.tv_usec = MILIS_TO_WAIT;

	/********************************************************
			Initializeaza tabla de joc, panoul cu informatii
		si celulele din tabla de joc.
	********************************************************/
	WINDOW *game = subwin(win, GAME_H, GAME_W, 0, 0);
	WINDOW *panel = subwin(win, GAME_H, PANEL_W, 0, GAME_W);
	WINDOW *cell[16];
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			cell[k++] = subwin(game, GAME_H / N, GAME_W / N,
							   i * (GAME_H / N), j * (GAME_W / N));

	/* Afiseaza jocul inaite de a primi comenzi de la jucator */
	game_display(win, panel, cell, board, info);

	while (FOREVER) {
		sel = select(nfds, &read_descriptors, NULL, NULL, &timeout);

		switch (sel) {
		case SELECT_EVENT:
			/*******************************************************
				Jucatorul a introdus o tasta si a apasat Enter.
				Se interpreteaza alegerea in functia game_option().
			*******************************************************/
			scanf("%s", buffer);
			sscanf(buffer, "%c", &key);
			game_option(win, game, cell, panel, board, vect, info, key);
			break;
		case SELECT_NO_EVENT:
			/*****************************************************************
				A trecut timpul predefinit si jucatorul nu a selectat nimic.
				Se calculeaza miscarea care elibereaza cele mai multe casute.
				Se apeleaza game_option() pentru miscarea calculata.
			*****************************************************************/
			auto_key = best_option(board);
			game_option(win, game, cell, panel, board, vect, info, auto_key);
			break;
		case SELECT_ERROR:
			break;
		default:
			continue;
		}

		FD_SET(0, &read_descriptors);
		timeout.tv_sec = S_TO_WAIT;
		timeout.tv_usec = MILIS_TO_WAIT;
	}
}

/* Afiseaza meniul */
void menu_display(WINDOW *menu, data *info, int choice)
{
	start_color();
	init_color(COLOR_GREY, 500, 500, 500);

	/* Culoarea specifica optiunii selectate */
	init_pair(13, COLOR_WHITE, COLOR_MAGENTA);

	/* Culoarea specifica optiunii selectate, dar invalide */
	init_pair(14, COLOR_GREY, COLOR_MAGENTA);

	/* Culoarea specifica optiunii neselectate si invalida */
	init_pair(15, COLOR_GREY, COLOR_BLACK);

	int text_h, text_w;
	char new[9] = "New Game";
	char res[7] = "Resume";
	char qt[5] = "Quit";

	/**********************************************
			Pozitia la care va fi afisata optinea
		din mijloc.
			Celelalte optiuni se vor pozitiona
		relativ la aceasta.
			Centrarea pe rand se face mai jos cu
		ajutorul functiei strlen().
	**********************************************/
	text_h = MENU_H / 2;
	text_w = MENU_W / 2;

	switch (choice) {
	case 1:
		/* Optiunea 'New Game' selectata */
		wattron(menu, COLOR_PAIR(13));
		mvwaddstr(menu, text_h - 3, text_w - strlen(new) / 2, new);
		wattroff(menu, COLOR_PAIR(13));

		if (!info->moves) {
			wattron(menu, COLOR_PAIR(15));
			mvwaddstr(menu, text_h, text_w - strlen(res) / 2, res);
			wattroff(menu, COLOR_PAIR(15));
		} else {
			mvwaddstr(menu, text_h, text_w - strlen(res) / 2, res);
		}

		mvwaddstr(menu, text_h + 3, text_w - strlen(qt) / 2, qt);
		break;
	case 2:
		/* Optiunea 'Resume' selectata */
		mvwaddstr(menu, text_h - 3, text_w - strlen(new) / 2, new);

		if (!info->moves) {
			wattron(menu, COLOR_PAIR(14));
			mvwaddstr(menu, text_h, text_w - strlen(res) / 2, res);
			wattroff(menu, COLOR_PAIR(14));
		} else {
			wattron(menu, COLOR_PAIR(13));
			mvwaddstr(menu, text_h, text_w - strlen(res) / 2, res);
			wattroff(menu, COLOR_PAIR(13));
		}

		mvwaddstr(menu, text_h + 3, text_w - strlen(qt) / 2, qt);
		break;
	case 3:
		/* Optiunea 'Quit' selectata */
		mvwaddstr(menu, text_h - 3, text_w - strlen(new) / 2, new);

		if (!info->moves) {
			wattron(menu, COLOR_PAIR(15));
			mvwaddstr(menu, text_h, text_w - strlen(res) / 2, res);
			wattroff(menu, COLOR_PAIR(15));
		} else {
			mvwaddstr(menu, text_h, text_w - strlen(res) / 2, res);
		}

		wattron(menu, COLOR_PAIR(13));
		mvwaddstr(menu, text_h + 3, text_w - strlen(qt) / 2, qt);
		wattroff(menu, COLOR_PAIR(13));
		break;
	default:
		break;
	}
	wrefresh(menu);
}

/* Alege optiunea din meniu */
void menu_option(WINDOW *win, mat *board, zero *vect, data *info, int choice)
{
	FILE *file;
	switch (choice) {
	case 1:
		/**************************************************
			Cazul unui joc nou(New Game).
			Se initializeaza jocul, apoi se intra in joc.
		**************************************************/
		init_game(board, vect, info);
		game(win, board, vect, info);
		break;
	case 2:
		/*****************************************************
				Cazul 'Resume'.
				Daca exista un joc existent, se intra in joc.
				Existenta unui joc nou este verificata prin
			numarul de mutari realizate pana atunci.
		*****************************************************/
		if (info->moves > 0)
			game(win, board, vect, info);
		break;
	case 3:
		/**********************************************
				Cazul 'Quit'.
				Se scriu cele 3 structuri in fisierul
			'save.txt'.
				Se inchide fereastra, se reseteaza
			cursorul, se iese din program.
		**********************************************/
		endwin();
		curs_set(2);
		file = fopen(SAVE, "wb");
		if (file) {
			fwrite(board, sizeof(mat), 1, file);
			fwrite(vect, sizeof(zero), 1, file);
			fwrite(info, sizeof(data), 1, file);
			fclose(file);
		}
		exit(0);
		break;
	default:
		/* In acest caz nu ar trebui sa se ajunga niciodata */
		endwin();
		curs_set(2);
		printf("Error");
		exit(1);
	}
}

/* Afiseaza desenul '2048' salvat in fisierul 'ascii_art.txt' */
void ascii_display(WINDOW *win)
{
	int win_height, win_width, pos_h, pos_w;
	char row[ASCII_LEN];
	int i;

	/****************************************************
			Se calculeaza pozitia desenului in functie
		de dimensiunile lui predefinite in ASCII_LEN si
		ASCII_ROWS si de dimensiunile meniului.
	****************************************************/
	getmaxyx(win, win_height, win_width);
	pos_h = (win_height - (MENU_H + ASCII_ROWS + SP)) / 2;
	pos_w = (win_width - ASCII_LEN) / 2;

	start_color();
	init_pair(16, COLOR_MAGENTA, COLOR_BLACK);
	wattron(win, COLOR_PAIR(16));

	/************************************************************
			Se citeste linie cu linie din fisierul
		'ascii_art.txt' in vectorul row si se afiseaza pe ecran.
	************************************************************/
	FILE *file = fopen(ASCII_FILE, "r");
	if (file) {
		for (i = 0; i < ASCII_ROWS; i++) {
			fgets(row, ASCII_LEN, file);
			fgetc(file); /* Citeste '\n' */
			mvwprintw(win, i + pos_h, pos_w, "%s", row);
		}
	}
	wattroff(win, COLOR_PAIR(16));
	fclose(file);
}

void menu(WINDOW *win, mat *board, zero *vect, data *info)
{
	/* Se curata ecranul inainte de orice afisare a meniului */
	clear();

	/*******************************************************
			Faptul ca alegerea implicita este New Game este
		reprezentat de choice = 1
	*******************************************************/
	int key, choice = 1;
	int win_height, win_width, pos_h, pos_w;

	/*****************************************************************
			Se calculeaza pozitia la care va fi plasat meniul in win
		in functie de dimensiunile meniului MENU_H si MENU_W.
			Se calculeaza pozitia pe vericala tinand cont de scrisul
		'ascii art' si spatiul (reprezentat de SP) dintre 'ascii
		art' si chenarul meniului.
	*****************************************************************/
	getmaxyx(win, win_height, win_width);
	pos_h = (win_height - (MENU_H + ASCII_ROWS + SP)) / 2 + ASCII_ROWS + SP;
	pos_w = (win_width - MENU_W) / 2;

	/* Initializeaza meniul */
	WINDOW *menu = subwin(win, MENU_H, MENU_W, pos_h, pos_w);

	/* Afiseaza desenul 'ascii art' */
	ascii_display(win);

	/* Contur pentru fereastra si meniu */
	box(win, 0, 0);
	box(menu, 0, 0);
	wrefresh(win);

	/* Se afiseaza optiunile din meniu */
	menu_display(menu, info, choice);

	keypad(stdscr, TRUE);
	while (FOREVER) {
		key = getch();

		/*********************************************************************
			Daca este apasata tasta up, se va trece la optiunea de deasupra.
			Daca suntem deja pe prima optiune, se trece la ultima.
		*********************************************************************/
		if (key == KEY_UP) {
			if (choice == 1)
				choice = 3;
			else
				choice--;
			menu_display(menu, info, choice);
		}
		/**********************************************************************
			Daca este apasata tasta down, se va trece la optiunea de dedesubt.
			Daca suntem deja pe ultima optiune, se trece la prima.
		**********************************************************************/
		else if (key == KEY_DOWN) {
			if (choice == 3)
				choice = 1;
			else
				choice++;
			menu_display(menu, info, choice);
		}
		/***********************************************************
			La apasarea tastei Enter se alege in optiunea selectata.
			Acest lucru se realizeaza in menu_option.
		***********************************************************/
		else if (key == '\n') {
			keypad(stdscr, FALSE);
			menu_option(win, board, vect, info, choice);
			keypad(stdscr, TRUE);
		}
	}
}

/* Testeaza existenta fisierului save.txt */
int file_existed(void)
{
	int existed = 0;
	FILE *file = fopen(SAVE, "rb");
	if (file) {
		existed = 1;
		fclose(file);
	}
	return existed;
}

int main(void)
{
	/* Se initializeaza ecranul */
	WINDOW *win = initscr();

	/* Se inhiba afisarea caracterelor introduse de la tastatura */
	noecho();

	/* Caracterele introduse sunt citite imediat - fara 'buffering' */
	cbreak();

	/* Se ascunde cursorul */
	curs_set(0);

	/* Se initializeaza functia rand() */
	unsigned int seed = time(NULL);
	srand(seed);

	mat board;
	zero vect;
	data info;

	/**************************************************************
			Daca nu exista fisierul save.txt, se initializeaza
		cel mai mare scor cu 0 pentru a se putea compara ulterior
		cu scorul curent. In acest caz se initializeaza numarul
		de mutari cu 0 pentru a dezactiva optiunea resume.
			Daca fisirul exista deja se incarca datele din el in
		structurile aferente.
	**************************************************************/
	int existed = file_existed();
	if (!existed) {
		info.h_score = 0;
		info.moves = 0;
	} else {
		FILE *file = fopen(SAVE, "rb");
		fread(&board, sizeof(mat), 1, file);
		fread(&vect, sizeof(zero), 1, file);
		fread(&info, sizeof(data), 1, file);
		fclose(file);
	}

	/* Se intra in functia pentru meniu */
	menu(win, &board, &vect, &info);

	/* Se inchide fereastra ncurses */
	endwin();

	return 0;
}
