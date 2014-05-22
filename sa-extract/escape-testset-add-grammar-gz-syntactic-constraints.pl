#!/usr/bin/perl -w

use utf8;
use strict;
use Getopt::Long;

binmode(STDIN,":utf8");
binmode(STDOUT,":utf8");

my @fh = ();
push @fh, \*STDIN;

my $prefix;
my $syn_con;
# Process command-line options
if (GetOptions(
  "syncon=s" => \$syn_con,
  "prefix=s" => \$prefix,
) == 0 || @ARGV > 0){
  print STDERR "Usage: escape-testset-add-grammar-gz-syntactic-constraints.pl --syncon syn_con_file --prefix gram_prefix < test.cn > my_source_file\n";
  die "\n";
}

$prefix || die "You must specify grammar prefix with -p (or --prefix)\n";
$syn_con || die "You must specify syntactic constraint file with -s (or --syncon)\n";

my @syn_con_files = split(',', $syn_con);

if (scalar(@fh) != scalar(@syn_con_files)) {
  die "The number of input files and the number of syntactic constraint files are inconsistent!\n";
}

my $id = -1;
my $file_index = 0;
for my $f (@fh) {
  my $syn_con_file = $syn_con_files[$file_index];
  my $syn_con_in;
  if ($syn_con_file =~ /^.*\.gz$/) {
    open $syn_con_in, "gunzip -c $syn_con_file |";
  } else {
    open $syn_con_in, "< $syn_con_file";
  }
  $file_index++;
  while(<$f>) {
    chomp;
    die "Empty line in test set" if /^\s*$/;
    die "Please remove <seg> tags from input:\n$_" if /^\s*<seg/i;
    $id++;
    my $syn_con_str = read_syn_con_4_one_sentence($syn_con_in);
    # print "<seg id=\"$id\"> $_ </seg>\n";
    print "<seg id=\"$id\" grammar=\"$prefix.$id.gz\" constraints=\"$syn_con_str\"> $_ </seg>\n";
  }
  close $syn_con_in;
}

sub read_syn_con_4_one_sentence{
  my $file_in = shift;
  my $num_word;
  my $syn_con_str = "";
 
  while (my $line = <$file_in>) {
    chomp $line;
    $num_word = $line;
    for (my $i = 0; $i < $num_word; $i++) {
      $line = <$file_in>;
      chomp $line;
      if ( length($line) == 0 ) {
        $syn_con_str .= ";";
        next;
      }
      my @words = split( ' ', $line );
      my $size = scalar(@words);
      my $j = 0;
      while ($j < $size) {
        if ($j >= 0 and $j < $size - 1) {
          if ($words[$j] == $words[$j-1] + 1 and $words[$j] == $words[$j+1] - 1) {
          } elsif ($words[$j] == $words[$j-1] + 1) {
            $syn_con_str .= "-$words[$j],";
          } elsif ($words[$j] == $words[$j+1] - 1) {
            $syn_con_str .= "$words[$j]";
          } else {
            $syn_con_str .= "$words[$j],"
          }
        } elsif ($j == 0) {
          $syn_con_str .= "$words[$j]";
          if ( $j < $size - 1 and $words[$j] != $words[$j+1] - 1) {
            $syn_con_str .= ",";
          }
        } else {
          if ( $j > 0 and $words[$j] == $words[$j-1] + 1) {
            $syn_con_str .= "-";
          }
          $syn_con_str .= "$words[$j]";
        }
        $j++;
      }
      $syn_con_str .= ";";
    }
    last;
  }
  return $syn_con_str;
}

