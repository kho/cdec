#!/usr/bin/perl -w
use strict;
use utf8;
binmode(STDIN,"utf8");
binmode(STDOUT,"utf8");
while(<STDIN>) {
  chomp;
  $_ = " $_ ";
  s/&\s*lt\s*;/</gi;
  s/&\s*gt\s*;/>/gi;
  s/&\s*squot\s*;/'/gi;
  s/&\s*quot\s*;/"/gi;
  s/&\s*amp\s*;/&/gi;
  s/ (\d\d): (\d\d)/ $1:$2/g;
  s/[\x{20a0}]\x{20ac}]/ EUR /g;
  s/[\x{00A3}]/ GBP /g;
  s/(\W)([A-Z]+\$?)(\d*\.\d+|\d+)/$1$2 $3/g;
  s/(\W)(euro?)(\d*\.\d+|\d+)/$1EUR $3/gi;
  s/&\s*#45\s*;\s*&\s*#45\s*;/--/g;
  s/&\s*#45\s*;/--/g;
  s/ï¿½c/--/g;
  s/ ,,/ "/g;
  s/``/"/g;
  s/''/"/g;
  s/[「」]/"/g;
  s/〃/"/g;
  s/¨/"/g;
  s/¡/ ¡ /g;
  s/¿/ ¿ /g;
  # â<U+0080><U+0099>
  s/â(\x{80}\x{99}|\x{80}\x{98})/'/g;
  s/â(\x{80}\x{9c}|\x{80}\x{9d})/"/g;
  s/ˇ/'/g;
  s/´/'/g;
  s/`/'/g;
  s/’/'/g;
  s/ ́/'/g;
  s/‘/'/g;
  s/ˉ/'/g;
  s/β/ß/g; # WMT 2010 error
  s/“/"/g;
  s/”/"/g;
  s/«/"/g;
  s/»/"/g;
  tr/！-～/!-~/;
  s/、/,/g;
  # s/。/./g;
  s/…/.../g;
  s/―/--/g;
  s/–/--/g;
  s/─/--/g;
  s/—/--/g;
  s/•/ * /g;
  s/\*/ * /g;
  s/،/,/g;
  s/؟/?/g;
  s/ـ/ /g;
  s/Ã ̄/i/g;
  s/â€™/'/g;
  s/â€"/"/g;
  s/؛/;/g;
		    
  s/\s+/ /g;
  s/^\s+//;
  s/\s+$//;
  s/[\x{00}-\x{1f}]//g;
  print "$_\n";
}

