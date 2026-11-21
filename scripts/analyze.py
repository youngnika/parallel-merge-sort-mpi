import csv
import statistics

data = {}

with open("results_all.csv") as f:
    for row in csv.DictReader(f):
        key = (int(row["N"]), int(row["P"]))
        if row["correct"] != "YES":
            raise ValueError("Incorrect result: " + str(key))
        data.setdefault(key, []).append(float(row["time"]))

print("N,P,median_time,speedup,efficiency")

for n in sorted(set(n for n, p in data)):
    t1 = statistics.median(data[(n, 1)])
    for p in sorted(p for nn, p in data if nn == n):
        times = data[(n, p)]
        if len(times) != 5:
            raise ValueError("Expected 5 runs: " + str((n, p)))
        t = statistics.median(times)
        s = t1 / t
        e = 100 * s / p
        print(f"{n},{p},{t:.6f},{s:.3f},{e:.2f}")
