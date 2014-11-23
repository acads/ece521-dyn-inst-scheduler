#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 3 - Dynamic Instruction Scheduler
#
# Module: run_tests.sh
#
# Shell test script to run the simulator with different combinations
# against gcc/perl traces and compare the output with the TA given
# validation runs.
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#

#!/bin/bash

NUM_PARAMS=2

function print_usage()
{
    echo "Usage: $0 <test-#> <diff-required>"
    echo "test-#: gcc - 1, perl - 2, gcc extra - 3, gcc perl - 4, all - 5"
    echo "diff-required: 0 - no diff, 1 - with diff"
}

function normal_exit()
{
    echo " "
    exit 0
}


# Run gcc/val_1 test
function gcc()
{
    echo "Begin gcc test run.."
    time ./sim 16 4 0 0 0 0 0 ../docs/val_gcc_trace_mem.txt > ad_gcc.10k
    echo "End gcc test run.."

    if [ "$1" -eq 1 ]
    then
    echo " "
        echo "Begin gcc test run diff"
        diff -iw ad_gcc.10k ../docs/val_1.txt
        echo "End of gcc test run diff"
    fi

    echo " "
    echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
}


# Run gcc extra/val_1 extra test
function gcc_extra()
{
    echo "Begin gcc extra test run.."
    time ./sim 16 4 32 2048 8 0 0 ../docs/val_gcc_trace_mem.txt > ad_gcc_extra.10k
    echo "End gcc extra test run.."

    if [ "$1" -eq 1 ]
    then
    echo " "
        echo "Begin gcc test run diff"
        diff -iw ad_gcc_extra.10k ../docs/val_extra_1.txt
        echo "End of gcc extra test run diff"
    fi

    echo " "
    echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
}



# Run perl/val_2 test
function perl()
{
    echo "Begin perl test run.."
    time ./sim 32 16 0 0 0 0 0 ../docs/val_perl_trace_mem.txt > ad_perl.10k
    echo "End perl test run.."

    if [ "$1" -eq 1 ]
    then
    echo " "
        echo "Begin gcc test run diff"
        diff -iw ad_perl.10k ../docs/val_2.txt
        echo "End of perl test run diff"
    fi

    echo " "
    echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
}


# Run perl extra/val_2 extra test
function perl_extra()
{
    echo "Begin perl extra test run.."
    time ./sim 32 8 32 1024 4 2048 8 ../docs/val_perl_trace_mem.txt > ad_perl_extra.10k
    echo "End perl extra test run.."

    if [ "$1" -eq 1 ]
    then
    echo " "
        echo "Begin gcc test run diff"
        diff -iw ad_perl_extra.10k ../docs/val_extra_2.txt
        echo "End of perl extra test run diff"
    fi

    echo " "
    echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
}


if [ $# -ne "$NUM_PARAMS" ]
then
    echo "Error: Invalid usage."
    print_usage
    normal_exit
fi

case "$1" in
    1) gcc $2 ;;
    2) perl $2 ;;
    3) gcc_extra $2 ;;
    4) perl_extra $2 ;;
    5) gcc $2
       perl $2
       gcc_extra $2
       perl_extra $2
       ;;
esac

