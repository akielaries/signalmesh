#!/bin/bash

# Doxygen header block (change as needed)
read -r -d '' DOXYGEN_HEADER <<'EOF'
/**
 * signalmesh 2025
 *
 * @file
 * @brief [Short description of the file/module]
 * @author Akiel Aries
 */
EOF

# File extensions to target

EXTENSIONS=("c" "v")

# Loop through each file extension
for ext in "${EXTENSIONS[@]}"; do
  find . -type f -name "*.${ext}" | while read -r file; do
    # Skip if file already contains @file (assumes Doxygen already added)
    if grep -q "@file" "$file"; then
      echo "[✓] Header already exists: $file"
    else
      echo "[+] Adding header to: $file"
      tmp=$(mktemp)
      echo "$DOXYGEN_HEADER" > "$tmp"
      echo "" >> "$tmp"
      cat "$file" >> "$tmp"
      mv "$tmp" "$file"
    fi
  done
done

