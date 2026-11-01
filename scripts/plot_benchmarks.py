#!/usr/bin/env python3
"""
Publication-Quality Benchmark Plotting Script for Succinct Rank and Select
Advanced Data Structures and Algorithms (ADSA Project)
Abhishek Nagar (206126002), Kishan Lambariya (206126017)
"""

import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Styling setup
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.sans-serif'] = 'Helvetica', 'Arial', 'DejaVu Sans'
plt.rcParams['axes.edgecolor'] = '#333333'
plt.rcParams['axes.linewidth'] = 1.0
plt.rcParams['grid.color'] = '#E0E0E0'
plt.rcParams['grid.linestyle'] = '--'
plt.rcParams['grid.alpha'] = 0.7

OUT_DIR = "results/figures"
os.makedirs(OUT_DIR, exist_ok=True)

# 1. Primitives Microbenchmark Plot
def plot_primitives():
    csv_file = "results/benchmark_primitives.csv"
    if not os.path.exists(csv_file):
        print(f"Skipping {csv_file}")
        return
    df = pd.read_csv(csv_file)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
    
    # Popcount
    df_pop = df[df['operation'] == 'popcount']
    colors_pop = ['#E74C3C', '#F39C12', '#2ECC71']
    bars1 = ax1.bar(df_pop['method'], df_pop['latency_ns'], color=colors_pop, edgecolor='#222', width=0.55)
    ax1.set_title("64-bit Popcount Latency", fontsize=13, fontweight='bold', pad=12)
    ax1.set_ylabel("Latency (nanoseconds / op)", fontsize=11)
    ax1.set_xlabel("Implementation Primitive", fontsize=11)
    for bar in bars1:
        yval = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2.0, yval + 0.08, f"{yval:.2f} ns", ha='center', va='bottom', fontweight='bold', fontsize=10)
    ax1.set_ylim(0, max(df_pop['latency_ns']) * 1.25)

    # Select64
    df_sel = df[df['operation'] == 'select64']
    colors_sel = ['#9B59B6', '#3498DB', '#1ABC9C']
    bars2 = ax2.bar(df_sel['method'], df_sel['latency_ns'], color=colors_sel, edgecolor='#222', width=0.55)
    ax2.set_title("64-bit Select64 Latency (Gog & Petri Listing 1)", fontsize=13, fontweight='bold', pad=12)
    ax2.set_ylabel("Latency (nanoseconds / op)", fontsize=11)
    ax2.set_xlabel("Implementation Primitive", fontsize=11)
    for bar in bars2:
        yval = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2.0, yval + 0.08, f"{yval:.2f} ns", ha='center', va='bottom', fontweight='bold', fontsize=10)
    ax2.set_ylim(0, max(df_sel['latency_ns']) * 1.25)

    plt.suptitle("Microbenchmark: 64-bit Word Primitives Comparison (TBL vs BW vs BLT)", fontsize=15, fontweight='bold', y=1.02)
    plt.tight_layout()
    plt.savefig(f"{OUT_DIR}/fig6_primitives_comparison.png", dpi=300, bbox_inches='tight')
    plt.savefig(f"{OUT_DIR}/fig6_primitives_comparison.svg", bbox_inches='tight')
    plt.close()
    print("  ✓ Saved fig6_primitives_comparison.png and .svg")

