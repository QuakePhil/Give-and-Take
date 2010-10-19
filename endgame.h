/* ========================== */
/* use retrograde analysis to */
/* compute the results of all */
/* kinds of endgames.         */
/* todo: use a less imbecilic */
/* index function (e.g. using */
/* combinatorial expressions) */ 
/* ========================== */

int database[4096][2][65][65];

// #define EGTB_DEBUG

void compute_endgame(int pieces[64], int * index, int total_pieces)
	{
	FILE * f;
	f = fopen("database.txt", "w");

	int i, j, k, best_value;

	int side_to_move;
	int tactical_square;
	int this_piece_must_take;

	printf("Computing endgame for: ");
	for (i = 0; i < total_pieces; ++i)
		{
		if (pieces[i] == -1)
			break;
		printf("%c", piece[pieces[i]]);
		}
	printf("\n");

	printf("Total %d pieces with at most %d positions\n", total_pieces, index[total_pieces]);

	for (i = 0; i < index[total_pieces]; ++i)
		{
		if (i > 0 && i % (index[total_pieces]/16) == 0)
			printf("%.2f%%...\n", (double)i/((double)index[total_pieces])*100.0);

		for (side_to_move = 0; side_to_move < 2; ++side_to_move)
		for (tactical_square = -1; tactical_square < 64; ++tactical_square)
		for (this_piece_must_take = -1; this_piece_must_take < 64; ++this_piece_must_take)
			{
#ifdef EGTB_DEBUG
			printf("For index %d; %s; tsq %d; must %d; "
				,i
				,side_to_move == 0 ? "white" : "black"
				,tactical_square
				,this_piece_must_take);
#endif
			state.side_to_move = side_to_move;
			state.tactical_square = tactical_square;
			state.this_piece_must_take = this_piece_must_take;
			for (j = 0; j < total_pieces; ++j)
				{
#ifdef EGTB_DEBUG
				printf("%c at %s, ", piece[pieces[j]], notation[(i / index[j])% 64]);
#endif
				board[(i / index[j]) % 64] = pieces[j];
				}
			// print_board(0);
			generate_moves(0, -1);
			best_value = -60000;

			for (j = 0; j < move_counter[0]; ++j)
				{
				//printf("move %d: ", j);
				move_do(&moves[0][j], 0);

				/* ===================================== */
				/* we need the opposite perspective here */
				/* ===================================== */
				k = -evaluate();
				//printf("(%d)", k);
				if (k > best_value)
					best_value = k;
				move_undo(&moves[0][j], 0);
				//print_move(&moves[0][j], 1);
				//printf("\n");
				}
#ifdef EGTB_DEBUG
			printf("Best value: ");
			if (best_value == -60000)
				printf("BROKEN");
			else if (best_value == 20000)
				printf("WIN");
			else if (best_value == -20000)
				printf("LOSE");
			else
				printf("%d", best_value);
			printf("\n");
#endif

			database[i][side_to_move][tactical_square+1][this_piece_must_take+1] = best_value;
			if (best_value == -60000)
				fputc('B', f);
			else if (best_value == 20000)
				fputc('W', f);
			else if (best_value == -20000)
				fputc('L', f);
			else
				fputc('D', f);

			for (j = 0; j < total_pieces; ++j)
				board[(i / index[j]) % 64] = 4;
			// printf("position done\n");
			}
		//break;
		}
	fclose(f);
	}

/* ====================================== */
/* endgames are described by white pieces */
/* before black with the pawns before the */
/* kings, e.g.: wBB or wWbB or wwWbBB etc */
/* ====================================== */
void compute_endgames(int max_pieces)
	{
	int w, b, W, B, i, k, total_pieces;
	int pieces[64];

	int index[64];
	index[0] = 1;
	for (i = 1; i < 64; ++i)
		index[i] = i > 4 ? 0 : index[i-1] * 64;

//	for (i = 0; i < 64; ++i)
//		printf("index[%d] = %d\n", i, index[i]);

	for (total_pieces = 2; total_pieces <= max_pieces; ++total_pieces)
	for (w = 0; w < total_pieces*4; ++w)
	for (b = 0; b < total_pieces*4; ++b)
	for (W = 0; W < total_pieces*4; ++W)
	for (B = 0; B < total_pieces*4; ++B)
		{
		if (w + W < 1 || b + B < 1)
			continue;
		if (w + W + b + B != total_pieces)
			continue;
		if (w > 0 || b > 0)
			continue;

		// prep the endgame
		new_game();
		for (k = 0; k < 64; ++k)
			{
			board[k] = 4;
			}
		k = 0;
		i = w; while (i--) pieces[k++] = 0;
		i = W; while (i--) pieces[k++] = 2;
		i = b; while (i--) pieces[k++] = 1;
		i = B; while (i--) pieces[k++] = 3;
		compute_endgame(pieces, index, total_pieces);
		}
	}

void load_endgames()
	{
	compute_endgames(2);

	printf("Database uses %dMB\n", sizeof(database)/1024/1024);
	}
