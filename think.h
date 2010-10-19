/* while testing this


don't forget to remove the restrictions:

* only pawns force moves
//* increase_depth for quescence
//* j = 1 in bestmove
//* turn on beta cutoff

*/

//#define SEARCH_DEBUG

//#define DUMP_PV

// #define SEARCH_WARNING

// this actually messes up some winning sequences...
// but if i disable it, root_ply = 0 after think in bestmove??
#define STAND_PAT

//#define DUMP_PV_UPDATE

//#define TRACE_TRI_PV

//#define TRACE_PV

int search(int ply, int depth, int alpha, int beta, int quiesce, double * ttl)
	{
//printf("search(%d,a=%d,b=%d,q=%d)\n",ply, alpha, beta, quiesce);
	int i, j, a, b;

	int hash_type = HASH_ALPHA;

	int increase_depth = 0;

	i = decision();
	if (i != 60000)
		return i;

	if (node_counter % 4096 == 0 && (*ttl == -1 || timer() > *ttl))
		{
		if (*ttl != -1)
			printf("search: timeout\n");
		*ttl = -1;
		return evaluate();
		}

#ifdef USE_HASH_SEARCH
	if (quiesce == 0)
		{
		hash_set();
		i = hash_probe(ply, alpha, beta);
		if (i != 70000)
			return i;
		}
#endif

#ifdef NEW_MOVE_STRATEGY
generate_move_strategy = 1;
#endif
		i = generate_moves(ply, quiesce);
#ifdef NEW_MOVE_STRATEGY
generate_move_strategy = 0;
#endif

	++node_counter;

	if (move_counter[ply] == 0 || (ply > depth && !quiesce) || ply > MAXPLY)
		{

#ifdef USE_HASH_SEARCH
		i = evaluate();
		if (quiesce == 0)
			{
			hash_set();
			hash_save(i, ply, HASH_EXACT);
			}
		return i;
#else
		return evaluate();
#endif
		}

//	b = beta;

#ifdef STAND_PAT
	if (quiesce == 1)
		{
		int stand_pat = evaluate();
		if (stand_pat >= beta)
			return beta; // fail hard -- this is usually a bit faster
			//return stand_pat; // fail soft
		if (alpha < stand_pat)
			alpha = stand_pat;
		}
#endif

	for (i = 0; i < move_counter[ply]; ++i)
		{
#ifdef NEW_MOVE_STRATEGY
if (i >= move_counter_sorted[0][ply])
	copy_move(&moves[ply][i], &moves_sorted[1][ply][i - move_counter_sorted[0][ply]]);
else
	copy_move(&moves[ply][i], &moves_sorted[0][ply][i]);
#endif

		j = board[moves[ply][i].from]%2; // color of the piece that just moved (for use in next if stmt)
		j = (j == 0 ? 1 : 0); // take the opposite of that

		increase_depth = 0;

#ifndef DUMBED
		if (depth < MAXPLY / 2
		&& (
			moves[ply][i].captured != -1
			|| (moves[ply][i].forcing == 1 // quiesce forcing moves
				&& kings[j] == 1 // but only if it forces the last king to move
				&& (material[j] == KING_VALUE || material[j] == -KING_VALUE)
				&& board[moves[ply][i].from] < 2
				)
		))
			increase_depth = 1;
#endif

		move_do(&moves[ply][i], ply);

		if (moves[ply][i].same_player_square == -1)
			a = -search(ply + 1, depth + increase_depth, -beta, -alpha, quiesce, ttl);
		else
			a = search(ply + 1, depth + increase_depth, alpha, beta, quiesce, ttl);

		move_undo(&moves[ply][i], ply);

		// beta cutoff
		if (a >= beta)
			{
#ifdef USE_HASH_SEARCH
			if (quiesce == 0)
				{
				hash_set();
				hash_save(beta, ply, HASH_BETA);
				}
#endif
			return beta;
			}

		if (a > alpha)
			{
			if (quiescence_depth < ply) // fixme: this should be done every node? (slow)
				quiescence_depth = ply;

#ifdef HISTORY_HEURISTIC
			++history_heuristic[moves[ply][i].from][moves[ply][i].to];
#endif
/* ============================= */
/* the triangular pv array might */
/* look something like this:     */
/* pv[0] a3-a4 h6-h5 a4-a5 a6xa4 */
/* pv[1] ??-?? h6-h5 a4-a5 a6xa4 */
/* pv[2] ??-?? ??-?? a4-a5 a6xa4 */
/* pv[3] ??-?? ??-?? ??-?? a6xa4 */
/* note that it is filled bottom */
/* up so pv[3] is set then maybe */
/* pv[2] and pv[2] will hold the */
/* pv[3] at that instance, after */
/* which pv[3] may be updated to */
/* hold a different sub-line and */
/* that may in turn be discarded */
/* so it should not affect moves */
/* in the pv higher up unless it */
/* is confirmed at that level    */
/* ============================= */

			copy_move(&pv[ply][ply], &moves[ply][i]);
			for (j = ply + 1; j <= quiescence_depth; ++j)
				copy_move(&pv[ply][j], &pv[j][j]);
			// fixme: bounds checking here?
			pv[ply+1][ply+1].from = -1;

			alpha = a;
#ifdef USE_HASH_SEARCH
			hash_type = HASH_EXACT;
#endif
			}

		// another cutoff
		//if (alpha >= beta)
		//	return alpha;

		// weird negascout piece
/*		if (alpha >= b)
			{
			move_do(&moves[ply][i], ply);
			alpha = search(ply + 1, -beta, -alpha, quiesce);
			if (state.this_piece_must_take == -1)
				alpha = -alpha;
			move_undo(&moves[ply][i], ply);

			if (alpha >= beta)
				return alpha;
			}
		b = alpha + 1;

*/
		}

#ifdef USE_HASH_SEARCH
	if (quiesce == 0)
		{
		hash_set();
		hash_save(alpha, ply, hash_type);
		}
#endif

	return alpha;
	}

