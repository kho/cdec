#!/usr/bin/perl

while(my $line=<STDIN>) {
	$line=~s/[\r\n]+//g;
        $line =~ s/^(\(\s*\(\S+)(.*)(\)\s*\))$/$1 (SOS <s>)$2 (EOS <\/s>)$3/;
	print "$line\n";
}
