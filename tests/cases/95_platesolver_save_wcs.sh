# Ain Platesolver: a real solve of AIN_TEST_IMAGE saved with Save WCS, compared with the WCS in the image header,
# also for a mirrored copy of the image. The .wcs file is written next to copies in the case directory.

need_image
need_platesolver
[ -x "$AIN_PLATESOLVER_BIN" ] || skip_case "no ain_platesolver at $AIN_PLATESOLVER_BIN, build it first"

# the image and a copy mirrored top to bottom, without SIP terms; fails if the header has no CD matrix
python3 -I - "$AIN_TEST_IMAGE" "$CASE_DIR" <<'PY' || skip_case "AIN_TEST_IMAGE has no CD matrix or is not a 2 dimensional FITS image"
import sys
source, out = sys.argv[1:3]
data = open(source, "rb").read()
cards, pos = [], 0
while True:
	record = data[pos:pos + 2880]
	pos += 2880
	cards += [record[i:i + 80] for i in range(0, 2880, 80)]
	if any(card[:8].strip() == b"END" for card in cards[-36:]):
		break
cards = cards[:[card[:8].strip() for card in cards].index(b"END") + 1]
values = {card[:8].decode().strip(): card[10:].decode().split("/")[0].strip() for card in cards if card[8:10] == b"= "}
if values.get("NAXIS") != "2" or "CD1_1" not in values:
	sys.exit(1)
width, height = int(values["NAXIS1"]), int(values["NAXIS2"])
row = width * abs(int(values["BITPIX"])) // 8
pixels = data[pos:pos + row * height]
open(out + "/image.fits", "wb").write(data)
mirrored = []
for card in cards:
	key = card[:8].decode().strip()
	if key.split("_")[0] in ("A", "B", "AP", "BP"):
		continue
	if key == "CRPIX2":
		card = ("CRPIX2  = %20.10f" % (height + 1 - float(values["CRPIX2"]))).ljust(80).encode()
	elif key in ("CD1_2", "CD2_2"):
		card = ("%-8s= %20.12G" % (key, -float(values[key]))).ljust(80).encode()
	elif key in ("CTYPE1", "CTYPE2"):
		card = ("%-8s= '%s'" % (key, "RA---TAN" if key == "CTYPE1" else "DEC--TAN")).ljust(80).encode()
	mirrored.append(card)
header = b"".join(mirrored)
header += b" " * (-len(header) % 2880)
flipped = b"".join(pixels[y * row:(y + 1) * row] for y in reversed(range(height)))
open(out + "/mirrored.fits", "wb").write(header + flipped + b"\0" * (-len(flipped) % 2880))
PY

