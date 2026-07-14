// Copyright (c) 2026
// All rights reserved.

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QTextStream>
#include <QVersionNumber>

#include "guidelogviewerwindow.h"

int main(int argc, char *argv[]) {
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication app(argc, argv);

#ifdef Q_OS_MACOS
	QApplication::setStyle("Fusion");
#endif

	int id = QFontDatabase::addApplicationFont(":/fonts/Hack-Regular.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Hack-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Hack-Italic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Hack-BoldItalic.ttf");
	if (id != -1) {
		QStringList families = QFontDatabase::applicationFontFamilies(id);
		if (!families.isEmpty()) {
			QFont::insertSubstitution("monospace", families.at(0));
		}
	}

	id = QFontDatabase::addApplicationFont(":/fonts/DejaVuSans.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Oblique.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-BoldOblique.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-ExtraLight.ttf");
	if (id != -1) {
		QString family = QFontDatabase::applicationFontFamilies(id).at(0);
#ifdef Q_OS_MACOS
		QFont font = QFont(family, 13, QFont::Light);
#else
		QFont font = QFont(family, 10, QFont::Medium);
#endif
		font.setKerning(false);
		app.setFont(font);
	}

	QVersionNumber runningVersion = QVersionNumber::fromString(qVersion());
	QVersionNumber thresholdVersion(5, 13, 0);
	QString qssResource(":qdarkstyle/style.qss");
	if (runningVersion >= thresholdVersion) {
		qssResource = ":qdarkstyle/style-5.13.qss";
	}

	QFile f(qssResource);
	if (f.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream ts(&f);
		app.setStyleSheet(ts.readAll());
		f.close();
	}

	GuideLogViewerWindow window;
	window.show();

	return app.exec();
}
