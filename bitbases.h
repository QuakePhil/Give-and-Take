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
#define BDB_2_SIZE (2*65*64*63)
#define BDB_3_SIZE (2*65*64*63*62)

#define BDB_SIZE (BDB_2_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE)
#define MAX_BDB_PIECES 3

//#define BDB_SIZE BDB_2_SIZE
//#define MAX_BDB_PIECES 2

#define BDB_SIZE_IN_BYTES ((BDB_SIZE)/8)

int piece_configuration_pieces[3][3][3][3][MAX_BDB_PIECES]; // for position_decode

int number_of_positions[4] = {0, 0, BDB_2_SIZE, BDB_3_SIZE};

/* ============================================== */
/* if a bit is set here, then we know it is a win */
/* for a position; this will hold results for all */
/* piece configurations, using the previous array */
/* to offset into the proper positoin here        */
/* ============================================== */
char bitbase[BDB_SIZE_IN_BYTES]; // e.g.: bitbase[piece_config + position_index]
// note - this large static array confuses valgrind :(

// can I has bit-twiddle to speed these up?
#define TEST_BIT(bit) bitbase[(bit)/(8)] & (1<<((bit)%(8)))
#define SET_BIT(bit) bitbase[(bit)/(8)] |= (1<<((bit)%(8)))
#define RESET_BIT(bit) bitbase[(bit)/(8)] &= ~(1<<((bit)%(8)))

inline int position_encode(int * pieces, int total_pieces)
	{
// should check here that other state as -1
	int position_index, i, j;
	position_index = state.side_to_move + (state.tactical_square + 1) * 2;

	int piece_factor[MAX_BDB_PIECES] = {-1, -1, -1};
	long piece_multiplier[MAX_BDB_PIECES] = {2 * 65, 2 * 65 * 64, 2 * 65 * 64 * 63};

	// we shouldn't have to do this here...
	for (i = 0; i < 64; ++i) if (board[i] != 4)
		{
		for (j = 0; j < total_pieces; ++j)
			{
			if (board[i] == pieces[j] && piece_factor[j] == -1)
				{
				piece_factor[j] = i * piece_multiplier[j];
				break;
				}
//			else if (piece_factor[j] == -1)
//				{
//				piece_factor[j] = i * piece_multiplier[j];
//				}
			}
		}

//	printf("initial index = %d\n", position_index);
//	for (j = 0; j < total_pieces; ++j)
//		printf("adding piece_factor[%d] = %d\n", j, piece_factor[j]);

	for (j = 0; j < total_pieces; ++j)
		position_index += piece_factor[j];

	return position_index;
	}

