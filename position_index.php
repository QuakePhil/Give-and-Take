<?php

$debug = 0;

//$_GET['s'] = 0;

$p = $_GET['p'];

$piece_counts = array(0, 0, 0, 0);

for ($i = 0; $i < strlen($p); ++$i)
	++$piece_counts[$p{$i}];

$total_pieces
= $piece_counts[0]
+ $piece_counts[1]
+ $piece_counts[2]
+ $piece_counts[3];

if ($piece_counts[0] > 0) $pieces[] = '0';
if ($piece_counts[1] > 0) $pieces[] = '1';
if ($piece_counts[2] > 0) $pieces[] = '2';
if ($piece_counts[3] > 0) $pieces[] = '3';

print "$piece_counts[0] ";
print "$piece_counts[1] ";
print "$piece_counts[2] ";
print "$piece_counts[3] ";

$piece_factor = array(0, 0, 0, 0);
$piece_multiplier = array(2 * 65, 2 * 65 * 64, 2 * 65 * 64 * 64);
$this_piece = 0;

if ($total_pieces < 4)
for ($j = 0; $j < $total_pieces; ++$j)
for ($i = 0; $i < strlen($p); ++$i) if ($p{$i} == $pieces[$j])
	{
	$piece_factor[$this_piece] += ($i + 0) * $piece_multiplier[$this_piece];
if($debug)print "pfac[$j] += ".($i+1)." * " .$piece_multiplier[$this_piece]."\n";
	++$this_piece;
//			break;
	}

$position_index = 0;

for ($j = 0; $j < $total_pieces; ++$j)
	{
	$position_index += $piece_factor[$j]; // * $piece_multiplier[$j];
if($debug)print "pos += (".$piece_factor[$j]/*."*".$piece_multiplier[$j]*/.") --> $position_index\n";
	}

$position_index += ($_GET['s']*1) + ((($_GET['t']*1)+1)) * 2;

print $position_index;

print " $p";

?>
