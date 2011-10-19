#define POS -33022085

// todo is the size right?  are we working in bits in the right places and bytes in the right places?

// once that is done, get a front end to browse a .bdb file, before working on 3 piece bdbs

/* =========================== */
/* here we will use retrograde */
/* analysis to create bitbases */
/* allocating one bit for each */
/* position specyfing if it is */
/* a win or notawin for black. */
/* the implication is if it is */
/* a win for black, white will */
/* see the loss when searching */
/* the alpha-beta tree         */
/* =========================== */

/* ==================================================== */
/* this array to point into which bitbase offset to use */
/* the 2*65 factor is to account for side to move (0/1) */
/* and tactical square (-1 to 64)                       */
/* ==================================================== */
long piece_configuration[3][3][3][3]; // [wp][bp][wk][bk]

/* =================================================== */
/* question: do we *need* a 2-piece database? it seems */
/* 3-piece database will imply 2-piece database in the */
/* same way a 2-piece database will imply a 1-piece db */
/* (when the two pieces are on the same square and the */
/* set-to-win side is the one piece that's kept on the */
/* board) answer: yes, it is needed in case we want to */
/* probe starting from 2 pieces only.  furthermore, we */
/* cannot use the implied less-piece databases because */
/* we cannot re-encode them to the same position index */
/* =================================================== */

#define MAX_BDB_PIECES 3

//#define ABS(a) ((a)>0?(a):(-(a)))
#define ABS(a) (a)
#define MAX(a,b) (ABS(a)>ABS(b)?(a):(b))

//#define BDB_SIZE BDB_2_SIZE
//#define MAX_BDB_PIECES 2

int piece_configuration_pieces[3][3][3][3][MAX_BDB_PIECES]; // for position_decode

int number_of_positions[4] = {0, 0, BDB_2_SIZE, BDB_3_SIZE};

int bitbase_stats[256];
int next_dtm[256]; // stores computed function dtm = (abs(dtm) - 1) * (abs(dtm) / -dtm)

inline int position_encode(int * pieces, int total_pieces)
	{
// should check here that other state as -1
	int position_index, i, j;
	position_index = state.side_to_move + (state.tactical_square + 1) * 2;

	int piece_factor[MAX_BDB_PIECES] = {-1, -1, -1};
	long piece_multiplier[MAX_BDB_PIECES] = {2 * 65, 2 * 65 * 64, 2 * 65 * 64 * 64};

	// we shouldn't have to do this here...
	for (i = 0; i < 64; ++i) if (board[i] != 4)
		{
		for (j = 0; j < total_pieces; ++j)
			{
			if (board[i] == pieces[j] && piece_factor[j] == -1)
				{
				piece_factor[j] = (i) * piece_multiplier[j];
				break;
				}
//			else if (piece_factor[j] == -1)
//				{
//				piece_factor[j] = (i) * piece_multiplier[j];
//				}
			}
		}

//printf("initial index = %d\n", position_index);
//for (j = 0; j < total_pieces; ++j)
//	printf("adding piece_factor[%d] = %d\n", j, piece_factor[j]);

	for (j = 0; j < total_pieces; ++j)
		position_index += piece_factor[j];

	return position_index;
	}

inline int position_decode(int position, int * pieces, int total_pieces)
	{
	int piece_collision = 0;

	// we assume here that other state flags are pre-set by new_game()

	// decode the side to move and tactical square
//printf("position = %d\n", position);
	state.side_to_move = position % 2;
	position >>= 1;
//printf("side to move = %d, position = %d\n", state.side_to_move, position);
	state.tactical_square = (position % 65) - 1;
	position /= 65;
//printf("tsq = %d, position = %d\n", state.tactical_square, position);

	// plenty of speed-up to be had here
	int i, j, k, current_piece;

	// clear all cells (dumb)
	for (i = 0; i < 64; ++i)
		board[i] = 4;
	material[0] = material[1] =
	kings[0] = kings[1] =
	pawns[0] = pawns[1] = 0;

	// decode the pieces
	k = 64;
	current_piece = 0;
	for (i = 0; i < total_pieces; ++i)
		{
//printf("i = %d, total_pieces = %d, k = %d, position = %d\n", i, total_pieces, k, position);
		j = position % k;
//printf("j = position mod k = %d\n", j);
		position /= k;
		//--k;

		// avoid placing (and maintaining kings/pawns/material for)
		// multiple pieces on one square
		if (board[j] == 4)
			{
			board[j] = pieces[current_piece++];

			// maintain material, pawns, kings arrays
			material[board[j]%2] += piece_values[board[j]];
			if (board[j] == 3) ++kings[1];
			else if (board[j] == 2) ++kings[0];
			else if (board[j] == 1) ++pawns[1];
			else if (board[j] == 0) ++pawns[0];
			}
		else
			piece_collision = 1;
		}

	return piece_collision;
	}

