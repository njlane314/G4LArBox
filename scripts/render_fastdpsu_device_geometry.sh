#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root_image_tag="${ROOT_IMAGE_TAG:-rootproject/root:6.30.06-ubuntu22.04}"
root_platform="${ROOT_PLATFORM:-linux/amd64}"
gdml_rel="${FASTDPSU_DEVICE_GDML:-gdml/fastdpsu_device_concept.gdml}"
out_prefix="${OUTPUT_PREFIX:-data/fastdpsu_device_renders/fastdpsu_device}"
render_width="${RENDER_WIDTH:-1200}"
render_height="${RENDER_HEIGHT:-850}"

python3 "$repo_root/scripts/make_fastdpsu_device_gdml.py" \
  --output "$repo_root/$gdml_rel"

docker_args=(run --rm)
if [ -n "$root_platform" ]; then
  docker_args+=(--platform "$root_platform")
fi
docker_args+=(
  -v "$repo_root:/work"
  -w /work
  "$root_image_tag"
  root -b -q
  "root_macros/render_fastdpsu_device_geometry.C(\"$gdml_rel\",\"$out_prefix\",$render_width,$render_height)"
)

docker "${docker_args[@]}"
python3 "$repo_root/scripts/crop_png_to_content.py" "$repo_root"/data/fastdpsu_device_renders/*.png
