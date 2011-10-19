//int debug_moves=0;
#define RULES_WARNINGS

//#define NEW_MOVE_STRATEGY

// show where the sorting is breaking
//#define SORT_STATISTIC

//#define RULES_DEBUG

//#define COMPUTE_FORCING
// note - if you enable COMPUTE_FORCING along with COMPUTE_DTM
//        the code will be dump and do duplicate move_do/move_undo
//#define COMPUTE_DTM

//#define CHECK_FORCING

// sometimes, halves the nps, but also reduces nodes by half or even 10 fold
// other times, does the opposite!
//#define ORDER_BY_EVAL

// this is broken ?
//#define FOLLOW_PV

// fixme: speed up if board[i]%2 is replaced with state.side_to_move in king_moves ???

/* ======================== */
/* execute a specified move */
/* keeping track of all the */
/* game state stuff that we */
/* need, in order to unmake */
/* the move cleanly         */
/* ======================== */
inline void move_do(MOVE * move, int ply)
	{
	int i;

	state_last[ply].this_piece_must_take = state.this_piece_must_take;
	state_last[ply].side_to_move = state.side_to_move;
	state_last[ply].tactical_square = state.tactical_square;

#ifdef RULES_WARNINGS
	if (board[move->to] != 4)
		{
		printf("move_do: moving to a non-empty square...");
		print_move(move, 1);
		printf("\n");
		}
#endif

	board[state.tactical_square = move->to] = board[move->from];
	board[move->from] = 4;

	// promotion
	if (board[move->to] == 0 && move->to < 8
		|| board[move->to] == 1 && move->to > 55)
		{
		state_last[ply].promotion = move->to;
		material[board[move->to]%2] -= piece_values[board[move->to]];
		board[move->to] += 2;
		material[board[move->to]%2] += piece_values[board[move->to]];

		++kings[board[move->to]%2];
		--pawns[board[move->to]%2];
		}
	else
		state_last[ply].promotion = -1;

	state_last[ply].last_pawn = -1;

	if (move->captured != -1)
		{
		material[board[move->captured]%2] -= piece_values[board[move->captured]];
		if (piece_values[board[move->captured]] == KING_VALUE
		 || piece_values[board[move->captured]] == -KING_VALUE)
			--kings[board[move->captured]%2];
		else
			--pawns[board[move->captured]%2];

		state_last[ply].captured_piece = board[move->captured];
		board[move->captured] = 4;

		// last man king
		if (material[state_last[ply].captured_piece%2] == piece_values[state_last[ply].captured_piece%2])
			{
			for (i = 0; i < 64; ++i)
				if (board[i] == state_last[ply].captured_piece%2)
					{
					state_last[ply].last_pawn = i;
					break;
					}
#ifdef RULES_WARNING
			if (state_last[ply].last_pawn == -1)
				{
				printf("move_do: material[] says last_pawn, but could not find it!\n");
				}
			else
#endif
				{
				material[board[i]%2] -= piece_values[board[i]];
				board[i] += 2;
				material[board[i]%2] += piece_values[board[i]];

				++kings[board[i]%2];
				--pawns[board[i]%2];
				}
			}
		}

	if (move->same_player_square != -1)
		{
		state.this_piece_must_take = move->same_player_square;
		}
	else
		{
		state.side_to_move ^= 1;
		state.this_piece_must_take = -1;
		}
	}

