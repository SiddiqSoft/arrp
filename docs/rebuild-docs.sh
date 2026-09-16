#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Regenerating API documentation from Doxygen XML..."
python3 "$ROOT_DIR/scripts/generate_api_docs.py"

# Check for mkdocs installation (system PATH or project venv)
MKDOCS_CMD=""
if command -v mkdocs &> /dev/null; then
  MKDOCS_CMD="mkdocs"
elif [ -x "$ROOT_DIR/venv/bin/mkdocs" ]; then
  MKDOCS_CMD="$ROOT_DIR/venv/bin/mkdocs"
fi

if [ -z "$MKDOCS_CMD" ]; then
  echo "mkdocs is not installed; please install it (e.g., pip install mkdocs-material) to build the site."
  exit 1
fi

if [ "$1" == "serve" ]; then
    echo "Starting local live-reload MkDocs server..."
    cd "$ROOT_DIR"
    "$MKDOCS_CMD" serve
else
    echo "Building MkDocs Material site using $MKDOCS_CMD..."
    cd "$ROOT_DIR"
    "$MKDOCS_CMD" build --strict
fi
