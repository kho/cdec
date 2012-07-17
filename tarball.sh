#!/bin/sh
COMMIT=`git log --pretty=format:'%h' -n 1`
OUTPUT="cdec-$COMMIT.tar.gz"
git archive --prefix=cdec-$COMMIT/ --format=tar HEAD | gzip - > "$OUTPUT"
echo "Saved `readlink -f "$OUTPUT"`"
