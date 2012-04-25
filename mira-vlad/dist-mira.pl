#!/usr/bin/env perl
use strict;
my @ORIG_ARGV=@ARGV;
use Cwd qw(getcwd);
my $SCRIPT_DIR; BEGIN { use Cwd qw/ abs_path /; use File::Basename; $SCRIPT_DIR = dirname(abs_path($0)); push @INC, $SCRIPT_DIR, "$SCRIPT_DIR/../environment"; }

# Skip local config (used for distributing jobs) if we're running in local-only mode
use LocalConfig;
use Getopt::Long;
use IPC::Open2;
use POSIX ":sys_wait_h";
my $QSUB_CMD = qsub_args(mert_memory());

require "libcall.pl";

# Default settings
my $srcFile;
my $refFiles;
my $bin_dir = $SCRIPT_DIR;
die "Bin directory $bin_dir missing/inaccessible" unless -d $bin_dir;
my $FAST_SCORE="$bin_dir/../mteval/fast_score";
die "Can't execute $FAST_SCORE" unless -x $FAST_SCORE;
#my $MAPINPUT = "$bin_dir/mr_vest_generate_mapper_input";
#my $MAPPER = "$bin_dir/mr_vest_map";
#my $REDUCER = "$bin_dir/mr_vest_reduce";
my $parallelize = "$bin_dir/parallelize.pl";
my $libcall = "$bin_dir/libcall.pl";
my $sentserver = "$bin_dir/sentserver";
my $sentclient = "$bin_dir/sentclient";
my $LocalConfig = "$SCRIPT_DIR/../environment/LocalConfig.pm";

my $SCORER = $FAST_SCORE;
my $cdec = "$bin_dir/../decoder/cdec";
die "Can't find decoder in $cdec" unless -x $cdec;
die "Can't find $parallelize" unless -x $parallelize;
die "Can't find $libcall" unless -e $libcall;
my $decoder = $cdec;
my $lines_per_mapper = 400;
my $rand_directions = 15;
my $iteration = 1;
my $run_local = 0;
my $best_weights;
my $max_iterations = 15;
my $optimization_iters = 6;
my $decode_nodes = 15;   # number of decode nodes
my $pmem = "9g";
my $disable_clean = 0;
my %seen_weights;
my $normalize;
my $help = 0;
my $epsilon = 0.0001;
my $interval = 5;
my $dryrun = 0;
my $last_score = -10000000;
my $metric = "ibm_bleu";
my $dir;
my $iniFile;
my $weights;
my $initialWeights;
my $decoderOpt;
my $noprimary;
my $maxsim=0;
my $oraclen=0;
my $oracleb=20;
my $bleu_weight=1;
my $use_make;  # use make to parallelize line search
my $dirargs='';
my $density_prune;
my $usefork;
my $pass_suffix = '';
my $cpbin=1;

# Process command-line options
Getopt::Long::Configure("no_auto_abbrev");
if (GetOptions(
	"decoder=s" => \$decoderOpt,
	"decode-nodes=i" => \$decode_nodes,
	"density-prune=f" => \$density_prune,
	"dont-clean" => \$disable_clean,
	"pass-suffix=s" => \$pass_suffix,
        "use-fork" => \$usefork,
	"dry-run" => \$dryrun,
	"epsilon=s" => \$epsilon,
	"help" => \$help,
	"interval" => \$interval,
	"iteration=i" => \$iteration,
	"local" => \$run_local,
	"use-make=i" => \$use_make,
	"max-iterations=i" => \$max_iterations,
	"normalize=s" => \$normalize,
	"pmem=s" => \$pmem,
        "cpbin!" => \$cpbin,
	"rand-directions=i" => \$rand_directions,
	"random_directions=i" => \$rand_directions,
        "bleu_weight=s" => \$bleu_weight,
        "no-primary!" => \$noprimary,
        "max-similarity=s" => \$maxsim,
        "oracle-directions=i" => \$oraclen,
        "n-oracle=i" => \$oraclen,
        "oracle-batch=i" => \$oracleb,
        "directions-args=s" => \$dirargs,
	"ref-files=s" => \$refFiles,
	"metric=s" => \$metric,
	"source-file=s" => \$srcFile,
	"weights=s" => \$initialWeights,
	"workdir=s" => \$dir,
    "opt-iterations=i" => \$optimization_iters,
) == 0 || @ARGV!=1 || $help) {
	print_help();
	exit;
}


