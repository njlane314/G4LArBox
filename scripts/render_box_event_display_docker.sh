#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

sim_image_tag="${IMAGE_TAG:-${SIM_IMAGE_TAG:-g4larbox}}"
root_image_tag="${ROOT_IMAGE_TAG:-rootproject/root}"
root_platform="${ROOT_PLATFORM:-linux/amd64}"
detector_macro="${1:-simplebox.mac}"
generator_macro="${2:-thesis_bnb_like.mac}"
event_index="${EVENT_INDEX:-0}"
event_count="${EVENT_COUNT:-3}"
event_indices="${EVENT_INDICES:-}"
gdml_file="${GDML_FILE:-gdml/lar_box.gdml}"
default_output_prefix='data/event_displays/box_event{event}_gdml'
output_prefix="${OUTPUT_PREFIX:-$default_output_prefix}"
output_formats="${OUTPUT_FORMATS:-png,pdf}"
max_segments="${MAX_SEGMENTS:-60000}"
render_modes="${RENDER_MODES:-clean,debug}"
track_pdgs="${TRACK_PDGS:-}"
primary_only="${PRIMARY_ONLY:-0}"
min_step_cm="${MIN_STEP_CM:-0.005}"
max_tracks="${MAX_TRACKS:-2000}"
simulation_events="${SIM_EVENTS:-}"
synthetic_gst_events="${SYNTHETIC_GST_EVENTS:-}"
synthetic_corsika_events="${SYNTHETIC_CORSIKA_EVENTS:-}"
random_seed="${RANDOM_SEED:-}"
synthetic_gst_seed="${SYNTHETIC_GST_SEED:-}"
synthetic_corsika_seed="${SYNTHETIC_CORSIKA_SEED:-}"
make_synthetic_gst="${MAKE_SYNTHETIC_GST:-auto}"
make_synthetic_corsika="${MAKE_SYNTHETIC_CORSIKA:-auto}"
synthetic_gst_macro="${SYNTHETIC_GST_MACRO:-}"
synthetic_gst_output="${SYNTHETIC_GST_OUTPUT:-}"
synthetic_corsika_macro="${SYNTHETIC_CORSIKA_MACRO:-}"
synthetic_corsika_output="${SYNTHETIC_CORSIKA_OUTPUT:-}"
build_in_mount="${BUILD_IN_MOUNT:-1}"
build_jobs="${BUILD_JOBS:-2}"
skip_docker_build="${SKIP_DOCKER_BUILD:-auto}"

if [ -z "$event_indices" ]; then
  if [ "$event_count" -gt 1 ]; then
    event_indices=""
    for ((i = 0; i < event_count; ++i)); do
      if [ -n "$event_indices" ]; then
        event_indices+=","
      fi
      event_indices+="$i"
    done
  else
    event_indices="$event_index"
  fi
fi

IFS=',' read -r -a event_indices_array <<< "$event_indices"
normalised_event_indices=()
max_event_index=0
for idx in "${event_indices_array[@]}"; do
  idx="${idx//[[:space:]]/}"
  [ -z "$idx" ] && continue
  normalised_event_indices+=("$idx")
  if [ "$idx" -gt "$max_event_index" ]; then
    max_event_index="$idx"
  fi
done
if [ "${#normalised_event_indices[@]}" -eq 0 ]; then
  normalised_event_indices=("$event_index")
  max_event_index="$event_index"
fi
event_indices="$(IFS=','; printf '%s' "${normalised_event_indices[*]}")"

if [ -z "$simulation_events" ]; then
  simulation_events=$((max_event_index + 1))
fi
if [ -z "$synthetic_gst_events" ]; then
  synthetic_gst_events="$simulation_events"
fi
if [ -z "$synthetic_corsika_events" ]; then
  synthetic_corsika_events="$simulation_events"
fi
if [ -z "$random_seed" ]; then
  random_seed=$(( ( $(date +%s) ^ ($$ << 16) ^ RANDOM ) & 2147483647 ))
