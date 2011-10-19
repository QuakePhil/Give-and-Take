/* ================ */
/* performance test */
/* and sanity check */
/* ================ */
long performance_test(int ply, double * ttl)
	{
	int i;
	long nodes = 0;

	if (ply == 0)
		return 1;

	// if (node_counter % 4096 == 0 && 
	if ((*ttl == -1 || timer() > *ttl))
                {
                if (*ttl != -1)
                        printf("performance_test: timeout\n");
                *ttl = -1;
                return 0;
                }

	generate_moves(ply, -1);

	node_counter += move_counter[ply];

	for (i = 0; i < move_counter[ply]; ++i)
		{
		move_do(&moves[ply][i], ply);
		nodes += performance_test(ply - 1, ttl);
		move_undo(&moves[ply][i], ply);
		if (*ttl == -1)
			break;
		}
	return nodes;
	}

void show_performance_test(int ply, double * ttl)
	{
	int i;
	double t1, t2;
	long nodes;

	node_counter = 0;

	t1 = timer();
	nodes = performance_test(ply, ttl);
	t2 = timer();

        if (ply == 1)
                printf(" ply        time    nodes     nps    moves\n");

        if (*ttl != -1)
                {
                printf("%2d    %10.2lf %8d %7.0lf %8d\n"
			,ply
			,t2-t1
			,node_counter
			,(t2-t1)>1?((double)node_counter)/(t2-t1):node_counter
			,nodes);
		}
	return;
	}

void iterative_performance_test()
	{
	int j = 0;
	double ttl = timer() + 10;

	while (++j <= MAXPLY / 2)
		{
		show_performance_test(j, &ttl);
		fflush(stdout);
		if (ttl == -1)
			break;
		}
	return;
	}
