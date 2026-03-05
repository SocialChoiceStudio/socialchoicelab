#!/usr/bin/env bash
# Regenerate docs/status/where_we_are.md from docs/status/consensus_plan.md.
# Finds the first item not marked ✅ Done and updates the cached pointer.
# Run from repo root.

set -e

cd "$(dirname "$0")/.."

plan="docs/status/consensus_plan.md"
out="docs/status/where_we_are.md"

if [ ! -f "$plan" ]; then
  echo "Error: $plan not found."
  exit 1
fi

if [ ! -f "$out" ]; then
  echo "Error: $out not found."
  exit 1
fi

# Parse plan: find first row not marked Done. Output: PHASE|NUM|TITLE|SEVERITY
# When all actionable items are Done, output: All phases complete|—|See docs/status/roadmap.md|—
next=$(python3 - "$plan" << 'PY'
import re
import sys

with open(sys.argv[1]) as f:
    content = f.read()

def parse_batch_table(text):
    """Find table rows with #, Item, ..., Severity, Status. Item may contain '|'. Return list of (num, item, severity, status)."""
    rows = []
    for line in text.split("\n"):
        if not line.strip().startswith("|") or "---" in line:
            continue
        parts = [p.strip() for p in line.split("|")]
        # Remove leading/trailing empty from markdown table
        if not parts or not parts[0]:
            parts = parts[1:]
        if not parts or not parts[-1]:
            parts = parts[:-1]
        # Columns: # (1) | Item (2..n-4) | File | Origin | Severity | Status
        if len(parts) >= 6 and parts[0].isdigit():
            num = parts[0]
            item = "|".join(parts[1:-4]).strip()  # Item cell may contain |
            severity = parts[-2]
            status = parts[-1]
            rows.append((num, item, severity, status))
    return rows

# Split by ## Batch N — (captures "Batch 1", "Batch 2", ...); bodies are following elements
batch_sections = re.split(r"\n## (Batch \d+)", content)
for i in range(1, len(batch_sections) - 1, 2):
    current_batch = batch_sections[i]   # "Batch 1", "Batch 2", ...
    section = batch_sections[i + 1]     # body including table
    if "| # |" not in section:
        continue
    rows = parse_batch_table(section)
    for num, item, severity, status in rows:
        if "Done" not in status:
            title = (item[:60] + "…") if len(item) > 60 else item
            title = title.replace("|", " ")
            print(f"{current_batch}|{num}|{title}|{severity}")
            sys.exit(0)

# All actionable items are Done (or no Batch tables found)
print("All phases complete|—|See docs/status/roadmap.md|—")
sys.exit(0)
PY
)

phase=$(echo "$next" | cut -d'|' -f1)
num=$(echo "$next" | cut -d'|' -f2)
title=$(echo "$next" | cut -d'|' -f3)
severity=$(echo "$next" | cut -d'|' -f4)

today=$(date +%Y-%m-%d)

# Update only the pointer block at the top (lines 1-7); preserve the Recent Work section below
python3 - "$out" "$phase" "$num" "$title" "$severity" "$today" << 'PY'
import sys

out_path, phase, num, title, severity, today = sys.argv[1:]

with open(out_path) as f:
    content = f.read()

# Replace everything up to the first "---" separator
divider = "\n---\n"
idx = content.find(divider)
if idx == -1:
    print(f"Error: could not find '---' separator in {out_path}", file=sys.stderr)
    sys.exit(1)

rest = content[idx:]  # keeps the "---\n\n## Recent Work..." section

if phase == "All phases complete":
    next_line = "- **Next item:** All consensus items complete. See `docs/status/roadmap.md` for next steps."
else:
    next_line = f"- **Next item:** {num} — {title.rstrip('.')}. Severity: {severity}."

new_header = f"""# Where We Are

**Single source for "what's next" so any agent on any machine can answer correctly.**

- **Current phase:** {phase}
{next_line}
- **Last updated:** {today}

**Authority:** This file is a cached pointer. The **Status** column in `docs/status/consensus_plan.md` is the source of truth. When you complete an item: (1) mark it ✅ Done in consensus_plan.md, (2) update this file (next item = first row in CONSENSUS_PLAN not marked Done; update Last updated date). If this file and the plan disagree, the plan wins — fix this file.

**Rule for agents:** When the user asks "where are we" or "what's next", read this file and consensus_plan.md. Use the plan's Status column to confirm; if they disagree, update this file."""

with open(out_path, "w") as f:
    f.write(new_header + rest)

print(f"Updated {out_path}: Phase={phase}, Item={num}, Severity={severity}")
PY

echo "$phase|$num|$title|$severity"
