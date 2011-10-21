<?php

// 8 -rw-r--r-- 1 phil phil 6187 2010-08-17 15:41 player.php
// 2011-09/2011-10 development - bitbases and player

if ($_GET['a'] == 'files')
	{
	$files = `ls *.pdn | grep tourname`;
//	$files = `ls *.pdn | grep _vs_`;
	if (isset($_GET['f']))
		{
		$files = explode("\n", $files);
		print time().' ';
		$this_file = $files[$_GET['f']];
		print `./moves.php $this_file`;
		}
	else
		{
		$files = explode("\n", $files);
		foreach ($files as $file) if (trim($file))
			{
			$decision = `./decision.php $file | tail -1`;
			$i = strrpos($decision, " ");
			if ($i !== false)
				$decision = substr($decision, $i);
			$decision = trim($decision);
			print "$file $decision\n";
			}
		}
	die();
	}
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<style type="text/css">
div#movelist
	{
	height: 200px;
	overflow: scroll;
	}
div#moves
	{
	width: 200px;
	overflow: scroll;
	}
</style>
<script type="text/javascript" src="http://code.jquery.com/jquery-latest.pack.js"></script>
<script type="text/javascript">
var move_list = [];
var move = 0;
var initial_board = '4444444411111111111111114444444444444444000000000000000044444444';
var clear_board = '4444444444444444444444444444444444444444444444444444444444444444';
var notation = [];
var board = initial_board;
var board_at_start = board;
var tsq = -1;
var side = 0;
var side_at_start = side;
var tsq_at_start = tsq;
var file = ['a','b','c','d','e','f','g','h']; // for display purposes
var pieces = ['r','R','k','K','-']; // for display purposes

var brush = 4;
var brush_next = [1,0,3,2,4]; // next click swaps colors

var w_pieces;
var b_pieces;
var w_kings;
var b_kings;

function precompute()
	{
	var notation_col = ['a','b','c','d','e','f','g','h'];
	var notation_row = [8, 7, 6, 5, 4, 3, 2, 1];

	notation = [];
	notation[-1] = 'n/a';
	for (var i = 0; i < 64; ++i)
		{
		notation[i] = notation_col[i%8] + notation_row[Math.floor(i/8)];
		}
	}

function new_game(x, title)
	{
	if (typeof title == "undefined")
		title = "untitled";
	move_list = [];
	move = 0;
	$('#filename').html(title);
	$('#moves').html('');
	$('#movelist').html('');
	if (x == 0)
		board = clear_board;
	else
		board = initial_board
	board_at_start = board;
	side = 0;
	tsq = -1;
	update_board();
	}

function flip_side()
	{
	if (side == 1)
		side = 0;
	else
		side = 1;
	update_board();
	}

function sort_number(a,b)
	{
	x = a.split(' ');
	y = b.split(' ');
	return y[0] - x[0];
	}

function variation(at_move, new_move)
	{
	$('#filename').html('variation');
	var new_move_string = '1234';
	for (var i = 0; i <= at_move; ++i) if (typeof move_list[i] !== "undefined")
		new_move_string += ' ' + move_list[i];
	new_move_string += ' ' + new_move;
	//board_at_start = board;
	//tsq_at_start = tsq;
	//side_at_start = side;

	load_file_complete(new_move_string);
	load_moves_up_to(at_move + 1);
	}

function position_index2(data)
	{
	var lines = data.split("\n");
	var moves = '';
	var unsorted_lines = [];
	for (var i in lines)
		{
		if (lines[i].substring(0,9) == 'position:')
			{
			moves += '<nobr>' + lines[i] + '</nobr><br/>';
			}
		if (lines[i].substring(0,4) == 'cmd:')
			{
			moves += '<nobr>' + lines[i] + '</nobr><br/>';
			}
		else if (lines[i].substring(0,5) == 'move:')
			{
			// move: 0 xx-yy blah blah blah
			move_desc = lines[i].split(' ');
			if (move_desc[4] == 'wins' || move_desc[4] == 'loses')
				move_desc[4] += ' ' + move_desc[5] + ' ' + move_desc[6];
			unsorted_lines[i] = move_desc[3] + ' ' + move_desc[2] + ' ' + move_desc[4];
//			moves += '<nobr>' + move_desc[2] + ' ' + move_desc[4] + '</nobr><br/>';
			}
		}
	unsorted_lines.sort(sort_number);
	var move_line;
	for (var i in unsorted_lines)
		{
		move_line = unsorted_lines[i].substring(unsorted_lines[i].indexOf(' ')).replace(/^\s\s*/, '').replace(/\s\s*$/, '');
		move1 = move_line.substring(0, move_line.indexOf(' '));
		move2 = move_line.substring(move_line.indexOf(' ')+1);

		moves += '<nobr>' + '<a href="#" onclick="variation('+move+', \''+move1+'\')">' + move1 +'</a> foe ' + move2 + '</nobr><br/>';
		}
	$('#moves').html(moves);
	}

