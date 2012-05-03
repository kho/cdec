#!/usr/bin/perl

while(my $line=<STDIN>) {
  $line=~s/[\r\n]+//g;
  print "<s> $line </s>\n";
}
