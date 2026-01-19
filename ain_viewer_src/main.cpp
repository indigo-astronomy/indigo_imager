// Copyright (c) 2021 Rumen G.Bogdanovski & David Hulse
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


#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QVersionNumber>
#include <viewerwindow.h>
#include <conf.h>

conf_t conf;
char config_path[PATH_LEN];

void write_conf() {
	char filename[PATH_LEN];
	snprintf(filename, PATH_LEN, "%s/%s", config_path, CONFIG_FILENAME);
	FILE * file= fopen(filename, "wb");
	if (file != nullptr) {
		fwrite(&conf, sizeof(conf), 1, file);
		fclose(file);
	}
}

void read_conf() {
	char filename[PATH_LEN];
	snprintf(filename, PATH_LEN, "%s/%s", config_path, CONFIG_FILENAME);
	FILE * file= fopen(filename, "rb");
	if (file != nullptr) {
		fread(&conf, sizeof(conf), 1, file);
		fclose(file);
	}
}


int main(int argc, char *argv[]) {
	// create config path if it does not exist
	QDir dir("");
	dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
	strncpy(config_path, QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation).toUtf8().constData(), PATH_LEN);

	memset(&conf,0,sizeof(conf_t));
	conf.use_state_icons = false;
	conf.reopen_file_at_start = false;
	conf.indigo_log_level = INDIGO_LOG_INFO;
	conf.preview_stretch_level = STRETCH_NORMAL;
	conf.preview_color_balance = CB_AUTO;
	conf.antialiasing_enabled = false;
	conf.window_width = 0;
	conf.window_height = 0;
	conf.restore_window_size = true;
	conf.statistics_enabled = false;
	conf.preview_bayer_pattern = 0;
	conf.show_reference = false;
	read_conf();

	if (!conf.reopen_file_at_start) {
		conf.file_open[0] = '\0';
	}
	// always default to error log level unless overridden from command line
	conf.indigo_log_level = INDIGO_LOG_ERROR;

	qunsetenv("LC_NUMERIC");

	// Parse command line arguments
	bool enable_inspector = false;
	int auto_save_seconds = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-I") == 0) {
			enable_inspector = true;
		} else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
			i++;
			strncpy(conf.file_open, argv[i], PATH_MAX);
		} else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
			i++;
			auto_save_seconds = atoi(argv[i]);
		} else if (strcmp(argv[i], "-vv") == 0) {
			conf.indigo_log_level = INDIGO_LOG_DEBUG;
		} else if (strcmp(argv[i], "-v") == 0) {
			conf.indigo_log_level = INDIGO_LOG_INFO;
		} else if (argv[i][0] != '-') {
			// Backwards compatibility: treat non-option as filename
			strncpy(conf.file_open, argv[i], PATH_MAX);
		}
	}

	indigo_set_log_level(conf.indigo_log_level);

	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication app(argc, argv);

	int id = QFontDatabase::addApplicationFont(":/fonts/Hack-Regular.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Hack-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Hack-Italic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Hack-BoldItalic.ttf");
	if (id != -1) {
		QStringList families = QFontDatabase::applicationFontFamilies(id);
		if (!families.isEmpty()) {
			QString monoFamily = families.at(0);
			QFont::insertSubstitution("monospace", monoFamily);
		}
	} else {
		indigo_error("Failed to load embedded Hack Mono font, using system default.");
	}

	id = QFontDatabase::addApplicationFont(":/fonts/DejaVuSans.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Oblique.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-BoldOblique.ttf");
	QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-ExtraLight.ttf");
	if (id != -1) {
		QString family = QFontDatabase::applicationFontFamilies(id).at(0);
		app.setFont(QFont(family, 10, QFont::Medium));
	} else {
		indigo_error("Failed to load embedded DejaVu Sans font, using system default.");
		QFont font("SansSerif", 10, QFont::Medium);
		font.setStyleHint(QFont::SansSerif);
		app.setFont(font);
	}

	QVersionNumber running_version = QVersionNumber::fromString(qVersion());
	QVersionNumber threshod_version(5, 13, 0);
	QString qss_resource(":qdarkstyle/style.qss");
	if (running_version >= threshod_version) {
		qss_resource = ":qdarkstyle/style-5.13.qss";
	}
	QFile f(qss_resource);
	f.open(QFile::ReadOnly | QFile::Text);
	QTextStream ts(&f);
	app.setStyleSheet(ts.readAll());
	f.close();

	ViewerWindow viewer_window;
	viewer_window.show();

	// Enable image analyzer if requested via command line
	if (enable_inspector) {
		viewer_window.enable_image_inspector(true);
	}

	// Schedule auto-save if requested
	if (auto_save_seconds > 0) {
		viewer_window.schedule_auto_save(auto_save_seconds);
	}

	return app.exec();
}
