
# Run gcc/val_1 test
echo "Begin gcc test run.."
time ./sim 16 4 0 0 0 0 0 ../docs/val_gcc_trace_mem.txt > ad_gcc.10k
echo "End gcc test run.."
diff ad_gcc.10k ../docs/val_1.txt
echo "End of gcc test run diff"

# Run perl/val_2 test
echo "Begin perl test run.."
time ./sim 32 16 0 0 0 0 0 ../docs/val_perl_trace_mem.txt > ad_perl.10k && diff ad_perl.10k ../docs/val_2.txt
echo "End perl test run.."
echo "End of perl test run diff"

