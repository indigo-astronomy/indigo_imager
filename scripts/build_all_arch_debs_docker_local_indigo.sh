#!/bin/sh

if [ ! -n "$1" ];
then
	echo "Please specify version";
	exit 1;
fi

if [ ! -n "$2" ];
then
        echo "Please specify INDIGO version";
        exit 1;
fi


VERSION=$1
INDIGO_VERSION=$2
echo "Building version $VERSION"
sh scripts/make_source_tarball.sh $VERSION
sh scripts/build_in_docker_local_indigo.sh "i386/debian:bullseye-slim" $VERSION "i386" $INDIGO_VERSION
sh scripts/build_in_docker_local_indigo.sh "amd64/debian:bullseye-slim" $VERSION "amd64" $INDIGO_VERSION
sh scripts/build_in_docker_local_indigo.sh "arm32v7/debian:bullseye-slim" $VERSION "armhf" $INDIGO_VERSION
sh scripts/build_in_docker_local_indigo.sh "arm64v8/debian:bullseye-slim" $VERSION "arm64" $INDIGO_VERSION
rm ain-imager-$VERSION.tar.gz