fi
if [ -z "$synthetic_gst_seed" ]; then
  synthetic_gst_seed="$random_seed"
fi
if [ -z "$synthetic_corsika_seed" ]; then
  synthetic_corsika_seed="$random_seed"
fi

if [ "$make_synthetic_gst" = "auto" ]; then
  make_synthetic_gst=0
  generator_macro_base="${generator_macro##*/}"
  if [ "$generator_macro_base" = "thesis_bnb_like.mac" ]; then
    make_synthetic_gst=1
    synthetic_gst_macro="${synthetic_gst_macro:-root_macros/make_thesis_bnb_gst.C}"
    synthetic_gst_output="${synthetic_gst_output:-data/thesis_bnb_like_gst.root}"
  elif [ "$generator_macro_base" = "lambda_cc.mac" ]; then
    make_synthetic_gst=1
    synthetic_gst_macro="${synthetic_gst_macro:-root_macros/make_lambda_cc_gst.C}"
    synthetic_gst_output="${synthetic_gst_output:-data/lambda_cc_gst.root}"
  elif [ "$generator_macro_base" = "corsika_overlay_genie.mac" ]; then
    make_synthetic_gst=1
    synthetic_gst_macro="${synthetic_gst_macro:-root_macros/make_thesis_bnb_gst.C}"
    synthetic_gst_output="${synthetic_gst_output:-data/corsika_overlay_genie_gst.root}"
  elif [ "$generator_macro_base" = "genie_pdecay.mac" ]; then
    make_synthetic_gst=1
    synthetic_gst_macro="${synthetic_gst_macro:-root_macros/make_genie_pdecay_gst.C}"
    synthetic_gst_output="${synthetic_gst_output:-data/genie_pdecay_gst.root}"
  fi
fi
if [ "$make_synthetic_gst" = "1" ]; then
  synthetic_gst_macro="${synthetic_gst_macro:-root_macros/make_thesis_bnb_gst.C}"
  synthetic_gst_output="${synthetic_gst_output:-data/thesis_bnb_like_gst.root}"
fi

if [ "$make_synthetic_corsika" = "auto" ]; then
  make_synthetic_corsika=0
  generator_macro_base="${generator_macro##*/}"
  if [ "$generator_macro_base" = "corsika_cosmics.mac" ]; then
    make_synthetic_corsika=1
    synthetic_corsika_macro="${synthetic_corsika_macro:-root_macros/make_corsika_cosmics.C}"
    synthetic_corsika_output="${synthetic_corsika_output:-data/corsika_cosmics.root}"
  elif [ "$generator_macro_base" = "corsika_overlay_genie.mac" ]; then
    make_synthetic_corsika=1
    synthetic_corsika_macro="${synthetic_corsika_macro:-root_macros/make_corsika_cosmics.C}"
    synthetic_corsika_output="${synthetic_corsika_output:-data/corsika_overlay_cosmics.root}"
  fi
fi
if [ "$make_synthetic_corsika" = "1" ]; then
  synthetic_corsika_macro="${synthetic_corsika_macro:-root_macros/make_corsika_cosmics.C}"
  synthetic_corsika_output="${synthetic_corsika_output:-data/corsika_cosmics.root}"
fi

mkdir -p "$repo_root/data/event_displays"

if [ "$skip_docker_build" = "auto" ]; then
  if docker image inspect "$sim_image_tag" >/dev/null 2>&1; then
    skip_docker_build=1
  else
    skip_docker_build=0
  fi
fi

if [ "$skip_docker_build" != "1" ]; then
  docker build -t "$sim_image_tag" "$repo_root"
fi

