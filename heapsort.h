// this guy doesn't pass the perft test

void heap_sort(MOVE array[], int (*compare)(const void*, const void*), int len)
	{
	int half, parent;
	MOVE temp;

	if (len <= 1)
		return;

	half = len >> 1;
	for (parent = half; parent >= 1; --parent)
		{
		int child = parent;
		int level = 0;

		// leaf search for largest child path
		while (child <= half)
			{
			++level;
			child += child;
			if ((child < len) && ((*compare)(&array[child], &array[child-1]) > 0))
				++child;
			}

		copy_move(&temp, &array[parent-1]);

		// bottom-up search for rotation point
		for (;;)
			{
			if (parent == child)
				break;
			if ((*compare)(&temp, &array[child - 1]) <= 0)
				break;
			child >>= 1;
			--level;
			}

		// rotate nodes from parent to the level
		for (; level > 0; --level)
			{
			copy_move(&array[(child >> level) - 1], &array[(child >> (level-1)) - 1]);
			}
		copy_move(&array[child - 1], &temp);

		--len;
		do
			{
			copy_move(&temp, &array[len]);
			copy_move(&array[len], &array[0]);
			copy_move(&array[len], &temp);

			child = parent = 1;
			half = len >> 1;

			// leaf search for largest child path
			while (child <= half)
				{
				++level;
				child += child;
				if ((child < len) && ((*compare)(&array[child], &array[child-1]) > 0))
					++child;
				}

			// bottom-up search for rotation point
			for (;;)
				{
				if (parent == child)
					break;
				if ((*compare)(&temp, &array[child - 1]) <= 0)
					break;
				child >>= 1;
				--level;
				}

			// rotate nodes from parent to the level
			for (; level > 0; --level)
				{
				copy_move(&array[(child >> level) - 1], &array[(child >> (level-1)) - 1]);
				}
			copy_move(&array[child - 1], &temp);
			}
		while(--len >= 1);
		}
	}
