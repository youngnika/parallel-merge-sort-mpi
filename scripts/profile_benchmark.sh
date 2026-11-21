#!/bin/bash
#SBATCH --job-name=merge_profile
#SBATCH --partition=test
#SBATCH --nodes=3
#SBATCH --ntasks=32
#SBATCH --time=00:15:00
#SBATCH --output=profile_%j.out
#SBATCH --error=profile_%j.err

cd "${MERGE_SORT_DIR:-/mnt/scratch/users/$USER/merge_sort}"

export PATH=/opt/mpi/openmpi-4.1.8-gcc12/bin:$PATH
export LD_LIBRARY_PATH=/opt/mpi/openmpi-4.1.8-gcc12/lib:${LD_LIBRARY_PATH:-}

echo "N,P,Run,Scatter,Sort,Send,Recv,Merge,MergedElements,SentElements,TotalTime,Correct" > profile_results.csv

for p in 1 2 4 8 16 32; do
    for run in 1 2 3 4 5; do
        echo "Running P=$p Run=$run"

        output=$(mpirun --mca pml ob1 \
            --mca btl self,tcp \
            --mca btl_tcp_if_include enp7s0 \
            -np "$p" ./merge_sort_mpi_profile 1000000)

        echo "$output" | python3 -c '
import sys, re
s = sys.stdin.read()
p = re.search(r"PROFILE N=(\d+) P=(\d+) Scatter=([\d.]+) Sort=([\d.]+) Send=([\d.]+) Recv=([\d.]+) Merge=([\d.]+) MergedElements=(\d+) SentElements=(\d+)", s)
t = re.search(r"N=(\d+) P=(\d+) Time=([\d.]+) s Correct=(YES|NO)", s)
if not p or not t:
    print("ERROR: Cannot parse output", file=sys.stderr)
    print(s, file=sys.stderr)
    sys.exit(1)
print(",".join([p.group(1), p.group(2), sys.argv[1]] +
               list(p.groups()[2:]) + [t.group(3), t.group(4)]))
' "$run" >> profile_results.csv

    done
done

echo "Experiment completed"
