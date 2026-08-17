#!/usr/bin/env sh
set -eu

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <macos|windows> <package.dmg|package.zip|extracted-root>" >&2
  exit 2
fi

platform=$1
artifact=$2
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
version=$(sed -n 's/^project(ZeusTools VERSION \([^ ]*\).*/\1/p' "$project_dir/CMakeLists.txt")
macos_minimum=$(sed -n 's/^set(ZEUS_MACOS_MIN_VERSION "\([^"]*\)".*/\1/p' "$project_dir/CMakeLists.txt")
if [ -z "$version" ]; then
  echo "Unable to read project version" >&2
  exit 1
fi

if [ -f "$artifact" ]; then
  if [ ! -f "$artifact.sha256" ]; then
    echo "Missing SHA-256 checksum: $artifact.sha256" >&2
    exit 1
  fi
  expected_hash=$(sed -n '1s/[[:space:]].*$//p' "$artifact.sha256")
  if command -v sha256sum >/dev/null 2>&1; then
    actual_hash=$(sha256sum "$artifact" | sed 's/[[:space:]].*$//')
  elif command -v shasum >/dev/null 2>&1; then
    actual_hash=$(shasum -a 256 "$artifact" | sed 's/[[:space:]].*$//')
  else
    echo "No SHA-256 tool is available" >&2
    exit 1
  fi
  if [ -z "$expected_hash" ] || [ "$actual_hash" != "$expected_hash" ]; then
    echo "SHA-256 mismatch: $artifact" >&2
    exit 1
  fi
fi

temporary_dir=
mounted=0
cleanup() {
  if [ "$mounted" -eq 1 ]; then
    hdiutil detach "$temporary_dir" >/dev/null 2>&1 || true
  fi
  if [ -n "$temporary_dir" ] && [ -d "$temporary_dir" ]; then
    rm -rf "$temporary_dir"
  fi
}
trap cleanup EXIT HUP INT TERM

if [ -d "$artifact" ]; then
  package_root=$(CDPATH= cd -- "$artifact" && pwd)
elif [ "$platform" = windows ]; then
  temporary_dir=$(mktemp -d "${TMPDIR:-/tmp}/zeus-package.XXXXXX")
  unzip -q "$artifact" -d "$temporary_dir"
  package_root=$(find "$temporary_dir" -type f -name ZeusTools.exe -print | sed -n '1p')
  if [ -z "$package_root" ]; then
    echo "Windows archive does not contain ZeusTools.exe" >&2
    exit 1
  fi
  package_root=$(dirname "$package_root")
elif [ "$platform" = macos ]; then
  if [ -z "$macos_minimum" ]; then
    echo "Unable to read the minimum macOS version" >&2
    exit 1
  fi
  command -v hdiutil >/dev/null 2>&1 || {
    echo "DMG validation requires macOS hdiutil" >&2
    exit 1
  }
  temporary_dir=$(mktemp -d "${TMPDIR:-/tmp}/zeus-package.XXXXXX")
  hdiutil attach -readonly -nobrowse -mountpoint "$temporary_dir" "$artifact" >/dev/null
  mounted=1
  package_root=$temporary_dir
else
  echo "Unsupported platform: $platform" >&2
  exit 2
fi

cmake \
  -DZEUS_PACKAGE_ROOT="$package_root" \
  -DZEUS_PACKAGE_PLATFORM="$platform" \
  -DZEUS_PACKAGE_VERSION="$version" \
  -DZEUS_MACOS_MIN_VERSION="$macos_minimum" \
  -P "$project_dir/cmake/validate_installed_package.cmake"
