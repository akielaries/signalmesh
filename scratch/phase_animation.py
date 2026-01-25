import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# --- Simulation Parameters ---
F_REF = 27e6  # 27 MHz Reference
F_OUT_TARGET = 84e6  # 84 MHz Target Output
RATIO_NUM = 28
RATIO_DEN = 9

# --- PLL Parameters (Simplified PI Controller) ---
KP = 0.2  # Proportional gain (Tuned for fast lock with less overshoot)
KI = 0.04  # Integral gain (Tuned for fast lock with less overshoot)
F_VCO_CENTER = 80e6  # Start the VCO off-frequency
VCO_SENSITIVITY = 10e6 # How much volts affect frequency

# --- Animation Parameters ---
SIM_TIME_NS = 800  # Total simulation time in nanoseconds
TIME_STEP_NS = 1.0 # Time step for each frame in nanoseconds

# --- Global State Variables ---
# We use a class to hold the state of our simulation
class PllState:
    def __init__(self):
        self.time = 0.0
        self.vco_freq = F_VCO_CENTER
        self.phase_error = 0.0
        self.integrated_error = 0.0
        self.ref_phase_accumulator = 0.0
        self.vco_phase_accumulator = 0.0
        self.feedback_divider_accumulator = 0.0

        # Store history for plotting
        self.history = {
            'time': [],
            'vco_freq': [],
            'phase_error': [],
            'ref_edges': [],
            'out_edges': []
        }

state = PllState()

# --- Set up the plot ---
fig = plt.figure(figsize=(12, 10))
fig.suptitle("Live PLL Lock Animation (from 80 MHz to 84 MHz)", fontsize=16)

# 1. Waveform plot (NEW)
ax1 = fig.add_subplot(3, 1, 1)
ax1.set_title("1. Waveforms (Reference vs. Live VCO)")
ax1.set_xlabel("Time (ns)")
ax1.set_ylabel("Amplitude")
ax1.set_ylim(-1.1, 1.1)
ax1.grid(True, linestyle=':', alpha=0.5)
ref_waveform_plot, = ax1.plot([], [], 'b-', label='Reference (27 MHz)', alpha=0.8)
vco_waveform_plot, = ax1.plot([], [], 'r-', label='VCO Output', alpha=0.8)
time_marker_ax1 = ax1.axvline(x=0, color='purple', linestyle='--', linewidth=2)
ax1.legend(loc='upper left')

# 2. Edge plot
ax2 = fig.add_subplot(3, 1, 2)
ax2.set_title("2. Clock Edges (Blue=Reference, Red=VCO Output)")
ax2.set_xlabel("Time (ns)")
ax2.set_yticks([0, 1], ['VCO Edges', 'Ref Edges'])
ax2.set_ylim(-0.5, 1.5)
ax2.grid(True, axis='x', linestyle=':', alpha=0.5)
ref_edge_plot, = ax2.plot([], [], 'bo', label='Reference Edge (27 MHz)')
out_edge_plot, = ax2.plot([], [], 'rx', label='VCO Edge')
time_marker_ax2 = ax2.axvline(x=0, color='purple', linestyle='--', linewidth=2)

# 3. PLL Internals plot
ax3 = fig.add_subplot(3, 1, 3)
ax3.set_title("3. PLL Internals")
ax3.set_xlabel("Time (ns)")
ax3.grid(True, linestyle=':', alpha=0.5)

# VCO Frequency plot on left y-axis
ax3.set_ylabel("VCO Frequency (MHz)", color='g')
ax3.tick_params(axis='y', labelcolor='g')
vco_freq_plot, = ax3.plot([], [], 'g-', label='VCO Frequency')
ax3.axhline(y=F_OUT_TARGET / 1e6, color='g', linestyle='--', label='Target Freq (84 MHz)')
ax3.set_ylim(F_VCO_CENTER / 1e6 - 5, F_OUT_TARGET / 1e6 + 5)
ax3.legend(loc='upper left')

# Phase Error plot on right y-axis
ax3b = ax3.twinx()
ax3b.set_ylabel("Phase Error", color='m')
ax3b.tick_params(axis='y', labelcolor='m')
phase_error_plot, = ax3b.plot([], [], 'm.', markersize=4, label='Phase Error')
ax3b.axhline(y=0, color='m', linestyle='--')
ax3b.set_ylim(-1, 1)
ax3b.legend(loc='upper right')