function position_index(data)
	{
	z = 'interface.php?arg=position '+data;
//	prompt(z,z,z);
	$.get(z, function(data){position_index2(data);});
	}

function position()
	{
	$.get('position_index.php?s='+side+'&t='+tsq+'&p='+board,
		function(data){position_index(data);});
	}

function update_material()
	{
	var i, piece;
	w_pieces = 
	b_pieces = 
	w_kings =
	b_kings = 0;
	for (i = 0; i < 64; ++i)
		{
		piece = board.substring(i, i+1) * 1;
		if (piece == 0) ++w_pieces;
		if (piece == 1) ++b_pieces;
		if (piece == 2) ++w_kings;
		if (piece == 3) ++b_kings;
		}
	}

function t(i)
	{

	j = board.substring(i, i + 1) * 1;

//	j = j + 1;
//	if (j == 5) j = 0;
//	board = insstr(board, i, j);

	if (j == brush)
		{
		board = insstr(board, i, brush_next[j]);
		if (brush == 4) // toggle tsq with empty brush
			{
			tsq = tsq == -1?i:-1
			}
		}
	else
		board = insstr(board, i, brush);

	// reset all historical information to avoid confusion
	board_at_start = board;
	side_at_start = side;
	tsq_at_start = tsq;
	move = 0;
	move_list = [];

	update_board();
	}

function swap_colors()
	{
	for (i = 0; i != 64; ++i)
		{
		piece = brush_next[board.substring(i, i+1)];
		board = insstr(board, i, piece);
		}
	update_board();
	}

function update_board()
	{
	var i, i_start, i_end, i_inc, i_tr_start, i_tr_end;

	if ($('#flip').is(':checked'))
		{
		i_start = 63;
		i_end = -1;
		i_inc = -1;
		i_tr_start = 7;
		i_tr_end = 0;
		}
	else
		{
		i_start = 0;
		i_end = 64;
		i_inc = 1;
		i_tr_start = 0;
		i_tr_end = 7;
		}

	update_material();

	var white_name = 'white';
	var black_name = 'black';
	if (side == 0)
		white_name = '<b>white</b>';
	else
		black_name = '<b>black</b>';

	var materials =	
		'ply ' + (move + 1) + ', '
		+ white_name + ' (' + w_pieces + '+' + w_kings + '), '
		+ black_name + ' (' + b_pieces + '+' + b_kings + ')'
		+ ' (' + notation[tsq] + ')';

	var table = '<table border=1 cellpadding=0 cellspacing=0><tr><td></td>';
	for (i = i_tr_start; i != i_tr_end + i_inc; i += i_inc)
		table += '<td>' + file[i] + '</td>';
	table += '<td></td></tr>';
	for (i = i_start; i != i_end; i += i_inc)
		{
		color = ((i + (Math.floor(i/8) % 2)) %2)?'b':'w';
		piece = pieces[board.substring(i, i+1)];
		row = 8 - Math.floor(i/8);

		if (i % 8 == i_tr_start) table += '<tr><td>' + row + '</td>';
		table += '<td onclick=t('+i+')><img src=/gntredux/gif/' + piece + '.gif></td>';
		if (i % 8 == i_tr_end) table += '<td>' + row + '</td></tr>';
		}

	table += '<tr><td></td>';
	for (i = i_tr_start; i != i_tr_end + i_inc; i += i_inc)
		table += '<td>' + file[i] + '</td>';

	table += '<td></td></tr></table><hr>' + materials;

	// piece pallette
	table += '<table border=0 cellpadding=0 cellspacing=0><tr>';
	table += '<td><img src=/gntredux/gif/r.gif onclick="brush=0"></td>'
	table += '<td><img src=/gntredux/gif/R.gif onclick="brush=1"></td>'
	table += '<td><img src=/gntredux/gif/k.gif onclick="brush=2"></td>'
	table += '<td><img src=/gntredux/gif/K.gif onclick="brush=3"></td>'
	table += '<td><img src=/gntredux/gif/-.gif onclick="brush=4"></td>'
	table += '</tr></table>';

	$('#board').html(table);
	}

// assuming b is a string of length 1, repace char a[i] with b
// e.g.: a[i] = b 
function insstr(a, i, b)
	{
	return a.substring(0, i) + b + a.substring(i + 1);
	}

