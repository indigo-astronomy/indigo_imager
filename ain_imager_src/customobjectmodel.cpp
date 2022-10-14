// Copyright (c) 2022 Rumen G.Bogdanovski & David Hulse
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

#include <QRegularExpression>
#include <QStandardPaths>
#include "customobjectmodel.h"
#include "conf.h"

#define OBJECT_FILENAME "indigo_imager.objects"

CustomObjectModel::CustomObjectModel() {
	m_logger = &Logger::instance();
}

CustomObjectModel::~CustomObjectModel() {
	saveObjects();
	while (!m_objects.isEmpty()) delete m_objects.takeFirst();
}


void CustomObjectModel::saveObjects() {
	char filename[PATH_LEN];
	snprintf(filename, PATH_LEN, "%s/%s", config_path, OBJECT_FILENAME);
	FILE *file = fopen(filename, "w");
	if (file != NULL) {
		for (auto i = m_objects.constBegin(); i != m_objects.constEnd(); ++i) {
			indigo_log("Saved object: \"%s\", %f, %f, %f, \"%s\"\n", (*i)->m_name.toUtf8().constData(), (*i)->m_ra, (*i)->m_dec, (*i)->m_mag, (*i)->m_description.toUtf8().constData());
			fprintf(file, "\"%s\", %f, %f, %f, \"%s\"\n", (*i)->m_name.toUtf8().constData(), (*i)->m_ra, (*i)->m_dec, (*i)->m_mag, (*i)->m_description.toUtf8().constData());
		}
		fclose(file);
	}
}

void CustomObjectModel::loadObjects() {
	char raw_line[1024] = {0};
	char filename[PATH_LEN];
	char name[128];
	char description[256];
	double ra;
	double dec;
	double mag;
	snprintf(filename, PATH_LEN, "%s/%s", config_path, OBJECT_FILENAME);
	FILE * file = fopen(filename, "r");
	if (file != NULL) {
		indigo_log("Objects file open: %s\n", filename);
		int l_num = 0;
		while (fgets(raw_line, 1023, file)) {
			l_num++;
			char *line = raw_line;
			while(isspace((unsigned char)*line)) line++;
			if (line[0]=='#' || line[0] == '\n' || line[0] == '\0') continue;
			name[0] = '\0';
			description[0] = '\0';
			ra = dec = mag = 0;
			int parsed = sscanf(line, "\"%[^\"]\", %lf, %lf, %lf, \"%[^\"]\"\n", name, &ra, &dec, &mag, description);
			if (parsed == 5) {
				indigo_log("Loading object: \"%s\", RA = %f, Dec = %f, mag = %f, \"%s\"\n", name, ra, dec, mag, description);
				addObject(name, ra, dec, mag, description);
			} else {
				indigo_error("Object file error: Parse error at line %d.", l_num);
			}
		}
		fclose(file);
	}
}

bool CustomObjectModel::addObject(QString name, double ra, double dec, double mag, QString description) {
	name = name.trimmed();
	description = description.trimmed();
	int i = findObject(name);
	if (i != -1) {
		indigo_log("OBJECT DUPLICATE [%s]\n", name.toUtf8().constData());
		return false;
	}

	indigo_log("OBJECT ADDED [%s]\n", name.toUtf8().constData());
	CustomObject *object = new CustomObject(name, ra, dec, mag, description);
	m_objects.append(object);
	return true;
}

bool CustomObjectModel::removeObject(QString name) {
	int i = findObject(name);
	if (i != -1) {
		CustomObject *object = m_objects.at(i);
		m_objects.removeAt(i);
		delete object;
		indigo_log("OBJECT REMOVED [%s]\n", name.toUtf8().constData());
		return true;
	}
	indigo_log("OBJECT DOES NOT EXIST [%s]\n", name.toUtf8().constData());
	return false;
}

int CustomObjectModel::findObject(const QString &name) {
	for (auto i = m_objects.constBegin(); i != m_objects.constEnd(); ++i) {
		if ((*i)->m_name == name) {
			return i - m_objects.constBegin();
		}
	}
	return -1;
}