docker run --rm \
  -e DETECTOR_MACRO="$detector_macro" \
  -e GENERATOR_MACRO="$generator_macro" \
  -e EVENT_INDEX="$event_index" \
  -e EVENT_INDICES="$event_indices" \
  -e SIM_EVENTS="$simulation_events" \
  -e OUTPUT_PREFIX="$output_prefix" \
  -e OUTPUT_FORMATS="$output_formats" \
  -e MAX_SEGMENTS="$max_segments" \
  -e RENDER_MODES="$render_modes" \
  -e TRACK_PDGS="$track_pdgs" \
  -e PRIMARY_ONLY="$primary_only" \
  -e MIN_STEP_CM="$min_step_cm" \
  -e MAX_TRACKS="$max_tracks" \
  -e G4LARBOX_RANDOM_SEED="$random_seed" \
  -e MAKE_SYNTHETIC_GST="$make_synthetic_gst" \
  -e SYNTHETIC_GST_MACRO="$synthetic_gst_macro" \
  -e SYNTHETIC_GST_OUTPUT="$synthetic_gst_output" \
  -e SYNTHETIC_GST_EVENTS="$synthetic_gst_events" \
  -e SYNTHETIC_GST_SEED="$synthetic_gst_seed" \
  -e MAKE_SYNTHETIC_CORSIKA="$make_synthetic_corsika" \
  -e SYNTHETIC_CORSIKA_MACRO="$synthetic_corsika_macro" \
  -e SYNTHETIC_CORSIKA_OUTPUT="$synthetic_corsika_output" \
  -e SYNTHETIC_CORSIKA_EVENTS="$synthetic_corsika_events" \
  -e SYNTHETIC_CORSIKA_SEED="$synthetic_corsika_seed" \
  -e BUILD_IN_MOUNT="$build_in_mount" \
  -e BUILD_JOBS="$build_jobs" \
  -v "$repo_root:/work" \
  -w /work \
  --entrypoint /bin/bash \
  "$sim_image_tag" \
  -lc '
set -euo pipefail

if ! command -v root >/dev/null 2>&1; then
  root_dir="${ROOTSYS:-/software/root_install}"
  old_pwd="$PWD"
  set +e
  cd "$root_dir" && source bin/thisroot.sh
  root_setup_status=$?
  cd "$old_pwd"
  set -euo pipefail

  if [ "$root_setup_status" -ne 0 ]; then
    exit "$root_setup_status"
  fi
fi

set +e
source /usr/local/bin/geant4.sh
geant4_setup_status=$?
set -euo pipefail

if [ "$geant4_setup_status" -ne 0 ]; then
  exit "$geant4_setup_status"
fi

sim_exe=/opt/G4LArBox/G4LArBox
if [ "$BUILD_IN_MOUNT" = "1" ]; then
  external_dir=/opt/G4LArBox/extern
  if [ ! -d "$external_dir/marley/build" ]; then
    external_dir=/work/extern
  fi

  cmake -S /work -B /work/build -DMAIN_EXTERNAL_DIR="$external_dir"
  cmake --build /work/build -j"$BUILD_JOBS"
  sim_exe=/work/G4LArBox
fi

generator_to_run="$GENERATOR_MACRO"
if [ "$SIM_EVENTS" -gt 0 ]; then
  generator_to_run=/tmp/g4larbox_render_generator.mac
  awk -v n="$SIM_EVENTS" '"'"'
    BEGIN { replaced = 0 }
    /^[[:space:]]*\/run\/beamOn[[:space:]]+/ {
      print "/run/beamOn " n
      replaced = 1
      next
    }
    { print }
    END {
      if (!replaced) {
        print "/run/beamOn " n
      }
    }
  '"'"' "$GENERATOR_MACRO" > "$generator_to_run"
fi

if [ "$MAKE_SYNTHETIC_GST" = "1" ]; then
  root -l -b -q "${SYNTHETIC_GST_MACRO}(\"${SYNTHETIC_GST_OUTPUT}\",${SYNTHETIC_GST_EVENTS},${SYNTHETIC_GST_SEED})"
fi
if [ "$MAKE_SYNTHETIC_CORSIKA" = "1" ]; then
  root -l -b -q "${SYNTHETIC_CORSIKA_MACRO}(\"${SYNTHETIC_CORSIKA_OUTPUT}\",${SYNTHETIC_CORSIKA_EVENTS},${SYNTHETIC_CORSIKA_SEED})"
