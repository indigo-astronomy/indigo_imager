// Copyright (c) 2020 Rumen G.Bogdanovski
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
#include "propertycache.h"
#include "conf.h"

void ImagerWindow::create_focuser_tab(QFrame *focuser_frame) {
	QGridLayout *focuser_frame_layout = new QGridLayout();
	focuser_frame_layout->setAlignment(Qt::AlignTop);
	focuser_frame->setLayout(focuser_frame_layout);
	focuser_frame->setFrameShape(QFrame::StyledPanel);
	focuser_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	focuser_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	// Focuser selection
	row++;
	QLabel *label = new QLabel("Focuser:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(label, row, 0);
	m_focuser_select = new QComboBox();
	focuser_frame_layout->addWidget(m_focuser_select, row, 1, 1, 3);
	connect(m_focuser_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_selected);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	focuser_frame_layout->addItem(spacer, row, 0);

	// Star Selection
	row++;
	label = new QLabel("Star X:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_star_x = new QSpinBox();
	m_star_x->setMaximum(100000);
	m_star_x->setMinimum(0);
	m_star_x->setValue(0);
	//m_star_x->setEnabled(false);
	focuser_frame_layout->addWidget(m_star_x , row, 1);
	connect(m_star_x, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_selection_changed);

	label = new QLabel("Star Y:");
	focuser_frame_layout->addWidget(label, row, 2);
	m_star_y = new QSpinBox();
	m_star_y->setMaximum(100000);
	m_star_y->setMinimum(0);
	m_star_y->setValue(0);
	//m_star_y->setEnabled(false);
	focuser_frame_layout->addWidget(m_star_y, row, 3);
	connect(m_star_y, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_selection_changed);

	// Exposure time
	row++;
	label = new QLabel("Exposure (s):");
	focuser_frame_layout->addWidget(label, row, 0);
	m_focuser_exposure_time = new QDoubleSpinBox();
	m_focuser_exposure_time->setMaximum(10000);
	m_focuser_exposure_time->setMinimum(0);
	m_focuser_exposure_time->setValue(1);
	focuser_frame_layout->addWidget(m_focuser_exposure_time, row, 1);

	label = new QLabel("Method:");
	focuser_frame_layout->addWidget(label, row, 2);
	m_focus_method_select = new QComboBox();
	m_focus_method_select->addItem("Manual");
	m_focus_method_select->addItem("Auto");
	focuser_frame_layout->addWidget(m_focus_method_select, row, 3);
	//connect(focus_method, QOverload<int>::of(&QComboBox::activate), this, &ImagerWindow::on_focuser_selected);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	focuser_frame_layout->addItem(spacer, row, 0);

	row++;
	label = new QLabel("Autofocus setings:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	label = new QLabel("Initial Step:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_initial_step = new QSpinBox();
	m_initial_step->setMaximum(1000000);
	m_initial_step->setMinimum(0);
	m_initial_step->setValue(0);
	//m_initial_step->setEnabled(false);
	focuser_frame_layout->addWidget(m_initial_step , row, 1);

	label = new QLabel("Final step:");
	focuser_frame_layout->addWidget(label, row, 2);
	m_final_step = new QSpinBox();
	m_final_step->setMaximum(100000);
	m_final_step->setMinimum(0);
	m_final_step->setValue(0);
	//m_final_step->setEnabled(false);
	focuser_frame_layout->addWidget(m_final_step, row, 3);

	row++;
	label = new QLabel("Backlash:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_focus_backlash = new QSpinBox();
	m_focus_backlash->setMaximum(1000000);
	m_focus_backlash->setMinimum(0);
	m_focus_backlash->setValue(0);
	//m_focus_backlash->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_backlash, row, 1);

	label = new QLabel("Stacking:");
	focuser_frame_layout->addWidget(label, row, 2);
	m_focus_stack = new QSpinBox();
	m_focus_stack->setMaximum(100000);
	m_focus_stack->setMinimum(0);
	m_focus_stack->setValue(0);
	//m_focus_stack->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_stack, row, 3);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	focuser_frame_layout->addItem(spacer, row, 0);

	row++;
	label = new QLabel("Manual focuser control:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	label = new QLabel("Absoute Position:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 2);
	m_focus_position = new QSpinBox();
	m_focus_position->setMaximum(1000000);
	m_focus_position->setMinimum(-1000000);
	m_focus_position->setValue(0);
	m_focus_position->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_position, row, 2, 1, 2);

	row++;
	label = new QLabel("Move:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 2);
	m_focus_steps = new QSpinBox();
	m_focus_steps->setMaximum(100000);
	m_focus_steps->setMinimum(0);
	m_focus_steps->setValue(0);
	m_focus_steps->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_steps, row, 2, 1, 2);

/*
	QPushButton *but = new QPushButton("In");
	but->setStyleSheet("min-width: 30px");
	//but->setIcon(QIcon(":resource/stop.png"));
	focuser_frame_layout->addWidget(but, row, 2);
	connect(but, &QPushButton::clicked, this, &ImagerWindow::on_abort);

	but = new QPushButton("Out");
	but->setStyleSheet("min-width: 30px");
	//but->setIcon(QIcon(":resource/stop.png"));
	focuser_frame_layout->addWidget(but);
	connect(but, &QPushButton::clicked, this, &ImagerWindow::on_abort);
*/
	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	focuser_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	QPushButton *but = new QPushButton("In");
	but->setStyleSheet("min-width: 15px");
	//but->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(but);
	connect(but, &QPushButton::clicked, this, &ImagerWindow::on_focus_in);

	but = new QPushButton("Out");
	but->setStyleSheet("min-width: 15px");
	//but->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(but);
	connect(but, &QPushButton::clicked, this, &ImagerWindow::on_focus_out);

	QPushButton *button = new QPushButton("Abort");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_abort);

	button = new QPushButton("Focus");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_focus_start);

	button = new QPushButton("Preview");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_preview);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	focuser_frame_layout->addItem(spacer, row, 0);
	//focuser_frame_layout->setRowStretch(row, 1 );

	row++;
	label = new QLabel("Drift (X, Y):");
	focuser_frame_layout->addWidget(label, row, 0);
	m_drift_label = new QLabel();
	m_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(m_drift_label, row, 1, 1, 2);

	row++;
	label = new QLabel("FWHM:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_FWHM_label = new QLabel();
	m_FWHM_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(m_FWHM_label, row, 1, 1, 2);

	row++;
	label = new QLabel("HFD:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_HFD_label = new QLabel();
	m_HFD_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(m_HFD_label, row, 1, 1, 2);

	row++;
	label = new QLabel("Peak:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_peak_label = new QLabel();
	m_peak_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(m_peak_label, row, 1, 1, 2);
}

void ImagerWindow::on_focuser_selected(int index) {
	static char selected_focuser[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
	QString q_focuser_str = m_focuser_select->currentText();
	int idx = q_focuser_str.indexOf(" @ ");
	if (idx >=0) q_focuser_str.truncate(idx);
	if (q_focuser_str.compare("No focuser") == 0) {
		strcpy(selected_focuser, "NONE");
	} else {
		strncpy(selected_focuser, q_focuser_str.toUtf8().constData(), INDIGO_NAME_SIZE);
	}
	get_selected_agent(selected_agent);

	indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_focuser);
	static const char * items[] = { selected_focuser };
	static bool values[] = { true };
	indigo_change_switch_property(nullptr, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME, 1, items, values);
}


void ImagerWindow::on_selection_changed(int value) {
	int x = m_star_x->value();
	int y = m_star_y->value();
	m_viewer->moveSelection(x, y);
	m_HFD_label->setText("n/a");
	m_FWHM_label->setText("n/a");
	m_peak_label->setText("n/a");
	m_drift_label->setText("n/a");
}


void ImagerWindow::on_image_right_click(int x, int y) {
	m_star_x->setValue(x);
	m_star_y->setValue(y);
}


void ImagerWindow::on_focus_start(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);


	change_agent_star_selection(selected_agent);
	change_agent_batch_property(selected_agent);
	change_agent_focus_params_property(selected_agent);
	change_ccd_frame_property(selected_agent);
	m_preview = true;
	m_focusing = true;
	if(m_focus_method_select->currentIndex() == 0) {
		change_agent_start_preview_property(selected_agent);
	} else {
		change_agent_start_focusing_property(selected_agent);
	}
}

void ImagerWindow::on_focus_in(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);

	change_focuser_focus_in_property(selected_agent);
	change_focuser_steps_property(selected_agent);
}

void ImagerWindow::on_focus_out(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);

	change_focuser_focus_out_property(selected_agent);
	change_focuser_steps_property(selected_agent);
}
