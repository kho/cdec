#!/usr/bin/perl 

my $prev = "";
while(my $line=<STDIN>) {
	my @tokens = split(/\s+\|\|\|\s+/,$line);
	if ($prev ne $tokens[0]) {
		print "$tokens[1]\n";
	}
	$prev = $tokens[0];
}
