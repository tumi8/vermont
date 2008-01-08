#!/usr/bin/perl -w

use strict;
use XML::XPath;

my @xmldata = <STDIN>;
my $xmldata = join("\n", @xmldata);

my $xp = new XML::XPath->new(xml => $xmldata);


sub getmname($)
{
	my $m = shift;
	my $mname = $xp->find('@name', $m) . " (" . $xp->find('@id', $m) . ")";
	return $mname;
}

print "digraph G {\n";
print "\tnode[fontsize=8,shape=box,fontname=Helvetica,height=.3]\n";
my $modules = $xp->find('/vermont/sensorData/module');
foreach my $m ($modules->get_nodelist) {
	my $mname = getmname($m);
	my $mid = $xp->find('@id', $m);
	print "\t$mid [label=\"$mname\"];\n";
	foreach my $n ($xp->findnodes('next', $m)) {
		my $id = $n->string_value;
		#my $nextname = getmname($xp->findnodes("/vermont/sensorData/module[\@id=$id]")->[0]);
		print "\t$mid -> $id;\n";
		#my $nmname = getmname($xp->findnodes('/vermont/sensorData/module[@id=
		#print "\t $mname ->
	}
}
print "}\n";
