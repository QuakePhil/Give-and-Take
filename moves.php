#!/usr/bin/php
<?php

/* this script takes a .pdn with gunk in it and spits out clean moves
all on one line in the format "m1 m2 m3 m4 m5 " */

$h = @fopen($argv[1], 'r');

if ($h) while ($line = fgets($h))
	{
	$out .= trim($line).' ';
	}

fclose($h);

// strip excess whitespace
$out = preg_replace('/\s\s+/', ' ', $out);

print $out;

?>
