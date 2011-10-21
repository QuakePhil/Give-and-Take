<?php

date_default_timezone_set('America/New_York');

$self = explode('/', $_SERVER['PHP_SELF']);
$self = $self[1];

session_name($self.'_challenge');

session_start();

if ($_GET['a'] == 'leave')
	{
	$_SESSION['challenger'] = false;
	header('Location: index.php');
	die();
	}

if ($_POST['challenger'])
	{
	$c = strtolower($_POST['challenger']);
	$allow = 'abcdefghijklmnopqrstuvwxyz';
	for ($i = 0; $i < strlen($c); ++$i)
		if (strpos($allow, $c{$i}) !== false)
			$c2 .= $c{$i};
	$_SESSION['challenger'] = ucfirst($c2);
	}

if (!$_SESSION['challenger'])
	{
	print "Challenger, enter your name: ";
	print "<form method=post action=challenge.php><input name=challenger type=text><input type=submit></form>";
	die();
	}

$goodluck = 0;
if (!$_SESSION['gamefile'] || $_POST['act'] == 'new')
	{
	$goodluck = 1;
	$_SESSION['evaluation'] = 0;
	$_SESSION['strength'] = 9;
	$_SESSION['timelimit'] = 10;
	$_SESSION['eng'] = './gnt';
	$_SESSION['gamefile'] = 'challenge/'.$_SESSION['challenger'].'-'.$_SERVER['REMOTE_ADDR'].'-vs-gnt_'.date('Ymd_his').'.pdn';
	}

$t1 = microtime(true);

$gamefile = $_SESSION['gamefile'];

?>
<script type="text/javascript" src="http://code.jquery.com/jquery-latest.pack.js"></script>
<script>
function cell(str)
	{
	document.getElementById('cellspan').innerHTML = 'Cell: ' + str;
	}

movefrom = '';

function move(str)
	{
	if (movefrom == '')
		{
		movefrom = str;
		document.getElementById('movespan').innerHTML = 'Move: ' + movefrom;
		}
	else
		{
		if (str == movefrom)
			{
			document.getElementById('movespan').innerHTML = 'Move: cancelled';
			}
		else
			{
			document.getElementById('movespan').innerHTML = 'Move: ' + movefrom + '-' + str;
			// location.href='/<?php echo $self ?>/?m='+movefrom+'-'+str;
			$('#m').val(movefrom+'-'+str);
			$('#game').submit();
			}
		movefrom = '';
		}
	}
</script>
<?php
if ($goodluck) print "This is the Give and Take challenge.<br/>
You have the first move.  Can you beat your opponent?<br/>
Good luck!<br/>";

$initial_board = '4444444411111111111111114444444444444444000000000000000044444444'; 

// $debug = 1;

function load_engine($file)
	{
	$pipe_spec = array(0 => array('pipe', 'r'), 1 => array('pipe', 'w'), 2 => array('file', 'play.err', 'w'));

	$engine->proc = proc_open($file, $pipe_spec, $engine->pipes);
	$engine->id = array('name' => $file, 'author' => 'unknown');

	if (is_resource($engine->proc))
		{
		stream_set_blocking($engine->pipes[0], 0);
		stream_set_blocking($engine->pipes[1], 0);
		fwrite($engine->pipes[0], "gnt\n");
		return $engine;
		}
	else
		return false;
	}

// $engines[] = load_engine('./gnt');

$selected_engine = 0;

$notation_col = array('a','b','c','d','e','f','g','h');
$notation_row = array(8, 7, 6, 5, 4, 3, 2, 1);
for ($i = 0; $i < 64; ++$i)
	{
	$notation[$i] = $notation_col[$i%8].$notation_row[floor($i/8)];
	}
$notation[-1] = 'n/a';

if (!$gamefile)
	die("No gamefile");

if (!file_exists($gamefile))
	{
	fclose(fopen($gamefile, "w"));
	chmod($gamefile, 0777);
	}

$out = '';
$move = '';
if ($_POST['m'])
	{
	$col_validate = array_flip($notation_col);
	$row_validate = array_flip($notation_row);
	$move = substr(trim($_POST['m']),0,5);
	$move{2} = '-';
	if (!isset($col_validate[$move{0}])) $move = 'a1-a1';
	if (!isset($col_validate[$move{3}])) $move = 'a1-a1';
	if (!isset($row_validate[$move{1}])) $move = 'a1-a1';
	if (!isset($row_validate[$move{4}])) $move = 'a1-a1';
	}
else if ($_POST['act'] != 'continue')
	$move = 'just_show_the_board';

if ($strength[$_POST['strength']])
	{
	$_SESSION['strength'] = $_POST['strength'];
	$_SESSION['eng'] = $engs[$_SESSION['strength']];
	}
if ($timelimit[$_POST['timelimit']])
	$_SESSION['timelimit'] = $_POST['timelimit'];

