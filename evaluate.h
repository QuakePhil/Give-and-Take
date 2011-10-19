//#define EVALUATE_BLOCKS

#define ENDGAME_EVALUATION

#define USE_EGTB

//#define EVALUATE_EDGE_BLOCKS
// haywire opening/middle game... dunno about endgame heh

/* ================================ */
/* special case evaluation to check */
/* if king and man vs king position */
/* is a sure win or not             */
/* ================================ */
int evaluate_king_and_man_vs_king()
	{
	int side_to_win = 0;
	if (material[1] != -KV)
		side_to_win = 1;

	int i;
	int lone_king, king_to_win, man_to_win;
	for (i = 0; i < 64; ++i) if (board[i] != 4)
		{
		if (board[i] == side_to_win + 2)
			king_to_win = i;
		else if (board[i] == side_to_win)
			man_to_win = i;
		else
			lone_king = i;
		}
	// king and man pair are on an edge file
	if (king_to_win % 8 == man_to_win % 8 && (king_to_win % 8 == 0 || king_to_win % 8 == 7))
		{
		// if king is in front of man (on its kinging square) then it is victory
		if (side_to_win == 0 && king_to_win < man_to_win)
			return KV+KV;
		if (side_to_win == 1 && king_to_win > man_to_win)
			return -KV-KV;
		}

	return 0;
	}

/* ================================ */
/* special case evaluation to check */
/* a lone king vs a bunch of men if */
/* it is a sure win or not          */
/* ================================ */
int evaluate_lone_king()
	{
	int side_to_win = KV + KV;
	if (kings[1] == 0)
		side_to_win = -KV - KV;

	int i;
	int block1 = 0;
	int block2 = 0;

	for (i = 8; i < 56; i = i + 8)
		{
		if (board[i] < 2 && board[i-8] == board[i])
			++block1;
		if (board[i+7] < 2 && board[i-1] == board[i+7])
			++block2;
		}

	if (block1 > 0 || block2 > 0)
		return side_to_win;

	return 0;
	}

/* ===================================== */
/* just a simplified version of evaluate */
/* to determine win/lose/draw conditions */
/* ===================================== */
inline int decision(int * ply)
	{
//	if (material[0] > 0 && material[1] == 0)
//		return (state.side_to_move == 0 ? 20000 : -20000);
//	if (material[1] > 0 && material[0] == 0)
//		return (state.side_to_move == 0 ? -20000 : 20000);
	if (material[1] == 0)
		return (state.side_to_move == 0 ? 20000 : -20000);
	if (material[0] == 0)
		return (state.side_to_move == 0 ? -20000 : 20000);

	// fixme: make draw detection smarter
//	if (state.this_piece_must_take == -1 // don't interrupt chains
//		&& kings[0] == 1
//		&& kings[1] == 1)
//		return 0;

	if (kings[0] + kings[1] + pawns[0] + pawns[1] < 4 && *ply != 0)
		{
		char dtm = encode_and_test();
		if (dtm == -128)
			return 60000;
		if (dtm == 0)
			return 0;
		if (dtm > 0)
			return 20000 + dtm;
		if (dtm < 0)
			return -20000 + dtm;
		}

	return 60000;
	}

