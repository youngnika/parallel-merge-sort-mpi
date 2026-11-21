#!/bin/bash
#SBATCH --job-name=weak_scaling
#SBATCH --partition=test
#SBATCH --nodes=3
#SBATCH --ntasks=32
#SBATCH --time=00:15:00
#SBATCH --output=weak_scaling_%j.out
#SBATCH --error=weak_scaling_%j.err

set -e

cd "${MERGE_SORT_DIR:-/mnt/scratch/users/$USER/merge_sort}"

export PATH=/opt/mpi/openmpi-4.1.8-gcc12/bin:$PATH
export LD_LIBRARY_PATH=/opt/mpi/openmpi-4.1.8-gcc12/lib:${LD_LIBRARY_PATH:-}

printf "N,P,run,time,correct\n" > weak_scaling_results.csv

for p in 1 2 4 8 16 32; do
    n=$((p * 1000000))

    for r in 1 2 3 4 5; do

        output=$(mpirun --mca pml ob1 \
            --mca btl self,tcp \
            --mca btl_tcp_if_include enp7s0 \
            -np "$p" ./merge_sort_mpi "$n")

        line=$(printf "%s\n" "$output" | grep "^N=")

        time=$(printf "%s\n" "$line" |
            sed -n 's/.*Time=\([0-9.]*\).*/\1/p')

        correct=$(printf "%s\n" "$line" |
            sed -n 's/.*Correct=\([A-Z]*\).*/\1/p')

        printf "%s,%s,%s,%s,%s\n" \
            "$n" "$p" "$r" "$time" "$correct" \
            >> weak_scaling_results.csv

        echo "N=$n P=$p Run=$r Time=$time Correct=$correct"

        test "$correct" = YES
    done
done

echo "Weak scaling benchmark completed"
