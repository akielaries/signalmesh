import numpy as np
import matplotlib.pyplot as plt

# Set up the frequencies
f_ref = 27e6  # 27 MHz reference crystal
f_out = 84e6  # 84 MHz desired output

# Calculate the divider ratio N (this would be a fraction in real hardware)
N = f_out / f_ref
print(f"Divider ratio N = {f_out}/{f_ref} = {N}")
print(f"This is {N} = 28/9 as a fraction")

# Let's simulate 9 cycles of the reference signal (since 84/27 = 28/9)
t_duration = 9 / f_ref  # Time for 9 reference cycles
t = np.linspace(0, t_duration, 10000)

# Create signals
reference = np.sin(2 * np.pi * f_ref * t)
output = np.sin(2 * np.pi * f_out * t)

# Mark the rising edges (phase alignment points)
ref_edges = []
out_edges = []

# Find where reference crosses zero with positive slope
for i in range(1, len(t)):
    if reference[i-1] < 0 and reference[i] >= 0:
        ref_edges.append(t[i])
        
# Find where output crosses zero with positive slope  
for i in range(1, len(t)):
    if output[i-1] < 0 and output[i] >= 0:
        out_edges.append(t[i])

# Plot everything
plt.figure(figsize=(14, 10)) # Increased figure height for 4 plots

# Plot the waveforms
plt.subplot(4, 1, 1)
plt.plot(t * 1e9, reference, 'b-', label=f'Reference: 27 MHz', linewidth=2, alpha=0.7)
plt.plot(t * 1e9, output, 'r-', label=f'Output: 84 MHz (Target)', linewidth=1.5, alpha=0.7)
plt.xlabel('Time (ns)')
plt.ylabel('Amplitude')
plt.title('Reference (27 MHz) vs. Output (84 MHz)')
plt.grid(True, alpha=0.3)
plt.legend()

# Mark the phase alignment points on the first plot
for edge in ref_edges:
    plt.axvline(x=edge * 1e9, color='blue', linestyle='--', alpha=0.3)
for edge in out_edges:
    plt.axvline(x=edge * 1e9, color='red', linestyle='--', alpha=0.3)

# Zoom in on a small section to see phase relationship
plt.subplot(4, 1, 2)
zoom_start = 0
zoom_end = 3 / f_ref  # Show first 3 reference cycles
zoom_indices = (t >= zoom_start) & (t <= zoom_end)

plt.plot(t[zoom_indices] * 1e9, reference[zoom_indices], 'b-', linewidth=3, label='27 MHz Reference')
plt.plot(t[zoom_indices] * 1e9, output[zoom_indices], 'r-', linewidth=2, label='84 MHz Output')
plt.xlabel('Time (ns)')
plt.ylabel('Amplitude')
plt.title('Zoomed in')
plt.grid(True, alpha=0.3)
plt.legend()

# Show what the divided output looks like (what the phase detector actually compares)
plt.subplot(4, 1, 3)

# Simulate the divided signal: output ÷ (28/9) = output × 9/28
# This should match the reference frequency!
divider_numerator = 28
divider_denominator = 9

# For simplicity, let's create what the divided signal WOULD look like.
# We give it a slightly smaller amplitude (0.8) to make it visually distinct.
divided_signal = 0.8 * np.sin(2 * np.pi * (f_out * divider_denominator/divider_numerator) * t)

plt.plot(t * 1e9, reference, 'b-', linewidth=2, label='Reference (Amp=1.0)', alpha=0.8)
plt.plot(t * 1e9, divided_signal, 'g-', linewidth=2, label=f'Divided Output (Amp=0.8)', alpha=0.8)
plt.xlabel('Time (ns)')
plt.ylabel('Amplitude')
plt.title('What the Phase Detector Compares (The Goal: Make These Identical)')
plt.grid(True, alpha=0.3)
plt.legend()

