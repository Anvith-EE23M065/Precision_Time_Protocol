import pandas as pd
import matplotlib.pyplot as plt
import os

# === Automatically use the directory of this script ===
script_dir = os.path.dirname(os.path.abspath(__file__))
master_path = os.path.join(script_dir, "master_data.csv")
slave_path = os.path.join(script_dir, "slave_data.csv")

# === Load CSVs ===
master_df = pd.read_csv(master_path)
slave_df = pd.read_csv(slave_path)

# === Convert timestamps to numeric ===
master_df['Timestamp'] = pd.to_numeric(master_df['Timestamp'])
slave_df['Timestamp'] = pd.to_numeric(slave_df['Timestamp'])

# === Sort both dataframes ===
master_df = master_df.sort_values("Timestamp").reset_index(drop=True)
slave_df = slave_df.sort_values("Timestamp").reset_index(drop=True)

# === Align nearest timestamps ===
merged_df = pd.merge_asof(master_df, slave_df, on="Timestamp", direction="nearest", suffixes=('_Master', '_Slave'))

# === Compute offset ===
merged_df['Offset'] = merged_df['Master Counter'] - merged_df['Slave Counter']

# === Plotting ===
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)

# --- Counter vs Timestamp ---
ax1.plot(merged_df['Timestamp'], merged_df['Master Counter'], label='Master', color='blue')
ax1.plot(merged_df['Timestamp'], merged_df['Slave Counter'], label='Slave', color='green')
ax1.set_ylabel("Counter")
ax1.set_title("Counters vs Timestamp")
ax1.legend()
ax1.grid(True)

# --- Offset vs Timestamp ---
ax2.plot(merged_df['Timestamp'], merged_df['Offset'], label='Offset', color='red')
ax2.set_xlabel("Timestamp (ms)")
ax2.set_ylabel("Offset (ticks)")
ax2.set_title("Offset vs Timestamp")
ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.show()
