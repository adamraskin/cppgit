# cppgit

A Git-compatible version control system written from scratch in C++20.

The project is inspired by the Git resources collected in Codecrafters' **Build Your Own X** repository, but the implementation is original C++ and focuses on Git's real storage and transport formats: content-addressed objects, zlib compression, the v2 index, trees, commits, refs, packfiles, delta objects, pkt-line framing, and Git smart HTTP.

## Features

- `init` — create a Git-compatible repository layout
- `hash-object [-w]` — compute and optionally store Git blob objects
- `cat-file -p|-t|-s` — inspect objects
- `add` — stage files into a Git index v2 file
- `write-tree` / `ls-tree` — serialize and inspect trees
- `commit-tree` / `commit -m` — create commits
- `log` — traverse commit history
- `branch` — list and create branches
- `checkout` — switch branches and materialize trees
- `status` — detect tracked working-tree modifications
- `diff` — basic line-oriented working-tree diff
- `unpack-pack` — parse a Git PACK stream into loose objects
- `fetch <url> [ref]` — discover a remote over smart HTTP, request a pack, resolve deltas, and update a ref
- `clone <url> [directory]` — perform a read-only smart-HTTP clone and check out the remote HEAD

## What the implementation covers

### Object database

Git object IDs are computed exactly as Git does:

```text
SHA1("<type> <size>\0" + payload)
```

Loose objects are zlib-compressed into `.git/objects/aa/...`.

### Packfiles

`cppgit` parses PACK v2/v3 streams, validates the trailing SHA-1 checksum, inflates packed objects, and resolves both Git delta formats:

- `OFS_DELTA` — base object addressed by backward pack offset
- `REF_DELTA` — base object addressed by SHA-1

Resolved objects are materialized into the normal loose-object database, so the rest of `cppgit` does not need a second read path.

### Smart HTTP

Remote discovery uses:

```text
GET <remote>/info/refs?service=git-upload-pack
```

`cppgit` parses pkt-lines and advertised refs/capabilities, including `symref=HEAD:...`, then requests the desired commit from `git-upload-pack` using side-band transport and extracts the returned PACK stream.

The network layer intentionally targets public HTTPS smart-HTTP repositories. Authentication, protocol v2, SSH transport, shallow clones, and negotiation against existing local history are not implemented yet.

## Architecture

```text
                              ┌──────────────────┐
Remote Git server ── HTTP ──► │ pkt-line parser  │
                              └────────┬─────────┘
                                       │
                                       ▼
                              ┌──────────────────┐
                              │ PACK + deltas    │
                              └────────┬─────────┘
                                       │ resolved objects
                                       ▼
Working tree             .git/objects (loose object DB)
    │                              │
    ├── Index (DIRC v2) ───────────┤
    │                              ▼
    │                         Tree objects
    │                              │
    ▼                              ▼
Blob objects                    Commits
                                   │
                                   ▼
                            refs/heads/* + HEAD
```

## Build

Requirements:

- C++20 compiler
- CMake 3.20+
- OpenSSL
- zlib
- libcurl
- pkg-config on Unix-like systems

Ubuntu/Debian:

```bash
sudo apt install build-essential cmake pkg-config libssl-dev zlib1g-dev libcurl4-openssl-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows with Visual Studio + vcpkg:

```powershell
vcpkg install openssl:x64-windows zlib:x64-windows curl:x64-windows pkgconf:x64-windows
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Example

Create local history:

```bash
mkdir demo && cd demo
/path/to/cppgit init

echo "hello" > hello.txt
/path/to/cppgit add hello.txt
/path/to/cppgit commit -m "initial commit"
/path/to/cppgit log
```

Clone a public smart-HTTP Git remote:

```bash
cppgit clone https://github.com/octocat/Hello-World.git
cd Hello-World
cppgit log
```

Fetch a branch into an existing repository:

```bash
cppgit fetch https://github.com/octocat/Hello-World.git master
```

## Git interoperability

The project deliberately emits real Git formats. Native Git can inspect repositories and objects produced by `cppgit`:

```bash
git cat-file -p HEAD
git ls-tree HEAD
git status
```

For packfile compatibility, the integration test creates history with native Git, asks Git to generate a deltified PACK stream, unpacks it through `cppgit`, and verifies every resulting object against native Git:

```bash
./tests/pack_interop.sh ./build/cppgit
```

The test corpus currently exercises non-delta objects plus delta chains of multiple levels.

## Current limitations

This is an educational implementation, not a production Git replacement. Important omissions include:

- push / receive-pack
- SSH and `git://` transports
- HTTP authentication / credential helpers
- Git protocol v2
- shallow and partial clones
- efficient fetch negotiation against existing history
- `.idx` pack-index reading and direct packed-object lookup
- merge and merge-base
- rename detection
- submodules
- reflogs
- advanced index extensions
- full config parsing
- production-grade checkout conflict handling
- Git's full Myers/patience diff behavior

## Background

The project idea comes from the Git section of `codecrafters-io/build-your-own-x`, which links guides such as *Write yourself a Git!*, *ugit*, *Gitlet*, and other bottom-up Git implementations. Those resources are learning references; this codebase implements the concepts independently in C++20.
