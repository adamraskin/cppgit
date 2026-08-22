#!/usr/bin/env bash
set -euo pipefail

CPPGIT="${1:-./build/cppgit}"
CPPGIT="$(cd "$(dirname "$CPPGIT")" && pwd)/$(basename "$CPPGIT")"
ROOT="$(mktemp -d)"
trap 'rm -rf "$ROOT"' EXIT
SRC="$ROOT/src"
DST="$ROOT/dst"
PACK="$ROOT/test.pack"
mkdir -p "$SRC" "$DST"

cd "$SRC"
git init -q
git config user.name "cppgit test"
git config user.email "cppgit@example.com"
python3 - <<'PY'
from pathlib import Path
base = "\n".join(f"common line {i}" for i in range(3000)) + "\n"
for n in range(12):
    Path(f"file{n}.txt").write_text(base + (f"variation {n}\n" * 100))
PY
git add .
git commit -qm initial
for n in $(seq 1 8); do
  echo "change $n" >> file0.txt
  git add file0.txt
  git commit -qm "change $n"
done

git rev-list --objects --all | awk '{print $1}' | git pack-objects --stdout > "$PACK"

cd "$DST"
"$CPPGIT" init . >/dev/null
"$CPPGIT" unpack-pack "$PACK" >/dev/null

cd "$SRC"
while read -r oid; do
  native_type="$(git cat-file -t "$oid")"
  cpp_type="$(cd "$DST" && "$CPPGIT" cat-file -t "$oid")"
  test "$native_type" = "$cpp_type"
done < <(git rev-list --objects --all | awk '{print $1}' | sort -u)

echo "pack interoperability test passed"