if (!$move) $move = 'move'; // computer to move

if ($_POST['act'] != 'new')
	{
	$e = popen($cmd = "$_SESSION[eng] pdn play $gamefile $move $_SESSION[strength] $_SESSION[timelimit]", 'r');
	fwrite($e, "quit\n");
	while (!feof($e))
		{
		$out .= fread($e, 1024);
		}
	fclose($e);
	}

$out_lines = explode("\n", $out);
foreach ($out_lines as $line)
	{
	list($a, $b, $c, $d, $e) = explode(' ', $line);
	if ($a == 'boardok')
		{
		$board = $b;
		$side = $d == 1 ? 'black' : 'white';
		}
	}

if (!$board)
	{
	$board = $initial_board;
	$side = 'white';
	}
$pieces = array('r','R','k','K','-');

//print $side;

$out_lines = explode("\n", $out);
foreach ($out_lines as $line)
	{
	if ($capture_eval)
		{
		$eval = trim(substr($line, 40, 8));
		if ($eval{0} == 'W' || $eval{0} == 'L' || strpos($eval, '.') !== false)
			$_SESSION['evaluation'] = $eval;
		}
	if (strpos($line, 'ply/quiesce') !== false)
		$capture_eval = true;
	if (strpos($line, 'bestmove') !== false)
		$bestmove = $line;
	}

if ($bestmove)
	{
	$bestmove = explode(' ', $bestmove);
	print "Opponent's move: ".$bestmove[1]."<br/>";
	}

print "Opponent: <font size=\"+2\" title=\"".$_SESSION['evaluation']."\">";

if ($_SESSION['evaluation'] > -0.5 && $_SESSION['evaluation'] < .5)
	print ":-|";
else if ($_SESSION['evaluation'] > 0)
	{
	if ($_SESSION['evaluation'] < 2)
		print ":-)";
	else if ($_SESSION['evaluation'] < 200)
		print ":-D";
	else
		print ":-P";
	}
else if ($_SESSION['evaluation'] < 0)
	{
	if ($_SESSION['evaluation'] > -2)
		print ":-/";
	else if ($_SESSION['evaluation'] > -200)
		print ":-(";
	else
		print ":-X";
	}
else
	print ":-?";
print "</font><br/>";



$flip = $_POST['flip'];

if ($flip)
	{
	$i_start = 63;
	$i_end = -1;
	$i_inc = -1;
	$i_tr_start = 7;
	$i_tr_end = 0;
	}
else
	{
	$i_start = 0;
	$i_end = 64;
	$i_inc = 1;
	$i_tr_start = 0;	$i_tr_end = 7;
	}


print "<table border=0 cellpadding=0 cellspacing=0>";

print "<tr><td></td>";
for ($i = $i_tr_start; $i != $i_tr_end + $i_inc; $i += $i_inc)
	print "<td>".$notation_col[$i]."</td>";
print "<td></td></tr>";

for ($i = $i_start; $i != $i_end; $i += $i_inc)
	{
	$color = (($i + (floor($i/8) % 2)) % 2) ? 'b' : 'w';
	$piece = $pieces[$board{$i}];
	$cell = $notation[$i];

	$row = 8 - floor($i / 8);

	if ($i % 8 == $i_tr_start)
		print "<tr><td>$row</td>";
	// print "<td><img onmouseover=\"cell('$cell')\" onclick=\"move('$cell')\" alt=\"$cell\" src=\"/chess/p/$piece$color.gif\"></td>";
	print "<td style=\"border:1px solid black\"><img onmouseover=\"cell('$cell')\" onclick=\"move('$cell')\" alt=\"$cell\" src=\"gif/$piece.gif\"></td>";
	if ($i % 8 == $i_tr_end)
		print "<td>$row</td></tr>";
	}

print "<tr><td></td>";
for ($i = $i_tr_start; $i != $i_tr_end + $i_inc; $i += $i_inc)
	print "<td>".$notation_col[$i]."</td>";
print "<td></td></tr>";

print "</table>";

print "<form method=post action=challenge.php id=game>
<input type=hidden name=m value=\"\" id=m>
<input type=submit name=act value=new>
</form>
";

?><span id="cellspan">Cell: --</span>&nbsp;&nbsp;&nbsp;
<span id="movespan">Move: --</span><br/><?php

print "Challenger: ".$_SESSION['challenger']."<br/>";
print "<a href=challenge.php?a=leave>Exit challenge</a>";

if (strpos($out, 'bestmove') === false && trim($_POST['m']) && strpos($out, 'move') !== false)
	{
	print "<script>alert(\"Valid moves: \\n";
	$valid_moves = explode("\n", $out);
	foreach ($valid_moves as $valid_move)
		if (substr($valid_move, 0, 4) == 'move')
			{
			$valid_move = explode(' ', $valid_move);
			print $valid_move[2] . "\\n";
			}
	print "And that's it!\");</script>";
	}
?>
