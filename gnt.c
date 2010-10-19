/* * * * * * * * * * * * * * * * * * * * * * * *
 * Give And Take                               *
 *                                             *
 * game by Christopher Elis from Union Square  *
 *                                             *
 * program by QuakePhil                        *
 *                                             *
 *                        http://quakephil.com *
 * * * * * * * * * * * * * * * * * * * * * * * */

// This program is free to do whatever you want with it as long as I get due credit :)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
// #include "qsort.h"
#include "data.h"
#include "hash.h"
#include "misc.h"
#include "precompute.h"
#include "rules.h"
#include "perft.h"
#include "bitbases.h"
#include "opening.h"
#include "evaluate.h"
#include "think.h"
#include "endgame.h"

int parse(char *command, char *test)
	{
	int i;
	int match = 1;
	for (i = 0; i < strlen(test); ++i)
		if (command[i] != test[i])
			match = 0;

	return match;
	}

int main(int argc, char *argv[])
	{
	precompute();
#ifdef USE_HASH
	hash_init();
#endif
	// load_endgames();
//	load_bitbases();
	if (argc == 2 && !strcmp(argv[1], "nobook"))
		{
		printf("Skipping opening book\n");
		}
	else if (argc == 2 && !strcmp(argv[1], "makebook"))
		{
		load_opening_book(1);
		opening_book_grow(0);
		return 0;
		}
	else if (argc == 2 && !strcmp(argv[1], "showbook"))
		{
		load_opening_book(1);
		opening_book_grow(1);
		return 0;
		}
	else
		load_opening_book(2);
	new_game();

	int ply;

	int i, j, k;

	FILE * input;
	FILE * output;

	char command[80] = "";
	char buffer[4100] = "";


/*	if (argc == 2 && !strcmp(argv[1], "files"))
		{
		printf("Using files for I/O\n");
		input = fopen("input.txt", "r+");
		output = fopen("output.txt", "w");
		}
	else */
		{
		input = stdin;
		output = stdout;
		}

	if (argc == 2 && !strcmp(argv[1], "bitbases"))
		{
		bitbase_report();
		return 0;
		}

	if (argc == 7 && !strcmp(argv[1], "position"))
		{
		i = bitbase_test(
			atoi(argv[2])
			,atoi(argv[3])
			,atoi(argv[4])
			,atoi(argv[5])
			,atoi(argv[6])
			);
		printf("%s\n", i ? "is a win for black" : "not a win for black");

		int pieces[MAX_BDB_PIECES];
		int total_pieces = 0;

		for (i = 0; i < atoi(argv[2]); ++i) pieces[total_pieces++] = 0;
		for (i = 0; i < atoi(argv[3]); ++i) pieces[total_pieces++] = 1;
		for (i = 0; i < atoi(argv[4]); ++i) pieces[total_pieces++] = 2;
		for (i = 0; i < atoi(argv[5]); ++i) pieces[total_pieces++] = 3;

		position_decode(atoi(argv[6]), pieces, total_pieces);

		print_board(0);

		return 0;
		}

	if (argc == 2 && !strcmp(argv[1], "sanity"))
		{
		sanity_checks();
		memory_report();
		return 0;
		}

	if (argc == 2 && !strcmp(argv[1], "perft"))
		{
		iterative_performance_test();
#ifdef SORT_STATISTIC
	for (i = 0; i < 5; ++i)
		printf("Returns at sort level %d: %d\n"
			, i
			, sort_statistic[i]);
#endif
		return 0;
		}

	if (argc >= 4 && !strcmp(argv[1], "pdn"))
		{
		ply = 0;

		FILE * pdn;

		pdn = fopen(argv[3], "a+");
		if (!pdn)
			{
			printf("error: fopen \"%s\" for append/create\n", argv[3]);
			// printf("err: %d - %s\n", errno, strerror(errno));
			// getcwd(buffer, sizeof(buffer));
			// printf("pwd: %s\n", buffer);
			return 1;
			}
		while (!feof(pdn)) // fixme: this loader sux...
			{
			if (!fgets(buffer, 4096, pdn))
				break;

			//if (!buffer || feof(pdn))
			//	break;

			i = 0;
			while (j = load_move_from_pdn(buffer, command, &i))
				{
				generate_moves(0, 0);
				k = find_move(0, command);
				if (k < 0 || k >= move_counter[0])
					{
					fprintf(output, "error invalid move from pdn (%s)\n"
						, command);
					for (k = 0; k < move_counter[0]; ++k)
						{
						fprintf(output, "move %d ",k);
						print_move(&moves[0][k], 0);
						fprintf(output, "\n");
						}
					}
				else
					{
					++ply;
					//fprintf(output, "executing move at index %d: ", k);
					if (!strcmp(argv[2], "show") || !strcmp(argv[2], "step"))
						{
						if (ply % 2 == 1)
							printf("%3d. ", (ply+1) / 2);
						print_move(&moves[0][k], 0);
						if (ply % 2 == 0)
							{
							printf("\n");
							//print_board(0);
							}
						else
							printf(" ");
						if (!strcmp(argv[2], "step") && ply % 2 == 0)
							{
							print_board(0);
							getc(stdin);
							}
						}
					//fprintf(output, "\n");
					move_do(&moves[0][k], 0);
					move_list_push(&moves[0][k]);
					}
				}
			if (feof(pdn))
				break;
			}
		fclose(pdn);

		if (!strcmp(argv[2], "play"))
			{
			pdn = fopen(argv[3], "a");
			if (!pdn)
				{
				printf("error: fopen \"%s\" for append\n", argv[3]);
				return 1;
				}

			if (argc >= 5 && strcmp(argv[4], "hint") && strcmp(argv[4], "move"))
				{
				generate_moves(0, 0);
				i = find_move(0, argv[4]);
				if (i < 0 || i >= move_counter[0])
					{
					print_moves(0);
					i = -1;
					}
				else
					{
					move_do(&moves[0][i], 0);
					move_list_push(&moves[0][i]);
					fprintf(pdn, "%s ", argv[4]);
					if (moves[0][i].same_player_square != -1)
						i = -1;
					}
				}

			if (i != -1)
				{
				if (argc >= 6)
					strength = atoi(argv[5]);
				if (argc >= 7)
					thinking_time = atoi(argv[6]);

				if (strength == 1)
					thinking_time = 1;

				if (!strcmp(argv[4], "hint"))
					bestmove(0, 0);
				else
					while (bestmove(pdn, 1));
				}

			fclose(pdn);
			fprintf(output, "boardok ");
			for (i = 0; i < 64; ++i)
				fprintf(output, "%d", board[i]);
			fprintf(output, " %d", state.tactical_square);
			fprintf(output, " %d", state.side_to_move);
			fprintf(output, " %d", state.this_piece_must_take);
			fprintf(output, "\n");
			}
		else
			{
			print_board(0);
//debug_moves = 1;
			generate_moves(0, 0);
			if (!strcmp(argv[2], "show"))
				print_moves(0);
			}

		if (!strcmp(argv[2], "load"))
			{
			printf("PDN loaded\n");
			command[0] = 0;
			buffer[0] = 0;
			}
		else
			return 0;
		}

	for (;;)
		{
		fscanf(input, "%20s", command);

		if (parse(command, "quit"))
			break;
		else if (parse(command, "gnt"))
			fprintf(output, "id name GNTredux\nid author QuakePhil\ngntok\n");
		else if (parse(command, "new"))
			new_game();
		else if (parse(command, "flipside"))
			state.side_to_move ^= 1;
		else if (parse(command, "flipkings"))
			{
			for (i = 0; i < 64; ++i) if (initial_board[i] != 4)
				{
				if (initial_board[i] < 2)
					initial_board[i] += 2;
				else
					initial_board[i] -= 2;
				}
			new_game();
			}
		else if (parse(command, "flipboardonly"))
			{ // fixme: need to re-do materials here?
			int temp[64];
			int flip_piece[5] = {1,0,3,2,4};
			j = 64;
			for (i = 0; i < 64; ++i)
				temp[--j] = flip_piece[board[i]];
			for (i = 0; i < 64; ++i)
				board[i] = temp[i];
			state.side_to_move ^= 1;
			}
		else if (parse(command, "rand"))
			rand_board();
		else if (parse(command, "think"))
			{
			bestmove(0, 0);
			}
		else if (parse(command, "go"))
			{
			bestmove(0, 1);
			}
		else if (parse(command, "perft"))
			{
			iterative_performance_test();
			}
		else if (parse(command, "ping"))
			{
			fprintf(output, "pong %.6lf\n", timer());
			}
		else if (parse(command, "pboard"))
			print_board(0);
		else if (parse(command, "board"))
			{
			fprintf(output, "boardok ");
			for (i = 0; i < 64; ++i)
				fprintf(output, "%d", board[i]);
			fprintf(output, " %d", state.tactical_square);
			fprintf(output, " %d", state.side_to_move);
			fprintf(output, " %d", state.this_piece_must_take);
			fprintf(output, "\n");
			}
		else if (parse(command, "time"))
			{
			fscanf(input, "%d", &thinking_time);
			printf("Think time limit set to: %d seconds\n", thinking_time);
			}
		else if (parse(command, "pdn"))
			{
			fscanf(input, "%20s", buffer);
			generate_moves(0, 0);
			i = find_move(0, buffer);
			if (i < 0 || i >= move_counter[0])
				fprintf(output, "error invalid move\n");
			else
				{
				move_do(&moves[0][i], 0);
				move_list_push(&moves[0][i]);
				//fprintf(output, "executing move at index %d: ", i);
				//print_move(&moves[0][i], 0);
				//fprintf(output, "\n");
				fprintf(output, "done\n");
				}
			}
		else if (parse(command, "do") || parse(command, "toggle"))
			{
			fscanf(input, "%d", &i);
			fprintf(output, "executing move %d\n", i);
			generate_moves(0, 0);
			if (i < 0)
				fprintf(output, "error move can't be negative\n");
			else if (i >= move_counter[0])
				fprintf(output, "error no such move\n");
			else
				{
				move_do(&moves[0][i], 0);
				move_list_push(&moves[0][i]);

				if (parse(command, "toggle"))
					{
					print_board(0);
					fprintf(output, "undoing move %d\n", i);
					move_undo(&moves[0][i], 0);
					print_board(0);
					}
				}
			fprintf(output, "done\n");
			}
		else if (parse(command, "list"))
			{
			print_move_list();
			}
		else if (parse(command, "pbook"))
			{
			print_book();
			}
		else if (parse(command, "moves"))
			{
			generate_moves(0, 0);
			for (i = 0; i < move_counter[0]; ++i)
				{
				fprintf(output, "move %d ",i);
				print_move(&moves[0][i], 1);
				fprintf(output, "\n");
				}
			fprintf(output, "movesok\n");
			}
		else // if (!strcmp(command, "help"))
			{
			// fprintf(output, "?\n");
			}

		fflush(output);
		}

#ifdef HASH_STATISTIC
	printf("Hash saves:      %d\n", hash_saves);
	printf("Hash collisions: %d\n", hash_collisions);
	printf("Hash probes:     %d\n", hash_probes);
	printf("Hash hits:       %d\n", hash_hits);
#endif

#ifdef SORT_STATISTIC
	for (i = 0; i < 5; ++i)
		printf("Returns at sort level %d: %d\n"
			, i
			, sort_statistic[i]);
#endif

	fclose(output);
	fclose(input);

	return 0;
	}
