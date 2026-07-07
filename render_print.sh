#!/usr/bin/env bash
# render_print.sh — generate 80-column UTF-8 plain-text renderings of the
# markdown docs for hardcopy on the Panasonic KX-P2123 (80-col, UTF-8).
#
# The .md SOURCE is kept freeform (written for on-screen / rendered reading
# and small git diffs); THIS script produces the print artifacts. Run it
# before printing, or after editing docs:
#     ./render_print.sh
#
# Output: docs/doc_txt/<doc>.txt  (git-ignored; regenerate on demand). pandoc
# strips markdown markup, renders tables as aligned ASCII, keeps code
# blocks verbatim, and wraps prose to 80 cols. Wide tables can still run a
# few columns over 80 (pandoc can't shrink a column below its widest cell);
# such lines are flagged below so they are never a silent surprise.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p docs/doc_txt
shopt -s nullglob

printf '%-46s %6s  %s\n' "print file" "lines" "width"
for md in README.md docs/*.md; do
   base=$(basename "${md%.md}")
   out="docs/doc_txt/${base}.txt"
   # -f gfm: parse GitHub-flavored markdown (pipe tables); no smart quotes,
   # so straight ' " survive to the printer unchanged.
   pandoc "$md" -f gfm -t plain --columns=80 --wrap=auto -o "$out"
   maxw=$(wc -L < "$out")               # max DISPLAY width (locale-aware)
   flag=$([ "$maxw" -gt 80 ] && echo "!! ${maxw} (wide table)" || echo "ok")
   printf '%-46s %6d  %s\n' "$out" "$(wc -l < "$out")" "$flag"
done
echo "docs/doc_txt/ regenerated for the KX-P2123."
