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

#ifndef FILENAMETEMPLATEDLG_H
#define FILENAMETEMPLATEDLG_H

#include <QDialog>
#include <QFont>
#include <QGridLayout>
#include <QLineEdit>

class FilenameTemplateDialog : public QDialog {
	Q_OBJECT
public:
	explicit FilenameTemplateDialog(const QString &initialDir, const QString &initialTemplate, QWidget *parent = nullptr);

	QString directory() const;
	QString filenameTemplate() const;

private slots:
	void onBrowseDirectory();
	void onRestoreDefaults();
	void onTemplateContextMenu(const QPoint &pos);

private:
	void addPlaceholderEntries(QGridLayout *grid, const QFont &boldFont, const QFont &smallFont);

	QLineEdit *m_dir_edit;
	QLineEdit *m_template_edit;
};

#endif // FILENAMETEMPLATEDLG_H
