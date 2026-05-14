#!/bin/sh

if [ ! -n "$1" ];
then
	echo "Please specify version";
	exit 1;
fi

if [ ! -n "$2" ];
then
        echo "Please specify INDIGO deb version (e.g. 2.0-226)";
        exit 1;
fi


VERSION=$1
INDIGO_DEB_VERSION=$2
# Major INDIGO version is derived from the deb version string (e.g. "3.0-1" -> 3)
INDIGO_VERSION=$(echo "$INDIGO_DEB_VERSION" | cut -d. -f1)
echo "Building version $VERSION against INDIGO $INDIGO_VERSION (local deb: $INDIGO_DEB_VERSION)"
sh scripts/make_source_tarball.sh $VERSION
sh scripts/build_in_docker_local_indigo.sh "i386/debian:bullseye-slim" $VERSION "i386" $INDIGO_DEB_VERSION
sh scripts/build_in_docker_local_indigo.sh "amd64/debian:bullseye-slim" $VERSION "amd64" $INDIGO_DEB_VERSION
sh scripts/build_in_docker_local_indigo.sh "arm32v7/debian:bullseye-slim" $VERSION "armhf" $INDIGO_DEB_VERSION
sh scripts/build_in_docker_local_indigo.sh "arm64v8/debian:bullseye-slim" $VERSION "arm64" $INDIGO_DEB_VERSION
rm ain-imager-$VERSION.tar.gz