/*
127 - win in 0
126 - win in 1
125 - win in 2
...
0 - draw
...
-126 - lose in 1
-127 - lose in 0
-128 - no data
*/

inline char bitbase_test(int white_pawns, int black_pawns, int white_kings, int black_kings, int position_index)
	{
	return bitbase_dtm[piece_configuration[white_pawns][black_pawns][white_kings][black_kings] + position_index];
	}

inline char encode_and_test()
	{
	static int total_pieces;
	static int pieces[MAX_BDB_PIECES];
	static int k;
	if (pawns[0] + kings[0] == 0 || pawns[1] + kings[1] == 0)
		{
		k = evaluate();
		if (k == -20000)
			return -127;
		if (k == 20000)
			return 127;
		return 0; // should never happen
		// return -128;
		}

	total_pieces = 0;
	for (k = 0; k < pawns[0]; ++k) pieces[total_pieces++] = 0;
	for (k = 0; k < pawns[1]; ++k) pieces[total_pieces++] = 1;
	for (k = 0; k < kings[0]; ++k) pieces[total_pieces++] = 2;
	for (k = 0; k < kings[1]; ++k) pieces[total_pieces++] = 3;
	return bitbase_test(pawns[0], pawns[1], kings[0], kings[1], position_encode(pieces, total_pieces));
	}

inline void bitbase_set(int white_pawns, int black_pawns, int white_kings, int black_kings, int position_index, char value)
	{
	bitbase_dtm[piece_configuration[white_pawns][black_pawns][white_kings][black_kings] + position_index] = value;
	}

inline void bitbase_reset(int white_pawns, int black_pawns, int white_kings, int black_kings, int position_index)
	{
	bitbase_dtm[piece_configuration[white_pawns][black_pawns][white_kings][black_kings] + position_index] = -128;
	}

/* ============================================ */
/* given here the numbers of various pieces, we */
/* will loop through all possible (and possibly */
/* impossible) positions, examine the moves for */
/* each, do them, see if the position we arrive */
/* at has been marked as won, and if so then we */
/* mark the parent as won also.  if there is no */
/* change for an iteration then for this set of */
/* pieces we have built our bitbase             */
/* ============================================ */
int build_bitbase2(int white_pawns, int black_pawns, int white_kings, int black_kings)
	{
	int nodata = 0;
	int last_nodata = 0;
	int i = 0;
	do
		{
		nodata = build_bitbase(white_pawns, black_pawns, white_kings, black_kings);
		if (nodata == 0)
			{
			printf("Everything has been calculated.\n");
			return 0;
			}
		if (nodata > 0 && nodata == last_nodata)
			{
			printf("Exiting infinite loop.\n");
			return 1;
			}
		last_nodata = nodata;
		}
	while (nodata > 0);
	return 0;
	}

