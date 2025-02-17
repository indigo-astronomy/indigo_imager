// Copyright (c) 2025 Rumen G.Bogdanovski
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

#ifndef SELECTOBJECT_H
#define SELECTOBJECT_H

#include <QWidget>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QCheckBox>
#include <QListWidget>
#include "customobjectmodel.h"
#include "indigo_cat_data.h"

Q_DECLARE_METATYPE(indigo_dso_entry*)
Q_DECLARE_METATYPE(indigo_star_entry*)

class SelectObject : public QFrame {
	Q_OBJECT

public:
	explicit SelectObject(QWidget *parent = nullptr);

signals:
	void objectSelected(const QString &name, double ra, double dec);

protected:
	void showEvent(QShowEvent *event) override;

private slots:
	void onSearchTextChanged(const QString &text);
	void onSearchEntered();
	void onCustomObjectsOnlyChecked(bool checked);
	void onObjectSelected();
	void onObjectClicked(QListWidgetItem *item);

private:
	QGridLayout *m_layout;
	QLineEdit *m_object_search_line;
	QCheckBox *m_custom_objects_only_cbox;
	QListWidget *m_object_list;
	CustomObjectModel *m_customObjectModel;

	void updateObjectList(const QString &text);
};

#endif // SELECTOBJECT_H