#!/usr/bin/perl -w
use strict;
while(<>)
{
  s/\$[A-Za-z]+_\(([^|]+)\|\|([^)]+)\)/$1/g;
  s/\@\@/ /g;
  s/\$[A-Za-z]+_\(([^)]+)\)/$1/g;
  print;
}