# 2. Rank Latency vs Bitvector Size
def plot_rank_scaling():
    csv_file = "results/benchmark_rank.csv"
    if not os.path.exists(csv_file): return
    df = pd.read_csv(csv_file)
    
    # Filter for density = 0.50
    df50 = df[df['density'] == 0.50]
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    methods = [
        ('BlockRank_512_64', 'BlockRank (512, 64)', '#2ECC71', 'o-', 2.5),
        ('BlockRank_1024_128', 'BlockRank (1024, 128)', '#27AE60', 's--', 2.0),
        ('Interleaved_1L_256', 'Interleaved RANK-1L (256 bits, 25% ovh)', '#E67E22', '^-', 2.5),
        ('Interleaved_1L_512', 'Interleaved RANK-1L (512 bits, 12.5% ovh)', '#D35400', 'v--', 2.0),
        ('Interleaved_1L_1024', 'Interleaved RANK-1L (1024 bits, 6.25% ovh)', '#C0392B', 'd:', 2.0),
    ]
    
    for m_id, label, col, fmt, lw in methods:
        sub = df50[df50['method'] == m_id].sort_values('bits')
        if not sub.empty:
            ax.plot(sub['bits'], sub['mean_ns'], fmt, label=label, color=col, linewidth=lw, markersize=8)

    ax.set_xscale('log', base=2)
    ax.set_xlabel("Bitvector Length n (bits)", fontsize=12, fontweight='bold')
    ax.set_ylabel("Query Latency (nanoseconds / rank query)", fontsize=12, fontweight='bold')
    ax.set_title("Rank Query Latency vs Bitvector Size (Density = 50%)\nStrict O(1) Scalability from 64K to 16M bits", fontsize=14, fontweight='bold', pad=12)
    ax.legend(frameon=True, facecolor='white', framealpha=0.9, fontsize=10, loc='upper left')
    ax.set_ylim(0, 10)
    
    # Add annotation for baseline
    naive_sub = df50[df50['method'] == 'Naive']
    if not naive_sub.empty:
        max_naive = naive_sub['mean_ns'].max()
        ax.text(0.70, 0.20, f"Naive Scan Baseline:\n~{max_naive:.0f} ns (O(n) linear scan)\n>1000x slower", 
                transform=ax.transAxes, bbox=dict(boxstyle='round,pad=0.5', facecolor='#FFEBEE', edgecolor='#E53935'),
                fontsize=10, color='#C62828', fontweight='bold')

    plt.tight_layout()
    plt.savefig(f"{OUT_DIR}/fig1_rank_latency_vs_size.png", dpi=300, bbox_inches='tight')
    plt.savefig(f"{OUT_DIR}/fig1_rank_latency_vs_size.svg", bbox_inches='tight')
    plt.close()
    print("  ✓ Saved fig1_rank_latency_vs_size.png and .svg")

# 3. Select Latency vs Bitvector Size
def plot_select_scaling():
    csv_file = "results/benchmark_select.csv"
    if not os.path.exists(csv_file): return
    df = pd.read_csv(csv_file)
    df50 = df[df['density'] == 0.50]

    fig, ax = plt.subplots(figsize=(10, 6))

    methods = [
        ('Sampled_s32', 'Sampled Select (s = 32)', '#3498DB', 'o-', 2.5),
        ('Sampled_s64', 'Sampled Select (s = 64)', '#2980B9', 's--', 2.0),
        ('Sampled_s128', 'Sampled Select (s = 128)', '#1F618D', '^-.', 2.0),
        ('Clark_SEL_C', 'Clark 2-Level (SEL-C)', '#9B59B6', 'd-', 2.5),
        ('BinarySearch_BS', 'Binary Search (SEL-BS, 0% Extra Space)', '#E74C3C', 'x--', 2.0),
    ]

    for m_id, label, col, fmt, lw in methods:
        sub = df50[df50['method'] == m_id].sort_values('bits')
        if not sub.empty:
            ax.plot(sub['bits'], sub['mean_ns'], fmt, label=label, color=col, linewidth=lw, markersize=8)

    ax.set_xscale('log', base=2)
    ax.set_xlabel("Bitvector Length n (bits)", fontsize=12, fontweight='bold')
    ax.set_ylabel("Query Latency (nanoseconds / select query)", fontsize=12, fontweight='bold')
    ax.set_title("Select Query Latency vs Bitvector Size (Density = 50%)\nComparison of Sampled, Clark SEL-C, and Binary Search Baselines", fontsize=14, fontweight='bold', pad=12)
    ax.legend(frameon=True, facecolor='white', framealpha=0.9, fontsize=10, loc='center right')
    ax.set_ylim(0, 60)

    naive_sub = df50[df50['method'] == 'Naive']
    if not naive_sub.empty:
        max_naive = naive_sub['mean_ns'].max()
        ax.text(0.05, 0.80, f"Naive Select Scan:\n~{max_naive:.0f} ns (O(n))\n>2500x slower", 
                transform=ax.transAxes, bbox=dict(boxstyle='round,pad=0.5', facecolor='#FFEBEE', edgecolor='#E53935'),
                fontsize=10, color='#C62828', fontweight='bold')

    plt.tight_layout()
    plt.savefig(f"{OUT_DIR}/fig2_select_latency_vs_size.png", dpi=300, bbox_inches='tight')
    plt.savefig(f"{OUT_DIR}/fig2_select_latency_vs_size.svg", bbox_inches='tight')
    plt.close()
    print("  ✓ Saved fig2_select_latency_vs_size.png and .svg")

