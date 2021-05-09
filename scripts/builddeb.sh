#!/bin/sh

VERSION=${1}

DEBFULLNAME="Rumen Bogdanovski"
EMAIL="rumen@skyarchive.org"

__check_file_exits() {
    [ ! -f ${1} ] && { echo "file '${1}' not found"; exit 1; }
}

# Check for files where version number shall be replaced.
__check_file_exits "ain_imager_src/version.h"

# Make sure debian/changelog does not exists because we will genrate it.
rm -f debian/changelog

# Check for Debian package building executables and tools.
[ ! $(which dch) ] && { echo "executable 'dch' not found install package: 'devscripts'"; exit 1; }
[ ! $(which dpkg-buildpackage) ] && { echo "executable 'dpkg-buildpackage' not found install package: 'dpkg-dev'"; exit 1; }
[ ! $(which cdbs-edit-patch) ] && { echo "executable 'cdbs' not found install package: 'cdbs'"; exit 1; }

# Build dependencies
./build_libs.sh

# Create entry in debian/changelog.
dch --create --package "ain-imager" --newversion ${VERSION} --distribution unstable --nomultimaint -t "Build from official upstream."

# Update version.h.
sed -i "s/\(AIN_VERSION \).*/\1\"${VERSION}\"/g" version.h

# Finally build the package.
dpkg-buildpackage \-us \-uc \-I.git \-I\*.out[0-9]\* \-I\*.swp

# Cleanup debian/changelog.
rm -f debian/changelog
