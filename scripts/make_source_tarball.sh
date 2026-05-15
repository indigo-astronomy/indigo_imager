#!/bin/bash

if [ -z "$1" ]
then
	echo "Please specify version"
	exit 1
fi

VERSION=$1

if git rev-parse --verify --quiet "refs/tags/${VERSION}" >/dev/null
then
	echo "Building tarball from tag ${VERSION}"
	REF=${VERSION}
else
	BRANCH=$(git rev-parse --abbrev-ref HEAD)
	COMMIT=$(git rev-parse --short HEAD)
	echo "Tag '${VERSION}' not found, building tarball from last commit of branch '${BRANCH}' (${COMMIT})"
	REF=HEAD
fi

git archive --format=tar --prefix=ain-imager-${VERSION}/ ${REF} | gzip >ain-imager-${VERSION}.tar.gz
