#!/usr/bin/php
<?php

/* this script plays a game between two engines until victory or draw */

$engine_1 = array('old', '../giventake/gnt.bak');
$engine_2 = array('new', './gnt');

$tournaments = trim(`ls tournament*pdn | wc -l`) + 1;

if ($tournaments % 2 == 0)
	{
	$tmp = $engine_1;
	$engine_1 = $engine_2;
	$engine_2 = $tmp;
	}

$game = 'tournament_'.$engine_1[0].'-'.$engine_2[0].'_'.$tournaments.'.pdn';

for (;;)
	{
	print "*** $engine_1[1] playing white\n";
	echo `$engine_1[1] pdn play $game move 9 1`;
	print "*** $engine_2[1] playing black\n";
	echo `$engine_2[1] pdn play $game move 9 1`;
	$decision = trim(`./decision.php $game`);
	print "{"."$engine_1[1] vs $engine_2[1]"."} $decision\n";
	if (substr($decision, -1) != '*')
		{
		break;
		}
	}

/*
drwxr-xr-x 2 phil phil  4096 2010-06-04 10:36 tournament
-rw-r--r-- 1 phil phil   396 2010-06-23 15:17 tournament_1.pdn
-rw-r--r-- 1 phil phil   564 2010-06-23 15:28 tournament_2.pdn
-rw-r--r-- 1 phil phil   588 2010-06-23 15:31 tournament_3.pdn
-rw-r--r-- 1 phil phil   384 2010-06-23 15:38 tournament_4.pdn
-rw-r--r-- 1 phil phil   834 2010-06-23 16:28 tournament_5.pdn
-rwxr-xr-x 1 phil phil  1092 2010-06-04 10:32 tournament/game_20100601_111224.pdn
-rwxr-xr-x 1 phil phil 64630 2010-06-04 10:33 tournament/gnt
-rwxr-xr-x 1 phil phil 64598 2010-06-04 10:36 tournament/gnt_eval_order
-rwxr-xr-x 1 phil phil   904 2011-10-19 13:57 tournament.php
-rw-r--r-- 1 phil phil 19712 2010-06-04 10:34 tournament/tables.dat
*/
?>