# 4. Time-Space Trade-off Analysis (Pareto Frontier)
def plot_tradeoffs():
    csv_file = "results/benchmark_tradeoff.csv"
    if not os.path.exists(csv_file): return
    df = pd.read_csv(csv_file)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # Rank Trade-off
    df_rank = df[df['type'] == 'Rank'].sort_values('overhead_pct')
    ax1.plot(df_rank['overhead_pct'], df_rank['latency_ns'], 'o-', color='#E67E22', linewidth=2.5, markersize=9)
    for _, row in df_rank.iterrows():
        b = int(row['parameter_value'])
        ax1.annotate(f"b={b} bits\n({row['overhead_pct']:.1f}%)", 
                     (row['overhead_pct'], row['latency_ns']),
                     textcoords="offset points", xytext=(0, 10), ha='center', fontsize=9, fontweight='bold')
    ax1.set_title("Rank Time-Space Trade-off (Varying Block Size b)", fontsize=13, fontweight='bold', pad=12)
    ax1.set_xlabel("Memory Overhead (% of original bitvector)", fontsize=11, fontweight='bold')
    ax1.set_ylabel("Query Latency (ns)", fontsize=11, fontweight='bold')
    ax1.set_ylim(0, max(df_rank['latency_ns']) * 1.3)

    # Select Trade-off
    df_sel = df[df['type'] == 'Select'].sort_values('overhead_pct')
    ax2.plot(df_sel['overhead_pct'], df_sel['latency_ns'], 's-', color='#2980B9', linewidth=2.5, markersize=9)
    for _, row in df_sel.iterrows():
        s = int(row['parameter_value'])
        ax2.annotate(f"s={s}\n({row['overhead_pct']:.1f}%)", 
                     (row['overhead_pct'], row['latency_ns']),
                     textcoords="offset points", xytext=(0, 10), ha='center', fontsize=9, fontweight='bold')
    ax2.set_title("Select Time-Space Trade-off (Varying Sample Interval s)", fontsize=13, fontweight='bold', pad=12)
    ax2.set_xlabel("Memory Overhead (% of original bitvector)", fontsize=11, fontweight='bold')
    ax2.set_ylabel("Query Latency (ns)", fontsize=11, fontweight='bold')
    ax2.set_ylim(0, max(df_sel['latency_ns']) * 1.3)

    plt.suptitle("Empirical Time-Space Trade-Off Analysis (Objective 5 & 6)", fontsize=15, fontweight='bold', y=1.02)
    plt.tight_layout()
    plt.savefig(f"{OUT_DIR}/fig3_tradeoff_pareto_rank.png", dpi=300, bbox_inches='tight')
    plt.savefig(f"{OUT_DIR}/fig3_tradeoff_pareto_rank.svg", bbox_inches='tight')
    plt.close()
    print("  ✓ Saved fig3_tradeoff_pareto_rank.png and .svg")

