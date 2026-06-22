#!/bin/sh
# Create gitignored local config from example (safe to re-run; won't overwrite).
set -e
cd "$(dirname "$0")/.."
if [ ! -f main/camera_build_config.h ]; then
	cp main/camera_build_config.example.h main/camera_build_config.h
	echo "Created main/camera_build_config.h — edit credentials before build."
else
	echo "main/camera_build_config.h already exists."
fi
