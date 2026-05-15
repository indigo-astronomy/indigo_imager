#!/bin/sh

VERSION=${1}
FLAVOR=${2:-indigo}

DEBFULLNAME="Rumen Bogdanovski"
EMAIL="rumenastro@gmail.com"

__check_file_exits() {
    [ ! -f ${1} ] && { echo "file '${1}' not found"; exit 1; }
}

# Validate the requested INDIGO flavor.
if [ "${FLAVOR}" != "indigo" ] && [ "${FLAVOR}" != "indigo3" ]; then
    echo "Invalid INDIGO flavor '${FLAVOR}', expected 'indigo' or 'indigo3'"
    exit 1
fi

if [ "${FLAVOR}" = "indigo3" ]; then
    PACKAGE="ain-imager-indigo3"
else
    PACKAGE="ain-imager"
fi

# Check for files where version number shall be replaced.
__check_file_exits "common_src/version.h"

# Make sure debian/changelog does not exists because we will genrate it.
rm -f debian/changelog

# Check for Debian package building executables and tools.
[ ! $(which dch) ] && { echo "executable 'dch' not found install package: 'devscripts'"; exit 1; }
[ ! $(which dpkg-buildpackage) ] && { echo "executable 'dpkg-buildpackage' not found install package: 'dpkg-dev'"; exit 1; }
[ ! $(which cdbs-edit-patch) ] && { echo "executable 'cdbs' not found install package: 'cdbs'"; exit 1; }

# Build dependencies
./build_libs.sh

# For an indigo3 build swap in the flavored debian/control and a matching
# install file. The originals are restored after the package is built.
if [ "${FLAVOR}" = "indigo3" ]; then
    __check_file_exits "debian/control.indigo3"
    cp debian/control debian/control.ain-bak
    cp debian/control.indigo3 debian/control
    cp debian/ain-imager.install debian/${PACKAGE}.install
fi

# Tell qmake (via ain_imager.pro / ain_viewer.pro) which INDIGO client
# library to link against.
export AIN_INDIGO_FLAVOR=${FLAVOR}

# Create entry in debian/changelog.
dch --create --package "${PACKAGE}" --newversion ${VERSION} --distribution unstable --nomultimaint -t "Build from official upstream."

# Update version.h.
sed -i "s/\(AIN_VERSION \).*/\1\"${VERSION}\"/g" version.h

# Finally build the package.
dpkg-buildpackage \-us \-uc \-I.git \-I\*.out[0-9]\* \-I\*.swp

# Cleanup debian/changelog.
rm -f debian/changelog

# Restore the original packaging files for an indigo3 build.
if [ "${FLAVOR}" = "indigo3" ]; then
    mv debian/control.ain-bak debian/control
    rm -f debian/${PACKAGE}.install
fi