# every installed index selected for use: index-00.5deg-gaia.isix is INDEX_00_5DEG_GAIA
link_indexes
mkdir -p "$XDG_CONFIG_HOME"
INDEXES=$(for file in "$HOME"/.indigo/solver/*.isix; do basename "$file" .isix; done | tr 'a-z.-' 'A-Z__' | paste -sd, -)
printf '[indexes]\nused=%s\n' "$INDEXES" > "$XDG_CONFIG_HOME/ain_platesolver_solver.conf"

enabled() {	# button text: 1 or 0
	ain_do enabled "$1"
	tac "$CASE_DIR/ain.out" | grep -m1 "^ENABLED $1 " | awk '{print $NF}'
}

is_enabled() {
	[ "$(enabled "$1")" = 1 ]
}

is_disabled() {
	[ "$(enabled "$1")" = 0 ]
}

# the saved file is a valid FITS header and maps the corners and the centre of the image within 60" of its header
# WCS, with the same parity and a pixel scale within 1%
matches_header() {	# image, wcs file
	python3 -I - "$1" "$2" <<'PY'
import math, sys
def header(path):
	data = open(path, "rb").read()
	values, end = {}, False
	for i in range(0, len(data) - 79, 80):
		card = data[i:i + 80].decode("latin1")
		if card[:8].strip() == "END":
			end = i + 80
			break
		if card[8:10] == "= ":
			values[card[:8].strip()] = card[10:].split("/")[0].strip().strip("'").strip()
	return values, end, data
def xy2rd(k, x, y):
	dx, dy = x - float(k["CRPIX1"]), y - float(k["CRPIX2"])
	xi = math.radians(float(k["CD1_1"]) * dx + float(k["CD1_2"]) * dy)
	eta = math.radians(float(k["CD2_1"]) * dx + float(k["CD2_2"]) * dy)
	ra0, dec0 = math.radians(float(k["CRVAL1"])), math.radians(float(k["CRVAL2"]))
	den = math.cos(dec0) - eta * math.sin(dec0)
	return ra0 + math.atan2(xi, den), math.atan2(math.sin(dec0) + eta * math.cos(dec0), math.hypot(xi, den))
def separation(a, b):
	return math.degrees(math.acos(min(1, math.sin(a[1]) * math.sin(b[1]) + math.cos(a[1]) * math.cos(b[1]) * math.cos(a[0] - b[0])))) * 3600
def det(k):
	return float(k["CD1_1"]) * float(k["CD2_2"]) - float(k["CD1_2"]) * float(k["CD2_1"])
image, _, _ = header(sys.argv[1])
wcs, end, data = header(sys.argv[2])
width, height = int(image["NAXIS1"]), int(image["NAXIS2"])
errors = []
if len(data) != 2880 or not end or data[end:].strip() or not data.startswith(b"SIMPLE  =                    T"):
	errors.append("not a FITS header of one record")
if (wcs.get("CTYPE1"), wcs.get("CTYPE2"), wcs.get("IMAGEW"), wcs.get("IMAGEH")) != ("RA---TAN", "DEC--TAN", str(width), str(height)):
	errors.append("projection or image size")
points = [(1, 1), (width, 1), (1, height), (width, height), ((width + 1) / 2, (height + 1) / 2)]
worst = max(separation(xy2rd(image, *p), xy2rd(wcs, *p)) for p in points)
scale = math.sqrt(abs(det(wcs)) / abs(det(image)))
print("  worst corner difference %.1f\", scale ratio %.4f, parity %s" % (worst, scale, "same" if det(wcs) * det(image) > 0 else "opposite"))
if worst > 60:
	errors.append("corners off by %.1f\"" % worst)
if abs(scale - 1) > 0.01:
	errors.append("scale off")
if det(wcs) * det(image) <= 0:
	errors.append("parity differs")
for error in errors:
	print("  " + error)
sys.exit(1 if errors else 0)
PY
}

solved_count_is() {	# count: the log has that many solutions, the log keeps the earlier ones
	[ "$(ain_log | grep -c "Solved:")" -eq "$1" ]
}

not_asked() {	# no question since the last check
	local count
	count=$(grep -c "^QUESTION" "$CASE_DIR/ain.out")
	[ "$count" = "${ASKED:-0}" ]
}

solve_and_save() {	# name, solutions in the log after this one
	ain_do open "$CASE_DIR/$1.fits"
	check "$1: Save WCS is disabled before solving" wait_for 10 is_disabled "Save WCS"
	ain_do button "Solve Image"
	check "$1: image is solved" wait_for 180 solved_count_is "$2"
	check "$1: Save WCS is enabled after solving" wait_for 10 is_enabled "Save WCS"
	ain_do button "Save WCS"
	check "$1: no question for a new file" not_asked
	check "$1: saving is logged" wait_for 10 log_has "Solution saved to $CASE_DIR/$1.wcs"
	matches_header "$CASE_DIR/$1.fits" "$CASE_DIR/$1.wcs"
	check "$1: the saved WCS matches the header" matches_header "$CASE_DIR/$1.fits" "$CASE_DIR/$1.wcs"
	if command -v wcsinfo >/dev/null; then
		check "$1: Astrometry.net reads the saved WCS" wcsinfo "$CASE_DIR/$1.wcs"
	fi
}

AIN_BIN=$AIN_PLATESOLVER_BIN start_ain
solve_and_save image 1
solve_and_save mirrored 2

# saving again asks before replacing the file, a path that can not be written is reported
BEFORE=$(stat -c %Y.%s "$CASE_DIR/mirrored.wcs")
sleep 1.1
ain_do answer No
ain_do button "Save WCS"
ASKED=$((ASKED + 1))
check "saving again asks to replace the file" grep -qF "QUESTION $CASE_DIR/mirrored.wcs already exists. Do you want to replace it?" "$CASE_DIR/ain.out"
check "declining keeps the file" equals "$(stat -c %Y.%s "$CASE_DIR/mirrored.wcs")" "$BEFORE"
check "declining is logged" wait_for 10 log_has "Solution not saved, $CASE_DIR/mirrored.wcs exists"
ain_do answer Yes
ain_do button "Save WCS"
ASKED=$((ASKED + 1))
check "accepting replaces the file" wait_for 10 test "$(stat -c %Y.%s "$CASE_DIR/mirrored.wcs")" != "$BEFORE"
mv "$CASE_DIR/mirrored.wcs" "$CASE_DIR/mirrored.saved"
mkdir "$CASE_DIR/mirrored.wcs"
ain_do button "Save WCS"
ASKED=$((ASKED + 1))
check "a failed save is logged" wait_for 10 log_has "Failed to save $CASE_DIR/mirrored.wcs"

stop_ain
finish_case
