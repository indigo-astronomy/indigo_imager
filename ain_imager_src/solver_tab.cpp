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

//#include "imagerwindow.h"
//#include "propertycache.h"
//#include "conf.h"
//#include "widget_state.h"
//#include "utils.h"

#include <imagerwindow.h>
#include <propertycache.h>
#include <conf.h>
#include <logger.h>

void write_conf();

void ImagerWindow::create_solver_tab(QFrame *solver_frame) {
	QSpacerItem *spacer;

	QGridLayout *solver_frame_layout = new QGridLayout();
	solver_frame_layout->setAlignment(Qt::AlignTop);
	solver_frame_layout->setColumnStretch(0, 1);
	solver_frame_layout->setColumnStretch(1, 1);
	solver_frame_layout->setColumnStretch(2, 1);
	solver_frame_layout->setColumnStretch(3, 1);

	solver_frame->setLayout(solver_frame_layout);
	solver_frame->setFrameShape(QFrame::StyledPanel);
	solver_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	solver_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;

	m_agent_solver_select = new QComboBox();
	solver_frame_layout->addWidget(m_agent_solver_select, row, 0, 1, 4);
	connect(m_agent_solver_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImagerWindow::on_solver_agent_selected);

	row++;
	QLabel *label = new QLabel("Image source:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_source_select1 = new QComboBox();
	solver_frame_layout->addWidget(m_solver_source_select1, row, 2, 1, 2);
	connect(m_solver_source_select1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImagerWindow::on_image_source1_selected);

	row++;
	// Exposure time
	label = new QLabel("Exposure time (s):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_exposure1 = new QDoubleSpinBox();
	m_solver_exposure1->setMaximum(10000);
	m_solver_exposure1->setMinimum(0);
	m_solver_exposure1->setValue(1);
	solver_frame_layout->addWidget(m_solver_exposure1, row, 2);

	m_solve_button = new QPushButton("Solve");
	m_solve_button->setStyleSheet("min-width: 30px");
	m_solve_button->setIcon(QIcon(":resource/play.png"));
	solver_frame_layout->addWidget(m_solve_button, row, 3);
	connect(m_solve_button, &QPushButton::clicked, this, &ImagerWindow::on_trigger_solve);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	solver_frame_layout->addItem(spacer, row, 0, 1, 4);

	QFont font;
	row++;
	label = new QLabel("WCS solution:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	solver_frame_layout->addWidget(label, row, 0, 1, 2);

	//row++;
	m_solver_status_label1 = new QLabel("");
	m_solver_status_label1->setTextFormat(Qt::RichText);
	m_solver_status_label1->setText("<img src=\":resource/led-grey.png\"> Idle");
	//set_idle(m_solver_status_label);
	solver_frame_layout->addWidget(m_solver_status_label1, row, 2, 1, 2);

	row++;
	label = new QLabel("Frame center RA :");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_ra_solution = new QLabel("");
	m_solver_ra_solution->setAlignment(Qt::AlignCenter);
	font = m_solver_ra_solution->font();
	font.setPointSize(font.pointSize() + 2);
	m_solver_ra_solution->setFont(font);
	set_ok(m_solver_ra_solution);
	solver_frame_layout->addWidget(m_solver_ra_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Frame center Dec :");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_dec_solution = new QLabel("");
	m_solver_dec_solution->setAlignment(Qt::AlignCenter);
	font = m_solver_dec_solution->font();
	font.setPointSize(font.pointSize() + 2);
	m_solver_dec_solution->setFont(font);
	set_ok(m_solver_dec_solution);
	solver_frame_layout->addWidget(m_solver_dec_solution, row, 2, 1, 2);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	solver_frame_layout->addItem(spacer, row, 0, 1, 4);

	row++;
	label = new QLabel("Frame size:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_fsize_solution = new QLabel("");
	set_ok(m_solver_fsize_solution);
	solver_frame_layout->addWidget(m_solver_fsize_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Rotation angle (E of N):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_angle_solution = new QLabel("");
	set_idle(m_solver_angle_solution);
	solver_frame_layout->addWidget(m_solver_angle_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Scale:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_scale_solution = new QLabel("");
	set_ok(m_solver_scale_solution);
	solver_frame_layout->addWidget(m_solver_scale_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Parity:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_parity_solution = new QLabel("");
	set_ok(m_solver_parity_solution);
	solver_frame_layout->addWidget(m_solver_parity_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Used index file:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_usedindex_solution = new QLabel("");
	set_ok(m_solver_usedindex_solution);
	solver_frame_layout->addWidget(m_solver_usedindex_solution, row, 2, 1, 2);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	solver_frame_layout->addItem(spacer, row, 0, 1, 4);

	row++;
	label = new QLabel("Solver hints:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	solver_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;

	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(0,0,0,0);
	toolbox->setContentsMargins(0,0,0,0);
	solver_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	label = new QLabel("RA (h) / Dec (°):");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	toolbox->addWidget(label);

	m_solver_ra_hint = new QLineEdit();
	m_solver_ra_hint->setEnabled(false);
	toolbox->addWidget(m_solver_ra_hint);

	m_solver_dec_hint = new QLineEdit();
	m_solver_dec_hint->setEnabled(false);
	toolbox->addWidget(m_solver_dec_hint);

	QPushButton *button = new QPushButton("Set");
	button->setStyleSheet("min-width: 25px");
	//button->setIcon(QIcon(":resource/calibrate.png"));
	toolbox->addWidget(button);
	connect(button , &QPushButton::clicked, this, &ImagerWindow::on_solver_ra_dec_hints_changed);

	row++;
	label = new QLabel("Search radius (°):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_radius_hint = new QDoubleSpinBox();
	m_solver_radius_hint->setMaximum(180);
	m_solver_radius_hint->setMinimum(0);
	m_solver_radius_hint->setValue(0);
	m_solver_radius_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_radius_hint, row, 3);
	connect(m_solver_radius_hint, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, QOverload<double>::of(&ImagerWindow::on_solver_hints_changed));

	row++;
	label = new QLabel("Downsample:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_ds_hint = new QSpinBox();
	m_solver_ds_hint->setMaximum(16);
	m_solver_ds_hint->setMinimum(0);
	m_solver_ds_hint->setValue(0);
	m_solver_ds_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_ds_hint, row, 3);
	connect(m_solver_ds_hint, QOverload<int>::of(&QSpinBox::valueChanged), this, QOverload<int>::of(&ImagerWindow::on_solver_hints_changed));

	row++;
	label = new QLabel("Parity:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_parity_hint = new QSpinBox();
	m_solver_parity_hint->setMaximum(16);
	m_solver_parity_hint->setMinimum(0);
	m_solver_parity_hint->setValue(0);
	m_solver_parity_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_parity_hint, row, 3);
	connect(m_solver_parity_hint, QOverload<int>::of(&QSpinBox::valueChanged), this, QOverload<int>::of(&ImagerWindow::on_solver_hints_changed));

	row++;
	label = new QLabel("Depth:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_depth_hint = new QSpinBox();
	m_solver_depth_hint->setMaximum(16);
	m_solver_depth_hint->setMinimum(0);
	m_solver_depth_hint->setValue(0);
	m_solver_depth_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_depth_hint, row, 3);
	connect(m_solver_depth_hint, QOverload<int>::of(&QSpinBox::valueChanged), this, QOverload<int>::of(&ImagerWindow::on_solver_hints_changed));

	row++;
	label = new QLabel("Time Limit (s):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_tlimit_hint = new QSpinBox();
	m_solver_tlimit_hint->setMaximum(16);
	m_solver_tlimit_hint->setMinimum(0);
	m_solver_tlimit_hint->setValue(0);
	m_solver_tlimit_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_tlimit_hint, row, 3);
	connect(m_solver_tlimit_hint, QOverload<int>::of(&QSpinBox::valueChanged), this, QOverload<int>::of(&ImagerWindow::on_solver_hints_changed));
}

void ImagerWindow::update_solver_widgets_at_start(const char *imager_agent, const char *solver_agent) {
	bool done = false;
	int wait_busy = 25;
	set_text(m_solver_status_label1, "<img src=\":resource/led-orange.png\"> Witing for image");
	set_text(m_solver_status_label2, "<img src=\":resource/led-orange.png\"> Witing for image");
	set_widget_state(m_mount_solve_and_sync_button, INDIGO_BUSY_STATE);
	set_widget_state(m_mount_solve_and_center_button, INDIGO_BUSY_STATE);
	set_widget_state(m_solve_button, INDIGO_BUSY_STATE);
	do {
		indigo_property *exp = properties.get((char*)imager_agent, CCD_EXPOSURE_PROPERTY_NAME);
		if (wait_busy) {
			if (exp && exp->state == INDIGO_BUSY_STATE) {
				done = true;
			} else {
				indigo_usleep(100000);
				wait_busy --;
				indigo_debug("%s(): WAIT_BUSY = %d", __FUNCTION__, wait_busy);
			}
		}
		if (wait_busy == 0) {
			done = true;
			indigo_property *wcs = properties.get((char*)solver_agent, AGENT_PLATESOLVER_WCS_PROPERTY_NAME);
			if (wcs && wcs->state != INDIGO_BUSY_STATE) {
				set_widget_state(m_mount_solve_and_sync_button, INDIGO_OK_STATE);
				set_widget_state(m_mount_solve_and_center_button, INDIGO_OK_STATE);
				set_widget_state(m_solve_button, INDIGO_OK_STATE);
				set_text(m_solver_status_label1, "<img src=\":resource/led-red.png\"> No image");
				set_text(m_solver_status_label2, "<img src=\":resource/led-red.png\"> No image");
				Logger::instance().log(NULL, "Solve failed, exposure did not start");
			}
		}
	} while (!done);
	indigo_debug("%s(): DONE", __FUNCTION__);
}

void ImagerWindow::on_solver_agent_selected(int index) {
	QtConcurrent::run([=]() {
		// Clear controls
		indigo_property *property = (indigo_property*)malloc(sizeof(indigo_property));
		memset(property, 0, sizeof(indigo_property));
		get_selected_solver_agent(property->device);
		property_delete(property, nullptr);
		free(property);

		indigo_enumerate_properties(nullptr, &INDIGO_ALL_PROPERTIES);
	});
}

void ImagerWindow::on_solver_ra_dec_hints_changed(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_solver_agent(selected_agent);

		change_solver_agent_hints_property(selected_agent);
	});
}

void ImagerWindow::on_solver_hints_changed(int value) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_solver_agent(selected_agent);

		change_solver_agent_hints_property(selected_agent);
	});
}

void ImagerWindow::on_solver_hints_changed(double value) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_solver_agent(selected_agent);

		change_solver_agent_hints_property(selected_agent);
	});
}

void ImagerWindow::on_trigger_solve() {
	trigger_solve();
}

void ImagerWindow::on_image_source1_selected(int index) {
	QString solver_source = m_solver_source_select1->currentText();
	strncpy(conf.solver_image_source1, solver_source.toUtf8().constData(), INDIGO_NAME_SIZE);
	write_conf();
	show_selected_preview_in_solver_tab(solver_source);
	indigo_debug("%s -> %s\n", __FUNCTION__, conf.solver_image_source1);
}
