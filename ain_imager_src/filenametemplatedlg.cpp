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

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include "filenametemplatedlg.h"
#include "conf.h"

namespace {
struct PlaceholderEntry { const char *placeholder; const char *desc; };
static const PlaceholderEntry k_placeholders[] = {
	{ "%o",  "Object name"                   },
	{ "%F",  "Frame type"                    },
	{ "%C",  "Filter name"                   },
	{ "%D",  "Date: YYYYMMDD"                },
	{ "%-D", "Date: YYYY-MM-DD"              },
	{ "%.D", "Date: YYYY.MM.DD"              },
	{ "%H",  "Time: HHMMSS"                  },
	{ "%-H", "Time: HH-MM-SS"                },
	{ "%.H", "Time: HH.MM.SS"                },
	{ "%T",  "Sensor temperature: \u00b0C"   },
	{ "%E",  "Exposure time: s"              },
	{ "%nE", "Exposure, n decimal digits"    },
	{ "%G",  "Gain"                          },
	{ "%O",  "Offset"                        },
	{ "%R",  "Resolution: WxH"               },
	{ "%B",  "Binning"                       },
	{ "%P",  "Focuser position"              },
};
static const int k_n = (int)(sizeof(k_placeholders) / sizeof(k_placeholders[0]));
} // namespace

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
	setWindowTitle(tr("Image output settings"));
	setMinimumWidth(600);

	QVBoxLayout *vbox = new QVBoxLayout(this);

	// Output directory + filename template rows, labels aligned in column 0
	m_dir_edit = new QLineEdit(initialDir);
	QPushButton *dir_btn = new QPushButton(tr("Browse..."));
	QLabel *out_label = new QLabel(tr("<b>Output directory:</b>"));
	out_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	m_template_edit = new QLineEdit(initialTemplate);
	QLabel *template_label = new QLabel(tr("<b>Filename template:</b>"));
	template_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	QGridLayout *fields_grid = new QGridLayout;
	fields_grid->setColumnStretch(1, 1);
	fields_grid->addWidget(out_label,       0, 0, Qt::AlignLeft | Qt::AlignVCenter);
	fields_grid->addWidget(m_dir_edit,      0, 1);
	fields_grid->addWidget(dir_btn,         0, 2);
	fields_grid->addWidget(template_label,  1, 0, Qt::AlignLeft | Qt::AlignVCenter);
	fields_grid->addWidget(m_template_edit, 1, 1, 1, 2);
	vbox->addLayout(fields_grid);

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

	//QLabel *note = new QLabel(tr("<i><b>Note:</b> <b>%nI</b> and <b>%M</b> (MD5 hash) are always appended automatically as <b>_idx%3I_%M</b></i>"));
	//note->setFont(small_font);
	//note->setWordWrap(true);
	//placeholder_vbox->addWidget(note);

	vbox->addWidget(placeholder_box);

	// Buttons: Defaults on left, OK/Cancel on right
	QHBoxLayout *buttons = new QHBoxLayout;
	QPushButton *defaults_btn = new QPushButton(tr("Restore defaults"));
	QPushButton *ok_btn       = new QPushButton(tr("OK"));
	QPushButton *cancel_btn   = new QPushButton(tr("Cancel"));
	buttons->addWidget(defaults_btn);
	buttons->addStretch();
	buttons->addWidget(ok_btn);
	buttons->addWidget(cancel_btn);
	vbox->addLayout(buttons);

	m_template_edit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_template_edit, &QWidget::customContextMenuRequested, this, &FilenameTemplateDialog::onTemplateContextMenu);

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

void FilenameTemplateDialog::onTemplateContextMenu(const QPoint &pos) {
	int click_pos = m_template_edit->cursorPositionAt(pos);
	QMenu *menu = m_template_edit->createStandardContextMenu();
	menu->addSeparator();
	QMenu *insert_menu = menu->addMenu(tr("Insert placeholder"));
	for (int i = 0; i < k_n; i++) {
		const auto &e = k_placeholders[i];
		// %nE is excluded from the context menu as it requires a number and is less convenient to insert via menu. Users can still type it manually if needed.
		if (QLatin1String(e.placeholder) == QLatin1String("%nE")) continue;
		QString ph = e.placeholder;
		QString label = QString("%2 (%1)").arg(ph).arg(e.desc);
		QAction *act = insert_menu->addAction(label);
		connect(act, &QAction::triggered, this, [this, ph, click_pos]() {
			m_template_edit->setCursorPosition(click_pos);
			m_template_edit->insert(ph);
		});
	}

	menu->addSeparator();
	QAction *default_act = menu->addAction(tr("Restore default template"));
	connect(default_act, &QAction::triggered, this, [this]() {
		m_template_edit->setText(QString(DEFAULT_FILENAME_TEMPLATE));
	});

	menu->exec(m_template_edit->mapToGlobal(pos));
	delete menu;
}

void FilenameTemplateDialog::addPlaceholderEntries(QGridLayout *grid, const QFont &boldFont, const QFont &smallFont) {
	const int half = (k_n + 1) / 2;

	for (int col = 0; col < 2; col++) {
		int start      = (col == 0) ? 0    : half;
		int count      = (col == 0) ? half : k_n - half;
		int baseColumn = (col == 0) ? 0    : 3;
		for (int i = 0; i < count; i++) {
			QLabel *placeholder_label = new QLabel(k_placeholders[start + i].placeholder);
			placeholder_label->setFont(boldFont);
			placeholder_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
			QLabel *desc_label = new QLabel(QString(" ") + k_placeholders[start + i].desc);
			desc_label->setFont(smallFont);
			grid->addWidget(placeholder_label, i, baseColumn,     Qt::AlignLeft | Qt::AlignVCenter);
			grid->addWidget(desc_label,        i, baseColumn + 1, Qt::AlignLeft | Qt::AlignVCenter);
		}
	}
}