/* ==================================== */
/* undo a move, note: only one sequence */
/* of move_do and move_undo is designed */
/* to work per ply, which should be all */
/* good because I cannot think of a way */
/* execute two moves in the same ply :) */
/* ==================================== */
inline void move_undo(MOVE * move, int ply)
	{
	state.this_piece_must_take = state_last[ply].this_piece_must_take;
	state.side_to_move = state_last[ply].side_to_move;
	state.tactical_square = state_last[ply].tactical_square;

#ifdef RULES_WARNINGS
	if (board[move->from] != 4)
		{
		printf("move_undo: unmoving to a non-empty square...");
		print_move(move, 1);
		printf("\n");
		}
#endif

	// unpromotion
	if (state_last[ply].promotion != -1)
		{
		material[board[state_last[ply].promotion]%2] -= piece_values[board[state_last[ply].promotion]];
		board[state_last[ply].promotion] -= 2;
		material[board[state_last[ply].promotion]%2] += piece_values[board[state_last[ply].promotion]];

		--kings[board[state_last[ply].promotion]%2];
		++pawns[board[state_last[ply].promotion]%2];
		}

	board[move->from] = board[move->to];
	board[move->to] = 4;

	if (move->captured != -1)
		{
		if (state_last[ply].last_pawn != -1)
			{
			material[board[state_last[ply].last_pawn]%2] -= piece_values[board[state_last[ply].last_pawn]];
			board[state_last[ply].last_pawn] -= 2;
			material[board[state_last[ply].last_pawn]%2] += piece_values[board[state_last[ply].last_pawn]];

			--kings[board[state_last[ply].last_pawn]%2];
			++pawns[board[state_last[ply].last_pawn]%2];
			}
		board[move->captured] = state_last[ply].captured_piece;
		material[board[move->captured]%2] += piece_values[board[move->captured]];

		if (piece_values[board[move->captured]] == KING_VALUE
		 || piece_values[board[move->captured]] == -KING_VALUE)
			++kings[board[move->captured]%2];
		else
			++pawns[board[move->captured]%2];
		}
	}

/* ========================== */
/* one place to fill the list */
/* of moves - be they pawn or */
/* king moves they should all */
/* use this one function      */
/* ========================== */
inline void add_move_old(int ply, int from, int to, int captured, int tactical, int same_player_square)
	{
	int i, j, r;


/*
if (debug_moves)
	printf("Candidate move %d: from %s, to %s, capturing %s (tsq %s), tactical %d, same square %s\n"
		,move_counter[ply]
		,notation[from]
		,notation[to]
		,captured == -1 ? "n/a" : notation[captured]
		,state.tactical_square == -1 ? "n/a" : notation[state.tactical_square]
		,tactical
		,same_player_square == -1 ? "n/a" : notation[same_player_square]
		);
*/
	moves[ply][move_counter[ply]].from = from;
	moves[ply][move_counter[ply]].to = to;
	moves[ply][move_counter[ply]].captured = captured;
	moves[ply][move_counter[ply]].tactical = tactical;
	moves[ply][move_counter[ply]].same_player_square = same_player_square;
	moves[ply][move_counter[ply]].forced = (captured != -1 && captured == state.tactical_square) ? 1 : 
		(state.this_piece_must_take == from && captured != -1 ? 1 : 0);

	moves[ply][move_counter[ply]].forcing = 0;

	// fixme: maybe move this check after the qsort and culling?

	// if it is forced or a capture, we don't check if it is also forcing to save time
#ifdef CHECK_FORCING
	//if (generate_quiescent && max_levels_of_forcing < MAXFORCING)
#endif

#ifdef COMPUTE_DTM
	// we don't do this in evaluate() in order to avoid the victory == 1 trap
	moves[ply][move_counter[ply]].dtm = -128;

	if (pawns[0] + pawns[1] + kings[0] + kings[1] < 4)
		{
		move_do(&moves[ply][move_counter[ply]], ply);
		moves[ply][move_counter[ply]].dtm = encode_and_test(); // this is dumb
		move_undo(&moves[ply][move_counter[ply]], ply);
		}
#endif
#ifdef COMPUTE_FORCING

	int total_kings = kings[0] + kings[1];

	if (moves[ply][move_counter[ply]].forced == 0
	 && moves[ply][move_counter[ply]].captured == -1
	 && moves[ply][move_counter[ply]].same_player_square == -1
	)
		{
		move_do(&moves[ply][move_counter[ply]], ply);
		/* =========================================== */
		/* a call to a full blown generate_moves, even */
		/* if it is only one level deep, drops the nps */
		/* down an order of magnitude; perhaps a check */
		/* for forcing moves can be done in pawn_moves */
		/* or king_moves, but the complexity is a pain */
		/* =========================================== */
		for (i = 0; i < 4; ++i)
			{
			// make sure there is at least one empty square on the opposite
			// side of me that the enemy can jump to
			if (remaining_by_dir[to][ray_dirs_opposite[i]][0] == -1 ||
			 (  //    remaining_by_dir[to][ray_dirs_opposite[i]][1] != 1 &&
			  board[remaining_by_dir[to][ray_dirs_opposite[i]][0]] != 4))
				continue;

			for (j = 0; j < 8; ++j)
				{
				// now check the side (opposite of the side which has somewhere to jump to)
				r = remaining_by_dir[to][i][j];

				if (total_kings == 0 && j > 0)
					break; // if there's no kings, there's no reason to check more than
					// the immediately adjacent cells

				if (r == -1)
					break; // make sure there is something there

				// if there is a non-empty square of a different color
				// and it is (either a king or adjacent (pawn)) then
				// it can jump over, and therefore this move is forcing
				if (board[r] != 4 && board[r]%2 != board[to]%2 && (board[r] > 1 || j == 0))
					{
					moves[ply][move_counter[ply]].forcing = 1;
					break;
					// print_board(0);
					// print_move(&moves[ply][move_counter[ply]], 1);
					// printf("\n");
					}
				}
			if (moves[ply][move_counter[ply]].forcing == 1)
				break;
			}
		move_undo(&moves[ply][move_counter[ply]], ply);
		}
#endif

	// fixme: make this smarter
	moves[ply][move_counter[ply]].evaluation = 0;
#ifdef ORDER_BY_EVAL
	if ((to&7) == (from&7))
		moves[ply][move_counter[ply]].evaluation += -5;
	if ((to&7) == 0 || (to&7) == 7 || to < 8 || to > 55)
		moves[ply][move_counter[ply]].evaluation += -5;

	//move_do(&moves[ply][move_counter[ply]], ply);
	//moves[ply][move_counter[ply]].evaluation = evaluate();
	//move_undo(&moves[ply][move_counter[ply]], ply);
#endif

//if (debug_moves)
//	{
//	print_move(&moves[ply][move_counter[ply]], 1);
//	printf("\n");
//	}

	++move_counter[ply];
	return;
	}

