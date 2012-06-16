#find
my $cmd;
if ($ARGV[2]=="combi"){
$cmd = `grep SCORE $ARGV[0] | cat -n | sort -k +2 | head -1`;
}
else {
$cmd = `grep SCORE $ARGV[0] | cat -n | sort -k +2 | tail -1`;
}
print $cmd;
$cmd =~ m/([0-9]+)/;
system("ln -s $ARGV[1]/weights.$1 $ARGV[1]/weights.tuned");
