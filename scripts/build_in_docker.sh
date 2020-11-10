#!/bin/bash

# Copyright (c) 2020 Rumen G.Bogdanovski
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license'
# (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

echo FROM $1 >Dockerfile
cat >>Dockerfile <<EOF
LABEL maintainer="rumen@skyarchive.org"
RUN apt-get -y update && apt-get -y install wget unzip build-essential autoconf autotools-dev libtool cmake libudev-dev libavahi-compat-libdnssd-dev libusb-1.0-0-dev fxload libcurl4-gnutls-dev libgphoto2-dev libz-dev git curl bsdmainutils qt5-default devscripts cdbs
RUN echo 'deb [trusted=yes] http://indigo-astronomy.org indigo main' >>/etc/apt/sources.list
RUN apt-get update
RUN apt-get install indigo
RUN git clone https://github.com/indigo-astronomy/indigo_imager.git
WORKDIR indigo_imager
RUN qmake
RUN scripts/builddeb.sh $2
EOF
docker build -t ain .
docker create --name ain ain
docker cp ain:/indigo-imager_$2_$3.deb .
docker container rm ain
docker image rm ain
rm Dockerfile
