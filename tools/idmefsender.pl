#
# VERMONT 
# Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

#!/usr/bin/perl -w

use strict;
use Fcntl ':flock';
use LWP::UserAgent;
use HTTP::Request::Common;
use XML::XPath;

if (scalar(@ARGV)!=1) {
	print "Usage: idmefsender.pl <idmef directory>\n";
	exit 1;
}

my ($dir) = @ARGV;

my $ua = LWP::UserAgent->new(agent => 'perl post');


sub send_idmef($$) {
	my ($url, $msg) = @_;

	my $client;

	my $res = $ua->request(POST $url, Content_Type => 'form-data', Content => [ 'IDMEF' => $msg ]);
	#print "request: " . $res->request->as_string;
	#print "response: " . $res->as_string;
	die "failed to transfer idmef message to url $url, response: '" . $res->as_string . "'" unless $res->is_success;
    # now trying to read answer
	eval {
		my $xmlres = XML::XPath->new(xml => $res->content);
		die "IDMEF receiver returned error, complete response:\n$res->as_string" if ($xmlres->getNodeText("/response/returnValue") ne "1");
	};
	die "\nIDMEF receiver returned non interpretable answer (error: $@), complete response:\n\n" . $res->as_string if ($@);
}

chdir $dir || die "failed to change to directory $dir: $!";
while (1) {
	opendir(DIR, ".") || die "failed to open directory $dir: $!";
	while ($_ = readdir(DIR)) {
		next if (/^\.\.?$/);
		my $file = $_;
		open(MSG, "<$_") || die "failed to open file $_: $!";
		flock(MSG, LOCK_EX);
		my @idmefdata = <MSG>;
		$_ = shift @idmefdata;
		chomp;
		my $url = $_;
		my $msg = join("", @idmefdata);
		print "Sending file $file ...\n";
		send_idmef($url, $msg);
		flock(MSG, LOCK_UN);
		close MSG;
		unlink $file || die "failed to delete file $file: $!";
	}
	sleep(1);
	closedir(DIR);
}



