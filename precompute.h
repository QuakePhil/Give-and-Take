// #define PRECOMPUTE_DEBUG

// this is broken now?
// check precompute_wtf.pdn with this enabled
// it is probably not saving/loading something
// new in the precompute func... 
// #define PRECOMPUTE_USE_STORED_TABLES

void precompute()
	{
	FILE * save;
	int i, j, k, y, z;

	srand(time(NULL));

#ifdef DUMBED
	for (i = 0; i < 64; ++i)
	for (j = 0; j < 64; ++j)
		{
		if (i % 8 == j % 8)
			rand_eval[i][j] = 5+rand()%5;
		else
			rand_eval[i][j] = rand()%5;
		}
#endif

#ifdef PRECOMPUTE_USE_STORED_TABLES
	save = fopen("tables.dat", "r");
	if (save)
		{
		fread(adjacent_cells, sizeof(int), 2*64*3, save);
		fread(adjacent_jump_cells, sizeof(int), 2*64*3, save);
		fread(rays, sizeof(int), 64*14, save);
		fread(rays_dir, sizeof(int), 64*14, save);
		fread(remaining_by_dir, sizeof(int), 64*4*8, save);
		fread(positional_values, sizeof(int), 2*64, save);
		fread(notation, sizeof(int), 64*3, save);
		fread(endgame_positional_values, sizeof(int), 2*64, save);
		fclose(save);
		printf ("Loaded lookup tables from file\n");
		return;
		}
#endif

	for (i = 0; i < 64; ++i)
		{
/* ================================== */
/* compute algebraic notation lookups */
/* ================================== */
		notation[i][0] = notation_columns[i % 8];
		notation[i][1] = notation_rows[i / 8];
		notation[i][2] = 0;

/* =========================================== */
/* compute adjacent cells, used for pawn moves */
/* =========================================== */
		for (k = 0; k < 2; ++k)
			{
			// not all squares have 3 moves possible
			adjacent_jump_cells[k][i][0] =
			adjacent_jump_cells[k][i][1] =
			adjacent_jump_cells[k][i][2] =
			adjacent_cells[k][i][0] =
			adjacent_cells[k][i][1] =
			adjacent_cells[k][i][2] = -1;

			// extra check for color here allows
			// for a possible variant with pawns on 1st rank
			if (k == WHITE && i < 8 || k == BLACK && i > 55)
				continue;

			/* =============================== */
			/* the order of these is important */
			/* so that adjacent corresponds to */
			/* adjacent_jumps - avoiding loops */
			/* =============================== */
			j = 0;
			if (i % 8 != 0 && i % 8 != 1)
				adjacent_jump_cells[k][i][j] = i - 2;
			if (i % 8 != 0)
				adjacent_cells[k][i][j++] = i - 1;

			if (i % 8 != 7 && i % 8 != 6)
				adjacent_jump_cells[k][i][j] = i + 2;
			if (i % 8 != 7)
				adjacent_cells[k][i][j++] = i + 1;

			if (k == WHITE && i - 16 >= 0)
				adjacent_jump_cells[k][i][j] = i - 16;
			if (k == WHITE)
				adjacent_cells[k][i][j++] = i - 8;

			if (k == BLACK && i + 16 < 64)
				adjacent_jump_cells[k][i][j] = i + 16;
			if (k == BLACK)
				adjacent_cells[k][i][j++] = i + 8;

			}

#ifdef PRECOMPUTE_DEBUG
		for (k = 0; k < 2; ++k) for (j = 0; j < 3; ++j)
			{
			printf("Adjacent[%s][%d = %s][%d] = %d\t"
				, k==0?"white":"black", i, notation[i], j, adjacent_cells[k][i][j]);
			printf("Jumps[%s][%d = %s][%d] = %d\n"
				, k==0?"white":"black", i, notation[i], j, adjacent_jump_cells[k][i][j]);
			}
#endif

/* ================================================= */
/* compute ray cells from a cell and their direction */
/* rays extend in up/down/left/right directions only */
/* rays have to be sorted the way they extend for ez */
/* directional blocking by friendly pieces           */
/* ================================================= */
		k = 0;
		for (j = i+1; j < 64; ++j)
			{
			if (i / 8 == j / 8)
				{
				rays[i][k] = j;
				rays_dir[i][k] = j > i ? 2 : 1; // right : left
				++k;
				}
			if (i % 8 == j % 8)
				{
				rays[i][k] = j;
				rays_dir[i][k] = j > i ? 3 : 0; // down : up
				++k;
				}
			}
		for (j = i-1; j > -1; --j)
			{
			if (i / 8 == j / 8)
				{
				rays[i][k] = j;
				rays_dir[i][k] = j > i ? 2 : 1; // right : left
				++k;
				}
			if (i % 8 == j % 8)
				{
				rays[i][k] = j;
				rays_dir[i][k] = j > i ? 3 : 0; // down : up
				++k;
				}
			}

/* ============================================ */
/* compute remaining cells by direction by cell */
/* variable length final array terminated by -1 */
/* ============================================ */
		for (k = 0; k < 4; ++k)
			{
			z = 0;
			// printf("Remain for %d: ", i);
			for (y = 1; y < 8; ++y)
				{
				j = i + (y * ray_dirs[k]);
				// printf("[%d (div %d %d, mod %d %d, d %d)]",j,j/8,i/8,j%8,i%8,k);

				remaining_by_dir[i][k][z] = -1;
				if (j < 0 || j > 63)
					break;
				if ((i / 8 != j / 8) && (k == 1 || k == 2))
					break;
				if ((i % 8 != j % 8) && (k == 0 || k == 3))
					break;
				remaining_by_dir[i][k][z] = j;
				++z;
				}
			// printf("\n");
			}

/* ========================= */
/* compute positional values */
/* (insert hand waving here) */
/* ========================= */
		positional_values[0][i] = positional_value[i];
		positional_values[1][i] = -positional_value[63 - i];

		endgame_positional_values[0][i] = endgame_positional_value[i];
		endgame_positional_values[1][i] = -endgame_positional_value[63 - i];
		}

/* ================ */
/* sanity checks... */
/* ================ */
/*	for (i = 0; i < 64; ++i)
		{
		printf ("Rays for %2d (direction):", i);
		for (j = 0; j < 14; ++j)
			printf(" %2d (%2d)", rays[i][j], rays_dir[i][j]);
		printf ("\n");
		}

	for (i = 0; i < 64; ++i) for (j = 0; j < 4; ++j)
		{
		printf ("Remain for %2d dir %d:", i, j);
		for (k = 0; k < 8; ++k)
			{
			if (remaining_by_dir[i][j][k] == -1)
				break;
			printf(" %2d,", remaining_by_dir[i][j][k]);
			}
		printf ("\n");
		}
*/

/* ========================== */
/* initialize zobristish hash */
/* ========================== */
/*
	srand(0);
	for (i = 0; i < 5; ++i)
	for (j = 0; j < 64; ++j)
		hash_piece[i][j] = hash_rand();
	for (j = 0; j < 64; ++j)
		hash_history[j] = hash_rand();
	hash_side = hash_rand();
*/

#ifdef PRECOMPUTE_USE_STORED_TABLES
	save = fopen("tables.dat", "w");
	if (save)
		{
		fwrite(adjacent_cells, sizeof(int), 2*64*3, save);
		fwrite(adjacent_jump_cells, sizeof(int), 2*64*3, save);
		fwrite(rays, sizeof(int), 64*14, save);
		fwrite(rays_dir, sizeof(int), 64*14, save);
		fwrite(remaining_by_dir, sizeof(int), 64*4*8, save);
		fwrite(positional_values, sizeof(int), 2*64, save);
		fwrite(notation, sizeof(int), 64*3, save);
		fwrite(endgame_positional_values, sizeof(int), 2*64, save);
		fclose(save);
		}
#endif
	}

void sanity_checks()
	{
	int i;
	printf("Positional values:\n");
	for (i = 0; i < 64; ++i)
		{
		printf("%4d,%4d ", positional_values[0][i], positional_values[1][i]);
		if (i % 8 == 7)
			printf("\n");
		}
	}
