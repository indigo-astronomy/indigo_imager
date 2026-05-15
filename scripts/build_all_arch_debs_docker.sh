#!/bin/sh

if [ ! -n "$1" ];
then
	echo "Please specify version";
	echo "Usage: $0 <version> [indigo|indigo3]";
	exit 1;
fi

VERSION=$1
# INDIGO flavor to build against: "indigo" (default, v2) or "indigo3" (v3).
FLAVOR=${2:-indigo}
if [ "$FLAVOR" != "indigo" ] && [ "$FLAVOR" != "indigo3" ];
then
	echo "Invalid INDIGO flavor '$FLAVOR', expected 'indigo' or 'indigo3'";
	exit 1;
fi

echo "Building version $VERSION against $FLAVOR"
sh scripts/make_source_tarball.sh $VERSION
sh scripts/build_in_docker.sh "i386/debian:bullseye-slim" $VERSION "i386" $FLAVOR
sh scripts/build_in_docker.sh "amd64/debian:bullseye-slim" $VERSION "amd64" $FLAVOR
sh scripts/build_in_docker.sh "arm32v7/debian:bullseye-slim" $VERSION "armhf" $FLAVOR
sh scripts/build_in_docker.sh "arm64v8/debian:bullseye-slim" $VERSION "arm64" $FLAVOR
rm ain-imager-$VERSION.tar.gz