($iniFile) = @ARGV;


sub write_config;
sub enseg;
sub print_help;

my $nodelist;
my $host =check_output("hostname"); chomp $host;
my $bleu;
my $interval_count = 0;
my $logfile;
my $projected_score;

my $refs_comma_sep = get_comma_sep_refs('r',$refFiles);

unless ($dir){
	$dir = "mira";
}
unless ($dir =~ /^\//){  # convert relative path to absolute path
	my $basedir = check_output("pwd");
	chomp $basedir;
	$dir = "$basedir/$dir";
}

if ($decoderOpt){ $decoder = $decoderOpt; }


# Initializations and helper functions
srand;

my @childpids = ();
my @cleanupcmds = ();

sub cleanup {
	print STDERR "Cleanup...\n";
	for my $pid (@childpids){ unchecked_call("kill $pid"); }
	for my $cmd (@cleanupcmds){ unchecked_call("$cmd"); }
	exit 1;
};

# Always call cleanup, no matter how we exit
*CORE::GLOBAL::exit = 
    sub{ cleanup(); }; 
$SIG{INT} = "cleanup";
$SIG{TERM} = "cleanup";
$SIG{HUP} = "cleanup";

my $decoderBase = check_output("basename $decoder"); chomp $decoderBase;
my $newIniFile = "$dir/$decoderBase.ini";
my $inputFileName = "$dir/input";
my $user = $ENV{"USER"};


# process ini file
-e $iniFile || die "Error: could not open $iniFile for reading\n";
open(INI, $iniFile);

use File::Basename qw(basename);
#pass bindir, refs to vars holding bin
sub modbin {
    local $_;
    my $bindir=shift;
    check_call("mkdir -p $bindir");
    -d $bindir || die "couldn't make bindir $bindir";
    for (@_) {
        my $src=$$_;
        $$_="$bindir/".basename($src);
        check_call("cp -p $src $$_");
    }
}
sub dirsize {
    opendir ISEMPTY,$_[0];
    return scalar(readdir(ISEMPTY))-1;
}


if (-e $dir && dirsize($dir)>1 && -e "$dir/hgs" ){ # allow preexisting logfile, binaries, but not dist-vest.pl outputs
    die "ERROR: working dir $dir already exists\n\n";
} else {
    -e $dir || mkdir $dir;
    mkdir "$dir/hgs";
    modbin("$dir/bin",\$LocalConfig,\$cdec,\$SCORER,\$MAPINPUT,\$MAPPER,\$REDUCER,\$parallelize,\$sentserver,\$sentclient,\$libcall) if $cpbin;
    mkdir "$dir/scripts";
    my $cmdfile="$dir/rerun-vest.sh";
    open CMD,'>',$cmdfile;
    print CMD "cd ",&getcwd,"\n";
#        print CMD &escaped_cmdline,"\n"; #buggy - last arg is quoted.
    my $cline=&cmdline."\n";
    print CMD $cline;
    close CMD;
    print STDERR $cline;
    chmod(0755,$cmdfile);
    unless (-e $initialWeights) {
	print STDERR "Please specify an initial weights file with --initial-weights\n";
	print_help();
	exit;
    }
    check_call("cp $initialWeights $dir/weights.0");
    die "Can't find weights.0" unless (-e "$dir/weights.0");
	}
write_config(*STDERR);