inline void add_move(int ply, int from, int to, int captured, int tactical, int same_player_square)
	{
	if (generate_move_strategy == 1)
		{
		int forced =
			(captured != -1 && captured == state.tactical_square)
			? 1
			: (state.this_piece_must_take == from && captured != -1 ? 1 : 0);

// fixme: when sort_level is 1, this breaks
// also, other factors to sort by seem to be necessary for better branching

		int sort_level = 0; // forced ? 0 : 1;

		moves_sorted[sort_level][ply][move_counter_sorted[sort_level][ply]].from = from;
		moves_sorted[sort_level][ply][move_counter_sorted[sort_level][ply]].to = to;
		moves_sorted[sort_level][ply][move_counter_sorted[sort_level][ply]].captured = captured;
		moves_sorted[sort_level][ply][move_counter_sorted[sort_level][ply]].tactical = tactical;
		moves_sorted[sort_level][ply][move_counter_sorted[sort_level][ply]].same_player_square = same_player_square;
		moves_sorted[sort_level][ply][move_counter_sorted[sort_level][ply]].forced = (captured != -1 && captured == state.tactical_square) ? 1 : 
			(state.this_piece_must_take == from && captured != -1 ? 1 : 0);

		moves_sorted[sort_level][ply][move_counter_sorted[sort_level][ply]].forcing = 0;

		++move_counter_sorted[sort_level][ply];
		return;
		}
	else
		add_move_old(ply, from, to, captured, tactical, same_player_square);
	}

