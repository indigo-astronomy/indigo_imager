// Copyright (c) 2019 Rumen G.Bogdanovski
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
#include "imagerwindow.h"
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
	indigo_main_argv = (const char**)argv;
	indigo_main_argc = argc;

	// create config path if it does not exist
	QDir dir("");
	dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
	strncpy(config_path, QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation).toUtf8().constData(), PATH_LEN);

	memset(&conf,0,sizeof(conf_t));
	conf.blobs_enabled = true;
	conf.auto_connect = true;
	conf.indigo_use_host_suffix = true;
	conf.use_state_icons = false;
	conf.use_system_locale = false;
	conf.indigo_log_level = INDIGO_LOG_INFO;
	conf.preview_stretch_level = STRETCH_NORMAL;
	conf.preview_color_balance = CB_AUTO;
	conf.guider_stretch_level = STRETCH_MODERATE;
	conf.guider_color_balance = CB_AUTO;
	conf.antialiasing_enabled = false;
	conf.guider_antialiasing_enabled = false;
	conf.focus_mode = 0;
	conf.guider_save_bandwidth = 1;
	conf.guider_subframe = 0;
	conf.focuser_subframe = 0;
	conf.focuser_display = SHOW_FWHM;
	conf.guider_display = SHOW_RA_DEC_DRIFT;
	conf.guider_save_log = false;
	conf.indigo_save_log = false;
	conf.save_noname_images = false;
	conf.data_dir_prefix[0] = '\0';
	conf.window_width = 0;
	conf.window_height = 0;
	conf.restore_window_size = true;
	conf.imager_show_reference = false;
	conf.sound_notifications_enabled = true;
	conf.save_images_on_server = false;
	conf.keep_images_on_server = false;
	conf.statistics_enabled = false;
	read_conf();

	if (!conf.use_system_locale) qunsetenv("LC_NUMERIC");

#ifndef INDIGO_WINDOWS
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet")) {
			freopen("/dev/null","a+", stderr);
			i++;
		}
	}
#endif

	indigo_set_log_level(conf.indigo_log_level);

	/* This shall be set only before connecting */
	indigo_use_host_suffix = conf.indigo_use_host_suffix;

	for (int i = 1; i < argc; i++) {
		if ((!strcmp(argv[i], "-T") || !strcmp(argv[i], "--master-token")) && i < argc - 1) {
			indigo_set_master_token(indigo_string_to_token(argv[i + 1]));
			i++;
		} else if ((!strcmp(argv[i], "-a") || !strcmp(argv[i], "--acl-file")) && i < argc - 1) {
			indigo_load_device_tokens_from_file(argv[i + 1]);
			i++;
		}
	}

	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication app(argc, argv);

	QFont font("SansSerif", 10, QFont::Medium);
	font.setStyleHint(QFont::SansSerif);

	app.setFont(font);
	//qDebug() << "Font: " << app.font().family() << app.font().pointSize();

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

	ImagerWindow imager_window;
	imager_window.show();

	return app.exec();
}
