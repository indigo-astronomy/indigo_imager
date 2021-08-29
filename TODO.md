#Ain Imager TODO list

- Make it possible to change host of all agents at once, and lock to one host.

## Sequencer (Observing Blocks)
- implement it
- make possible to load a sequence and disconnect, once connected again download the data and
monitor the progress (use Imager Agent's unattended operation capabilities)

## Scheduler
- implement scheduler (once the scheduler agent is ready)

## Viewer
- add Histogram
- add better level stretching
- display MRW, CR2 and other raw files

## Solver
- **DONE** show coordinates of each pixel after the image is solved
- show grid and scale on image
- index handling from the app

## Telescope
- **DONE** Point on image and center on this after the image is solved
- save user defined objects
- Larger DSO library (from Stellarium? Alex gave permission)
- Show star chart and select objects to goto
- alignment point management

## Local operation
- make possible to load drivers and agents internally without indigo server (way better performance for planetary)

## Dashboard
- be able to pin selected properties to be monitored

## BUGS
- solver does not show the image from if the imager agent and astrometry agent are on different host
- fix preview exposure time tooltip
