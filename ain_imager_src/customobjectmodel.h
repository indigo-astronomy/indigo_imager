// Copyright (c) 2022 Rumen G.Bogdanovski
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


#ifndef CUSTOMOBJECTMODEL_H
#define CUSTOMOBJECTMODEL_H

#include <QString>
#include <QList>
#include <customobject.h>
#include "logger.h"

class CustomObjectModel {

public:
	CustomObjectModel();
	~CustomObjectModel();

	void saveObjects();
	void loadObjects();
	bool addObject(QString name, double ra, double dec, double mag = 0, QString description = "");
	bool removeObject(QString name);
	QList<CustomObject*> m_objects;
	int findObject(const QString &name);

private:
	Logger* m_logger;
};

#endif // CUSTOMOBJECTMODEL_H
