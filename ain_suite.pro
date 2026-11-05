TEMPLATE = subdirs
SUBDIRS = ain_imager_src ain_viewer_src ain_guidelog_viewer_src ain_platesolver_src
ain_imager_src.file = ain_imager_src/ain_imager.pro
ain_viewer_src.file = ain_viewer_src/ain_viewer.pro
ain_guidelog_viewer_src.file = ain_guidelog_viewer_src/ain_guidelog_viewer.pro
ain_platesolver_src.file = ain_platesolver_src/ain_platesolver.pro

DISTFILES += \
		README.md \
		LICENCE.md

