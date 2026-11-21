# shared setup for the vivado scripts
#   expects BOARD_DIR (absolute path to boards/<target>) set by the caller
#   sets ROOT, and PART / TOP / XDC / SRCS from the board's board.tcl

set ROOT [file normalize [file join [file dirname [info script]] .. ..]]

if {![info exists BOARD_DIR]} {
  error "BOARD_DIR not set (pass -tclargs <board dir> <out dir>)"
}
source [file join $BOARD_DIR board.tcl]

# shared rtl from rtl.f, skipping comments and blank lines
proc read_filelist {path root} {
  set out {}
  set fh [open $path r]
  foreach line [split [read $fh] "\n"] {
    set line [string trim $line]
    if {$line eq "" || [string index $line 0] eq "#"} {
      continue
    }
    lappend out [file join $root $line]
  }
  close $fh
  return $out
}

set RTL_FILES [read_filelist [file join $ROOT rtl.f] $ROOT]
foreach f $SRCS {
  lappend RTL_FILES [file join $BOARD_DIR $f]
}

set XDC_FILES {}
foreach f $XDC {
  lappend XDC_FILES [file join $BOARD_DIR $f]
}
