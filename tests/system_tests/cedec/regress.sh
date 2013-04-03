#!/bin/sh

old=$1
new=$2

mkdir -p "regress.$new"

if [ -z "$show_diff" ]; then
    show_diff=false
fi

function run_test {
    info=$1; shift

    echo "+---------------------------------------"
    echo "| TEST-$tid: $info"
    echo "+---------------------------------------"

    out1="regress.$new/cdec.out.$tid"
    err1="regress.$new/cdec.err.$tid"
    out2="regress.$new/cedec.out.$tid"
    err2="regress.$new/cedec.err.$tid"

    ./compare.sh "$out1" "$err1" "$out2" "$err2" "$@"

    for i in cedec; do
	for j in out err; do
	    cmp "regress.$old/$i.$j.$tid" "regress.$new/$i.$j.$tid"
	done
    done

    if $show_diff; then
	for i in out err; do
	    f1="regress.$new/cdec.$i.$tid"
	    f2="regress.$new/cedec.$i.$tid"
	    if ! cmp -s "$f1" "$f2"; then
		vimdiff "$f1" "$f2"
	    fi
	done
    fi

    tid=$(($tid+1))
    echo
}

tid=1

tmp=`mktemp -d`

run_test "simple" -c cdec.ini -w weights -i input.txt.1
run_test "kbest" -c cdec.ini -w weights -k 10 -i input.txt.1
run_test "unique kbest" -c cdec.ini -w weights -k 10 -r -i input.txt.1
run_test "sentid in kbest" -c cdec.ini -w weights -k 10 -r -i input.txt.2

run_test "simple with lm" -c cdec.ini -w weights -i input.txt.1 -F "KLanguageModel lm.klm"
run_test "kbest with lm" -c cdec.ini -w weights -k 10 -i input.txt.1 -F "KLanguageModel lm.klm"
run_test "unique kbest with lm" -c cdec.ini -w weights -k 10 -r -i input.txt.1 -F "KLanguageModel lm.klm"
run_test "sentid in kbest with lm" -c cdec.ini -w weights -k 10 -r -i input.txt.2 -F "KLanguageModel lm.klm"

# run_test "simple-graphviz" -c cdec.ini -w weights -i input.txt.1 --graphviz
run_test "simple-show-johusa" -c cdec.ini -w weights -i input.txt.1 -J -F "KLanguageModel lm.klm"
run_test "simple-show-expected-length" -c cdec.ini -w weights -i input.txt.1 --show_expected_length -F "KLanguageModel lm.klm"
run_test "simple-show-partition" -c cdec.ini -w weights -i input.txt.1 --show_partition -F "KLanguageModel lm.klm"
run_test "simple-show-deriv" -c cdec.ini -w weights -i input.txt.1 --show_derivations "$tmp" -F "KLanguageModel lm.klm"
ls -l "$tmp"; rm -rf "$tmp"/*

run_test "kbest-show-deriv" -c cdec.ini -w weights -i input.txt.2 --show_derivations "$tmp" -k 10 -F "KLanguageModel lm.klm"
ls -l "$tmp"; rm -rf "$tmp"/*

# run_test "output forest" -c cdec.ini -w weights -i input.txt.2 --forest_output "$tmp"
# ls -l "$tmp"; rm -rf "$tmp"/*

run_test "hasref" -c cdec.ini -w weights -i input.txt
run_test "hasref-remove" -c cdec.ini -w weights -i input.txt --remove_intersected_rule_annotations

rm -rf "$tmp"
