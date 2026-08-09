#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
mkdocs build --config-file ../mkdocs.yml
