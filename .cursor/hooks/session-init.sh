#!/bin/bash
# Session init hook: injects model-awareness context into every conversation.
# Runs once at session start via the sessionStart hook event.
# From https://github.com/coyvalyss1/model-matchmaker (project-level install)

cat > /dev/null

cat << 'EOF'
{
 "additional_context": "Model guidance: Haiku is ideal for git ops, renames, formatting, and simple edits. Sonnet is the default for feature work, debugging, and planning. Opus is for architecture decisions, deep analysis, and multi-system reasoning. If you notice the current task is simpler than the model being used, briefly mention it."
}
EOF

exit 0