int build_bitbase(int white_pawns, int black_pawns, int white_kings, int black_kings)
	{
	char bitbase_file[10];

	int position, this_position;
	int total_pieces = 0; // = white_pawns + black_pawns + white_kings + black_kings;

	/* ============================== */
	/* this will contain what kind of */
	/* pieces are being indexed, e.g. */
	/* if it is Km vs K, then we will */
	/* have {2, 0, 3} or if we are up */
	/* to KKmm vs mmmmmK then we will */
	/* have {2,2,0,0,1,1,1,1,1,3}     */
	/* ============================== */
	int pieces[MAX_BDB_PIECES];
	int i, j, k;

	int total_pieces2;
	int pieces2[MAX_BDB_PIECES]; // to be used for secondary lookups

	// the order of pieces[] is important...  i think
	for (i = 0; i < white_pawns; ++i)
		{
		pieces[total_pieces++] = 0;
		bitbase_file[total_pieces-1] = piece[pieces[total_pieces-1]];
		}
	for (i = 0; i < black_pawns; ++i)
		{
		pieces[total_pieces++] = 1;
		bitbase_file[total_pieces-1] = piece[pieces[total_pieces-1]];
		}
	for (i = 0; i < white_kings; ++i)
		{
		pieces[total_pieces++] = 2;
		bitbase_file[total_pieces-1] = piece[pieces[total_pieces-1]];
		}
	for (i = 0; i < black_kings; ++i)
		{
		pieces[total_pieces++] = 3;
		bitbase_file[total_pieces-1] = piece[pieces[total_pieces-1]];
		}
	bitbase_file[total_pieces] = '.';
	bitbase_file[total_pieces+1] = 'b';
	bitbase_file[total_pieces+2] = 'd';
	bitbase_file[total_pieces+3] = 'b';
	bitbase_file[total_pieces+4] = 0;

	for (i = 0; i < 256; ++i) bitbase_stats[i] = 0;

	printf("Building bitbase: %s\n", bitbase_file);

	new_game();

	double t1;

	t1 = timer();

	// check moves leading to wins and mark those too
	char current_best_case_scenario = -128;
	char all_moves_known = 0;
	int original_side_to_move;

	for (position = 0; position < number_of_positions[total_pieces]; ++position)
		{
		if (bitbase_test(white_pawns, black_pawns, white_kings, black_kings, position) != -128)
			continue;

		if (position % 5000000 == 0 && position != 0)
			printf("Position %d out of %d (%.2f%%, %.2lf elapsed)\n", position, number_of_positions[total_pieces]
			,1.0*position/number_of_positions[total_pieces]*100.0, timer()-t1);

		i = position_decode(position, pieces, total_pieces);
		original_side_to_move = state.side_to_move;

//		if (i || kings[0] + kings[1] + pawns[0] + pawns[1] != total_pieces)
//			continue;

		generate_moves(0, 0);
if (position == POS) {print_board(0);}

		current_best_case_scenario = -128;
		all_moves_known = 1;
		if (move_counter[0] == 0 || pawns[0] + kings[0] == 0 || pawns[1] + kings[1] == 0)
			{
			// if no moves, we still need to store something besides draw or -128
			// for example when the board is: W W - - - - - -
			j = evaluate();
			if (j == -20000)
				current_best_case_scenario = -127; // note - this is reversed, because it would
			else if (j == 20000) // only ever be seen from one move up
				current_best_case_scenario = 127;
			else
				current_best_case_scenario = 0;
			}
		else for (i = 0; i < move_counter[0]; ++i)
			{
if (position == POS) {print_move(&moves[0][i], 0);printf("\n");}

			move_do(&moves[0][i], 0);
			j = encode_and_test();

			if (j != -128)
				{
				if (original_side_to_move == state.side_to_move)
					current_best_case_scenario = MAX(-next_dtm[j+128], current_best_case_scenario);
				else
					current_best_case_scenario = MAX(next_dtm[j+128], current_best_case_scenario);
				}

			if (j == -128)
				{
				if (pawns[0] == 0 && pawns[1] == 0
				&& kings[0] == 1 && kings[1] == 1) // this should only happen in K-K
					{
					j = evaluate();
					if (j == -20000) // -20000 is when <side> won and just moved
						current_best_case_scenario = MAX(current_best_case_scenario, 126);
					// draws calculated in K-K only, the rest have to remain -128 to
					// be recalculated again
					else // if (pawns[0] == 0 && pawns[1] == 0 && kings[0] == 1 && kings[1] == 1)
						current_best_case_scenario = MAX(current_best_case_scenario, 0); // draw?

	if (position == POS) {printf("eval() = %d\n", j);}
					}
				else
					{
					all_moves_known = 0;
					}
				}
			move_undo(&moves[0][i], 0);
if (position == POS) {printf("current best case = %d\n", current_best_case_scenario);getc(stdin);}
			}

		if (all_moves_known == 0)
			{
			if (current_best_case_scenario > 0)
				{
				; // preserve known wins for side to move (losses don't work)
				// this way, subsequent passes will calculate longer
				// winning variations, until there is no more to calculate
				}
			else
				current_best_case_scenario = -128;
			}

/*
		// heuristic..  does this belong her?
		// corner set up is a lose in 4 for the 1 king side to move
		if (pawns[0] == 0 && pawns[1] == 0 && kings[0] == 2 && kings[1] == 1 && state.side_to_move == 1)
			{
			if (board[0] == 2 && board[54] == 2 && board[63] == 3) current_best_case_scenario = -123;
			if (board[7] == 2 && board[49] == 2 && board[56] == 3) current_best_case_scenario = -123;
			if (board[63] == 2 && board[9] == 2 && board[0] == 3) current_best_case_scenario = -123;
			if (board[56] == 2 && board[14] == 2 && board[7] == 3) current_best_case_scenario = -123;
			}
		if (pawns[0] == 0 && pawns[1] == 0 && kings[0] == 1 && kings[1] == 2 && state.side_to_move == 0)
			{
			if (board[0] == 3 && board[54] == 3 && board[63] == 2) current_best_case_scenario = -123;
			if (board[7] == 3 && board[49] == 3 && board[56] == 2) current_best_case_scenario = -123;
			if (board[63] == 3 && board[9] == 3 && board[0] == 2) current_best_case_scenario = -123;
			if (board[56] == 3 && board[14] == 3 && board[7] == 2) current_best_case_scenario = -123;
			}
*/

		bitbase_set(white_pawns, black_pawns, white_kings, black_kings, position, current_best_case_scenario);
		++bitbase_stats[127-current_best_case_scenario];
		}
	int total_positions_calculated = 0;
	for (i = 0; i < 256; ++i)
		total_positions_calculated += bitbase_stats[i];

	for (i = 0; i < 256; ++i) if (bitbase_stats[i] > 0)
		{
//printf("bitbase_stats[%d]=%d\n", i, bitbase_stats[i]);
		printf("(%d) ", i);
		if (i == 255)
			printf("No data:   \t%d (%.2f%%)\n"
				,bitbase_stats[i]
				,(float)bitbase_stats[i]/total_positions_calculated*100.0);
		else
			{
			if (i < 127)
				printf("Win in %d:\t%d (%.2f%%)\n"
					,i
					,bitbase_stats[i]
					,(float)bitbase_stats[i]/total_positions_calculated*100.0);
			else if (i > 127)
				printf("Lose in %d:\t%d (%.2f%%)\n"
					,254 - i
					,bitbase_stats[i]
					,(float)bitbase_stats[i]/total_positions_calculated*100.0);
			else
				printf("Draw:      \t%d (%.2f%%)\n"
					,bitbase_stats[i]
					,(float)bitbase_stats[i]/total_positions_calculated*100.0);
			}
		}
	printf("Pass complete in %.2lf seconds, valid positions considered: %d out of %d\n", 
		timer()-t1, total_positions_calculated, number_of_positions[total_pieces]);
	if (bitbase_stats[255] > 0)
		{
		printf("Unknown entries remain, more passes needed\n");
		return bitbase_stats[255];
		}
	return 0;
	}

