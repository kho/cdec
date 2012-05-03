#!/bin/sh
# Run BOLT timing experiments

usage () {
    cat <<EOF
$0 -i INPUT -j WORKER_NUM -e LOG_DIR -x EXTRACT.ini -c CDEC.ini -w WEIGHTS
EOF
    exit 1
}

cpus=12				# num of parallel workers
error=""			# log directory
econf=""			# extract.ini
dconf=""			# cdec.ini
dweight=""			# weights
input=""			# input

while getopts j:e:x:c:w:i:h o; do
    case "$o" in
	h)
	    usage;;
	j)
	    cpus="$OPTARG";;
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
    esac
done
shift $(($OPTIND - 1))

if [ -z "$input" ]; then
    echo Input not given
    usage
fi

if [ -z "$error" ]; then
    error=`mktemp -d`
    echo "Log stored under $error"
else
    mkdir -p "$error"
fi

SCRIPTS="$(readlink -f $(dirname $0))"

# grammar extraction
GRAMMAR=`mktemp -d`
(/usr/bin/time -v "$SCRIPTS/grammar_extraction.sh" -g "$GRAMMAR" -c "$econf" -j "$cpus" < "$input" 2> "$error/extract.log") || failure_extraction=1
mkdir -p "$error/extract.ER"
cp "$GRAMMAR"/error/*.ER "$error/extract.ER/"

if [ "$failure_extraction" ]; then
    echo "Grammar extraction failed; logs can be found under $error"
    rm -rf "$GRAMMAR"
    exit 1
fi
echo "Grammar extraction done"

# decoding
DECERR=`mktemp -d`
(/usr/bin/time -v "$SCRIPTS/decode.sh" -g "$GRAMMAR" -j "$cpus" -e "$DECERR" -- -c "$dconf" -w "$dweight" -k200 -r < "$input" > "$error/decode.out" 2> "$error/decode.log") || failure_decoding=1
mkdir -p "$error/decode.ER"
cp "$DECERR"/*.ER "$error/decode.ER"

if [ "$failure_decoding" ]; then
    echo "Decoding failed; logs can be found under $error"
    rm -rf "$GRAMMAR" "$DECERR"
fi
echo "Decoding done"