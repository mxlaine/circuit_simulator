"""Plot the CSV produced by butterworth_demo (requires matplotlib)."""
import csv
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

root = Path(__file__).resolve().parents[1]
with (root / "docs/results/butterworth.csv").open() as stream:
    rows = [{key: float(value) for key, value in row.items()} for row in csv.DictReader(stream)]
fig, ax = plt.subplots(figsize=(8, 4.5), layout="constrained")
ax.semilogx([r["frequency_hz"] for r in rows], [r["analytical_db"] for r in rows],
            label="Analytical Butterworth response", color="#94a3b8", linewidth=3)
ax.semilogx([r["frequency_hz"] for r in rows][::4], [r["normalized_db"] for r in rows][::4],
            "o", label="C++ MNA solver", color="#2563eb", markersize=4)
ax.axvline(1000, color="#c2410c", linestyle="--", alpha=.6)
ax.annotate("1 kHz: −3.01 dB\n0.3536 V RMS", xy=(1000, -3.0103),
            xytext=(70, -32), arrowprops={"arrowstyle": "->", "color": "#c2410c"})
ax.set(xlabel="Frequency (Hz)", ylabel="Gain relative to 0.5 V passband (dB)",
       title="Fifth-order Butterworth low-pass · 1 kΩ source and load")
ax.spines[["top", "right"]].set_visible(False)
ax.grid(alpha=.2, which="both"); ax.legend(loc="lower left")
fig.savefig(root / "docs/images/butterworth-response.png", dpi=170)
