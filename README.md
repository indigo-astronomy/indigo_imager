# indigo_imager

To build this code on Mac, it is necessary to create a symlink to the location of your indigo source tree:

ln -s <path-to-indigo> indigo

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

