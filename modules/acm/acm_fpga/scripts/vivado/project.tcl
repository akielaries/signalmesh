# create a vivado project (.xpr) for browsing in the gui. sources are referenced
# in place, not copied, so edits in the repo show up in the project
#   vivado -mode batch -source project.tcl -tclargs <board dir> <out dir>

set BOARD_DIR [file normalize [lindex $argv 0]]
set OUT_DIR   [file normalize [lindex $argv 1]]
source [file join [file dirname [info script]] common.tcl]

set name acm_[file tail $BOARD_DIR]
create_project -force $name [file join $OUT_DIR proj] -part $PART

add_files -norecurse $RTL_FILES
add_files -fileset constrs_1 -norecurse $XDC_FILES
set_property top $TOP [current_fileset]
update_compile_order -fileset sources_1

puts "RESULT: project at [file join $OUT_DIR proj $name.xpr]"
