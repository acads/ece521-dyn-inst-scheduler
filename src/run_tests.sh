
# Run gcc/val_1 test
echo "Begin gcc test run.."
time ./sim 16 4 0 0 0 0 0 ../docs/val_gcc_trace_mem.txt > ad_gcc.10k
echo "End gcc test run.."
diff -iw ad_gcc.10k ../docs/val_1.txt
echo "End of gcc test run diff"
echo " "
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

# Run perl/val_2 test
echo "Begin perl test run.."
time ./sim 32 16 0 0 0 0 0 ../docs/val_perl_trace_mem.txt > ad_perl.10k
echo "End perl test run.."
diff -iw ad_perl.10k ../docs/val_2.txt
echo "End of perl test run diff"
echo " "
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

# Run gcc extra/val_1 extra test
echo "Begin gcc extra test run.."
time ./sim 16 4 32 2048 8 0 0 ../docs/val_gcc_trace_mem.txt > ad_gcc_extra.10k
echo "End gcc extra test run.."
diff -iw ad_gcc_extra.10k ../docs/val_extra_1.txt
echo "End of gcc extra test run diff"
echo " "
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

