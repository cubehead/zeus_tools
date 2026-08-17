#!/usr/bin/env sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"

required_files="
README.md
README_zh-CN.md
LICENSE
PRIVACY.md
SECURITY.md
CONTRIBUTING.md
CHANGELOG.md
THIRD_PARTY_NOTICES.md
docs/adr/001-ui-framework.md
docs/adr/002-processing-registry.md
docs/README.md
docs/development.md
docs/implementation-plan.md
docs/packaging.md
docs/product-requirements.md
docs/release-checklist.md
docs/releases/v0.1.0.md
docs/releases/v0.2.0.md
docs/assets/overview-dark.jpg
docs/assets/csv-table.jpg
docs/assets/digest-hmac.jpg
docs/assets/jwt-inspect.jpg
docs/licenses/EUI-NEO-APACHE-2.0.txt
docs/licenses/FONT-AWESOME-FREE.txt
docs/licenses/FREETYPE.txt
docs/licenses/GLFW.md
docs/licenses/LIBPNG.txt
docs/licenses/MD4C-MIT.md
docs/licenses/PUGIXML-MIT.md
docs/licenses/TRAY-MIT.txt
docs/licenses/YAML-CPP-MIT.txt
docs/licenses/YYJSON-MIT.txt
docs/licenses/ZLIB.txt
docs/licenses/tomlplusplus.txt
cmake/validate_installed_package.cmake
scripts/check-package.sh
scripts/check-package.ps1
"

for file in $required_files; do
  if [ ! -s "$file" ]; then
    echo "Missing or empty required file: $file" >&2
    exit 1
  fi
done

for revision in \
  f2a3b72104bd946988f8ebe0a13dda956f3455ae \
  ee86beb30e4973f5feffe3ce63bfa4fbadf72f38 \
  56e3bb550c91fd7005566f19c079cb7a503223cf; do
  if ! rg -q "$revision" CMakeLists.txt || ! rg -q "$revision" THIRD_PARTY_NOTICES.md; then
    echo "Dependency revision is missing from CMake or third-party notices: $revision" >&2
    exit 1
  fi
done

for local_path in \
  build/.zeus-ignore-probe \
  .codegraph/.zeus-ignore-probe \
  .cursor/.zeus-ignore-probe \
  resources/icons/zeus-tools.iconset/.zeus-ignore-probe; do
  if ! git check-ignore -q "$local_path"; then
    echo "Local/generated path is not ignored: $local_path" >&2
    exit 1
  fi
done

if [ -d .github/workflows ]; then
  echo "Unexpected .github/workflows directory: this repository currently uses manual checks." >&2
  exit 1
fi

if rg -n --hidden --glob '!build/**' --glob '!.git/**' \
  '(BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY|AKIA[0-9A-Z]{16})' .; then
  echo "Possible credential material detected." >&2
  exit 1
fi

echo "Repository publication checks passed."