/* ============================== */
/* calculate pawn moves and jumps */
/* ============================== */
void pawn_moves(int i, int ply)
	{
	int j, a1, a2, k, a3;

	int m1, m2;

//	int initial_move_counter = move_counter[ply];

	int more_to_take;

	for (j = 0; j < 3; ++j)
		{
		a1 = adjacent_cells[(board[i]&1)][i][j];

		if (a1 == -1)
			break;

		/* ============ */
		/* simple moves */
		/* ============ */
		if (board[a1] == 4)
			add_move(ply, i, a1, -1, 0, -1);

		/* ============ */
		/* simple jumps */
		/* ============ */
		else if ((board[a1]&1) != state.side_to_move)
			{
			a2 = adjacent_jump_cells[(board[i]&1)][i][j];

			if (a2 != -1 && board[a2] == 4)
				{
				// must take until there is no more to take
				more_to_take = 0;

				// make the move
				m1 = board[i];
				board[a2] = board[i];
				board[i] = 4;

				// and the capture
				m2 = board[a1];
				board[a1] = 4;

				// is there any more to take?
				for (k = 0; k < 3; ++k)
					{
					//printf("[%d][%d][%d]",board[a2]&1,a2,k);
					a3 = adjacent_jump_cells[(board[a2]&1)][a2][k];
					//printf("=[%d]\n",a3);
#ifdef RULES_DEBUG
					printf("At %s->%s, jump = %s, more to take before (%d) "
						,notation[i]
						,notation[a2]
						,a3 == -1 ? "n/a" : notation[a3], more_to_take);
#endif

					if (a3 != -1 && board[a3] == 4
						&& board[adjacent_cells[(board[a2]&1)][a2][k]] != 4
						&&(board[adjacent_cells[(board[a2]&1)][a2][k]]&1) != state.side_to_move)
						++more_to_take; // fixme: maybe just =1 and break?
#ifdef RULES_DEBUG
					printf("after (%d)\n", more_to_take);
#endif
					}

				// restore the board
				board[a2] = 4;
				board[i] = m1;
				board[a1] = m2;

				if (more_to_take > 0)
					add_move(ply, i, a2, a1, 1, a2);
				else
					add_move(ply, i, a2, a1, 1, -1);
				}
			} // end jumps
		}
	}

