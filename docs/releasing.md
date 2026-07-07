# Releasing

Releases are driven by per-module `version.yml` files, via GitHub Actions. There
is no manual tagging - you bump a version and push.

## How it works

Each releasable component owns a `version.yml` (newest entry first):

```yaml
releases:
  - version: 1.1.0
    notes:
      - what changed
  - version: 1.0.0
    notes:
      - initial release
```

The top entry's `version`:
- feeds the firmware **boot banner** (`cmake/version.cmake` reads it, then appends
  the git short hash and a `-dirty` flag), e.g. `APM v1.1.0-9d089aa8-dirty`, and
- drives the **release**: on push to `main`, `release.yml` cuts a GitHub release
  tagged `<component>/v<version>` with that entry's notes as the body.

It is idempotent: if a release for the top version already exists (e.g. you only
edited an older entry's notes), the job skips. Bump the number to cut a release.

## Components

| component  | version.yml                  | tag             | asset(s)                     |
|------------|------------------------------|-----------------|------------------------------|
| apm        | `modules/apm/version.yml`    | `apm/v*`        | `.smup`                      |
| bootloader | `modules/bootloader/version.yml` | `bootloader/v*` | `.hex`, `.bin`           |
| acm (fpga) | `modules/acm/version.yml`    | `acm/v*`        | notes only (bitstream pending Gowin CI) |
| fw-tools   | `tools/fw_update/version.yml`| `tools/v*`      | per-platform `mkupdate-*` / `update-*` (see below) |

The bootloader release runs in the `bootloader-release` GitHub **environment** -
add required reviewers (repo Settings > Environments) so a golden-image release
needs manual approval. A check job gates it, so approval is only requested when
the bootloader version actually changed.

The demo app is built and smoke-tested in `ci.yml` but not released.

## Cut a release

1. Edit the component's `version.yml`: add a new top entry with the new version + notes.
2. Commit and push to `main`.
3. The release appears under the repo's **Releases** page with its assets attached.

## Host tools targets

`mkupdate` is portable; `update` uses Linux-only serial ioctls. The `tools`
release builds a matrix and attaches:

| asset | mkupdate | update |
|-------|----------|--------|
| `linux-x86_64`  | yes | yes |
| `linux-aarch64` | yes (cross) | yes (cross) |
| `macos-arm64`   | yes | no (Linux serial) |
| `windows-x86_64.exe` | yes | no |

Plus a `SHA256SUMS` file. Porting `update` to macOS/Windows means replacing the
`custom_baud.c` termios2 serial layer.

## Notes

- `ci.yml` builds every module on push/PR; `release.yml` only runs when a
  `version.yml` changes.
- Each `.smup` carries an image CRC and a `.sha256` published alongside.
- A `.smup` targets a bootloader speaking the `BLM1` image format - note the
  minimum bootloader version in the entry when the format changes.
- FPGA (acm) is not wired up yet - it needs a self-hosted x86-64 runner with the
  Gowin `gw_sh` toolchain, plus its own `version.yml`.
- `tools/ci/version.py` is the dependency-free reader CI uses to pull the top
  version/notes out of a `version.yml`.
