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

# Add header if not already present
for ext in "${EXTENSIONS[@]}"; do
  find . -type f -name "*.${ext}" | while read -r file; do
    if ! grep -q "@file" "$file"; then
      echo "Adding header to $file"
      tmp=$(mktemp)
      echo "$DOXYGEN_HEADER" > "$tmp"
      echo "" >> "$tmp"
      cat "$file" >> "$tmp"
      mv "$tmp" "$file"
    else
      echo "Header already exists in $file, skipping."
    fi
  done
done

