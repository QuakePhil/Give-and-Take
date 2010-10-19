MOVE think(int, int, double *, int *);

// after making a new book, remember to make it fits with these two variables
// or you will get weird bugs such as overwritte pre-compute arrays :)
#define MAX_BOOK_LINES (62*2)
#define MAX_BOOK_LINE_LENGTH (6*8)

char book[MAX_BOOK_LINES][MAX_BOOK_LINE_LENGTH][6];
int book_lines;
// could have a "deepest line" variable to avoid checking
// book after a certain amount of moves

static int book_move_sort(const void *a, const void *b)
	{
	return strcmp(*(char **)a, *(char **)b);
	}

MOVE opening_book_move()
	{
	MOVE candidate;
	candidate.from = -1;
	int i, j, k;

	char possible_moves[MAX_BOOK_LINES][6];
	char possible_unique_moves[MAX_BOOK_LINES][6];
	int unique_move;
	int unique_moves = 0;

	// if (line_to_cull[book_line] == -1) then do not pick from this line
	int line_to_cull[MAX_BOOK_LINES] = {0};

	// contain indexes to book[] that match the current game
	int lines_to_choose[MAX_BOOK_LINES];

	/* ============================== */
	/* first we look through the book */
	/* and see which book lines apply */
	/* to our current position        */
	/* ============================== */
	for (i = 0; i < book_lines; ++i) if (line_to_cull[i] == 0)
		{
		for (j = 0; j < MAX_BOOK_LINE_LENGTH; ++j)
			{
			if (book[i][j][0] == 0 && move_list_counter == j)
				{
				// printf("Line %d has no more moves\n", i);
				line_to_cull[i] = -1;
				break;
				}

			// cull book move to current line here
			if (move_list_counter > j)
				{
				if (strcmp(move_list[j], book[i][j]) != 0)
					{
					// printf("Culling line %d\n", i);
					line_to_cull[i] = -1;
					break;
					}
				}
			}
		}

	/* ========================================== */
	/* compute lines_to_choose[] to hold the line */
	/* indices that apply to the current position */
	/* ========================================== */
	j = 0;
	for (i = 0; i < book_lines; ++i) if (line_to_cull[i] == 0)
		{
		lines_to_choose[j] = i;
		++j;
		}

	/* ===================== */
	/* once the dust settles */
	/* we pick our book move */
	/* ===================== */
	if (j)
		{
		printf("Openings available:\n");
		for (i = 0; i < j; ++i)
			{
			for (k = 0; k < MAX_BOOK_LINE_LENGTH; ++k)
				{
				if (book[lines_to_choose[i]][k][0] == 0)
					break;
				if (k == move_list_counter)
					printf("[%s] ", book[lines_to_choose[i]][k]);
				else
					printf("%s ", book[lines_to_choose[i]][k]);
				}

			/* ==================================== */
			/* we also want to know what the unique */
			/* set of book moves is, so a move does */
			/* not get picked more often just cause */
			/* it happens to occur in more openings */
			/* ==================================== */
			unique_move = 1;

			for (k = 0; k < i; ++k) // k = previous possible_moves
				{
				if (strcmp(book[lines_to_choose[k]][move_list_counter]
					, book[lines_to_choose[i]][move_list_counter]) == 0)
					{
					unique_move = 0;
					break;
					}
				}

			for (k = 0; k < 5; ++k) // k = character 0 thru 5
				{
				possible_moves[i][k] = book[lines_to_choose[i]][move_list_counter][k];
				if (unique_move)
					possible_unique_moves[unique_moves][k]
					= book[lines_to_choose[i]][move_list_counter][k];
				}
			possible_moves[i][5] = 0;
			if (unique_move)
				{
				possible_unique_moves[unique_moves][5] = 0;
				++unique_moves;
				}

			printf("\n");
			}

		/*
		printf("UNIQUE:");
		for (i = 0; i < unique_moves; ++i)
			printf(" %s", possible_unique_moves[i]);
		printf("\n");
		*/

		//i = rand()%j;
		//printf("move selected: %s\n", book[lines_to_choose[i]][move_list_counter]);

		i = rand()%unique_moves;
		printf("Selected: %s\n", possible_unique_moves[i]);
		generate_moves(0, 0);
		k = find_move(0, possible_unique_moves[i]);
		if (k < 0 || k >= move_counter[0])
			{
			printf("error invalid move from opening book\n");
			}
		else
			copy_move(&candidate, &moves[0][k]);
		}
	//else
	//	printf("no more book moves\n");
	return candidate;
	}

