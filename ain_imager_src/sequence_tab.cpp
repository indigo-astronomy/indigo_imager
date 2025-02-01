// Copyright (c) 2023 Rumen G.Bogdanovski
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

#include "imagerwindow.h"
#include "indigoclient.h"
#include "propertycache.h"
#include <conf.h>
#include <utils.h>

void ImagerWindow::create_sequence_tab(QFrame *sequence_frame) {
	QGridLayout *sequence_frame_layout = new QGridLayout();
	sequence_frame_layout->setAlignment(Qt::AlignTop);
	sequence_frame->setLayout(sequence_frame_layout);
	sequence_frame->setFrameShape(QFrame::StyledPanel);
	sequence_frame->setMinimumWidth(TOOLBAR_MIN_WIDTH);
	sequence_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	QLabel *label = new QLabel("Image preview:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	sequence_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	m_seq_imager_viewer = new ImageViewer(this);
	m_seq_imager_viewer->showStretchButton(false);
	m_seq_imager_viewer->showZoomButtons(false);
	m_seq_imager_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	sequence_frame_layout->addWidget(m_seq_imager_viewer, row, 0, 1, 4);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	sequence_frame_layout->addItem(spacer, row, 0);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	sequence_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	m_seq_start_button = new QPushButton("Run");
	m_seq_start_button->setStyleSheet("min-width: 30px");
	m_seq_start_button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(m_seq_start_button);
	connect(m_seq_start_button, &QPushButton::clicked, this, &ImagerWindow::on_sequence_start_stop);

	m_seq_pause_button = new QPushButton("Pause");
	toolbox->addWidget(m_seq_pause_button);
	m_seq_pause_button->setStyleSheet("min-width: 30px");
	m_seq_pause_button->setIcon(QIcon(":resource/pause.png"));
	connect(m_seq_pause_button, &QPushButton::clicked, this, &ImagerWindow::on_pause);

	QPushButton *button = new QPushButton("Abort");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_abort);


	row++;
	spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	sequence_frame_layout->addItem(spacer, row, 0);

	row++;
	label = new QLabel("Sequence progress:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	sequence_frame_layout->addWidget(label, row, 0, 1, 2);
	m_imager_status_label = new QLabel("<img src=\":resource/led-grey.png\"> Idle");
	sequence_frame_layout->addWidget(m_imager_status_label, row, 2, 1, 2);

	row++;
	m_seq_exposure_progress = new QProgressBar();
	sequence_frame_layout->addWidget(m_seq_exposure_progress, row, 0, 1, 4);
	m_seq_exposure_progress->setFormat("Exposure: Idle");
	m_seq_exposure_progress->setMaximum(1);
	m_seq_exposure_progress->setValue(0);

	row++;
	m_seq_batch_progress = new QProgressBar();
	sequence_frame_layout->addWidget(m_seq_batch_progress, row, 0, 1, 4);
	m_seq_batch_progress->setMaximum(1);
	m_seq_batch_progress->setValue(0);
	m_seq_batch_progress->setFormat("Batch: Idle");

	row++;
	m_seq_sequence_progress = new QProgressBar();
	sequence_frame_layout->addWidget(m_seq_sequence_progress, row, 0, 1, 4);
	m_seq_sequence_progress->setMaximum(1);
	m_seq_sequence_progress->setValue(0);
	m_seq_sequence_progress->setFormat("Sequence: Idle");

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	sequence_frame_layout->addItem(spacer, row, 0);

	row++;
	m_seq_esimated_duration = new QLabel(QString("Sequence duration: ") + indigo_dtos(0, "%02d:%02d:%02.0f"));
	m_seq_esimated_duration->setToolTip("This is approximate sequence duration as download, focusing, filter change etc., times are unpredicatble.");
	sequence_frame_layout->addWidget(m_seq_esimated_duration, row, 0, 1, 4);
}

void ImagerWindow::on_sequence_updated() {
	double duration = 0; // m_sequence_editor->approximate_duration();
	m_seq_esimated_duration->setText(QString("Sequence duration: ") + QString(indigo_dtos(duration, "%02d:%02d:%02.0f")));
}

void ImagerWindow::on_sequence_name_changed(const QString &object_name) {
	if (!m_is_sequence) {
		return;
	}
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_ccd_localmode_property(selected_agent, object_name);
		add_fits_keyword_string(selected_agent, "OBJECT", object_name);
	});
}

void ImagerWindow::on_request_sequence() {
	indigo_debug("Sequence requested");
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_imager_agent(selected_agent);

	QString name;
	QString sequence;
	QList<QString> batches;

	indigo_property *p = properties.get(selected_agent, CCD_FITS_HEADERS_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (client_match_item(&p->items[i], "OBJECT")) {
				name = QString(p->items[i].text.value).trimmed().remove("\'");
				break;
			}
		}
	}

	p = properties.get(selected_agent, AGENT_IMAGER_SEQUENCE_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (client_match_item(&p->items[i], AGENT_IMAGER_SEQUENCE_ITEM_NAME)) {
				sequence = indigo_get_text_item_value(&p->items[i]);
			} else {
				QString batch(indigo_get_text_item_value(&p->items[i]));
				if (!batch.isEmpty()) {
					batches.append(batch);
				};
			}
		}
		//m_sequence_editor->set_sequence(name, sequence, batches);
	}
	double duration = 0; //m_sequence_editor->approximate_duration();
	m_seq_esimated_duration->setText(QString("Sequence duration: ") + indigo_dtos(duration, "%02d:%02d:%02.0f"));
}

void ImagerWindow::on_sequence_start_stop(bool clicked) {
	//m_sequence_editor->clear_selection();
	exposure_start_stop(clicked, true);
}