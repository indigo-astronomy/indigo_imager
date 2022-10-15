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


#ifndef QADDCUSTOMOBJECT_H
#define QADDCUSTOMOBJECT_H

#include <QObject>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QLineEdit>
#include "customobject.h"

class QAddCustomObject : public QDialog
{
	Q_OBJECT
public:
	QAddCustomObject(QWidget *parent = nullptr);
	~QAddCustomObject(){ };

	signals:
	void requestAddCustomObject(CustomObject object);

public slots:
	void onClose();
	void onAddCustomObject();

private:
	QDialogButtonBox* m_button_box;
	QWidget* m_view_box;
	QWidget* m_add_service_box;

	QLineEdit* m_name_line;
	QLineEdit* m_ra_line;
	QLineEdit* m_dec_line;
	QDoubleSpinBox* m_mag_box;
	QLineEdit* m_description_line;

	QPushButton* m_add_button;
	QPushButton* m_close_button;

	void clear();
};

#endif // QADDCUSTOMOBJECT_H