int load_bitbases()
	{
	/* ================================= */
	/* if you think about how the pieces */
	/* can be configured, you can see it */
	/* like this:                        */
	/* K vs K                            */
	/* K vs KK                           */
	/* K vs Km                           */
	/* K vs mm                           */
	/* KK vs K                           */
	/* Km vs K                           */
	/* mm vs K                           */
	/* then the four pieces will require */
	/* about 2 gigs of dumb space, so we */
	/* will worry about those later :p   */
	/* ================================= */
	int i, j, k;
	piece_configuration[0][0][1][1] = i = 0; // white K vs K
	piece_configuration[0][0][2][0] = i = i + BDB_2_SIZE; // KK vs
	piece_configuration[0][0][0][2] = i = i + BDB_2_SIZE; // vs KK
	piece_configuration[0][0][1][2] = i = i + BDB_2_SIZE; // K vs KK
	piece_configuration[0][1][1][1] = i = i + BDB_3_SIZE; // K vs Km
	piece_configuration[0][2][1][0] = i = i + BDB_3_SIZE; // K vs mm
	piece_configuration[0][0][2][1] = i = i + BDB_3_SIZE; // KK vs K
	piece_configuration[1][0][1][1] = i = i + BDB_3_SIZE; // Km vs K
	piece_configuration[2][0][0][1] = i = i + BDB_3_SIZE; // mm vs K

        if (lzo_init() != LZO_E_OK)
                {
                printf("lzo failed to init, no bitbase support\n");
		return -1;
                }

	double t1 = timer();
	if (decompress_bitbase())
		{
		printf("Loaded bitbases from %s in %.2lf seconds\n", bitbase_dtm_file, timer()-t1);
		return 0;
		}
/*	save = fopen(bitbase_dtm_file, "r");
	if (save) // && 1 == 2)
		{
		double t1 = timer();
		save = fopen(bitbase_dtm_file, "r");
		fread(bitbase_dtm, sizeof(char), BDB_SIZE, save);
		fclose(save);
		printf("Loaded bitbases from %s in %.2lf seconds\n", bitbase_dtm_file, timer()-t1);
		return 0;
		}
*/

	// stores computed function dtm = (abs(dtm) - 1) * (abs(dtm) / -dtm)
//	printf("i\ti-128\tnext dtm\n");
	for (i = 0; i < 256; ++i)
		{
		j = i-128;
		k = j>0?j:-j; // abs(j)
		if (k == 0)
			next_dtm[i] = 0;
		else
			next_dtm[i] = (k - 1) * (k / -j);
//			printf("%d\t%d\t%d\n", i, i-128, next_dtm[i]);
		}

	printf("Resetting bitbase...\n");
	for (i = 0; i < BDB_SIZE; ++i) bitbase_dtm[i] = -128;

	// build in proper order
	build_bitbase2(0, 0, 1, 1); // K-K - only one must be pass required here
	build_bitbase2(0, 0, 2, 0); // KK- and -KK are required also, e.g. so you can know the solution to:
	build_bitbase2(0, 0, 0, 2); // W - B W - - - - ; white to move

	int loops_detected = 0;
	int last_loops_detected = 0;
	do
		{
		loops_detected = 0;
		loops_detected += build_bitbase2(0, 0, 2, 1); // KK-K
		loops_detected += build_bitbase2(0, 0, 1, 2); // K-KK

		loops_detected += build_bitbase2(0, 1, 1, 1); // K-Km
		loops_detected += build_bitbase2(0, 2, 1, 0); // K-mm
		loops_detected += build_bitbase2(1, 0, 1, 1); // Km-K
		loops_detected += build_bitbase2(2, 0, 0, 1); // mm-K

		if (loops_detected > 0 && loops_detected == last_loops_detected)
			{
			printf("Exiting infinite loop of exits of infinite loops :)\n");
			break;
			}
		last_loops_detected = loops_detected;
		}
	while (loops_detected > 0);

	compress_bitbase();

/*
	printf("Writing %s\n", bitbase_dtm_file);
	save = fopen(bitbase_dtm_file, "w");
	if (save)
		{
		fwrite(bitbase_dtm, sizeof(char), BDB_SIZE, save);
		fclose(save);
		}
	else
		printf("Could not write to file\n"); */
	printf("Done\n");
	return 1;
	}

