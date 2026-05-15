#!/bin/sh

if [ ! -n "$1" ];
then
	echo "Please specify version";
	echo "Usage: $0 <version> <indigo_version> [indigo|indigo3]";
	exit 1;
fi

if [ ! -n "$2" ];
then
        echo "Please specify INDIGO version";
        echo "Usage: $0 <version> <indigo_version> [indigo|indigo3]";
        exit 1;
fi


VERSION=$1
INDIGO_VERSION=$2
# INDIGO flavor to build against: "indigo" (default, v2) or "indigo3" (v3).
# The local INDIGO .deb files must be named "<flavor>-<indigo_version>-<arch>.deb".
FLAVOR=${3:-indigo}
if [ "$FLAVOR" != "indigo" ] && [ "$FLAVOR" != "indigo3" ];
then
	echo "Invalid INDIGO flavor '$FLAVOR', expected 'indigo' or 'indigo3'";
	exit 1;
fi

echo "Building version $VERSION against $FLAVOR $INDIGO_VERSION"
sh scripts/make_source_tarball.sh $VERSION
sh scripts/build_in_docker_local_indigo.sh "i386/debian:bullseye-slim" $VERSION "i386" $INDIGO_VERSION $FLAVOR
sh scripts/build_in_docker_local_indigo.sh "amd64/debian:bullseye-slim" $VERSION "amd64" $INDIGO_VERSION $FLAVOR
sh scripts/build_in_docker_local_indigo.sh "arm32v7/debian:bullseye-slim" $VERSION "armhf" $INDIGO_VERSION $FLAVOR
sh scripts/build_in_docker_local_indigo.sh "arm64v8/debian:bullseye-slim" $VERSION "arm64" $INDIGO_VERSION $FLAVOR
rm ain-imager-$VERSION.tar.gz
