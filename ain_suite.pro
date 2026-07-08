TEMPLATE = subdirs
SUBDIRS = ain_imager_src ain_viewer_src indigo_guidelog_analyzer_src
ain_imager_src.file = ain_imager_src/ain_imager.pro
ain_viewer_src.file = ain_viewer_src/ain_viewer.pro
indigo_guidelog_analyzer_src.file = indigo_guidelog_analyzer_src/indigo_guidelog_analyzer.pro

DISTFILES += \
		README.md \
		LICENCE.md

