#!/bin/bash

# SocialChoiceLab Linting and Formatting Script
# Usage: ./lint.sh [--strict] [format|lint|both] [file_or_directory]
#   --strict  In lint mode, exit with failure if cpplint reports errors (for CI).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

STRICT_MODE=0
ACTION=""
TARGET=""

# Parse optional --strict and positional args
for arg in "$@"; do
    if [ "$arg" = "--strict" ]; then
        STRICT_MODE=1
    elif [ -z "$ACTION" ]; then
        ACTION="$arg"
    elif [ -z "$TARGET" ]; then
        TARGET="$arg"
        break
    fi
done

# Default action if not provided
if [ -z "$ACTION" ]; then
    ACTION="both"
fi
if [ -z "$TARGET" ] || [ "$TARGET" = "--strict" ]; then
    TARGET="."
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if tools are available
check_tools() {
    if ! command -v clang-format &> /dev/null; then
        echo_error "clang-format not found. Install with: brew install clang-format"
        exit 1
    fi
    
    if ! command -v cpplint &> /dev/null; then
        echo_error "cpplint not found. Install with: pip3 install cpplint"
        exit 1
    fi
}

# Format code
format_code() {
    echo_info "Formatting C++ code with clang-format..."
    
    if [ -f "$TARGET" ]; then
        # Single file
        clang-format -i "$TARGET"
        echo_info "Formatted: $TARGET"
    else
        # Default: only our code (include, src, tests/unit). Otherwise use TARGET.
        SEARCH_DIRS="$TARGET"
        if [ "$TARGET" = "." ]; then
            SEARCH_DIRS="include src tests/unit"
        fi
        # Directory or pattern (parentheses required for correct -o precedence)
        find $SEARCH_DIRS \( -name "*.h" -o -name "*.cpp" \) 2>/dev/null | while read -r file; do
            clang-format -i "$file"
            echo_info "Formatted: $file"
        done
    fi
}

# Lint code
lint_code() {
    echo_info "Linting C++ code with cpplint..."
    
    if [ -f "$TARGET" ]; then
        # Single file
        cpplint --root=include "$TARGET"
    else
        # Default: only our code (include, src, tests/unit). Otherwise use TARGET.
        SEARCH_DIRS="$TARGET"
        if [ "$TARGET" = "." ]; then
            SEARCH_DIRS="include src tests/unit"
        fi
        # Directory or pattern (parentheses required for correct -o precedence)
        while IFS= read -r file; do
            echo_info "Linting: $file"
            if [ "$STRICT_MODE" -eq 1 ]; then
                cpplint --root=include "$file"
            else
                cpplint --root=include "$file" || true
            fi
        done < <(find $SEARCH_DIRS \( -name "*.h" -o -name "*.cpp" \) 2>/dev/null)
    fi
}

# Main execution
main() {
    check_tools
    
    case "$ACTION" in
        "format")
            format_code
            ;;
        "lint")
            lint_code
            ;;
        "both")
            format_code
            echo ""
            lint_code
            ;;
        *)
            echo_error "Unknown action: $ACTION"
            echo "Usage: $0 [--strict] [format|lint|both] [file_or_directory]"
            exit 1
            ;;
    esac
}

main "$@"
