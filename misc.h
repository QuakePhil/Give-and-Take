//#define WINDOWS

#ifdef WINDOWS
#include <windows.h>
double timer()
	{
	SYSTEMTIME st;
	GetSystemTime(&st);
	return (st.wHour*3600)
		+ (st.wMinute*60)
		+ (st.wSecond)
		+ (st.wMilliseconds)/100.0;
/* this implementation goes back in time???
		FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	unsigned long tmpres = 0;
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
	tmpres -= 11644473600000000ULL;
	tmpres /= 10;
	return (long)(tmpres / 1000000UL) +
		((long)(tmpres % 1000000UL)/1000000.0);
		*/
	}
#endif

#ifndef WINDOWS
#include <sys/time.h>
#include <sys/time.h>

/* ========================================== */
/* should be a different function for windows */
/* ========================================== */
double timer()
	{
	static struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec+(t.tv_usec/1000000.0);
	}
#endif

/* ============================================ */
/* helper to print a move with optional details */
/* ============================================ */
void print_move(MOVE * move, int detail)
	{
	// squares are numbered 1...64
	if (move->from == move->to || move->from == -1)
		printf("\?\?-\?\?");
	else
		printf("%s%s%s"
			, notation[move->from]
			, move->captured == -1 ? "-" : "x"
			, notation[move->to]);
	if (detail)
		{
		if (move->same_player_square != -1)
			printf(",");
		printf(" (%s from %s to %s"
			,move->tactical ? "tactical" : "quiet"
			,notation[move->from]
			,notation[move->to]);
		if (move->captured != -1)
			{
			if (move->forced)
				printf(", forced to capture %s", notation[move->captured]);
			else
				printf(", capturing %s", notation[move->captured]);
			}
		if (move->same_player_square != -1)
			printf(", same player must capture from %s", notation[move->same_player_square]);
		printf(", if %s can be captured, it must be captured", notation[move->to]);

		if (move->forcing == 1)
			printf(", next move is forced");

		printf(", value %d)", move->evaluation);
		}
	}

void print_moves(int ply)
	{
	int i;
	for (i = 0; i < move_counter[ply]; ++i)
		{
		printf("move %d ",i);
		print_move(&moves[ply][i], 0);
		printf("\n");
		}
	}

/* ======================= */
/* copy a move description */
/* ======================= */
inline void zcopy_move(MOVE * d, MOVE * s)
	{
	d->from = s->from;
	d->to = s->to;
	d->captured = s->captured;
	d->tactical = s->tactical;
	d->same_player_square = s->same_player_square;
	d->forced = s->forced;
	d->forcing = s->forcing;
	}
#define copy_move(a,b) do {(*(a)) = (*(b));} while(0)

void print_board(int flip)
	{
	int i, j;

	printf("Side to move: %s\n",
		state.side_to_move == WHITE ? "white" : (state.side_to_move == BLACK ? "black" : "unknown"));
	printf("Tactical square: %s\n",
		state.tactical_square == -1 ? "not yet set" : notation[state.tactical_square]); 
	hash_set();
//	printf("white = %d, black = %d - kings: %d, %d - pawns: %d, %d); hash = %X, hashvalue = %d; eval = %d\n"
	printf("white = %d, black = %d *** kings: %d, %d *** pawns: %d, %d *** eval = %d\n"
		, material[0]
		, material[1]*-1
		, kings[0]
		, kings[1]
		, pawns[0]
		, pawns[1]
		//, hash
		//, hash_probe_eval()
		, evaluate());
/*
	for (i = 1; i >= 0; --i)
		{
		for (j = 0; j < 8; ++j)
			printf("%d ", blocks[i][j]);
		printf ("<- %s blocks\n", i == 0 ? "white" : "black");
		}
*/	printf("=================\n");

	if (flip == 1)
		{
		flip = 0;
		for (i = 7; i > -1; --i)
			printf("%c ", notation_columns[i]);
		printf("\n");

		for (i = 63; i > -1; --i)
			{
			printf("%c ", piece[board[i]]);
			if (i % 8 == 0)
				printf("%d \n", ++flip);
			}
		}
	else
		{
		flip = 9;
		for (i = 0; i < 8; ++i)
			printf("%c ", notation_columns[i]);
		printf("\n");

		for (i = 0; i < 64; ++i)
			{
			printf("%c ", piece[board[i]]);
			if (i % 8 == 7)
				printf("%d \n", --flip);
			}
		}
	}

void new_game()
	{
	int i;

	thinking_ply = MAXPLY;

	generate_quiescent = 0;

	material[0] = material[1] =
	kings[0] = kings[1] = 0;
	pawns[0] = pawns[1] = 16;

	for (i = 0; i < 64; ++i)
		{
		board[i] = initial_board[i];
		if (board[i] != 4)
			material[board[i]%2] += piece_values[board[i]];
		if (board[i] == 2) ++kings[0];
		if (board[i] == 3) ++kings[1];
		}

	state.tactical_square = -1;

	state.this_piece_must_take = -1;

	state.side_to_move = WHITE;
	}

