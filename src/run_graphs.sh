#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 3 - Dynamic Instruction Scheduler
#
# Module: run_graphs.sh
#
# Shell script to automate DIS sim runs with required configurations for 
# generating graphs as mentioned docs/pa2_spec.pdf, sectionn 8.2.
# 
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
# 

#!/bin/bash

RED_COLOR='\e[91m \e[1m'
NO_COLOR='\e[0m'

n="1"
s="8"
n_limit="8"
s_limit="256"

SIM="./sim"
ZERO="0"
GCC_OFILE="gcc.dat"
PERL_OFILE="perl.dat"


function run_tests()
{
    # outer loop for 's'
    while [ $s -le $s_limit ]
    do
        echo "#s $s"
        # inner loop for 'n'
        while [ $n -le $n_limit ]
        do
            echo "$SIM $s $n $ZERO $ZERO $ZERO $ZERO $ZERO $1 >> $2"
            n=$[$n*2]
        done

        n="1" 
        s=$[$s*2]
    done

    n="1"
    s="8"
}


echo -e "#${RED_COLOR}runs for gcc trace.. start${NO_COLOR}"
run_tests ../docs/val_gcc_trace_mem.txt $GCC_OFILE
echo -e "#${RED_COLOR}runs for gcc.. stop${NO_COLOR}"
echo " "

echo -e "#${RED_COLOR}runs for perl trace.. start${NO_COLOR}"
run_tests ../docs/val_perl_trace_mem.txt $PERL_OFILE
echo -e "#${RED_COLOR}runs for perl.. stop${NO_COLOR}"
echo " "

