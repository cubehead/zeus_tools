#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_png="$project_dir/resources/icons/zeus-tools-master-transparent.png"
icon_dir="$project_dir/resources/icons"
iconset_dir="$icon_dir/zeus-tools.iconset"

command -v sips >/dev/null 2>&1 || {
  echo "sips is required to regenerate platform icons on macOS" >&2
  exit 1
}

mkdir -p "$iconset_dir"
for size in 16 32 128 256 512; do
  double_size=$((size * 2))
  sips -z "$size" "$size" "$source_png" --out "$iconset_dir/icon_${size}x${size}.png" >/dev/null
  sips -z "$double_size" "$double_size" "$source_png" --out "$iconset_dir/icon_${size}x${size}@2x.png" >/dev/null
done

sips -z 1024 1024 "$source_png" --out "$icon_dir/zeus-tools-1024.png" >/dev/null

if command -v iconutil >/dev/null 2>&1; then
  iconutil -c icns "$iconset_dir" -o "$icon_dir/zeus-tools.icns"
fi

sips -s format ico "$iconset_dir/icon_256x256.png" --out "$icon_dir/zeus-tools.ico" >/dev/null

echo "Generated Zeus Tools application icons in $icon_dir"
