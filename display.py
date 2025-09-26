import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV (adjust path if needed)
df = pd.read_csv("file_transfer_times.csv")

# Optional: sort by size just in case
df = df.sort_values("size_bytes")

# Plot
plt.figure(figsize=(8,5))
plt.plot(df["size_bytes"], df["average_seconds"], marker="o", linewidth=2)

# X-axis in MB for readability
plt.xscale("log")  # log scale shows wide size range better
# plt.yscale("log")  # optional: log for clearer growth pattern

plt.xlabel("File Size (bytes)")
plt.ylabel("Average Transfer Time (seconds)")
plt.title("File Transfer Time vs File Size")
plt.grid(True, which="both", linestyle="--", alpha=0.6)

# Optionally annotate points
for x, y in zip(df["size_bytes"], df["average_seconds"]):
    plt.text(x, y*1.05, f"{y:.2f}s", ha="center", fontsize=8)

plt.tight_layout()
plt.show()
