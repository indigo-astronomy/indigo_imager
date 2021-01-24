#!/bin/bash

if [ -z $1 ]
then
	echo "Please specify git tag"
	exit 1
fi

if [ -z `git tag | grep -w $1` ]
then
	echo "Specified tag not found, building tarball from HEAD"
	git archive --format=tar --prefix=ain-imager-$1/ HEAD | gzip >ain-imager-$1.tar.gz
	exit 0
fi
	
echo "Building tarball from tag $1"
git archive --format=tar --prefix=ain-imager-$1/ $1 | gzip >ain-imager-$1.tar.gz