void load_opening_book(int lines_to_create)
	{
	char * file = "book.txt";
	char line[80] = "";
	char movestr[80] = "";
	FILE * h;
	int i, j;

	int book_move_counter;

	h = fopen(file, "r");

	if (!h)
		{
		printf("Could not load opening book: %s\n", file);
		}
	else
		{
		book_lines = 0;

		while (!feof(h))
			{
			if (book_lines >= MAX_BOOK_LINES)
				break;

			if (!fgets(line, 80, h))
				break;

			if (line[0] == '#' || strlen(line) == 0)
				continue;

			i = 0;

			for (book_move_counter = 0; book_move_counter < MAX_BOOK_LINE_LENGTH; ++book_move_counter)
				book[book_lines][book_move_counter][0] = 0;

			book_move_counter = 0;

			while (j = load_move_from_pdn(line, movestr, &i))
				{
				sprintf(book[book_lines][book_move_counter], "%s", movestr);
				if (lines_to_create == 2)
					{
					movestr[0] = flip_file(movestr[0]);
					movestr[3] = flip_file(movestr[3]);
					sprintf(book[book_lines+1][book_move_counter], "%s", movestr);
					}
				++book_move_counter;
				}

			book_lines += lines_to_create;
			}
		fclose(h);
		printf("Loaded opening book: %s\n", file);
		}
	}

void print_book()
	{
	int i, j;
	for (i = 0; i < book_lines; ++i)
		{
		printf("Line %d:", i + 1);
		for (j = 0; j < MAX_BOOK_LINE_LENGTH; ++j)
			{
			if (book[i][j][0] == 0)
				break;
			printf(" %s", book[i][j]);
			}
		printf("\n");
		}
	printf("Total book lines: %d\n", book_lines);
	}

void opening_book_grow(int display_only)
	{
	int i, j, k;
	int victory = 0;
	double ttl;

	FILE * h;
	char new_book_line[80] = "";

	if (!display_only)
		h = fopen("book2.txt", "w");

	for (i = 0; i < book_lines; ++i)
		{
		new_game();
		new_book_line[0] = 0;
		if (display_only)
			printf("Line %d:", i + 1);
		for (j = 0; j < MAX_BOOK_LINE_LENGTH; ++j)
			{
			if (book[i][j][0] == 0)
				break;

			generate_moves(0, 0);
			k = find_move(0, book[i][j]);
			if (k < 0 || k >= move_counter[0])
				{
				printf("error invalid move from opening book\n");
				}
			else
				move_do(&moves[0][k], 0);
			if (display_only)
				printf(" %s", book[i][j]);
			strcat(new_book_line, book[i][j]);
			strcat(new_book_line, " ");
			}
		if (display_only)
			printf("\n");

		if (display_only)
			{
			print_board(0);
			}
		else
			{
			for (k = 8; k <= 9; ++k)
				{
				ttl = timer() + 99999; // actually possible to get timeout on some lines!
				node_counter = 0;
				think(0, k, &ttl, &victory);
				//print_move(&pv[0][0], 1);
				//printf("\n");
				printf("Writing line: %s%s%c%s\n\n"
					, new_book_line
					, notation[pv[0][0].from]
					, pv[0][0].captured == -1 ? '-' : 'x'
					, notation[pv[0][0].to]);
				fflush(stdout);
				fprintf(h, "%s%s%c%s\n"
					, new_book_line
					, notation[pv[0][0].from]
					, pv[0][0].captured == -1 ? '-' : 'x'
					, notation[pv[0][0].to]);
				fflush(h);
				}
			}
		}
	printf("Total book lines: %d\n", book_lines);
	if (!display_only)
		fclose(h);
	}
