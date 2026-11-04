#!/bin/bash
# Copies the solver of ain_platesolver from INDIGO, through the indigo link in the project root. The image loaders of
# indigo_solver.c and indigo_solver.h are left out, the application opens images with the code in common_src.
set -e
cd "$(dirname "$0")/../.."

AGENT=indigo/indigo_drivers/agent_platesolver
LIBS=indigo/indigo_libs
TARGET=external/indigo_solver
trap 'rm -f "$TARGET"/indigo_solver.c.tmp "$TARGET"/indigo_solver.h.tmp' EXIT

copy_without() {
	local source=$1 first=$2 next=$3 target=$4
	if ! grep -q "$first" "$source" || ! grep -q "$next" "$source"; then
		echo "$source has changed, '$first' or '$next' not found. $target is not updated." >&2
		exit 1
	fi
	sed "/$first/,/$next/{/$next/!d}" "$source" > "$target.tmp"
}

for file in "$AGENT/indigo_solver.c" "$AGENT/indigo_solver.h" "$AGENT/indigo_solver_utils.c" "$AGENT/indigo_solver_utils.h" "$AGENT/indigo_solver_healpix.c" "$AGENT/indigo_solver_healpix.h" "$LIBS/indigo_raw_utils.c"; do
	if [ ! -f "$file" ]; then
		echo "$file not found, is the indigo link in the project root valid?" >&2
		exit 1
	fi
done

copy_without "$AGENT/indigo_solver.c" '^static indigo_raw_header \*mono16_image(int width, int height)' '^\/\/ Star detection works on a copy of the frame' "$TARGET/indigo_solver.c"
sed -i -e '/^#include <setjmp.h>$/d' -e '/^#include <jpeglib.h>$/d' -e '/^#include <indigo\/indigo_dslr_raw.h>$/d' "$TARGET/indigo_solver.c.tmp"
copy_without "$AGENT/indigo_solver.h" '^\/\*\* Any supported input image as a newly allocated' '^\/\*\* Averages bin x bin pixels in place' "$TARGET/indigo_solver.h"
mv "$TARGET/indigo_solver.c.tmp" "$TARGET/indigo_solver.c"
mv "$TARGET/indigo_solver.h.tmp" "$TARGET/indigo_solver.h"

cp "$AGENT/indigo_solver_utils.c" "$AGENT/indigo_solver_utils.h" "$AGENT/indigo_solver_healpix.c" "$AGENT/indigo_solver_healpix.h" "$LIBS/indigo_raw_utils.c" "$TARGET/"

echo "Solver files updated, rebuild ain_platesolver and check the changes with git diff."
