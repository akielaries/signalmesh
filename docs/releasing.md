# Releasing

The monorepo cuts independent, per-module releases from git tags, via GitHub
Actions (`.github/workflows/`).

## What is released

| component  | tag prefix      | asset(s)                     | delivered by      |
|------------|-----------------|------------------------------|-------------------|
| apm        | `apm/v*`        | `.smup`                      | bootloader update |
| bootloader | `bootloader/v*` | `.hex`, `.bin`               | debugger flash    |
| fw-tools   | `tools/v*`      | `update.bin`, `mkupdate.bin` | host download     |
| acm (fpga) | `acm/v*`        | `.smup` (later - gowin CI)   | bootloader update |

The demo app is built and smoke-tested in `ci.yml` but not released (test payload).

## Cut a release

Tag with the component prefix and push the tag:

```
git tag apm/v1.2.0
git push origin apm/v1.2.0
```

That runs the matching `<component>-release.yml` workflow: build -> wrap into the
asset -> `gh release create`. The release and its assets appear under the repo's
**Releases** page.

The bootloader release runs in the `bootloader-release` GitHub **environment** - a
bad golden image bricks the board, so configure required reviewers on that
environment (repo Settings > Environments) to enforce manual approval.

## What a push does

- Push to `main` / a PR -> `ci.yml` builds all modules + host tools and
  smoke-wraps the demo image. No releases.
- Push a `<component>/v*` tag -> only that component's release workflow runs.

## Notes

- Each `.smup` carries an image CRC; a `.sha256` is published alongside.
- A `.smup` targets a bootloader speaking the `BLM1` image header - note the
  minimum bootloader version in release notes when the format changes.
- FPGA (acm) builds need a self-hosted x86-64 runner with the Gowin `gw_sh`
  toolchain; not wired up yet.