fi

"$sim_exe" -d "$DETECTOR_MACRO" -g "$generator_to_run"
'

root_platform_args=()
if [ -n "$root_platform" ]; then
  root_platform_args=(--platform "$root_platform")
fi

docker run --rm \
  "${root_platform_args[@]}" \
  -e EVENT_INDEX="$event_index" \
  -e EVENT_INDICES="$event_indices" \
  -e GDML_FILE="$gdml_file" \
  -e OUTPUT_PREFIX="$output_prefix" \
  -e OUTPUT_FORMATS="$output_formats" \
  -e MAX_SEGMENTS="$max_segments" \
  -e RENDER_MODES="$render_modes" \
  -e TRACK_PDGS="$track_pdgs" \
  -e PRIMARY_ONLY="$primary_only" \
  -e MIN_STEP_CM="$min_step_cm" \
  -e MAX_TRACKS="$max_tracks" \
  -v "$repo_root:/work" \
  -w /work \
  --entrypoint /bin/bash \
  "$root_image_tag" \
  -lc 'set -euo pipefail
IFS="," read -r -a render_indices <<< "$EVENT_INDICES"
for render_event_index in "${render_indices[@]}"; do
  render_event_index="${render_event_index//[[:space:]]/}"
  [ -z "$render_event_index" ] && continue

  event_output_prefix="$OUTPUT_PREFIX"
  if [[ "$event_output_prefix" == *'{event}'* ]]; then
    event_output_prefix="${event_output_prefix//\{event\}/$render_event_index}"
  elif [ "${#render_indices[@]}" -gt 1 ]; then
    event_output_prefix="${event_output_prefix}_event${render_event_index}"
  fi

  root -l -b -q "root_macros/box_gdml_event_display.C(\"data/output.root\",${render_event_index},\"${GDML_FILE}\",\"${event_output_prefix}\",\"${OUTPUT_FORMATS}\",${MAX_SEGMENTS},\"${RENDER_MODES}\",\"${TRACK_PDGS}\",${PRIMARY_ONLY},${MIN_STEP_CM},${MAX_TRACKS})"
done'

printf 'Generated event-display files:\n'
IFS=',' read -r -a formats <<< "$output_formats"
IFS=',' read -r -a modes <<< "$render_modes"
emit_mode_suffixes=()
for mode in "${modes[@]}"; do
  mode="${mode//[[:space:]]/}"
  mode="$(printf '%s' "$mode" | tr '[:upper:]' '[:lower:]')"
  if [ "$mode" = "both" ] || [ "$mode" = "all" ]; then
    emit_mode_suffixes+=("clean" "debug")
  elif [ "$mode" = "clean" ] || [ "$mode" = "debug" ]; then
    emit_mode_suffixes+=("$mode")
  fi
done
if [ "${#emit_mode_suffixes[@]}" -eq 0 ]; then
  emit_mode_suffixes+=("clean")
fi

for render_event_index in "${normalised_event_indices[@]}"; do
  event_output_prefix="$output_prefix"
  if [[ "$event_output_prefix" == *'{event}'* ]]; then
    event_output_prefix="${event_output_prefix//\{event\}/$render_event_index}"
  elif [ "${#normalised_event_indices[@]}" -gt 1 ]; then
    event_output_prefix="${event_output_prefix}_event${render_event_index}"
  fi

  if [ "${#emit_mode_suffixes[@]}" -gt 1 ]; then
    for mode in "${emit_mode_suffixes[@]}"; do
      for format in "${formats[@]}"; do
        format="${format//[[:space:]]/}"
        [ -n "$format" ] && printf '  %s\n' "$repo_root/${event_output_prefix#./}_$mode.$format"
      done
    done
  else
    for format in "${formats[@]}"; do
      format="${format//[[:space:]]/}"
      [ -n "$format" ] && printf '  %s\n' "$repo_root/${event_output_prefix#./}.$format"
    done
  fi
done