/* ============================== */
/* calculate king moves and jumps */
/* in a partal yet legal way      */
/* ============================== */
void king_moves(int i, int ply)
	{
	int j, k, m1, m2, n, r;
	/* ===================================== */
	/* jump_count - regular jump moves found */
	/* active_squares - forcing jump moves   */
	/* in an effort to be efficient while we */
	/* calculate these moves we maintain the */
	/* moves array something like this:      */
	/*                                       */
	/* given king K                          */
	/* check all cells to the left C         */
	/* if jump_count == 0                    */
	/*     add move K -> C                   */
	/* if we can jump over C                 */
	/*     if landing on prev move square    */
	/*         ++jump_count                  */
	/*     if jump_count == 1                */
	/*         reset moves array             */
	/*     if active_squares == 0            */
	/*         add move K x C -> C'          ============================= */
	/*     check all cells to which we can jump C' (in a similar way)      */
	/*         if C' is an active square                                   */
	/*             ++active_squares                                        */
	/*             if active_squares == 1                                  */
	/*                 reset moves array                                   */
	/*             add move K jumps over C on C' with further forced jumps */
	/* check the other directions up, down, right...                       */
	/*                                                                     */
	/* =================================================================== */
	int jump_count = 0;
	int active_squares[4] = {0, 0, 0, 0};
	int active_square;

	int jump_count_by_dir[4] = {0, 0, 0, 0}; // to avoid jumping a second piece in the same dir

	int direction_blocked[4] = {0, 0, 0, 0};

	int direction_blocked_n[4];

#ifdef NEW_MOVE_STRATEGY
	int initial_move_counter0 = move_counter_sorted[0][ply];
	int initial_move_counter1 = move_counter_sorted[1][ply];
#else
	int initial_move_counter = move_counter[ply];
#endif

	for (j = 0; j < 14; ++j)
		{
		// (re)set the counter here, so we don't erase every direction
		if (j > 0 && rays_dir[i][j] != rays_dir[i][j-1])
			{
#ifdef NEW_MOVE_STRATEGY
			initial_move_counter0 = move_counter_sorted[0][ply];
			initial_move_counter1 = move_counter_sorted[1][ply];
#else
			initial_move_counter = move_counter[ply];
#endif
			}

		/* =================================== */
		/* determine if a direction is blocked */
		/* note - this code is duplicated in a */
		/* ghetto way in the search for active */
		/* square below using remaining_by_dir */
		/* =================================== */
		if (board[rays[i][j]] < 4 && board[rays[i][j]]%2 == board[i]%2)
			direction_blocked[rays_dir[i][j]] = 1; // by same piece

		if (board[rays[i][j]] < 4 && board[rays[i][j]]%2 != board[i]%2
			&& board[remaining_by_dir[rays[i][j]][rays_dir[i][j]][0]] != 4)
			direction_blocked[rays_dir[i][j]] = 1; // by doubled enemy piece
		// fixme: shouldn't the above check that the doubling piece
		// actually exists, and is not past a boundary in this direction?

		/* ==================== */
		/* we can move here but */
		/* if we already have a */
		/* jump then don't need */
		/* to bother with moves */
		/* ==================== */
		if (board[rays[i][j]] == 4 && !direction_blocked[rays_dir[i][j]]
			&& active_squares[rays_dir[i][j]] == 0)
			add_move(ply, i, rays[i][j], -1, 0, -1);

		/* ============================= */
		/* we can jump over a piece here */
		/* ============================= */
		else if (board[rays[i][j]] < 4 && board[rays[i][j]]%2 != board[i]%2 && !direction_blocked[rays_dir[i][j]]
			&& jump_count_by_dir[rays_dir[i][j]] == 0)
			{
			/* ============================== */
			/* check all the squares past the */
			/* captured piece in the captured */
			/* direction to see active ones   */
			/* ============================== */
			for (k = 0; k < 8; ++k)
				{
				// rays[i][j] is the piece we are jumping over
				r = remaining_by_dir[rays[i][j]][rays_dir[i][j]][k];

				if (r == -1 || board[r] != 4)
					break;

				/* ================================= */
				/* count if there are any more jumps */
				/* aka active squares after this one */
				/* avoid recursion for speed, if any */
				/* ================================= */
				active_square = 0;
				direction_blocked_n[0] = 0;
				direction_blocked_n[1] = 0;
				direction_blocked_n[2] = 0;
				direction_blocked_n[3] = 0;

//if (debug_moves)
//	printf("tsq: %s, jumping over: %s\n", notation[state.tactical_square], notation[rays[i][j]]);
				if (state.tactical_square == rays[i][j])
					++jump_count;

				++jump_count_by_dir[rays_dir[i][j]];

//				if (jump_count == 1) // on first jump found, erase previous moves if any
				if (jump_count == 1 && state.tactical_square == rays[i][j]) // above if is a bug? check bug_movelist.pdn
					{
//if (debug_moves)
//					printf("Resetting move counter on jump_count = %d\n", jump_count);
#ifdef NEW_MOVE_STRATEGY
					move_counter_sorted[0][ply] = initial_move_counter0;
					move_counter_sorted[1][ply] = initial_move_counter1;
#else
					move_counter[ply] = initial_move_counter;
#endif
					}

				// make the move
				m1 = board[i];
				board[r] = board[i];
				board[i] = 4;

				// and the capture
				m2 = board[rays[i][j]];
				board[rays[i][j]] = 4;

				// count remaining jumps if any
				for (n = 0; n < 14; ++n)
					{
					if (board[rays[r][n]] < 4 && board[rays[r][n]]%2 == board[r]%2)
						direction_blocked_n[rays_dir[r][n]] = 1;
					if (board[rays[r][n]] < 4 && board[rays[r][n]]%2 != board[r]%2
						&& board[remaining_by_dir[rays[r][n]][rays_dir[r][n]][0]] != 4)
						direction_blocked_n[rays_dir[r][n]] = 1;
					if (board[rays[r][n]] < 4 && board[rays[r][n]]%2 != board[r]%2
						&& !direction_blocked_n[rays_dir[r][n]])
						{
						// printf("active square found on %s\n", notation[rays[r][n]]);
						active_square = 1;
						++active_squares[rays_dir[i][j]];
						if (active_squares[rays_dir[i][j]] == 1) // on first forcing jump found, erase previous moves if any
							{
//if (debug_moves)
//							printf("Resetting move counter on active_squares = %d\n", active_squares);
#ifdef NEW_MOVE_STRATEGY
							move_counter_sorted[0][ply] = initial_move_counter0;
							move_counter_sorted[1][ply] = initial_move_counter1;
#else
							move_counter[ply] = initial_move_counter;
#endif
							}

						break; // don't have to count them all
						}
					}
				// restore the board
				board[r] = 4;
				board[i] = m1;
				board[rays[i][j]] = m2;

				/* ================================ */
				/* we can now finally add the jumps */
				/* differentiating between forceful */
				/* and quiet (multiple) jumps       */
				/* ================================ */
//if (debug_moves)
//	printf("active_square %d, active_squares %d\n", active_square, active_squares[rays_dir[i][j]]);

				if (active_square)
					{
					add_move(ply, i, r, rays[i][j], 1, r);
					direction_blocked[rays_dir[i][j]] = 1; // no more moves during non-tactical_square jumps
					}
				else if (active_squares[rays_dir[i][j]] == 0)
					{
					add_move(ply, i, r, rays[i][j], 0, -1);
					direction_blocked[rays_dir[i][j]] = 1; // no more moves during non-tactical_square jumps
					}

				} // end active square search

			} // end jump code
		}
	}

