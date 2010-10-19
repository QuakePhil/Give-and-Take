// how many seconds to think
//#define THINKING_TIME 1

// #define DUMBED

int thinking_time = 1;
int strength = 9; // 1 - dumbest, 9 - standard

// to avoid looping the search
#define MAXPLY 64
int thinking_ply = MAXPLY;

// two-player game
#define WHITE 0
#define BLACK 1

/* ==================================== */
/* these arrays hold the adjacent cells */
/* that a pawn may move to or jump over */
/* note that the adjacent_jump array is */
/* not guaranteed to make sense without */
/* a corresponding entry that is not -1 */
/* in the basic adjacency array         */
/* ==================================== */
// [2][64][3] rounded to [4]
int adjacent_cells[2][64][4]; // -1, +1, -8, etc
int adjacent_jump_cells[2][64][4]; // -2, +2, -16, etc

/* ======================================================== */
/* one place to keep track of up/right/left/down directions */
/* the order is important here so that we can know if given */
/* a ray what the next/previous cell is relative to another */
/* ======================================================== */
int ray_dirs[4] = {-8, -1, 1, 8};
int ray_dirs_opposite[4] = {3, 2, 1, 0};

/* ========================================================== */
/* these two tables are complementary - for each cell we know */
/* the 14 cells (from the rays array) on a ray to it and also */
/* the the direction each of them is - up/down/left/right     */
/* ========================================================== */
// [64][14] rounded to [16]
int rays[64][16];
int rays_dir[64][16];

/* ==================================================== */
/* this table is a bit redundant in respect to the rays */
/* array, but the main difference is that closest cells */
/* are stored first for forcing moves detection         */
/* ==================================================== */
// [64][14] rounded to [16]
int ray_cells[64][16];

/* ======================================================= */
/* finally with this array if given a cell and a direction */
/* we know all the remaining cells in sequence on that ray */
/* ======================================================= */
int remaining_by_dir[64][4][8];

/* ======================================= */
/* essential game state variables are next */
/* they should probably be encapsulated in */
/* a data structure for purposes of sanity */
/* ======================================= */
typedef struct
	{
	// side to move
	int side_to_move;

	// if this square can be captured, then it must be captured
	int tactical_square;

	// defines the piece that must take on this move
	// used for same side's move continuation over multiple ply
	int this_piece_must_take;

	// holds what kind of piece was captured last move
	// could also just be part of the move struct
	int captured_piece;

	// holds the square on which a promotion happened
	int promotion;

	// holds the square on which last piece became king
	int last_pawn;
	}
GAMESTATE;

GAMESTATE state;
GAMESTATE state_last[MAXPLY];

/* ============================= */
/* end essential game state vars */
/* ============================= */

// the current playing board, not considered essentual for unmake purposes
int board[64];

// running counts of material for evaluation and last man kinging
// material is stored as a positive value for white, and negative for black
int material[2];

// running count of kings and pawns, stored as unsigned for both sides
int kings[2];
int pawns[2];

// size of the biggest block per side per file (for evaluation purposes)
int blocks[2][8];

// pieces for display purposes
int piece[5] = {'w', 'b', 'W', 'B', '-'};

/* =================================== */
/* this array is indexed by [from][to] */
/* and will hold how many times a move */
/* has been the best move to help with */
/* move ordering.  a poor man's tt, if */
/* you will.                           */
/* =================================== */
int history_heuristic[64][64];
#define HISTORY_HEURISTIC

// persistent random values (for consistency between movedo/undo)
#ifdef DUMBED
int rand_eval[64][64];
#endif

// also used in positional values.  don't go over 1000 or you will confuse some implied limits
// (search for 20000 and 60000)
#ifdef DUMBED
#define KING_VALUE 150
#define KV 150
#else
#define KING_VALUE 350
#define KV 350
#endif

// 3-pawn block on edge, usually becomes a king in endgame
#define EDGEBLOCK 75

// piece values for evaluation purposes
int piece_values[5] = {100, -100, KING_VALUE, -KING_VALUE, 0};

/* ===================================== */
/* structure to define any possible move */
/* ===================================== */
typedef struct
	{
	// squares
	int from; // -1 if no move
	int to;
	int captured; // -1 if no capture

	// flags
	int tactical; // for quiescence purposes
		// = 0 - quiet move
		// = 1 - next move must capture from move.to

	/* ================================= */
	/* for regular moves this will be -1 */
	/* otherwise the sides do not change */
	/* and this holds the cell which the */
	/* player has to move from again     */
	/* ================================= */
	int same_player_square;

	/* ================================ */
	/* for move sorting purposes - this */
	/* flag sets apart the move(s) that */
	/* must be played during this turn  */
	/* ================================ */
	int forced;

	/* ================================= */
	/* also for move sorting - this flag */
	/* if set specified that the move to */
	/* play next is forced; note that if */
	/* it is not set, it does not always */
	/* guarantee the move is not forcing */
	/* because we don't check every move */
	/* only those that are not otherwise */
	/* marked as non-quiescent           */
	/* ================================= */
	int forcing;

	// holds the evaluation of the position after this move
	// for move ordering
	int evaluation;
	}
