#!/bin/sh

out1="$1"; shift
err1="$1"; shift
out2="$1"; shift
err2="$1"; shift

../../../decoder/cdec "$@" > "$out1" 2> "$err1"
../../../new-decoder/cedec "$@" > "$out2" 2> "$err2"