#ifdef SORT_STATISTIC
int sort_statistic[10] = {0};
#endif

/* ==================== */
/* this little function */
/* is important for the */
/* alpha-beta algorithm */
/* to quickly find a pv */
/* ==================== */
int move_sort(const void * a, const void * b)
	{
#ifdef FOLLOW_PV
	if (sort_last_pv_move > 1)
		{
		if(((MOVE *)a)->to == pv[sort_last_pv_move-1][1].to
		&& ((MOVE *)a)->from == pv[sort_last_pv_move-1][1].from)
			return -1;
		if(((MOVE *)b)->to == pv[sort_last_pv_move-1][1].to
		&& ((MOVE *)b)->from == pv[sort_last_pv_move-1][1].from)
			return 1;
		}
#endif

//	if (((MOVE *)a)->dtm == ((MOVE *)b)->dtm)
//	  {
	  if (((MOVE *)a)->forced == ((MOVE *)b)->forced)
		{
		if (((MOVE *)a)->forcing == ((MOVE *)b)->forcing)
			{
			// this rand is a bad idea, messes with moves/do
			// if (((MOVE *)a)->captured == ((MOVE *)b)->captured)
			//	return rand()%3-1;
#ifdef DUMBED
			if (((MOVE *)a)->captured == ((MOVE *)b)->captured)
				return rand_eval[((MOVE*)a)->from][((MOVE*)a)->to]
					< rand_eval[((MOVE*)b)->from][((MOVE*)b)->to];
#endif

#ifdef HISTORY_HEURISTIC
			if (((MOVE *)a)->captured == ((MOVE *)b)->captured)
				{
#ifdef SORT_STATISTIC
++sort_statistic[3];
#endif
				return history_heuristic[((MOVE*)a)->from][((MOVE*)a)->to]
					< history_heuristic[((MOVE*)b)->from][((MOVE*)b)->to];
				}
#endif

#ifdef ORDER_BY_EVAL
			if (((MOVE *)a)->captured == ((MOVE *)b)->captured)
				return ((MOVE *)a)->evaluation > ((MOVE *)b)->evaluation;
#endif
#ifdef SORT_STATISTIC
++sort_statistic[2];
#endif
			return ((MOVE *)a)->captured < ((MOVE *)b)->captured;
			}
#ifdef SORT_STATISTIC
++sort_statistic[1];
#endif
		return ((MOVE *)a)->forcing < ((MOVE *)b)->forcing;
		}
#ifdef SORT_STATISTIC
++sort_statistic[0];
#endif
	  return ((MOVE *)a)->forced < ((MOVE *)b)->forced;
//	  }
//	return ((MOVE *)a)->dtm > ((MOVE *)b)->dtm;
	}

