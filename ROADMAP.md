# Roadmap

## Phase 1 — Git object database ✅
- Repository initialization
- SHA-1 object IDs
- zlib loose-object storage
- Blob read/write
- Abbreviated object resolution

## Phase 2 — Staging and snapshots ✅
- Git index v2 serialization/parsing
- `add`
- Recursive tree generation
- `write-tree`
- `ls-tree`

## Phase 3 — History and refs ✅
- Commit object generation
- Parent links
- `commit`
- `log`
- Symbolic `HEAD`
- Branch creation/listing
- Branch checkout

## Phase 4 — Working tree inspection ✅ / partial
- Modified-file detection
- Basic line-oriented diff
- Next: untracked files, staged-vs-HEAD reporting, staged deletions

## Phase 5 — Packfiles ✅
- PACK v2/v3 header parsing
- Pack checksum validation
- zlib stream boundary handling
- Normal packed objects
- `OFS_DELTA`
- `REF_DELTA`
- Delta instruction interpreter
- Multi-level delta resolution
- Materialization into loose-object storage
- Native Git pack interoperability test

## Phase 6 — Networking ✅ / partial
- pkt-line framing/parsing
- Smart HTTP ref discovery
- advertised capability parsing
- remote HEAD `symref` resolution
- upload-pack request
- side-band PACK extraction
- public-HTTPS `fetch`
- read-only `clone`
- checkout and index creation after clone

Next networking work:
- protocol v2
- fetch negotiation using local `have` commits
- authentication
- shallow clone
- remote-tracking refs for all advertised branches

## Phase 7 — Advanced Git
- Myers diff
- commit-ish parser (`HEAD~2`, tags, abbreviated refs)
- merge-base
- three-way merge
- conflict markers and index stages
- reflog

## Phase 8 — Native packed-object database
- `.idx` pack-index parser
- direct packed-object lookup without expanding to loose objects
- pack fan-out table
- object CRC validation
- optional pack writing / repacking

The strongest next resume milestone after clone is **merge-base + three-way merge** or **native `.idx`-backed packed-object lookup**. Either adds another substantial Git subsystem rather than simply expanding command count.
