#!/bin/sh
printf "[+] Formatting C and/or C++ files...\n"
find . -regex '.*\.\(c\|cpp\|hpp\|h\|cxx\)' -exec clang-format -style=file -i {} \;
