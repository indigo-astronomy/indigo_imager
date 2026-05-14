#!/bin/sh

if [ ! -n "$1" ];
then
	echo "Please specify version";
	exit 1;
fi

VERSION=$1
# $2: optional INDIGO version string (e.g. "3.0-1"); major is derived automatically; defaults to 2
INDIGO_VER=${2:-2}
INDIGO_VERSION=$(echo "$INDIGO_VER" | cut -d. -f1)
echo "Building version $VERSION against INDIGO $INDIGO_VERSION"
sh scripts/make_source_tarball.sh $VERSION
sh scripts/build_in_docker.sh "i386/debian:bullseye-slim" $VERSION "i386" $INDIGO_VER
sh scripts/build_in_docker.sh "amd64/debian:bullseye-slim" $VERSION "amd64" $INDIGO_VER
sh scripts/build_in_docker.sh "arm32v7/debian:bullseye-slim" $VERSION "armhf" $INDIGO_VER
sh scripts/build_in_docker.sh "arm64v8/debian:bullseye-slim" $VERSION "arm64" $INDIGO_VER
rm ain-imager-$VERSION.tar.gz