/* ==================================== */
/* this value is in terms of the player */
/* who made the move and not the player */
/* who is to make this current move     */
/* ==================================== */
int evaluate()
	{
	int value = 0;

#ifdef USE_EGTB
/*	char dtm;
	if (kings[0] + kings[1] + pawns[0] + pawns[1] < 4)
		{
		dtm = encode_and_test();
		if (dtm == -128)
			{
			; // no data - continue with the rest of evaluation
			}
		else if (dtm == 0)
			return 0;
		else if (dtm > 0)
			return 30000-dtm;
		else
			return -30000-dtm;
		}
*/
#endif

#ifndef DUMBED
	if (kings[0] == 1 && kings [1] == 1 && 
		((material[0] == KV + 100 && material[1] == -KV) || (material[0] == KV && material[1] == -KV - 100))
		)
		value = evaluate_king_and_man_vs_king();
	else if ((kings[0] == 1 && kings[1] == 0 && material[0] == KV)
		|| (kings[1] == 1 && kings[0] == 0 && material[1] == -KV))
		value = evaluate_lone_king();
#endif

	int i;

#ifdef ENDGAME_EVALUATION
	int endgame = 0;
#endif

//	int pieces_by_side[2] = {0, 0};

	int block_continues[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	memset(blocks, 0, sizeof(blocks));

#ifdef ENDGAME_EVALUATION
	if (material[0] - material[1] < KV+KV)
		endgame = 1;
#endif

	for (i = 0; i < 64; ++i)
		{
		if (board[i] == 4)
			{
#ifdef EVALUATE_BLOCKS
//			block_continues[i&7] = 0;
#endif
			continue;
			}

//		++pieces_by_side[board[i]%2];
		// using the material array instead of this
		value += piece_values[board[i]];

		if (board[i] < 2)
			{
#ifdef ENDGAME_EVALUATION
			if (endgame)
				{
#endif

#ifdef EVALUATE_BLOCKS
				if (i > 7 && board[i-8] == board[i])
					++blocks[(board[i])&1][i&7];
#endif
#ifdef ENDGAME_EVALUATION
				value += endgame_positional_values[board[i]][i];
				}
			else
#endif
				value += positional_values[board[i]][i];
			}
		else	// give bonuses to kings on the edges
			{
			// fixme: can be coded faster
			if (i < 8 || i > 55 || i%8==0 || i%8==7)
				value += (board[i] == 2 ? 50 : -50);
			}
		}

#ifdef EVALUATE_EDGE_BLOCKS
	for (i = 8; i < 56; i = i + 8)
		{
		if (board[i] < 2 && board[i-8] == board[i])
			++blocks[(board[i])&1][0];
		if (board[i+7] < 2 && board[i-1] == board[i+7])
			++blocks[(board[i])&1][7];
		}

	// white has free left block
	if (blocks[0][0] > 0 && blocks[1][0] == 0) value += KING_VALUE;
	// white has free left block
	if (blocks[0][7] > 0 && blocks[1][7] == 0) value += KING_VALUE;
	// black has free left block
	if (blocks[1][0] > 0 && blocks[0][0] == 0) value -= KING_VALUE;
	// black has free left block
	if (blocks[1][7] > 0 && blocks[0][7] == 0) value -= KING_VALUE;
#endif

#ifdef EVALUATE_BLOCKS
	for (i = 0; i < 8; ++i)
		value += (blocks[0][i]<<3) - (blocks[1][i]<<3);

	if (blocks[0][0] > 1) value += EDGEBLOCK;
	if (blocks[0][7] > 1) value += EDGEBLOCK;

	if (blocks[1][0] > 1) value -= EDGEBLOCK;
	if (blocks[1][7] > 1) value -= EDGEBLOCK;
#endif

//	value += material[0] + material[1];

	/* =========================== */
	/* this is used as a heuristic */
	/* to guide selection of those */
	/* moves which can promote the */
	/* last standing man as a king */
	/* =========================== */
	//if (material[0] == 200) value += 6000;
	//if (material[0] == 300) value += 2000;
	//if (material[1] == -200) value -= 6000;
	//if (material[1] == -300) value -= 2000;

//	if (pieces_by_side[0] > 0 && pieces_by_side[1] == 0)
//		value = 20000;
//	if (pieces_by_side[1] > 0 && pieces_by_side[0] == 0)
//		value = -20000;

	// fixme: move this stuff out of here since it is in decision() now?
//	if (material[0] > 0 && material[1] == 0)
//		value = 20000;
//	if (material[1] > 0 && material[0] == 0)
//		value = -20000;
	if (material[1] == 0)
		value = 20000;
	if (material[0] == 0)
		value = -20000;
/*
	// fixme: make draw detection smarter
	if (state.this_piece_must_take == -1 // don't interrupt chains
		&& pieces_by_side[0] == pieces_by_side[1] 
		&& pieces_by_side[0] == 1 
		&& material[0] == material[1] 
		&& material[0] == KING_VALUE)
		return 0;
*/
// printf("{W%dB%d=%d}", pieces_by_side[0], pieces_by_side[1], value * (state.side_to_move == 0 ? -1 : 1));
	i = (state.side_to_move == 0 ? value : -value);
	return i;
	}
