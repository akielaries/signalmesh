#!/usr/bin/env bash
# render every .dot diagram in this directory to SVG + PNG.
# needs graphviz (the `dot` binary).  usage: ./render.sh
set -euo pipefail
cd "$(dirname "$0")"

for f in *.dot; do
  name="${f%.dot}"
  echo "rendering ${name}"
  dot -Tsvg "${f}" -o "${name}.svg"
  dot -Tpng -Gdpi=150 "${f}" -o "${name}.png"
done

echo "done -> $(ls *.svg | tr '\n' ' ')"