MOVE;

// for move generation
MOVE moves[MAXPLY][1024];
int move_counter[MAXPLY];

MOVE moves_sorted[4][MAXPLY][1024];
int move_counter_sorted[4][MAXPLY];

// when generate_move_strategy == 1, then moves_sorted will be utilized
int generate_move_strategy = 0;

char move_list[999][6]; // pdn listing of game (for book purposes)
int move_list_counter = 0;

// for principal variations
MOVE pv[MAXPLY][MAXPLY];

// for quiescence depth
int quiescence_depth;

// for the very important nps
long node_counter;

// forcing check is done only if we are generating quiescent moves
int generate_quiescent;

int max_levels_of_forcing;
#define MAXFORCING 1

// for sorting the last pv move
int sort_last_pv_move;

int initial_board[64] =
	{
	4, 4, 4, 4, 4, 4, 4, 4,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	4, 4, 4, 4, 4, 4, 4, 4


	// white to win in 6 moves
/*	4, 4, 4, 4, 4, 4, 4, 4,
	1, 4, 4, 4, 4, 4, 4, 1,
	4, 1, 4, 4, 1, 1, 4, 4,
	0, 4, 4, 4, 4, 4, 0, 0,
	4, 4, 4, 4, 4, 4, 4, 1,
	4, 4, 4, 4, 4, 4, 0, 4,
	4, 4, 0, 0, 4, 0, 4, 0,
	4, 4, 4, 4, 4, 4, 4, 4
*/
	// white to win in 3 moves
/*	4, 4, 4, 4, 4, 4, 4, 4,
	1, 4, 4, 4, 4, 4, 4, 1,
	4, 1, 4, 4, 1, 4, 4, 4,
	0, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 1,
	4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 0, 0, 4, 1, 4, 0,
	4, 4, 4, 4, 4, 4, 4, 4
*/
	// white to win in 2 moves
/*	4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 1,
	4, 1, 4, 4, 1, 4, 4, 4,
	1, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 1,
	4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 0, 0, 4, 1, 4, 0,
	4, 4, 4, 4, 4, 4, 4, 4
*/
	// black to win in 5 moves
/*	4, 4, 4, 4, 4, 4, 4, 2,
	1, 4, 4, 1, 4, 4, 4, 1,
	4, 4, 4, 4, 1, 4, 0, 4,
	4, 1, 4, 4, 1, 4, 4, 4,
	1, 4, 4, 4, 0, 4, 4, 4,
	4, 0, 4, 4, 0, 0, 4, 4,
	0, 4, 4, 4, 0, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4
*/
	};

int positional_values[2][64];
int endgame_positional_values[2][64];
#ifdef DUMBED
int positional_value[64] =
	{
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
int endgame_positional_value[64] =
	{
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
#else
int positional_value[64] =
	{
	KV,KV,KV,KV,KV,KV,KV,KV,
	KV/1.5,KV/2,KV/2,KV/2,KV/2,KV/2,KV/2,KV/1.5,
	KV/2.5,KV/3,KV/3,KV/3,KV/3,KV/3,KV/3,KV/2.5,
	50,20,20,20,20,20,20,50,
	10, 0, 0, 0, 0, 0, 0,10,
	5, 0, 0, 0, 0, 0, 0, 5,
	-19,-9,-9,-9,-9,-9,-9,-19,
	 0, 0, 0, 0, 0, 0, 0, 0
	};
int endgame_positional_value[64] =
	{
	 90, 39, 29, 19, 19, 29, 39, 90,
	 80, 38, 28, 18, 18, 28, 38, 80,
	 70, 37, 27, 17, 17, 27, 37, 70,
	 60, 36, 26, 16, 16, 26, 36, 60,
	 50, 35, 25, 15, 15, 25, 35, 50,
	 40, 34, 24, 14, 14, 24, 34, 40,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0
	};
#endif

char notation_columns[8] = {'a','b','c','d','e','f','g','h'};
char notation_rows[8] =    {'8','7','6','5','4','3','2','1'};
char notation[64][3];

/* ======================== */
/* 0 - tactical square      */
/* 1 - this piece must take */
/* 2 - captured piece       */
/* 3 - promotion  (are 2-4) */
/* 4 - last pawn  (needed?) */
/* ======================== */
// fixme: powers of two for the array sizes
unsigned int hash_state[5][65];
unsigned int hash_board_piece[64][5];
unsigned int hash_side_to_move[2];

unsigned int hash;

#define HASH_EXACT 1
#define HASH_ALPHA 2
#define HASH_BETA 3

typedef struct
	{
	unsigned int hash;
	int ply;
	int value;
	int type;
	}
HASH;

// #define HASH_SIZE 0x0000100
#define HASH_SIZE 0x1000000
#define EHASH_SIZE 0x1000000
HASH hash_table[HASH_SIZE];
HASH ehash_table[EHASH_SIZE];