void bitbase_report()
	{
	int i;
	printf("Size required for 2 pieces: %d bits, %d bytes, %d kb\n", BDB_2_SIZE, BDB_2_SIZE/8, BDB_2_SIZE/8/1024);
	printf("Size required for 3 pieces: %d bits, %d bytes, %d kb\n", BDB_3_SIZE, BDB_3_SIZE/8, BDB_3_SIZE/8/1024);
	printf("Size required for 2+3: %d bits, %d bytes, %d kb\n", BDB_SIZE, BDB_SIZE/8, BDB_SIZE/8/1024);
	int first = 0;
	for (i = 0; i < 256; ++i) bitbase_stats[i] = 0;
	for (i = 0; i < BDB_SIZE; ++i)
		{
		++bitbase_stats[127-bitbase_dtm[i]];
		if (first == 0 && bitbase_dtm[i] == - 128 && i >= piece_configuration[0][0][2][1])
			first = i;
		}
	for (i = 0; i < 256; ++i) if (bitbase_stats[i] > 0)
		{
		printf("(%d) ", i);

		if (i == 255)
			printf("No data:   \t%d (first - %d)\n"
				,bitbase_stats[i], first);
		else
			{
			if (i < 127)
				printf("Win in %d:\t%d\n"
					,i
					,bitbase_stats[i]);
			else if (i > 127)
				printf("Lose in %d:\t%d\n"
					,254 - i
					,bitbase_stats[i]);
			else
				printf("Draw:      \t%d\n"
					,bitbase_stats[i]);
			}
		}
	}
