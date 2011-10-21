#!/usr/bin/php
<?php

/* this script looks at a .pdn file and tells you the outcome */

if (!file_exists($argv[1]))
	die("File does not exist: $argv[1]"); // fixme: put a check for read permission also?

$moves = explode(' ', `./moves.php $argv[1]`);

$board = '4444444411111111111111114444444444444444000000000000000044444444';

foreach ($moves as $move)
	{
	$x1 = ord($move{0}) - 97;
	$y1 = 8 - (ord($move{1}) - 48);
	$x2 = ord($move{3}) - 97;
	$y2 = 8 - (ord($move{4}) - 48);

	$from = $y1 * 8 + $x1;
	$to = $y2 * 8 + $x2;

	$board{$to} = $board{$from};

	if ($to < 8 || $to > 55)
		{
		if ($board{$to} * 1 < 2)
			{
			$p = $board{$to} * 1 + 2;
			$board{$to} = "$p";
			}
		}

	$step_until = $to - $from;
	$step_dir = $to > $from ? 1 : -1;
	$step = abs($step_until) < 8 ? 1 : 8;

	$loop = 0;
	for ($i = $from; $i != $to; $i += $step * $step_dir)
		{
		$board{$i} = 4;
		if (++$loop > 100)
			die("breaking loop");
		}

	$w_pieces =
	$b_pieces =
	$w_kings =
	$b_kings = 0;

	for ($i = 0; $i < 64; ++$i)
		{
		if ($board{$i} == '0') ++$w_pieces;
		if ($board{$i} == '1') ++$b_pieces;
		if ($board{$i} == '2') ++$w_kings;
		if ($board{$i} == '3') ++$b_kings;

		if (0)
			{
			print $board{$i};
			if ($i % 8 == 7)
				print "\n";
			}
		}

	++$positions[$board];

	if ($w_pieces + $b_pieces == 0)
		{
		if ($w_kings == 1 && $b_kings == 1)
			++$draw;

		if (    ($w_kings == 1 && ($b_kings == 1 || $b_kings == 2))
		     || ($b_kings == 1 && ($w_kings == 1 || $w_kings == 2)) )
			++$draw;
		}

	if ($w_pieces + $w_kings == 0) $victory = '0-1';
	if ($b_pieces + $b_kings == 0) $victory = '1-0';
	}

$piece = array('w','b','W','B','-');

print "{\n";
for ($i = 0; $i < 64; ++$i)
	{
	print $piece[$board{$i}];
	if ($i % 8 == 7)
		print "\n";
	}
print "}\n";

$most_repeats = 0;
foreach ($positions as $position => $repeat)
	$most_repeats = max($repeat, $most_repeats);

if ($most_repeats > 1)
	$draw = max($most_repeats, $draw);

if ($victory)
	print "$victory\n";
else if ($draw > 10)
	{
//var_dump($w_pieces, $b_pieces, $w_kings, $b_kings, $draw, $most_repeats);
	print "1/2-1/2\n";
	}
else
	{
	if ($draw > 0)
		print "{draw in ".(11-$draw)."} ";
	print "*\n";
	}

?>
