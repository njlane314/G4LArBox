#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

dunecore_dir="${DUNECORE_DIR:-$repo_root/extern/dunecore}"
dunecore_repo="${DUNECORE_REPO:-https://github.com/DUNE/dunecore.git}"
dunecore_ref="${DUNECORE_REF:-}"
official_gdml_rel="${DUNE_VD_GDML:-dunecore/Geometry/gdml/dunevd10kt_3view_30deg_v7_refactored_2x8x40_nowires.gdml}"
out_prefix="${OUTPUT_PREFIX:-data/dunevd_arapuca_renders/dunevd_arapuca}"
root_image_tag="${ROOT_IMAGE_TAG:-rootproject/root:6.30.06-ubuntu22.04}"
root_platform="${ROOT_PLATFORM:-linux/amd64}"
render_width="${RENDER_WIDTH:-1900}"
render_height="${RENDER_HEIGHT:-720}"

if [ ! -d "$dunecore_dir/.git" ]; then
  mkdir -p "$(dirname "$dunecore_dir")"
  git clone --filter=blob:none --sparse "$dunecore_repo" "$dunecore_dir"
  git -C "$dunecore_dir" sparse-checkout set dunecore/Geometry/gdml
fi

if [ -n "$dunecore_ref" ]; then
  git -C "$dunecore_dir" fetch --tags origin
  git -C "$dunecore_dir" checkout "$dunecore_ref"
fi

official_gdml="$dunecore_dir/$official_gdml_rel"
if [ ! -f "$official_gdml" ]; then
  echo "Official GDML not found: $official_gdml" >&2
  exit 2
fi
official_gdml_container="$official_gdml"
case "$official_gdml" in
  "$repo_root"/*) official_gdml_container="${official_gdml#$repo_root/}" ;;
esac

mkdir -p "$repo_root/data/dunevd_arapuca_renders"

echo "Using official dunecore checkout:"
git -C "$dunecore_dir" log -1 --format='  commit %H%n  date   %ci%n  title  %s'
echo "Using official GDML: $official_gdml_rel"

docker_args=(run --rm)
if [ -n "$root_platform" ]; then
  docker_args+=(--platform "$root_platform")
fi
docker_args+=(
  -v "$repo_root:/work"
  -w /work
  "$root_image_tag"
  root -b -q
  "root_macros/render_dunevd_arapuca.C(\"$official_gdml_container\",\"$out_prefix\",$render_width,$render_height)"
)

docker "${docker_args[@]}"

python3 "$repo_root/scripts/crop_png_to_content.py" "$repo_root"/data/dunevd_arapuca_renders/*.png
