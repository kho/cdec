#!/bin/sh
# Run BOLT timing experiments

usage () {
    cat <<EOF
$0 -i INPUT -e LOG_DIR -x EXTRACT.ini -c CDEC.ini -w WEIGHTS -o OUTPUT_DIR
EOF
    exit 1
}

export TMPDIR=/scratch/umd
mkdir -p "$TMPDIR"

ecpus=12			# num of parallel workers for grammar extraction
dcpus=48                        # num of parallel workers for decoding
error=""			# log directory
econf=""			# extract.ini
dconf=""			# cdec.ini
dweight=""			# weights
input=""			# input
output=""			# output dir

while getopts i:o:e:x:c:w:h o; do
    case "$o" in
	h)
	    usage;;
	e)
	    error="$OPTARG";;
	x)
	    econf="$OPTARG";;
	c)
	    dconf="$OPTARG";;
	w)
	    dweight="$OPTARG";;
	i)
	    input="$OPTARG";;
	o)
	    output="$OPTARG";;
    esac
done
shift $(($OPTIND - 1))

if [ -z "$input" ]; then
    echo Input not given
    usage
fi

if [ -z "$output" ]; then
    echo Output directory not given
    usage
fi

mkdir -p "$output"

if [ -z "$error" ]; then
    error=`mktemp -d`
    echo "Log stored under $error"
else
    mkdir -p "$error"
fi

SCRIPTS="$(readlink -f $(dirname $0))"

echo "Input size:"
wc -l "$input"

# grammar extraction
GRAMMAR=`mktemp -d`
(/usr/bin/time -v "$SCRIPTS/grammar_extraction.sh" -g "$GRAMMAR" -c "$econf" -j "$ecpus" < "$input" 2> "$error/extract.log") || failure_extraction=1
mkdir -p "$error/extract.ER"
cp "$GRAMMAR"/error/*.ER "$error/extract.ER/"

if [ "$failure_extraction" ]; then
    echo "Grammar extraction failed; logs can be found under $error"
    rm -rf "$GRAMMAR"
    exit 1
fi
echo "Grammar disk usage:"
du -hs "$GRAMMAR"
echo "Grammar extraction done"


# decoding
DECERR=`mktemp -d`
(/usr/bin/time -v "$SCRIPTS/decode.sh" -g "$GRAMMAR" -j "$dcpus" -e "$DECERR" -- -c "$dconf" -w "$dweight" -k100 -r < "$input" 2> "$error/decode.log" | "$SCRIPTS/clean-nbest.py" | tee "$output/nbest" | "$SCRIPTS/mbr-1-best.pl" > "$output/1best") || failure_decoding=1
mkdir -p "$error/decode.ER"
cp "$DECERR"/*.ER "$error/decode.ER"

if [ "$failure_decoding" ]; then
    echo "Decoding failed; logs can be found under $error"
    rm -rf "$GRAMMAR" "$DECERR"
fi
echo "Decoding done"

rm -rf "$GRAMMAR" "$DECERR"