function move_do(i)
	{
	var this_move = move_list[i];
//	console.log(this_move, i);
	var x1 = this_move.charCodeAt(0) - 97;
	var y1 = 8 - (this_move.charCodeAt(1) - 48);
	var x2 = this_move.charCodeAt(3) - 97;
	var y2 = 8 - (this_move.charCodeAt(4) - 48);

	var from = y1 * 8 + x1;
	var to = y2 * 8 + x2;
	tsq = to;

	side = board.substring(from, from+1);
	if (side == 0 || side == 2) // which side just moved
		side = 0;
	else
		side = 1;

//	console.log(move_list);
//	console.log(from + '(' + x1 + ',' + y1 + ') to ' + to + '(' + x2 + ',' + y2 + ')');

	// var temp = board.substring(to, to+1);
	board = insstr(board, to, board.substring(from, from+1));

	// promote
	if (to < 8 || to > 55)
		{
		if (board.substring(to, to + 1) * 1 < 2)
			{
			var promote = board.substring(to, to + 1)*1 + 2;
//console.log(board.substring(to, to + 1), promote);
			board = insstr(board, to, promote);
			}
		}

	var step_until = to - from;
	var step_dir = to > from ? 1 : -1;
	var step = Math.abs(step_until) < 8 ? 1 : 8;

	var loop = 0;
	for (var k = from; k != to; k += step * step_dir)
		{
		board = insstr(board, k, '4');
		if (++loop > 100){alert('breaking loop!'); break;}
		}
	}

function load_moves_up_to(x)
	{
	$('.mtxt').css('font-weight','normal');
	$('#mov'+x).css('font-weight','bold');
	var final_side = -1;
	if (x + 1 < move_list.length - 2)
		{
		board = board_at_start;
		side = side_at_start;
		tsq = tsq_at_start;
		var i;
		for (i = 0; i < x + 2; ++i)
			{
			move_do(i);
			}
		final_side = side;
		}

	board = board_at_start;
	side = side_at_start;
	tsq = tsq_at_start;
	var i;
	for (i = 0; i < x + 1; ++i)
		{
		move_do(i);
		}
	if (final_side != -1)
		side = final_side;
	else
		side = side == 1? 0: 1;
	move = i - 1;
	// console.log("finally move = " + (i + 1));
	update_board();

// this should at least be delayed,
// or it should only be called when the position has stabilized from
// the ajax calls
//	position();
	}

function prev_move()
	{
	if (move >= 0)
		load_moves_up_to(move - 1);
	else
		alert("No more previous moves");
	}

function next_move()
	{
	if (move < move_list.length - 2)
		load_moves_up_to(move + 1);
	else
		alert("No more next moves");
	}

function load_file_complete(data)
	{
	var ply = 0;
	var move_count = 1;
	var move_type = "";
	var movelist = '';

	move_list = data.split(" ");
	move_list.shift(); // first element is always a timestamp, for cache fooling
	board = board_at_start;
	side = side_at_start;;
	tsq = tsq_at_start;
	last_side = 1;
	for (var i in move_list)
		{
		move_type = move_list[i].substring(2, 3);
		if (move_type != "-" && move_type != "x")
			continue;

		move_do(i);

		if (side == 0 && last_side == 1) // white moved after black
			movelist += (move_count > 1?'<br>':'') + move_count++ + '.';

		if (side == last_side) // white or black moves again
			movelist += 'x';
		else
			movelist += ' ';

		movelist += ("<a href=\"#\" class=\"mtxt\" id=\"mov"+i+"\" onclick=\"load_moves_up_to(" + i +  ")\">"
			+ move_list[i] + "</a>");

//		movelist += ' ' + side + ' moved after ' + last_side + '<br>';

		last_side = side;
		}
	$('#movelist').html(movelist);
	load_moves_up_to(-1);
	}

function load_file(i, title)
	{
	$.get('player.php?a=files&f='+i, function(data){new_game(1, title);load_file_complete(data);});
	}

function update_files(data)
	{
	var file_list = "";
	var files = data.split("\n");
	for (var i in files)
		{
		var files2 = files[i].split(" ");
		if (typeof files2[0] !== "undefined")
			file_list += "<a href=# onclick=load_file('" + i + "','" + files2[0] + "')>" + 
				files2[0] + "</a> " + files2[1] + "<br/>";
		}
	$('#files').html(file_list);
	}

$(document).ready(function()
	{
	$.ajaxSetup({cache: false});
	precompute();
	new_game(0);
	$.get('player.php?a=files', function(data){update_files(data);});
	update_board();
	}
);
</script>
<body link=blue alink=blue vlink=blue>
<table border=1>
<tr valign=top>
<td><div id="filename"></div><div id="board"></div>
<a href=# onclick="next_move()">next</a>
<a href=# onclick="prev_move()">prev</a><br>
<a href=# onclick="new_game(1)">new</a>
<a href=# onclick="new_game(0)">clear</a>
<a href=# onclick="flip_side()">side</a>
<a href=# onclick="position()">check</a>
<input type=checkbox id="flip" onclick="update_board()"><label for="flip">flip</label>
<a href=# onclick="swap_colors()">swap</a>
</td>
<td><div id="moves"></div></td></tr>
<tr><td colspan=2><div id="movelist"></div></td></tr>
<tr><td colspan=2><div id="files"></div></td></tr>
</table>
</body>
