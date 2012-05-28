#!/bin/sh
# Runs cdec to decode STDIN, writes output to STDOUT

usage () {
    cat <<EOF
$0 -g GRAMMAR_DIR [-j WORKER_NUM -e ERROR_DIR] -- [cdec args]
EOF
    exit 1
}

cpus=10				# num of parallel workers
grammar=""			# grammar directory
error=""			# error output directory

while getopts g:j:e:h o; do
    case "$o" in
	g)
	    grammar="$OPTARG";;
	j)
	    cpus="$OPTARG";;
	e)
	    error="$OPTARG";;
	h)
	    usage;;
    esac
done
shift $(($OPTIND - 1))

if [ -z "$grammar" ]; then
    echo "grammar dir is required."
    usage
fi

if [ -z "$error" ]; then
    error=`mktemp -d`
fi

CDEC="$(readlink -f $(dirname $0)/..)"
echo "using cdec: $CDEC" 1>&2
echo "cdec args: $@" 1>&2
echo "using $cpus workers" 1>&2
echo "using grammar: $grammar" 1>&2
echo "error dir: $error" 1>&2

"$CDEC/bolt-scripts/declass-ibm.pl" | "$CDEC/bolt-scripts/add-sos-eos.pl" | "$CDEC/sa-extract/escape-testset-add-grammar.pl" "$grammar/grammar" | "$CDEC/dpmert/parallelize.pl" -e "$error" -j "$cpus" --use-fork -m -- /usr/bin/time -v "$CDEC/decoder/cdec" "$@" | "$CDEC/bolt-scripts/remove-sos-eos-sgml.pl"