# 5. Density Impact (5%, 20%, 50%)
def plot_density_impact():
    csv_file = "results/benchmark_select.csv"
    if not os.path.exists(csv_file): return
    df = pd.read_csv(csv_file)
    
    # Pick n = 4M bits
    df_4m = df[df['bits'] == (1 << 22)]
    if df_4m.empty: df_4m = df[df['bits'] == df['bits'].max()]

    methods = ['Sampled_s64', 'BinarySearch_BS', 'Clark_SEL_C']
    sub = df_4m[df_4m['method'].isin(methods)]

    pivot_time = sub.pivot(index='density', columns='method', values='mean_ns')
    pivot_space = sub.pivot(index='density', columns='method', values='overhead_pct')

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5))

    densities_pct = [f"{int(d*100)}%" for d in pivot_time.index]
    x = np.arange(len(densities_pct))
    width = 0.25

    # Query Latency across densities
    ax1.bar(x - width, pivot_time['Sampled_s64'], width, label='Sampled Select (s=64)', color='#3498DB', edgecolor='#222')
    ax1.bar(x, pivot_time['Clark_SEL_C'], width, label='Clark Select (SEL-C)', color='#9B59B6', edgecolor='#222')
    ax1.bar(x + width, pivot_time['BinarySearch_BS'], width, label='Binary Search (SEL-BS)', color='#E74C3C', edgecolor='#222')
    ax1.set_xticks(x)
    ax1.set_xticklabels(densities_pct)
    ax1.set_xlabel("Bit Density (percentage of 1s)", fontsize=11, fontweight='bold')
    ax1.set_ylabel("Mean Latency (ns)", fontsize=11, fontweight='bold')
    ax1.set_title("Impact of Bit Density on Select Query Latency", fontsize=13, fontweight='bold', pad=12)
    ax1.legend(frameon=True, facecolor='white')

    # Space Overhead across densities
    ax2.bar(x - width, pivot_space['Sampled_s64'], width, label='Sampled Select (s=64)', color='#3498DB', edgecolor='#222')
    ax2.bar(x, pivot_space['Clark_SEL_C'], width, label='Clark Select (SEL-C)', color='#9B59B6', edgecolor='#222')
    ax2.bar(x + width, pivot_space['BinarySearch_BS'], width, label='Binary Search (SEL-BS)', color='#E74C3C', edgecolor='#222')
    ax2.set_xticks(x)
    ax2.set_xticklabels(densities_pct)
    ax2.set_xlabel("Bit Density (percentage of 1s)", fontsize=11, fontweight='bold')
    ax2.set_ylabel("Space Overhead (%)", fontsize=11, fontweight='bold')
    ax2.set_title("Impact of Bit Density on Memory Overhead", fontsize=13, fontweight='bold', pad=12)
    ax2.legend(frameon=True, facecolor='white')

    plt.suptitle("Analysis Across Bit Densities (5%, 20%, 50%) - Slide 9 & 10", fontsize=15, fontweight='bold', y=1.02)
    plt.tight_layout()
    plt.savefig(f"{OUT_DIR}/fig5_density_impact.png", dpi=300, bbox_inches='tight')
    plt.savefig(f"{OUT_DIR}/fig5_density_impact.svg", bbox_inches='tight')
    plt.close()
    print("  ✓ Saved fig5_density_impact.png and .svg")

