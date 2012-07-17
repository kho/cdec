#!/bin/sh

usage () {
    echo "Usage: $0 output_dir"
    exit 1
}

if [ -z "$1" ]; then
    usage
fi
COMMIT=`git log --pretty=format:'%h' -n 1`
OUTPUT="$1/cdec-$COMMIT.tar.gz"
git archive --prefix=cdec-$COMMIT/ --format=tar HEAD | gzip - > "$OUTPUT"
echo "Saved `readlink -f "$OUTPUT"`"
