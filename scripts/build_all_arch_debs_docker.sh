#!/bin/sh

if [ ! -n "$1" ];
then
	echo "Please specify version";
	exit 1;
fi

VERSION=$1
echo "Building version $VERSION"
sh scripts/make_source_tarball.sh $VERSION
#sh scripts/build_in_docker.sh "i386/debian:stretch-slim" $VERSION "i386"
#sh scripts/build_in_docker.sh "amd64/debian:stretch-slim" $VERSION "amd64"
sh scripts/build_in_docker.sh "arm32v7/debian:buster-slim" $VERSION "armhf"
#sh scripts/build_in_docker.sh "arm64v8/debian:stretch-slim" $VERSION "arm64"
rm ain-imager-$VERSION.tar.gz

