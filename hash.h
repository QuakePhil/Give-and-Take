//#define USE_HASH

//#define USE_HASH_SEARCH

//#define HASH_STATISTIC

// #define HASH_TRY_AGAIN

#ifdef HASH_STATISTIC
int hash_probes;
int hash_hits;
int hash_saves;
int hash_collisions;
#endif

unsigned int hash_rand()
	{
	int r = 0, i;
	for (i = 0; i < 32; ++i)
		r ^= rand() << i;
	return r;
	}

void hash_init()
	{
	int i, j;
	srand(0);

#ifdef HASH_STATISTIC
	hash_probes =
	hash_hits =
	hash_saves =
	hash_collisions = 0;
#endif

	for (i = 0; i < 64; ++i)
	for (j = 0; j < 5; ++j)
		hash_board_piece[i][j] = hash_rand();

	hash_side_to_move[0] = hash_rand();
	hash_side_to_move[1] = hash_rand();

	for (j = 0; j < 5; ++j)
	for (i = 0; i < 65; ++i)
		hash_state[j][i] = hash_rand();

	for (i = 0; i < HASH_SIZE; ++i)
		hash_table[i].value = 70000;
	}

inline void hash_set()
	{
	int i;
	hash = 0;
	for (i = 0; i < 64; ++i)
		hash ^= hash_board_piece[board[i]][i];
	hash ^= hash_side_to_move[state.side_to_move];
	hash ^= hash_state[0][state.tactical_square + 1];
	hash ^= hash_state[1][state.this_piece_must_take + 1];
	hash ^= hash_state[2][state.captured_piece + 1];
	hash ^= hash_state[2][state.promotion + 1];
	hash ^= hash_state[2][state.last_pawn + 1];
	}

/* =============================== */
/* hash lookups used during search */
/* =============================== */

void hash_save(int value, int ply, int type)
	{
//printf("[%d]hash_save(v=%d,%d,t=%d)\n", hash, value, ply, type);

#ifdef HASH_STATISTIC
		++hash_saves;
#endif
	HASH *h = &hash_table[hash % HASH_SIZE];

//	if (h->hash == hash && h->ply > ply)
//		return;

#ifdef HASH_TRY_AGAIN
	if (h->value != 70000)
		h = &hash_table[(hash + 1) % HASH_SIZE];

	if (h->hash == hash && h->ply > ply)
		return;
#endif

	if (h->value == 70000)
		{
		h->hash = hash;
		h->value = value;
		h->type = type;
		h->ply = ply;
		}
#ifdef HASH_STATISTIC
	else
		++hash_collisions;
#endif
	}

/*
inline void hash_save_leaf(int value, int alpha, int beta)
	{
	if (alpha < value)
		hash_save(value, 0, HASH_EXACT);
	else if (beta == value)
		hash_save(value, 0, HASH_BETA);
	else
		hash_save(value, 0, HASH_ALPHA);
	}
*/

int hash_probe(int ply, int alpha, int beta)
	{
//printf("[%d]hash_probe(%d,a=%d,b=%d)\n", hash, ply, alpha, beta);
#ifdef HASH_STATISTIC
	++hash_probes;
#endif
	HASH *h = &hash_table[hash % HASH_SIZE];

#ifdef HASH_TRY_AGAIN
	if (h->hash != hash)
		h = &hash_table[(hash + 1) % HASH_SIZE];
#endif

	if (h->hash == hash && h->ply >= ply)
		{
#ifdef HASH_STATISTIC
	++hash_hits;
#endif
		if (h->type == HASH_EXACT)
			{
//			printf("*** exact found\n");
			return h->value;
			}
		if (h->type == HASH_ALPHA && h->value <= alpha)
			{
//			printf("*** alpha found\n");
			return alpha;
			}
		if (h->type == HASH_BETA && h->value >= beta)
			{
//			printf("*** beta found\n");
			return beta;
			}
#ifdef HASH_STATISTIC
	--hash_hits;
#endif
		}


	return 70000;
	}
