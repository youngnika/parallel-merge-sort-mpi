import csv
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

steps = []
addresses = []
arrays = []

with open("memory_trace.csv", "r") as f:
    reader = csv.DictReader(f)

    for row in reader:
        steps.append(int(row["step"]))
        addresses.append(int(row["address"]))
        arrays.append(row["array"])

plt.rcParams["font.family"] = "DejaVu Sans"
plt.rcParams["font.size"] = 11

# График 1. Общий профиль обращений
fig, ax = plt.subplots(figsize=(12, 6))

ax.scatter(
    steps, addresses,
    s=0.3, c="red", alpha=0.5,
    rasterized=True
)

ax.set_title("Общий профиль обращений к памяти")
ax.set_xlabel("Номер обращения")
ax.set_ylabel("Условный адрес элемента")
ax.grid(True, alpha=0.25)

fig.tight_layout()
fig.savefig("locality_full.png", dpi=250)
plt.close(fig)

# График 2. Детальный профиль
fig, ax = plt.subplots(figsize=(12, 6))

limit = min(1000, len(steps))

ax.scatter(
    steps[:limit], addresses[:limit],
    s=5, c="red", alpha=0.7,
    rasterized=True
)

ax.set_title("Детальный профиль обращений к памяти")
ax.set_xlabel("Номер обращения")
ax.set_ylabel("Условный адрес элемента")
ax.grid(True, alpha=0.25)

fig.tight_layout()
fig.savefig("locality_detail.png", dpi=250)
plt.close(fig)

print("Созданы графики:")
print("locality_full.png")
print("locality_detail.png")