def init():
    """Initialize the animation plots."""
    ref_waveform_plot.set_data([], [])
    vco_waveform_plot.set_data([], [])
    ref_edge_plot.set_data([], [])
    out_edge_plot.set_data([], [])
    vco_freq_plot.set_data([], [])
    phase_error_plot.set_data([], [])
    time_marker_ax1.set_xdata([0])
    time_marker_ax2.set_xdata([0])
    return ref_waveform_plot, vco_waveform_plot, ref_edge_plot, out_edge_plot, vco_freq_plot, phase_error_plot, time_marker_ax1, time_marker_ax2

def animate(frame_time):
    """
    This function is called for each frame of the animation.
    It simulates one time step of the PLL.
    """
    global state

    # --- 1. Advance Time and Generate Edges ---
    dt = TIME_STEP_NS * 1e-9
    state.time += dt
    
    state.ref_phase_accumulator += F_REF * dt
    state.vco_phase_accumulator += state.vco_freq * dt
    
    if state.ref_phase_accumulator >= 1.0:
        state.ref_phase_accumulator -= 1.0
        state.history['ref_edges'].append(state.time * 1e9)
        
        # --- 2. PFD and Loop Filter ---
        state.phase_error = state.feedback_divider_accumulator
        state.history['phase_error'].append(state.phase_error)

        state.integrated_error += state.phase_error * KI
        control_signal = (state.phase_error * KP) + state.integrated_error
        
        # --- 3. Update VCO ---
        state.vco_freq = F_VCO_CENTER + (VCO_SENSITIVITY * control_signal)

    if state.vco_phase_accumulator >= 1.0:
        state.vco_phase_accumulator -= 1.0
        state.history['out_edges'].append(state.time * 1e9)
        state.feedback_divider_accumulator += float(RATIO_DEN) / float(RATIO_NUM)
        if state.feedback_divider_accumulator >= 1.0:
            state.feedback_divider_accumulator -= 1.0

    state.history['time'].append(state.time * 1e9)
    state.history['vco_freq'].append(state.vco_freq)

    # --- 4. Update Plots ---
    current_time_ns = state.time * 1e9
    window_size_ns = 200
    plot_start_ns = max(0, current_time_ns - window_size_ns)

    # Update Waveform plot
    t_plot = np.linspace(plot_start_ns, current_time_ns + 50, 1500)
    ref_y = np.sin(2 * np.pi * F_REF * t_plot * 1e-9)
    # This is an approximation for visualization, using the current VCO frequency for the whole window
    vco_y = np.sin(2 * np.pi * state.vco_freq * t_plot * 1e-9)
    ref_waveform_plot.set_data(t_plot, ref_y)
    vco_waveform_plot.set_data(t_plot, vco_y)
    time_marker_ax1.set_xdata([current_time_ns])
    ax1.set_xlim(plot_start_ns, current_time_ns + 50)
    
    # Update Edges plot
    ref_edge_plot.set_data(state.history['ref_edges'], np.ones_like(state.history['ref_edges']))
    out_edge_plot.set_data(state.history['out_edges'], np.zeros_like(state.history['out_edges']))
    time_marker_ax2.set_xdata([current_time_ns])
    ax2.set_xlim(plot_start_ns, current_time_ns + 50)

    # Update Internals plot
    error_plot_times = state.history['ref_edges']
    phase_error_plot.set_data(error_plot_times, state.history['phase_error'])
    vco_freq_plot.set_data(state.history['time'], np.array(state.history['vco_freq']) / 1e6)
    ax3.set_xlim(plot_start_ns, current_time_ns + 50)

    return ref_waveform_plot, vco_waveform_plot, ref_edge_plot, out_edge_plot, vco_freq_plot, phase_error_plot, time_marker_ax1, time_marker_ax2


# --- Run Animation ---
ani = animation.FuncAnimation(
    fig, 
    animate, 
    frames=np.arange(0, SIM_TIME_NS, TIME_STEP_NS),
    init_func=init, 
    blit=False, # blit=False is required for dynamically changing x-axis limits
    interval=5
)

plt.tight_layout(rect=[0, 0, 1, 0.96])
plt.show()
