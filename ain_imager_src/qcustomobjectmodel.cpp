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
#include "qcustomobjectmodel.h"
#include "conf.h"

#define SERVICE_FILENAME "indigo_imager.objects"

QCustomObjectModel::QCustomObjectModel() {
	m_logger = &Logger::instance();
}

QCustomObjectModel::~QCustomObjectModel() {
	saveObjects();
	while (!m_objects.isEmpty()) delete m_objects.takeFirst();
}


void QCustomObjectModel::saveObjects() {
	char filename[PATH_LEN];
	snprintf(filename, PATH_LEN, "%s/%s", config_path, SERVICE_FILENAME);
	FILE *file = fopen(filename, "w");
	if (file != NULL) {
		for (auto i = m_objects.constBegin(); i != m_objects.constEnd(); ++i) {
			indigo_log("Saved object: \"%s\", %f, %f, %f, %s\n", (*i)->m_name.toUtf8().constData(), (*i)->m_ra, (*i)->m_dec, (*i)->m_mag, (*i)->m_description.toUtf8().constData());
			fprintf(file, "\"%s\", %f, %f, %f, \"%s\"\n", (*i)->m_name.toUtf8().constData(), (*i)->m_ra, (*i)->m_dec, (*i)->m_mag, (*i)->m_description.toUtf8().constData());
		}
		fclose(file);
	}
}

void QCustomObjectModel::loadObjects() {
	char filename[PATH_LEN];
	char name[128] = {0};
	char description[256] = {0};
	double ra;
	double dec;
	double mag;
	snprintf(filename, PATH_LEN, "%s/%s", config_path, SERVICE_FILENAME);
	FILE * file = fopen(filename, "r");
	if (file != NULL) {
		indigo_debug("Objects file open: %s\n", filename);
		while (fscanf(file,"%127[^,] %lf, %lf, %lf, \"%255[^\"]\n", name, &ra, &dec, &mag, description) == 5) {
			indigo_log("Loading object: \"%s\", RA = %d, Dec = %f, mag = %d, \"%s\"\n", name, ra, dec, mag, description);
			addObject(name, ra, dec, mag, description);
		}
		fclose(file);
	}
}

bool QCustomObjectModel::addObject(QString name, double ra, double dec, double mag, QString description) {
	name = name.trimmed();
	description = description.trimmed();
	int i = findObject(name);
	if (i != -1) {
		indigo_log("OBJECT DUPLICATE [%s]\n", name.toUtf8().constData());
		return false;
	}

	indigo_log("OBJECT ADDED [%s]\n", name.toUtf8().constData());
	QCustomObject *object = new QCustomObject(name, ra, dec, mag, description);
	m_objects.append(object);
	saveObjects();
	return true;
}

bool QCustomObjectModel::removeObject(QString name) {
	int i = findObject(name);
	if (i != -1) {
		QCustomObject *object = m_objects.at(i);
		m_objects.removeAt(i);
		delete object;
		indigo_log("OBJECT REMOVED [%s]\n", name.toUtf8().constData());
		saveObjects();
		return true;
	}
	indigo_log("OBJECT DOES NOT EXIST [%s]\n", name.toUtf8().constData());
	return false;
}

int QCustomObjectModel::findObject(const QString &name) {
	for (auto i = m_objects.constBegin(); i != m_objects.constEnd(); ++i) {
		if ((*i)->m_name == name) {
			return i - m_objects.constBegin();
		}
	}
	return -1;
}
