#!/usr/bin/perl -w

use utf8;
use strict;

binmode(STDIN,":utf8");
binmode(STDOUT,":utf8");

my @fh = ();
push @fh, \*STDIN;

my $id = -1;
for my $f (@fh) {
  while(<$f>) {
    chomp;
    die "Empty line in test set" if /^\s*$/;
    die "Please remove <seg> tags from input:\n$_" if /^\s*<seg/i;
    $id++;
    # print "<seg id=\"$id\"> $_ </seg>\n";
    if (scalar(@ARGV)>0) {
      print "<seg id=\"$id\" grammar=\"$ARGV[0].$id\"> $_ </seg>\n";
    } else {
      print "<seg id=\"$id\"> $_ </seg>\n";
    }
  }
}

