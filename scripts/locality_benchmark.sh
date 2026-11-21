#!/bin/bash
#SBATCH --job-name=locality
#SBATCH --partition=test
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --time=00:10:00
#SBATCH --output=locality_%j.out
#SBATCH --error=locality_%j.err

set -e

cd "${MERGE_SORT_DIR:-/mnt/scratch/users/$USER/merge_sort}"

export PATH=/opt/mpi/openmpi-4.1.8-gcc12/bin:$PATH
export LD_LIBRARY_PATH=/opt/mpi/openmpi-4.1.8-gcc12/lib:${LD_LIBRARY_PATH:-}

echo "Node: $(hostname)"

printf "N,run,time,correct\n" > locality_results.csv

for n in 10000 50000 100000 500000 1000000 5000000 10000000; do
    for r in 1 2 3 4 5; do

        output=$(mpirun --mca pml ob1 \
            --mca btl self,tcp \
            --mca btl_tcp_if_include enp7s0 \
            -np 1 ./merge_sort_mpi "$n")

        line=$(printf "%s\n" "$output" | grep "^N=")

        time=$(printf "%s\n" "$line" |
            sed -n 's/.*Time=\([0-9.]*\).*/\1/p')

        correct=$(printf "%s\n" "$line" |
            sed -n 's/.*Correct=\([A-Z]*\).*/\1/p')

        printf "%s,%s,%s,%s\n" \
            "$n" "$r" "$time" "$correct" >> locality_results.csv

        echo "N=$n Run=$r Time=$time Correct=$correct"

        test "$correct" = YES
    done
done

echo "Locality benchmark completed"