// if quiesce == 1 then quiet moves are culled (so we generate
// only captures, and some forcing moves, etc...)
int generate_moves(int ply, int quiesce)
	{
//	double t1 = timer();
	int i, j;
	int count_forcing = 0;
	move_counter[ply] =
	move_counter_sorted[0][ply] =
	move_counter_sorted[0][ply] = 0;

#ifdef CHECK_FORCING
	//if (quiesce == 1)
	//	generate_quiescent = 1;
#endif

	if (state.this_piece_must_take != -1)
		{
		if (board[state.this_piece_must_take] < 2)
			pawn_moves(state.this_piece_must_take, ply);
		else
			king_moves(state.this_piece_must_take, ply);
		}
	else for (i = 0; i < 64; ++i)
		{
		if (board[i] == 4)
			continue;

		if ((board[i]&1) != state.side_to_move)
			continue;

		if (board[i] < 2)
			pawn_moves(i, ply);
		else
			king_moves(i, ply);
		}


#ifdef CHECK_FORCING
//	generate_quiescent = 0;
#endif


  if (generate_move_strategy == 0)
	{
	//QSORT(MOVE, moves[ply], move_counter[ply], move_sort);
	qsort(moves[ply], move_counter[ply], sizeof(MOVE), move_sort);
	//insertion_sort(moves[ply], move_sort, move_counter[ply]);
	//bubble_sort2(moves[ply], move_sort, move_counter[ply]);
	//heap_sort(moves[ply], move_sort, move_counter[ply]);

	/* ================================================ */
	/* cull the move list to forced moves only - if any */
	/* this for loop could probably be avoided with the */
	/* use of a global forced_moves_count variable, but */
	/* the code to keep track of it, especially when we */
	/* consider the move_counter resets, is too much of */
	/* a pain to write right now (for what's probably a */
	/* very small payoff anyway)                        */
	/* ================================================ */
	j = 0;
	for (i = 0; i < move_counter[ply]; ++i)
		{
		if (moves[ply][i].forced
			|| (quiesce == 1 && moves[ply][i].captured != -1)
			|| (quiesce == 1 && moves[ply][i].forcing))
			j = i + 1;
		if (quiesce == 1 && moves[ply][i].forcing)
			++count_forcing;
		}
	if (j > 0)
		move_counter[ply] = j;
	else if (quiesce == 1)
		move_counter[ply] = 0;

/*	if (sort_last_pv_move)
		{
		for (j = 0; j < move_counter[ply]; ++j)
			{
			print_move(&moves[ply][j],1);
			printf("\n");
			}
		printf("Found %d moves in %.6lf seconds\n", move_counter[ply], timer()-t1);
		}
*/

	/* ============================== */
	/* use this code to sort only the */
	/* first depth to the previous pv */
	/* ============================== */
//	if (sort_last_pv_move)
//		sort_last_pv_move = 0;
	}
  else
	{
	// quiesce == 1 will not cull non-forcing moves here,
	// instead, they will be ignored in think()
	count_forcing = move_counter_sorted[0][ply];
	move_counter[ply] = move_counter_sorted[0][ply] + move_counter_sorted[1][ply];
	}

	return count_forcing;
	}
// fixme: speed up if board[i]%2 is replaced with state.side_to_move in king_moves ???
