#!/bin/sh

GIT_VERSION=""
DEB_VERSION=""

DEBFULLNAME="Rumen Bogdanovski"
EMAIL="rumen@skyarchive.org"

__check_file_exits() {
    [ ! -f ${1} ] && { echo "file '${1}' not found"; exit 1; }
}

# Check for files where version number shall be replaced.
__check_file_exits "version.h"

# Make sure debian/changelog does not exists because we will genrate it.
rm -f debian/changelog

# Check for Debian package building executables and tools.
[ ! $(which dch) ] && { echo "executable 'dch' not found install package: 'devscripts'"; exit 1; }
[ ! $(which dpkg-buildpackage) ] && { echo "executable 'dpkg-buildpackage' not found install package: 'dpkg-dev'"; exit 1; }
[ ! $(which cdbs-edit-patch) ] && { echo "executable 'cdbs' not found install package: 'cdbs'"; exit 1; }

# Read GIT TAG version via command git describe
GIT_VERSION=$(git describe --tags)
[ $? -ne 0 ] && { echo "GIT tag version cannot be read"; exit 1; }

# If parameter is provided, try this tag to checkout and build package.
[ "$#" -eq 1 ] && GIT_VERSION=${1}

# Get current branch
CURRENT_BRANCH=$(git branch | grep \* | cut -d ' ' -f2);

# Checkout the desired tag/version we want to build packages.
git checkout ${GIT_VERSION} >/dev/null 2>&1
[ $? -ne 0 ] && { echo "version '${1}' does not exists in git"; exit 1; }

# Build dependencies
./build_libs.sh

# Create entry in debian/changelog.
dch --create --package "indigo-imager" --newversion ${GIT_VERSION} --distribution unstable --nomultimaint -t "Build from official upstream."

# Update version.h.
sed -i "s/\(AIN_VERSION \).*/\1\"${GIT_VERSION}\"/g" version.h

# Finally build the package.
dpkg-buildpackage \-us \-uc \-I.git \-I\*.out[0-9]\* \-I\*.swp

# Cleanup debian/changelog.
rm -f debian/changelog

# Return to current branch
git checkout $CURRENT_BRANCH
