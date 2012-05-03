#!/usr/bin/perl 
while(my $line=<STDIN>) {
	$line =~ s/[\r\n]+//g;
	$line =~ s/<\/s>//g;
	$line =~ s/<s>//g;
	$line =~ s/^\s+//g;
	$line =~ s/\s+$//g;
	my @words = split(/\s+/,$line);
	print join(" ",@words)."\n";
}
