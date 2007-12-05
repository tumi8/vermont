#!/usr/bin/perl

use strict;

my $state;
my @start;
my @graph;
my @end;

sub get_timevalue {
	my $line = shift;



	if ($_ !~ /(\d\d):(\d\d):(\d\d)\.(\d\d\d) FATAL.*/) {
		die "line doesn' match\n"
	}

	my ($hour, $min, $sec, $msec) = ($1, $2, $3, $4);

}

sub to_ms {
	my ($hour, $min, $sec, $msec) = @_;
	my $result = $msec;

	#print "$hour:$min:$sec:$msec\n";
	$result += $sec * 1000;
	$result += $min * 60   * 1000;
	$result += $hour* 60*60* 1000;

	return $result;
}

while (<STDIN>) {
	next if ($_ !~ /MESSUNG/);

	if ($_ =~ /MESSUNG: START/) {
		$state = "start";
		my @list = get_timevalue($_);
		push @start, \@list;
	}

	if ($state eq "start" && $_ =~ /MESSUNG: VOR RECONNECT/) {
		$state = "reconnect";
		my @list = get_timevalue($_);
		push @graph, \@list;
	}


	if ($state eq "reconnect" && $_ =~ /MESSUNG: ENDE/) {
		my @list = get_timevalue($_);
		push @end, \@list;
		$state = "ende";
		print $_;
	}

}

my $begin_zeit;
my $graph_zeit;
my $end_zeit;

my $counter = 0.0;

foreach (@start) {
	$begin_zeit += to_ms(@$_);
	$counter++;
}

foreach (@graph) {
	$graph_zeit += to_ms(@$_);
}

foreach(@end) {
	$end_zeit += to_ms(@$_);
}

my $tv = ($graph_zeit - $begin_zeit)/$counter;
my $tr = ($end_zeit - $graph_zeit)/$counter;
my $tg = ($end_zeit - $begin_zeit)/$counter;

printf "vorlauf | reconf | gesamt\n";
printf "%.2f ms & %.2f ms & %.2f ms \\\\\n", $tv, $tr, $tg;
