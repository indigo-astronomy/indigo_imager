#!/bin/sh

VERSION=${1}
# Derive INDIGO major version from the version string (e.g. "3.0-1" -> 3, "2.0-226" -> 2)
INDIGO_VERSION=$(echo "${2:-2}" | cut -d. -f1)

DEBFULLNAME="Rumen Bogdanovski"
EMAIL="rumenastro@gmail.com"

__check_file_exits() {
    [ ! -f ${1} ] && { echo "file '${1}' not found"; exit 1; }
}

# Derive package name from INDIGO major version
if [ "$INDIGO_VERSION" = "3" ]; then
    PKG_NAME="ain-imager-indigo3"
else
    PKG_NAME="ain-imager"
fi

# Check for files where version number shall be replaced.
__check_file_exits "common_src/version.h"

# Make sure debian/changelog does not exists because we will genrate it.
rm -f debian/changelog

# Check for Debian package building executables and tools.
[ ! $(which dch) ] && { echo "executable 'dch' not found install package: 'devscripts'"; exit 1; }
[ ! $(which dpkg-buildpackage) ] && { echo "executable 'dpkg-buildpackage' not found install package: 'dpkg-dev'"; exit 1; }
[ ! $(which cdbs-edit-patch) ] && { echo "executable 'cdbs' not found install package: 'cdbs'"; exit 1; }

# Generate debian/control from the template for the selected INDIGO version
make -f debian/rules gen-control INDIGO_VERSION=${INDIGO_VERSION}

# Build dependencies
./build_libs.sh

# Create entry in debian/changelog.
dch --create --package "${PKG_NAME}" --newversion ${VERSION} --distribution unstable --nomultimaint -t "Build from official upstream."

# Update version.h.
sed -i "s/\(AIN_VERSION \).*/\1\"${VERSION}\"/g" version.h

# Finally build the package.
dpkg-buildpackage \-us \-uc \-I.git \-I\*.out[0-9]\* \-I\*.swp

# Cleanup debian/changelog.
rm -f debian/changelog
