// Copyright (c) 2026 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