inline int position_decode(int position, int * pieces, int total_pieces)
	{
	int piece_collision = 0;

	// we assume here that other state flags are pre-set by new_game()

	// decode the side to move and tactical square
	state.side_to_move = position % 2;
	position >>= 1;
	state.tactical_square = (position % 65) - 1;
	position /= 65;

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
		j = position % k;
		position /= k;
		--k;

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

inline int bitbase_test(int white_pawns, int black_pawns, int white_kings, int black_kings, int position_index)
	{
	long bit = piece_configuration[white_pawns][black_pawns][white_kings][black_kings] + position_index;
	return TEST_BIT(bit);
	}

inline void bitbase_set(int white_pawns, int black_pawns, int white_kings, int black_kings, int position_index)
	{
	long bit = piece_configuration[white_pawns][black_pawns][white_kings][black_kings] + position_index;
	SET_BIT(bit);
	}

inline void bitbase_reset(int white_pawns, int black_pawns, int white_kings, int black_kings, int position_index)
	{
	long bit = piece_configuration[white_pawns][black_pawns][white_kings][black_kings] + position_index;
	RESET_BIT(bit);
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
void build_bitbase(int white_pawns, int black_pawns, int white_kings, int black_kings)
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
	int i, j;

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

	printf("Building bitbase: %s\n", bitbase_file);

	new_game();

	double t1;

	// reset the bitbase to zero, and also make sure position decode/encode works
	t1 = timer();
	int positions_with_overlapped_pieces = 0;
	int positions_examined = 0;
	for (position = 0; position < number_of_positions[total_pieces]; ++position)
		{
		i = position_decode(position, pieces, total_pieces);

		// skip positions with overlapping pieces, even though these
		// could be legal, they only confuse the encode/decode
		if (i || kings[0] + kings[1] + pawns[0] + pawns[1] != total_pieces)
			{
			++positions_with_overlapped_pieces;
			continue;
			}
		++positions_examined;

		i = position_encode(pieces, total_pieces);

// this check is no good because our index has duplicates
// e.g. the same index will give multiple position: (B1,W1,W2), (B1,W2,W1)
/*		if (position != i)
			{
			printf("Mismatch: position index: %d, re-encoded: %d\n", position, i);
			print_board(0);
			getc(stdin);
			}
*/

		/* =================================================================== */
		/* we could evaluate here, but only illegalish positions would qualify */
		/* (e.g. those with overlapping pieces) and we skip those as per above */
		/* =================================================================== */
		bitbase_reset(white_pawns, black_pawns, white_kings, black_kings, position);
		}

	printf("Bitbase reset %d, skipped %d, total %d in %.2lf seconds\n"
		, positions_examined
		, positions_with_overlapped_pieces
		, number_of_positions[total_pieces]
		, timer()-t1);

	// check moves leading to wins and mark those too
	int positions_already_marked_as_won;
	int positions_marked_as_won;
	int all_moves_are_won_for_black;

	for (;;)
	  {
	  positions_already_marked_as_won =
	  positions_marked_as_won = 0;

	  t1 = timer();

	  for (position = 0; position < number_of_positions[total_pieces]; ++position)
		{
		i = bitbase_test(white_pawns, black_pawns, white_kings, black_kings, position);
		if (i)
			{
			++positions_already_marked_as_won;
			continue; // already marked as won for black
			}

		i = position_decode(position, pieces, total_pieces);
		if (i || kings[0] + kings[1] + pawns[0] + pawns[1] != total_pieces)
			continue;

// todo -- better bitbase get/set wrappers
// how to manage constructing/deconstructing position_index easily and quickly
		generate_moves(0, 0);
/*
if (move_counter[0] > 0)
{

print_board(0);print_moves(0);
getc(stdin);
}*/
		all_moves_are_won_for_black = 1;
		for (i = 0; i < move_counter[0]; ++i)
			{
			move_do(&moves[0][i], 0);
			if (kings[0] + kings[1] + pawns[0] + pawns[1] == total_pieces)
				{
				j = bitbase_test(white_pawns, black_pawns, white_kings, black_kings, position_encode(pieces, total_pieces));
				if (!j)
					all_moves_are_won_for_black = 0;
				}
			else
				{
				j = evaluate();
				if (j != -20000)
					all_moves_are_won_for_black = 0;
				}
			move_undo(&moves[0][i], 0);
			if (all_moves_are_won_for_black == 0)
				break;
			}

		if (all_moves_are_won_for_black)
			{
			++positions_marked_as_won;
			bitbase_set(white_pawns, black_pawns, white_kings, black_kings, position);
			}
		}

	  printf("Pass complete in %.2lf seconds\n", timer()-t1);
	  printf("  Positions already marked as won: %d of %d\n"
		, positions_already_marked_as_won, number_of_positions[total_pieces]);
	  printf("  Positions marked as won: %d\n", positions_marked_as_won);
	  if (positions_marked_as_won == 0)
		break;
	  }
	}

int load_bitbases()
	{
	char bitbase_file[] = "bitbases.dat";

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
	int i;
	piece_configuration[0][0][1][1] = i = 0; // white K vs K
	piece_configuration[0][0][1][2] = i = i + BDB_2_SIZE; // K vs KK
	piece_configuration[0][1][1][1] = i = i + BDB_3_SIZE; // K vs Km
	piece_configuration[0][2][1][0] = i = i + BDB_3_SIZE; // K vs mm
	piece_configuration[0][0][2][1] = i = i + BDB_3_SIZE; // KK vs K
	piece_configuration[1][0][1][1] = i = i + BDB_3_SIZE; // Km vs K
	piece_configuration[2][0][0][1] = i = i + BDB_3_SIZE; // mm vs K

	FILE * save;

	save = fopen(bitbase_file, "r");
	if (save)
		{
		fread(bitbase, sizeof(char), BDB_SIZE_IN_BYTES, save);
		fclose(save);
		printf("Loaded bitbases from file\n");
		return;
		}


	// build in proper order
	build_bitbase(0, 0, 1, 1); // K-K
	build_bitbase(0, 0, 2, 1); // KK-K
	build_bitbase(0, 0, 1, 2); // K-KK

	build_bitbase(0, 1, 1, 1); // K-Km
	build_bitbase(0, 2, 1, 0); // K-mm
	build_bitbase(1, 0, 1, 1); // Km-K
	build_bitbase(2, 0, 0, 1); // mm-K

	printf("Writing %s\n", bitbase_file);
	save = fopen(bitbase_file, "w");
	if (save)
		{
		fwrite(bitbase, sizeof(char), BDB_SIZE_IN_BYTES, save);
		fclose(save);
		}
	else
		printf("Could not write to file\n");
	printf("Done\n");
	}

void bitbase_report()
	{
	int i;

	printf("Size required for 2 pieces: %d bits, %d bytes, %d kb\n", BDB_2_SIZE, BDB_2_SIZE/8, BDB_2_SIZE/8/1024);
	printf("Size required for 3 pieces: %d bits, %d bytes, %d kb\n", BDB_3_SIZE, BDB_3_SIZE/8, BDB_3_SIZE/8/1024);
	printf("Size required for 2+3: %d bits, %d bytes, %d kb\n", BDB_SIZE, BDB_SIZE/8, BDB_SIZE/8/1024);
	i = piece_configuration[2][0][0][1];
	printf("Offset for last piece configuration: %d bits, %d bytes, %d kb\n", i, i/8, i/8/1024);
	printf("(%d + %d should equal %d)\n", i/8/1024, BDB_3_SIZE/8/1024, BDB_SIZE/8/1024);
	// for (i = 0; i < 16; ++i) printf("TEST_BIT(%d) = %d\n", i, TEST_BIT(i));
	// for (i = 0; i < 16; ++i) if (i%2) {printf("SET_BIT(%d)\n", i); SET_BIT(i);}
	// for (i = 0; i < 16; ++i) printf("TEST_BIT(%d) = %d\n", i, TEST_BIT(i));

	}