/* ===================================== */
/* here, ply starts from zero, and depth */
/* defines some arbitrary point at which */
/* we should stop searching; we may wish */
/* to increase depth dynamically, a kind */
/* of ghetto quiescence search, but this */
/* actually makes things easier, smarter */
/* ===================================== */
MOVE think(int ply, int depth, double * ttl, int * victory)
	{
	//precompute();

	int i, a;

	double t1, t2;

	t1 = timer();

	quiescence_depth = depth;

	a = search(ply, depth, -60000, 60000, 0, ttl);

	if (a == 20000 || a == -20000)
		*victory = 1;

	t2 = timer();

	if (depth == 1)
		printf("ply/quiesce    time        nodes     nps  value pv\n");

	if (*ttl != -1)
		{
		printf("%3d/%3d %11.2lf %12d %7.0lf %6.2f "
			,depth
			,quiescence_depth - depth
			,t2-t1
			,node_counter
			,(t2-t1)>1?((double)node_counter)/(t2-t1):node_counter
			,a/100.0);

		for (i = 0; i < quiescence_depth; ++i)
			{
			if (pv[0][i].from == -1)
				break;
			printf(" ");
			print_move(&pv[0][i], 0);
			}
		printf("\n");
		}

	return pv[0][0];
	}

int bestmove(FILE * out, int play)
	{
	int j = 1;
	int i, k;
	int victory = 0;
	double ttl = timer() + thinking_time; // (60 * 60 * 1);
	MOVE candidate, best;

	node_counter = 0;

	best.from = -1;

	printf("timelimit: %d, strength: %d\n", thinking_time, strength);

#ifdef HISTORY_HEURISTIC
	memset(history_heuristic, 0, sizeof(history_heuristic));
#endif

	for (i = 0; i < MAXPLY; ++i)
	for (k = 0; k < MAXPLY; ++k)
		pv[i][k].from = -1;

	if (play)
		{
		generate_moves(0, 0);
		if (move_counter[0] == 1)
			copy_move(&best, &moves[0][0]);
		}

	if (best.from == -1)
		best = opening_book_move();

	if (best.from == -1) do
		{
		candidate = think(0, j, &ttl, &victory);

		// fixme: this will break if ttl == -1 during ply = 1
		// which shouldn't ever happen, of course...
		if (ttl != -1)
			copy_move(&best, &candidate);

		fflush(stdout);

		if (ttl == -1 || victory == 1)
			break;
		}
#ifdef DUMBED
	while (++j < 2);
#else
	while (++j < thinking_ply);
#endif
	/* fixme: this is a hack fix */
	if (best.from == best.to || best.from == -1) return 0; // avoid hanging the browser

	if (play)
		{
		move_do(&best, 0);
		move_list_push(&best);
		}

	if (best.from != best.to)
		{
		printf("bestmove ");
		print_move(&best, 0);
		printf("\n");
		}

	if (out) fprintf(out, "%s%s%s "
		, notation[best.from]
		, best.captured == -1 ? "-" : "x"
		, notation[best.to]);

	return best.same_player_square == -1 ? 0 : 1;
	}