# 6. Comprehensive 4-Panel Executive Dashboard
def plot_executive_dashboard():
    fig, axes = plt.subplots(2, 2, figsize=(15, 11))
    
    # 1. Primitives (ax[0,0])
    df_prim = pd.read_csv("results/benchmark_primitives.csv")
    df_pop = df_prim[df_prim['operation'] == 'popcount']
    ax = axes[0, 0]
    ax.bar(df_pop['method'], df_pop['latency_ns'], color=['#E74C3C', '#F39C12', '#2ECC71'], edgecolor='#222', width=0.5)
    ax.set_title("(A) 64-bit Popcount Microbenchmark", fontsize=12, fontweight='bold')
    ax.set_ylabel("Latency (ns)", fontsize=10)
    for i, v in enumerate(df_pop['latency_ns']):
        ax.text(i, v + 0.05, f"{v:.2f} ns", ha='center', fontweight='bold')

    # 2. Rank Scalability (ax[0,1])
    df_rank = pd.read_csv("results/benchmark_rank.csv")
    df_rank_50 = df_rank[df_rank['density'] == 0.50]
    ax = axes[0, 1]
    for m, col in [('BlockRank_512_64', '#2ECC71'), ('Interleaved_1L_256', '#E67E22'), ('Interleaved_1L_1024', '#C0392B')]:
        sub = df_rank_50[df_rank_50['method'] == m].sort_values('bits')
        ax.plot(sub['bits'], sub['mean_ns'], 'o-', label=m, color=col, linewidth=2)
    ax.set_xscale('log', base=2)
    ax.set_title("(B) Rank Latency vs Bitvector Length (Density=50%)", fontsize=12, fontweight='bold')
    ax.set_xlabel("Bitvector Length n (bits)", fontsize=10)
    ax.set_ylabel("Latency (ns)", fontsize=10)
    ax.legend(fontsize=9, facecolor='white')
    ax.set_ylim(0, 10)

    # 3. Select Scalability (ax[1,0])
    df_sel = pd.read_csv("results/benchmark_select.csv")
    df_sel_50 = df_sel[df_sel['density'] == 0.50]
    ax = axes[1, 0]
    for m, col in [('Sampled_s64', '#2980B9'), ('Clark_SEL_C', '#9B59B6'), ('BinarySearch_BS', '#E74C3C')]:
        sub = df_sel_50[df_sel_50['method'] == m].sort_values('bits')
        ax.plot(sub['bits'], sub['mean_ns'], 's-', label=m, color=col, linewidth=2)
    ax.set_xscale('log', base=2)
    ax.set_title("(C) Select Latency vs Bitvector Length (Density=50%)", fontsize=12, fontweight='bold')
    ax.set_xlabel("Bitvector Length n (bits)", fontsize=10)
    ax.set_ylabel("Latency (ns)", fontsize=10)
    ax.legend(fontsize=9, facecolor='white')

    # 4. Trade-off Pareto (ax[1,1])
    df_to = pd.read_csv("results/benchmark_tradeoff.csv")
    ax = axes[1, 1]
    df_to_r = df_to[df_to['type'] == 'Rank'].sort_values('overhead_pct')
    df_to_s = df_to[df_to['type'] == 'Select'].sort_values('overhead_pct')
    ax.plot(df_to_r['overhead_pct'], df_to_r['latency_ns'], 'o-', label='Rank (Interleaved)', color='#E67E22', linewidth=2.5)
    ax.plot(df_to_s['overhead_pct'], df_to_s['latency_ns'], 's-', label='Select (Sampled)', color='#2980B9', linewidth=2.5)
    ax.set_title("(D) Time-Space Trade-Off Pareto Frontier", fontsize=12, fontweight='bold')
    ax.set_xlabel("Memory Overhead (%)", fontsize=10)
    ax.set_ylabel("Latency (ns)", fontsize=10)
    ax.legend(fontsize=9, facecolor='white')

    plt.suptitle("Advanced Data Structures: Succinct Rank & Select Comprehensive Evaluation Dashboard\nAbhishek Nagar (206126002) & Kishan Lambariya (206126017)", 
                 fontsize=15, fontweight='bold', y=0.99)
    plt.tight_layout()
    plt.savefig(f"{OUT_DIR}/fig7_overall_summary_dashboard.png", dpi=300, bbox_inches='tight')
    plt.savefig(f"{OUT_DIR}/fig7_overall_summary_dashboard.svg", bbox_inches='tight')
    plt.close()
    print("  ✓ Saved fig7_overall_summary_dashboard.png and .svg")

if __name__ == "__main__":
    print("Generating High-Resolution Benchmark Plots via Matplotlib...")
    plot_primitives()
    plot_rank_scaling()
    plot_select_scaling()
    plot_tradeoffs()
    plot_density_impact()
    plot_executive_dashboard()
    print("All plots generated successfully in results/figures/")