void rand_board()
	{
	srand(time(NULL));

	int i;

	new_game();

	state.side_to_move = rand()%2;

	for (i = 0; i < 64; ++i)
		board[i] = 4;

	for (i = 0; i < 8; ++i)
		board[rand()%16+16] = 1;
	for (i = 0; i < 8; ++i)
		board[rand()%16+32] = 0;

	return;


	state.side_to_move = WHITE;

	for (i = 0; i < 64; ++i)
		board[i] = 4;
	board[rand()%64] = 2;
	board[rand()%64] = 2;
	board[rand()%64] = 1;
	board[rand()%64] = 1;
	board[rand()%64] = 1;
	board[rand()%64] = 1;
	board[state.tactical_square = rand()%64] = 1;

	state.this_piece_must_take = -1;
	}

void memory_report()
	{
	printf("Search hash sized at %dMB for %d entries\n"
		, sizeof(hash_table)/1024/1024
		, HASH_SIZE);
	printf("Evaluation hash sized at %dMB for %d entries\n"
		, sizeof(ehash_table)/1024/1024
		, EHASH_SIZE);
	}

void move_list_push(MOVE * move)
	{
	sprintf(move_list[move_list_counter], "%s-%s", notation[move->from], notation[move->to]);
	++move_list_counter;
	}

void print_move_list()
	{
	int i;

	printf ("Move list contains %d moves:\n", move_list_counter);
	for (i = 0; i < move_list_counter; ++i)
		printf ("%s\n", move_list[i]);
	printf ("End of move list\n");
	}

int load_move_from_pdn(char * buffer, char * movestr, int * i)
	{
	int j = 0;

	while (buffer[(*i)] <= 32 && buffer[(*i)])
		++(*i);
	if (buffer[(*i)] < 32)
		return 0;

	while (buffer[(*i)] > 32)
		movestr[j++] = buffer[(*i)++];
	movestr[j] = 0;

	return 1;
	}

int find_move(int ply, char *pdn)
	{
	int i, j, match;
	char candidate_pdn[10];

	for (i = 0; i < move_counter[ply]; ++i)
		{
		match = 1;
		sprintf(candidate_pdn, "%s%s%s"
		,notation[moves[ply][i].from]
		,moves[ply][i].captured == -1 ? "-" : "x"
		,notation[moves[ply][i].to]);
		for (j = 0; j < 5; ++ j)
			if (candidate_pdn[j] != pdn[j] && j != 2)
				match = 0;
			if (match)
				{
				return i;
				}
		}
	return -1;
	}

char flip_file(char file)
	{
	return file + (('h' - file) * 3) + (file - 'h' - 7);
	}

// I had it working for a while, but now it stopped working :(
void insertion_sort(MOVE array[], int (*compare)(const void*, const void*), int len)
	{
	int i;
	MOVE *cur_val;
	MOVE *prev_val;

	if (len <= 1)
		return;

	prev_val = &array[0];

	for (i = 1; i < len; ++i)
		{
		cur_val = &array[i];
		if ((*compare)(prev_val, cur_val) >= 0)
			{
			/* out of order: array[i-1] > array[i] */
			int j;
			copy_move(&array[i], prev_val); /* move up the larger item first */

			/* find the insertion point for the smaller item */
			for (j = i - 1; j > 0;)
				{
				MOVE *temp_val = &array[j - 1];
				if ((*compare)(temp_val, cur_val) >= 0)
					{
					copy_move(&array[j--], temp_val);
					/* still out of order, move up 1 slot to make room */
					}
				else
					break;
				}
			copy_move(&array[j], cur_val); /* insert the smaller item right here */
			}
		else
			{
			/* in order, advance to next element */
			prev_val = cur_val;
			}
		}

	}

void bubble_sort2(MOVE array[], int (*compare)(const void*, const void*), int len)
	{
	int i, j;
	MOVE *val1;
	MOVE *val2;
	MOVE tmp;
	int flipped;

	if (len <= 1)
		return;

	i = 1;
	do
		{
		flipped = 0;
		for (j = len - 1; j >= i; --j)
			{
			val1 = &array[j];
			val2 = &array[j - 1];
			if ((*compare)(val1, val2) <= 0)
				{
				tmp = *val1;
				array[j] = array[j-1];
				array[j-1] = tmp;
				flipped = 1;
				}
			}
		}
	while ((++i < len) && flipped);
	}

void bubble_sort(MOVE array[], int (*compare)(const void*, const void*), int len)
	{
	int i, j;
	MOVE tmp;

	if (len <= 1)
		return;

	for (i = 0; i < len; ++i)
		for (j = 0; j < len; ++j)
			if ((*compare)(&array[i],&array[j]) <= 0)
				{
				copy_move(&tmp, &array[i]);
				copy_move(&array[i], &array[j]);
				copy_move(&array[j], &tmp);
				}
	}

// #include "heapsort.h"
