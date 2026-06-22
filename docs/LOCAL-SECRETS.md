# Local secrets and sharing this repo

## Files that stay local (never commit)

| File | Purpose |
|------|---------|
| `main/camera_build_config.h` | Wi-Fi/HaLow credentials, static IP, camera title |

## First-time setup

```bash
cp main/camera_build_config.example.h main/camera_build_config.h
# Edit credentials and CAMERA_TITLE
idf.py build flash monitor
```

Or run: `sh scripts/setup-local-config.sh`

## Sharing with another person

They clone the repo, copy the example config, and set their own mesh credentials.
Your `main/camera_build_config.h` is not pushed.

## Past commits

Older commits may still contain real passwords in `camera_build_config.h`. Rotate
credentials if needed; use `git filter-repo` before a public release.
