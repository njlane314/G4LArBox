#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${OUTPUT_DIR:-data/background_uncertainty}"
seed="${SEED:-314159}"
events="${EVENTS:-2000}"
root_image="${ROOT_IMAGE_TAG:-rootproject/root}"
root_platform="${ROOT_PLATFORM:-linux/amd64}"
copy_to_thesis="${COPY_TO_THESIS:-1}"
thesis_figures_dir="${THESIS_FIGURES_DIR:-$repo_root/../thesis/figures}"

mkdir -p "$repo_root/$output_dir"

platform_args=()
if [ -n "$root_platform" ]; then
  platform_args=(--platform "$root_platform")
fi

docker run --rm \
  "${platform_args[@]}" \
  -v "$repo_root:/work" \
  -w /work \
  --entrypoint /bin/bash \
  "$root_image" \
  -lc "set -euo pipefail; root -l -b -q 'root_macros/dunevd_background_uncertainty_study.C(\"${output_dir}\",${seed},${events})'"

if [ "$copy_to_thesis" = "1" ] && [ -d "$thesis_figures_dir" ]; then
  cp "$repo_root/$output_dir"/dunevd_*.pdf "$repo_root/$output_dir"/dunevd_*.png "$thesis_figures_dir"/
  printf 'Copied figures to %s\n' "$thesis_figures_dir"
fi

printf 'Generated DUNE VD background-uncertainty artifacts in %s\n' "$repo_root/$output_dir"
