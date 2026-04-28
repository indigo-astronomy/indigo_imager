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

#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include "filenametemplatedlg.h"
#include "conf.h"

QString FilenameTemplateDialog::directory() const {
	return m_dir_edit->text().trimmed();
}

QString FilenameTemplateDialog::filenameTemplate() const {
	QString t = m_template_edit->text().trimmed();
	// Strip placeholders that are always appended automatically and are disallowed in user templates
	t.remove(QRegularExpression("%I"));
	t.remove(QRegularExpression("%[1-9]I"));
	t.remove(QRegularExpression("%M"));
	t.remove(QRegularExpression("%[1-9]S"));
	t.remove(QRegularExpression("%S"));
	return t;
}

FilenameTemplateDialog::FilenameTemplateDialog(const QString &initialDir, const QString &initialTemplate, QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Select output data directory and filename template"));
	setMinimumWidth(650);

	QVBoxLayout *vbox = new QVBoxLayout(this);

	// Form with output directory and filename template rows
	QFormLayout *form = new QFormLayout;

	m_dir_edit = new QLineEdit(initialDir);
	QPushButton *dir_btn = new QPushButton(tr("Browse..."));
	QWidget *dir_widget = new QWidget;
	QHBoxLayout *dir_h = new QHBoxLayout(dir_widget);
	dir_h->setContentsMargins(0, 0, 0, 0);
	dir_h->addWidget(m_dir_edit);
	dir_h->addWidget(dir_btn);
	QLabel *out_label = new QLabel(tr("<b>Output directory:</b>"));
	form->addRow(out_label, dir_widget);

	m_template_edit = new QLineEdit(initialTemplate);
	QLabel *template_label = new QLabel(tr("<b>Filename template:</b>"));
	form->addRow(template_label, m_template_edit);

	vbox->addLayout(form);

	// Framed group box with supported placeholder reference
	QGroupBox *placeholder_box = new QGroupBox(tr("Supported placeholders"));
	placeholder_box->setStyleSheet("QGroupBox { font-weight: normal; }");
	QVBoxLayout *placeholder_vbox = new QVBoxLayout(placeholder_box);

	QFont bold_font;
	bold_font.setBold(true);
	QFont small_font;
	small_font.setPointSizeF(small_font.pointSizeF() - 1.0);

	// Two-column grid: col 0,1 = left group; col 2 = stretch; col 3,4 = right group
	QGridLayout *grid = new QGridLayout;
	grid->setHorizontalSpacing(4);
	grid->setVerticalSpacing(2);
	grid->setColumnStretch(2, 1);

	addPlaceholderEntries(grid, bold_font, small_font);
	placeholder_vbox->addLayout(grid);

	QLabel *note = new QLabel(tr("<i><b>Note:</b> <b>%nI</b> and <b>%M</b> (MD5 hash) are always appended automatically as <b>_idx%3I_%M</b></i>"));
	note->setFont(small_font);
	note->setWordWrap(true);
	placeholder_vbox->addWidget(note);

	vbox->addWidget(placeholder_box);

	// Buttons: Defaults on left, OK/Cancel on right
	QHBoxLayout *buttons = new QHBoxLayout;
	QPushButton *defaults_btn = new QPushButton(tr("Defaults"));
	QPushButton *ok_btn       = new QPushButton(tr("OK"));
	QPushButton *cancel_btn   = new QPushButton(tr("Cancel"));
	buttons->addWidget(defaults_btn);
	buttons->addStretch();
	buttons->addWidget(ok_btn);
	buttons->addWidget(cancel_btn);
	vbox->addLayout(buttons);

	connect(dir_btn,      &QPushButton::clicked, this, &FilenameTemplateDialog::onBrowseDirectory);
	connect(defaults_btn, &QPushButton::clicked, this, &FilenameTemplateDialog::onRestoreDefaults);
	connect(cancel_btn,   &QPushButton::clicked, this, &QDialog::reject);
	connect(ok_btn,       &QPushButton::clicked, this, &QDialog::accept);
}

void FilenameTemplateDialog::onBrowseDirectory() {
	QString start = m_dir_edit->text();
	if (start.isEmpty()) start = QDir::toNativeSeparators(QDir::homePath());
	QString d = QFileDialog::getExistingDirectory(this, tr("Select output data directory..."), start,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!d.isEmpty()) m_dir_edit->setText(d);
}

void FilenameTemplateDialog::onRestoreDefaults() {
	m_dir_edit->setText(QDir::toNativeSeparators(QDir::homePath()));
	m_template_edit->setText(QString(DEFAULT_FILENAME_TEMPLATE));
}

void FilenameTemplateDialog::addPlaceholderEntries(QGridLayout *grid, const QFont &boldFont, const QFont &smallFont) {
	struct PlaceholderEntry { const char *placeholder; const char *desc; };
	static const PlaceholderEntry entries[] = {
		{ "%o",   "object name"                   },
		{ "%F",   "frame type: Light, Dark, etc." },
		{ "%C",   "filter name: R, G, Ha, etc."   },
		{ "%D",   "date: YYYYMMDD"                },
		{ "%-D",  "date: YYYY-MM-DD"              },
		{ "%.D",  "date: YYYY.MM.DD"              },
		{ "%H",   "time: HHMMSS"                  },
		{ "%-H",  "time: HH-MM-SS"                },
		{ "%.H",  "time: HH.MM.SS"                },
		{ "%T",   "sensor temperature (\u00b0C)"  },
		{ "%E",   "exposure time"                 },
		{ "%nE",  "exposure, n decimal digits"    },
		{ "%G",   "gain"                          },
		{ "%O",   "offset"                        },
		{ "%R",   "resolution (WxH)"              },
		{ "%B",   "binning"                       },
		{ "%P",   "focuser position"              },
	};
	const int N = (int)(sizeof(entries) / sizeof(entries[0]));
	const int half = (N + 1) / 2;

	for (int col = 0; col < 2; col++) {
		int start      = (col == 0) ? 0    : half;
		int count      = (col == 0) ? half : N - half;
		int baseColumn = (col == 0) ? 0    : 3;
		for (int i = 0; i < count; i++) {
			QLabel *placeholder_label = new QLabel(entries[start + i].placeholder);
			placeholder_label->setFont(boldFont);
			placeholder_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
			QLabel *desc_label = new QLabel(QString(" ") + entries[start + i].desc);
			desc_label->setFont(smallFont);
			grid->addWidget(placeholder_label, i, baseColumn,     Qt::AlignLeft | Qt::AlignVCenter);
			grid->addWidget(desc_label,        i, baseColumn + 1, Qt::AlignLeft | Qt::AlignVCenter);
		}
	}
}