# --- NEW SUBPLOT ---
# Visualize the "Lock" by showing the alignment of rising edges
plt.subplot(4, 1, 4)
plt.title("Clock comparison. 9 reference cycles vs 28 output cycles")

# Plot reference and output edges on two separate lines
y_ref = np.ones_like(ref_edges) # Plot all points at y=1
y_out = np.zeros_like(out_edges) # Plot all points at y=0
plt.plot(np.array(ref_edges) * 1e9, y_ref, 'bo', markersize=8, label='Reference (27 MHz) Rising Edge')
plt.plot(np.array(out_edges) * 1e9, y_out, 'rx', markersize=6, label='Output (84 MHz) Rising Edge')

# Find and highlight the exact alignment points
alignment_period_ns = (9 / f_ref) * 1e9
num_alignments = int(t_duration / (9 / f_ref))

for i in range(num_alignments + 1):
    t_align_ns = i * alignment_period_ns
    plt.axvline(x=t_align_ns, color='purple', linestyle=':', linewidth=2, alpha=0.8)
    # Add text. Stagger the y-position to avoid overlap.
    if i < num_alignments: # Avoid text running off the plot
        plt.text(t_align_ns + 5, 0.4 + (i%2 * 0.2), f'LOCK POINT\n({i*9} ref, {i*28} out)',
                 color='purple', ha='left', fontsize=9)

plt.xlabel('Time (ns)')
plt.yticks([0, 1], ['Output Edges', 'Reference Edges'])
plt.ylim(-0.5, 1.5)
plt.grid(True, axis='x', linestyle=':', alpha=0.5)
plt.legend(loc='upper right')

plt.tight_layout()
plt.show()

# Now let's create a clearer visualization of the phase alignment
print("\n" + "="*60)
print("KEY INSIGHT: The fractional-N divider")
print("="*60)

# Show how edges align over 9 reference cycles
print(f"\nOver 9 cycles of the 27 MHz reference:")
print(f"Time for 9 reference cycles = {9/f_ref*1e9:.2f} ns")

print(f"\nDuring this same time period:")
print(f"84 MHz completes {f_out * (9/f_ref):.1f} cycles = 28 cycles exactly!")

print("\n" + "-"*40)
print("The phase detector sees this:")
print("-"*40)
print("Reference edges (27 MHz): every {:.2f} ns".format(1/f_ref*1e9))
print("Divided output edges: every {:.2f} ns".format((divider_numerator/divider_denominator)/f_out*1e9))
print("\nThese two should be EQUAL when locked:")
print(f"1/27 MHz = {1/f_ref*1e9:.2f} ns")
print(f"(28/9)/84 MHz = {(divider_numerator/divider_denominator)/f_out*1e9:.2f} ns")

# Create a simple text-based timing diagram
print("\n" + "="*60)
print("Timing Alignment Over 9 Reference Cycles")
print("="*60)

ref_period = 1/f_ref
out_period = 1/f_out

print(f"\nReference period: {ref_period*1e9:.2f} ns")
print(f"Output period: {out_period*1e9:.2f} ns")
print(f"Ratio: {ref_period/out_period:.4f} = 28/9")

print("\nEdge alignment pattern (R=Reference, O=Output):")
print("Time (ns) | Ref Edge | Out Edge | Aligned?")
print("-"*40)

for i in range(10):
    t_ref = i * ref_period
    # Find nearest output edge
    out_edge_num = round(t_ref / out_period)
    t_out = out_edge_num * out_period
    aligned = abs(t_ref - t_out) < 1e-12
    
    if i <= 9:  # Show first 10 edges
        print(f"{t_ref*1e9:8.2f} |    R{i}    |    O{out_edge_num:2d}  | {'YES' if aligned else 'no'}")

print("\nNote: Every 9 reference cycles (R0-R9),")
print("the output completes exactly 28 cycles (O0-O28).")
print("At these points, the edges are perfectly aligned!")
