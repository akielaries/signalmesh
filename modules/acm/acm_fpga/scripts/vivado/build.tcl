# non-project vivado build: synth -> place -> route -> bitstream
#   vivado -mode batch -source build.tcl -tclargs <board dir> <out dir>
#
# outputs in <out dir>: top.bit, post_synth.dcp, post_route.dcp, reports/*.rpt

set BOARD_DIR [file normalize [lindex $argv 0]]
set OUT_DIR   [file normalize [lindex $argv 1]]
source [file join [file dirname [info script]] common.tcl]

file mkdir $OUT_DIR
file mkdir [file join $OUT_DIR reports]
set RPT [file join $OUT_DIR reports]

read_verilog $RTL_FILES
read_xdc $XDC_FILES

synth_design -top $TOP -part $PART
write_checkpoint -force [file join $OUT_DIR post_synth.dcp]
report_utilization -file [file join $RPT post_synth_util.rpt]

opt_design
place_design
phys_opt_design
route_design
write_checkpoint -force [file join $OUT_DIR post_route.dcp]

report_utilization       -file [file join $RPT util.rpt]
report_utilization -hierarchical -file [file join $RPT util_hier.rpt]
report_timing_summary    -file [file join $RPT timing.rpt]
report_clock_utilization -file [file join $RPT clocks.rpt]
report_io                -file [file join $RPT io.rpt]
report_drc               -file [file join $RPT drc.rpt]
report_power             -file [file join $RPT power.rpt]

write_bitstream -force [file join $OUT_DIR top.bit]

set wns [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]
puts "RESULT: board=[file tail $BOARD_DIR] part=$PART wns=$wns"
if {$wns ne "" && $wns < 0} {
  puts "RESULT: timing NOT met"
  exit 2
}
