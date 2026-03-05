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
next=$(python3 - "$plan" << 'PY'
import re
import sys

with open(sys.argv[1]) as f:
    content = f.read()

# Phase display names (phase number + subtitle)
phase_names = {
    "Phase 0": "Phase 0 (Immediate Stabilization)",
    "Phase 1": "Phase 1 (Reproducibility, Safety, and Test Quality)",
    "Phase 2": "Phase 2 (Documentation Truthfulness)",
    "Phase 3": "Phase 3 (Developer Experience and Process)",
    "Backlog": "Backlog (Quality and Modernization)",
}

def parse_table(text, phase_name):
    """Find table rows, return (num, item, severity, status) for each."""
    lines = text.split("\n")
    rows = []
    for line in lines:
        if line.strip().startswith("|") and "---" not in line:
            parts = [p.strip() for p in line.split("|")]
            parts = [p for p in parts if p]
            if len(parts) >= 4 and parts[0].isdigit():
                num = parts[0]
                item = parts[1]
                severity = parts[3] if len(parts) > 3 else ""
                status = parts[4] if len(parts) > 4 else ""
                rows.append((num, item, severity, status))
    return rows

# Split by phase sections
sections = re.split(r"(## Phase \d+ / Week \d+:|## Backlog:)", content)
current_phase = None
for i, section in enumerate(sections):
    if re.match(r"## Phase \d+ / Week \d+:", section) or section == "## Backlog:":
        m = re.search(r"Phase \d+", section)
        current_phase = m.group(0) if m else ("Backlog" if "Backlog" in section else None)
        continue
    if current_phase and "|" in section and "| # |" in section:
        rows = parse_table(section, current_phase)
        for num, item, severity, status in rows:
            if "✅ Done" not in status:
                display = phase_names.get(current_phase, current_phase)
                print(f"{display}|{num}|{item}|{severity}")
                sys.exit(0)

print("Phase 0 (Immediate Stabilization)|0|No items found|Unknown")
sys.exit(1)
PY
)

if [ $? -ne 0 ] && [ -z "$next" ]; then
  echo "Error: Could not parse CONSENSUS_PLAN."
  exit 1
fi

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

new_header = f"""# Where We Are

**Single source for "what's next" so any agent on any machine can answer correctly.**

- **Current phase:** {phase}
- **Next item:** {num} — {title.rstrip('.')}. Severity: {severity}.
- **Last updated:** {today}

**Authority:** This file is a cached pointer. The **Status** column in `docs/status/consensus_plan.md` is the source of truth. When you complete an item: (1) mark it ✅ Done in consensus_plan.md, (2) update this file (next item = first row in CONSENSUS_PLAN not marked Done; update Last updated date). If this file and the plan disagree, the plan wins — fix this file.

**Rule for agents:** When the user asks "where are we" or "what's next", read this file and consensus_plan.md. Use the plan's Status column to confirm; if they disagree, update this file."""

with open(out_path, "w") as f:
    f.write(new_header + rest)

print(f"Updated {out_path}: Phase={phase}, Item={num}, Severity={severity}")
PY

echo "$phase|$num|$title|$severity"
