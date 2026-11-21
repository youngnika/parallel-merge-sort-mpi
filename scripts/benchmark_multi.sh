#!/bin/bash
set -e
cd "${MERGE_SORT_DIR:-/mnt/scratch/users/$USER/merge_sort}"
printf "N,P,run,time,correct\n" > results_multi.csv
for n in 1000000 5000000 10000000; do
 for p in 16 32; do
  for r in 1 2 3 4 5; do
   output=$(mpirun --mca pml ob1 --mca btl self,tcp --mca btl_tcp_if_include enp7s0 -np "$p" ./merge_sort_mpi "$n")
   line=$(printf "%s\n" "$output" | grep "^N=")
   time=$(printf "%s\n" "$line" | sed -n "s/.*Time=\([0-9.]*\).*/\1/p")
   correct=$(printf "%s\n" "$line" | sed -n "s/.*Correct=\([A-Z]*\).*/\1/p")
   printf "%s,%s,%s,%s,%s\n" "$n" "$p" "$r" "$time" "$correct" >> results_multi.csv
   echo "N=$n P=$p Run=$r Time=$time Correct=$correct"
   test "$correct" = YES
  done
 done
done
