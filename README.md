# indigo_imager

## Configuring build
Install Qt (qt6-base-dev qtchooser qmake6 qt6-base-dev-tools qt6-multimedia-dev libz-dev) and and build tools: gcc, g++, make etc.

Install Qt (qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qtmultimedia5-dev libz-dev) and build tools: gcc g++ make libtool.

1. To build this code on Linux where you do not have INDIGO installed, it is necessary to create a symlink to the location of your indigo source tree (INDIGO should be built there):
```
ln -s <path-to-indigo> indigo
```
2. execute:
```
./build_libs.sh
```
3. execute:
```
qmake6
```

## Build
4. execute:
```
make
```

# Linux users note:
If the image download is very slow (~ 5sec with the CCD Imager Simulator) this may be a result of a slow mDNS response. This can be fixed by editing the following system files:

1. Edit /etc/nsswitch.conf

change line:
```
hosts:          files mdns4_minimal [NOTFOUND=return] dns
```
to read:
```
hosts:          files mdns4 [NOTFOUND=return] dns
```

2. Edit /etc/mdns.allow (file may not exist) and add following lines:
```
.local.
.local
```
