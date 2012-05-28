#!/bin/sh
# Extracts grammar from the standard input

usage () {
    cat 1>&2 <<EOF
$0
  -g: grammar output dir
  -c: config file (extract.ini)
  -j: number of parallel workers

  -h: show this message
EOF
    exit 1
}

if [ `hostname -d` != 'umiacs.umd.edu' ]; then
    export PATH=/bolt/mt/code/bin/UMD/python-current/bin:$PATH
fi

cpus=10				# num of parallel workers
grammar=""			# where to put grammar output
config=""			# extract.ini

while getopts g:j:c:h o; do
    case "$o" in
	g)
	    grammar="$OPTARG";;
	j)
	    cpus="$OPTARG";;
	c)
	    config="$OPTARG";;
	h)
	    usage;;
    esac
done

if [ -z "$grammar" ]; then
    echo "grammar output dir is required." 1>&2
    usage
fi

if [ -z "$config" ]; then
    echo "extract.ini is required." 1>&2
    usage
fi

CDEC="$(readlink -f $(dirname $0)/..)"
echo "using cdec: $CDEC" 1>&2
echo "config file: $config" 1>&2
echo "using $cpus workers" 1>&2
echo "writing to $grammar" 1>&2

"$CDEC/sa-extract/escape-testset.pl" | "$CDEC/dpmert/parallelize.pl" -e "$grammar/error" -j "$cpus" --use-fork -- /usr/bin/time -v "$CDEC/sa-extract/extractor.py" -c "$config" -x "$grammar/grammar" > /dev/null
