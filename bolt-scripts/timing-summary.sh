#!/bin/sh

usage () {
    cat <<EOF
$0 -i INPUT -e LOGDIR -l TIMINGLOG
EOF
    exit 1
}

sec () {
    echo "$(TZ=utc date -d "1970-01-01 00:$1" +%s)"
}

INPUT=""
LOGDIR=""
TIMINGLOG=""

while getopts i:e:l:h o; do
    case "$o" in
	i)
	    INPUT="$OPTARG";;
	e)
	    LOGDIR="$OPTARG";;
	l)
	    TIMINGLOG="$OPTARG";;
	h)
	    usage;;
    esac
done
shift $(($OPTIND-1))

if [ -z "$INPUT" -o -z "$LOGDIR" -o -z "$TIMINGLOG" ]; then
    usage
fi

# collect input stats
SENTS=`wc -l < "$INPUT"`
WORDS=`wc -w < "$INPUT"`

cat <<EOF
* Data
$INPUT
$SENTS sents
$WORDS words
$(echo "scale = 2; $WORDS / $SENTS" | bc) words/sent

EOF

# grammar extraction timing
ETIME=`grep Elapsed "$LOGDIR/extract.log" | grep -oE '[0-9:.]+$' | head -n1`
EMEM=`grep Maximum "$LOGDIR/extract.ER"/*.ER | grep -oE '[0-9]+$' | python -c 'import sys; print sum(map(float, sys.stdin)) / 4096 / 1024'`
EWORKDERS=`ls "$LOGDIR/extract.ER"/*.ER | wc -l`
cat<<EOF
* Grammar Extraction
Wall clock time: $ETIME = `sec $ETIME`s
Memory usage: $EMEM GB
Workers: $EWORKDERS

EOF

# decoding timing
DTIME=`grep Elapsed "$LOGDIR/decode.log" | grep -oE '[0-9:.]+$' | head -n1`
DMEM=`grep Maximum "$LOGDIR/decode.ER"/*.ER | grep -oE '[0-9]+$' | python -c 'import sys; print sum(map(float, sys.stdin)) / 4096 / 1024'`
DWORKDERS=`ls "$LOGDIR/decode.ER"/*.ER | wc -l`
cat <<EOF
* Decoding
Wall clock time: $DTIME = `sec $DTIME`s
Memory usage: $DMEM GB
Workers: $DWORKDERS

EOF

# overall
TIME=`grep Elapsed "$TIMINGLOG" | grep -oE '[0-9:.]+$' | head -n1`
SEC=`sec $TIME`
cat<<EOF
* Overall
Wall clock time: $TIME = ${SEC}s
`echo "scale=2;$WORDS/$SEC*600" | bc` words/6min
`echo "scale=2;$SENTS/$SEC*600" | bc` sents/6min
EOF