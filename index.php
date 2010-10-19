<?php

$self = explode('/', $_SERVER['PHP_SELF']);
$self = $self[1];

session_name($self);

session_start();

if (!$_SESSION['gamefile'] || $_POST['act'] == 'new')
	{
	$_SESSION['eng'] = './gnt';
	$_SESSION['gamefile'] = 'game_'.$_SERVER['REMOTE_ADDR'].'_'.date('Ymd_his').'.pdn';
	}

$t1 = microtime(true);

if ($_POST['act'] == 'comment')
	{
	$comments = unserialize(file_get_contents('comments.txt'));
	$comments[] = array($_SERVER['REMOTE_ADDR'], time(), $_POST['n'], $_POST['c']);
	$h = fopen('comments.txt', 'w');
	fwrite($h, serialize($comments));
	fclose($h);
	print "<script>location.href='index.php';</script>";
	die();
	}

$gamefile = $_SESSION['gamefile'];

if ($_GET['act'] == 'hint')
	{
	$e = popen($cmd = "./gnt pdn play $gamefile hint 9 1", 'r');
	fwrite($e, "quit\n");
	while (!feof($e))
		{
		$out .= fread($e, 1024);
		}
	fclose($e);
	print substr($out, strpos($out, 'bestmove') + 9, 5);
	die();
	}

?>
<script type="text/javascript" src="http://code.jquery.com/jquery-latest.pack.js"></script>
<script>
function hint()
	{
	$.get('index.php?act=hint', function(data)
		{
		alert(data);
		});
	return false;
	}

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
			document.getElementById('movespan').innerHTML = 'Move: ' + movefrom + ' to ' + str;
			// location.href='/<?php echo $self ?>/?m='+movefrom+'-'+str;
			$('#m').val(movefrom+'-'+str);
			$('#game').submit();
			}
		movefrom = '';
		}
	}
</script>
<a target=_blank href=rules.html>Rules</a>&nbsp;&nbsp;&nbsp;
<span id="cellspan">Cell: --</span>&nbsp;&nbsp;&nbsp;
<span id="movespan">Move: --</span>
<?php

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

if ($_POST['act'] == 'undo')
	{
	$file = file_get_contents($gamefile);
	$h = fopen($gamefile, 'w');
	fwrite($h, substr(trim($file), 0, -5));
	fclose($h);
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

$strength['1'] = 'Easy';
$strength['9'] = 'Standard';
$engs['1'] = './gntez';
$engs['9'] = './gnt';

$timelimit['1'] = '1s';
$timelimit['5'] = '5s';
$timelimit['10'] = '10s';

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

print $side;

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

print "<form method=post action=index.php id=game>
Strength: <select name=strength>";
foreach ($strength as $strength_val => $strength_name)
	print "<option value=$strength_val".
		($_SESSION['strength']==$strength_val?' selected=selected':'')
		.">$strength_name</option>";
print "</select> CPU time: <select name=timelimit>";
foreach ($timelimit as $timelimit_val => $timelimit_name)
	print "<option value=$timelimit_val".
		($_SESSION['timelimit']==$timelimit_val?' selected=selected':'')
		.">$timelimit_name</option>";
print "</select><br/>
<input type=hidden name=m value=\"\" id=m>
<input type=submit name=act value=continue>
<input type=submit name=act value=new>
<input type=submit name=act value=undo>
<input type=button name=act value=hint onclick=\"javascript:return hint()\">
</form>
";

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
if ($self == 'gntredux') print "<pre>$cmd<hr>$out</pre>";

print "<hr>Comments: ";
print "<form method=post action=index.php>
Name: <input type=text name=n><br>
<textarea name=c rows=10 cols=40></textarea><br>
<input type=submit name=act value=comment>
</form>";

$comments = unserialize(file_get_contents('comments.txt'));
if ($comments) foreach ($comments as $comment)
	{
	list ($ip, $time, $name, $text) = $comment;
	$name = strip_tags($name);
	$text = stripslashes(strip_tags($text));
	print "<hr>$name (".date('m/d/Y', $time).")<br>".strip_tags($text);
	}

print "<hr>".(microtime(true)-$t1);

?>
