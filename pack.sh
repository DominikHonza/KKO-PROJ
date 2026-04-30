#!/usr/bin/env bash

set -euo pipefail

ARCHIVE_NAME="${1:-xhonza04.zip}"

zip -r "$ARCHIVE_NAME" \
    include \
    src \
    Makefile \
    README.pdf

echo "Vytvoren archiv: $ARCHIVE_NAME"
