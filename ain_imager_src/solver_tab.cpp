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
#include <indigoclient.h>
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
	solver_frame->setMinimumWidth(TOOLBAR_MIN_WIDTH);
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
	label = new QLabel("Filter:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_filter_select = new QComboBox();
	m_solver_filter_select->setToolTip("Filter used for solving, offered when the image source is an Imager Agent with a filter wheel");
	m_solver_filter_select->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_filter_select, row, 2, 1, 2);
	connect(m_solver_filter_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_solver_filter_selected);

	row++;
	// Exposure time
	label = new QLabel("Exposure time (s):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);

	m_solver_exposure1 = new QDoubleSpinBox();
	m_solver_exposure1->setDecimals(3);
	m_solver_exposure1->setMaximum(1000);
	m_solver_exposure1->setMinimum(0);
	m_solver_exposure1->setValue(1);
	m_solver_exposure1->setEnabled(false);
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

	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(0,0,0,0);
	toolbox->setContentsMargins(0,0,0,0);
	solver_frame_layout->addWidget(toolbar, row, 2, 1, 2);

	m_solver_status_label1 = new QLabel("");
	m_solver_status_label1->setTextFormat(Qt::RichText);
	m_solver_status_label1->setText("<img src=\":resource/led-grey.png\"> Idle");
	//set_idle(m_solver_status_label);
	toolbox->addWidget(m_solver_status_label1);

	m_load_coords_button = new QToolButton(this);
	m_load_coords_button->setToolTip(tr("Load solved coordinates in the mount RA / Dec input fileds"));
	m_load_coords_button->setIcon(QIcon(":resource/guide.png"));
	m_load_coords_button->setEnabled(false);
	toolbox->addWidget(m_load_coords_button);
	connect(m_load_coords_button, &QToolButton::clicked, this, &ImagerWindow::on_solver_load_coords);

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

	toolbar = new QWidget;
	toolbox = new QHBoxLayout(toolbar);
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
	m_solver_radius_hint->setSpecialValueText("no limit");
	solver_frame_layout->addWidget(m_solver_radius_hint, row, 3);
	connect(m_solver_radius_hint, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, QOverload<double>::of(&ImagerWindow::on_solver_hints_changed));

	row++;
	label = new QLabel("Pixel scale (\"/px):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_scale_hint = new QDoubleSpinBox();
	m_solver_scale_hint->setDecimals(3);
	m_solver_scale_hint->setMaximum(7200);
	m_solver_scale_hint->setMinimum(-0.1);
	m_solver_scale_hint->setSingleStep(0.1);
	m_solver_scale_hint->setValue(0);
	m_solver_scale_hint->setSpecialValueText("camera");
	m_solver_scale_hint->setToolTip("Pixel scale:<br> = 0: automatic (slow)<br> &lt; 0: use pixel scale from camera<br> &gt; 0: use specified pixel scale");
	m_solver_scale_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_scale_hint, row, 3);
	connect(m_solver_scale_hint, &QDoubleSpinBox::editingFinished, this,QOverload<>::of(&ImagerWindow::on_solver_hints_changed));

	row++;
	label = new QLabel("Downsample:");
	solver_frame_layout->addWidget(label, row, 0, 1, 3);
	m_solver_ds_hint = new QSpinBox();
	m_solver_ds_hint->setMaximum(16);
	m_solver_ds_hint->setMinimum(0);
	m_solver_ds_hint->setValue(0);
	m_solver_ds_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_ds_hint, row, 3);
	connect(m_solver_ds_hint, QOverload<int>::of(&QSpinBox::valueChanged), this, QOverload<int>::of(&ImagerWindow::on_solver_hints_changed));

	row++;
	label = new QLabel("Parity:");
	solver_frame_layout->addWidget(label, row, 0, 1, 3);
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
	m_solver_depth_hint->setSpecialValueText("no limit");
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
	int wait_busy = 40; // wait 4s to start exposure
	set_text(m_solver_status_label1, "<img src=\":resource/led-orange.png\"> Starting process");
	set_text(m_solver_status_label2, "<img src=\":resource/led-orange.png\"> Starting process");
	set_widget_state(m_mount_solve_and_sync_button, INDIGO_BUSY_STATE);
	set_widget_state(m_mount_solve_and_center_button, INDIGO_BUSY_STATE);
	set_widget_state(m_mount_start_pa_button, INDIGO_BUSY_STATE);
	set_widget_state(m_mount_recalculate_pe_button, INDIGO_BUSY_STATE);
	set_widget_state(m_solve_button, INDIGO_BUSY_STATE);
	do {
		indigo_property *exp = properties.get((char*)imager_agent, CCD_EXPOSURE_PROPERTY_NAME);
		indigo_property *proc = properties.get((char*)solver_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		indigo_property *solution = properties.get((char*)solver_agent, AGENT_PLATESOLVER_WCS_PROPERTY_NAME);
		if (wait_busy) {
			if ((exp && exp->state == INDIGO_BUSY_STATE) || (proc && proc->state == INDIGO_BUSY_STATE) || (solution && solution->state == INDIGO_BUSY_STATE)){
				done = true;
			} else {
				indigo_usleep(100000);
				wait_busy --;
				indigo_debug("%s(): WAIT_BUSY = %d", __FUNCTION__, wait_busy);
			}
		}
		if (wait_busy == 0) {
			done = true;
			indigo_property *proc = properties.get((char*)solver_agent, AGENT_START_PROCESS_PROPERTY_NAME);
			indigo_property *solution = properties.get((char*)solver_agent, AGENT_PLATESOLVER_WCS_PROPERTY_NAME);
			if ((proc && proc->state != INDIGO_BUSY_STATE) && (solution && solution->state != INDIGO_BUSY_STATE)) {
				set_widget_state(m_mount_solve_and_sync_button, INDIGO_OK_STATE);
				set_widget_state(m_mount_solve_and_center_button, INDIGO_OK_STATE);
				set_widget_state(m_mount_start_pa_button, INDIGO_OK_STATE);
				set_widget_state(m_mount_recalculate_pe_button, INDIGO_OK_STATE);
				set_widget_state(m_solve_button, INDIGO_OK_STATE);
				set_text(m_solver_status_label1, "<img src=\":resource/led-red.png\"> Process failed");
				set_text(m_solver_status_label2, "<img src=\":resource/led-red.png\"> Process failed");
				Logger::instance().log(NULL, "Process failed to start");
			}
		}
	} while (!done);
	indigo_debug("%s(): DONE", __FUNCTION__);
}

bool ImagerWindow::open_image(QString file_name, int *image_size, unsigned char **image_data) {
	char msg[PATH_LEN];
	if (file_name == "") return false;
	FILE *file;
	strncpy(m_image_path, file_name.toUtf8().data(), PATH_LEN);
	file = fopen(m_image_path, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		*image_size = (size_t)ftell(file);
		fseek(file, 0, SEEK_SET);
		if (*image_data == nullptr) {
			*image_data = (unsigned char *)malloc(*image_size + 1);
		} else {
			*image_data = (unsigned char *)realloc(*image_data, *image_size + 1);
		}
		fread(*image_data, *image_size, 1, file);
		fclose(file);
	} else {
		snprintf(msg, PATH_LEN, "File '%s'\nCan not be open for reading.", QDir::toNativeSeparators(m_image_path).toUtf8().data());
		show_message("Error!", msg);
		return false;
	}

	char *image_formrat = strrchr(m_image_path, '.');
	const stretch_config_t sc = {(uint8_t)conf.preview_stretch_level, (uint8_t)conf.preview_color_balance, conf.preview_bayer_pattern};
	preview_image *image = create_preview(*image_data, *image_size, (const char*)image_formrat, sc);
	if (image) {
		QString key = QString(AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME);
		preview_cache.add(key, image);
		if (show_preview_in_imager_viewer(key)) {
			m_imager_viewer->setText(file_name);
			m_imager_viewer->setToolTip(file_name);
		}
	} else {
		snprintf(msg, PATH_LEN, "File: '%s'\nDoes not seem to be a supported image format.", QDir::toNativeSeparators(m_image_path).toUtf8().data());
		show_message("Error!", msg);
		return false;
	}
	return true;
}

static const char *solver_driver_name(int type) {
	return type == AIN_SOLVER_PLATESOLVER ? "indigo_agent_platesolver" : "indigo_agent_astrometry";
}

static const char *solver_agent_name(int type) {
	return type == AIN_SOLVER_PLATESOLVER ? "Platesolver Agent" : "Astrometry Agent";
}

static int other_solver(int type) {
	return type == AIN_SOLVER_PLATESOLVER ? AIN_SOLVER_ASTROMETRY : AIN_SOLVER_PLATESOLVER;
}

void ImagerWindow::on_solver_agent_astrometry() {
	switch_solver_agent(AIN_SOLVER_ASTROMETRY);
}

void ImagerWindow::on_solver_agent_platesolver() {
	switch_solver_agent(AIN_SOLVER_PLATESOLVER);
}

void ImagerWindow::sync_solver_agent_menu() {
	(m_solver_switch_target == AIN_SOLVER_PLATESOLVER ? m_solver_platesolver_act : m_solver_astrometry_act)->setChecked(true);
}

void ImagerWindow::switch_solver_agent(int type) {
	if (!m_solver_switch_pending.isEmpty()) {
		window_log("Solver agent switch is already in progress", INDIGO_ALERT_STATE);
		sync_solver_agent_menu();
		return;
	}
	if (type == m_solver_switch_target) return;

	for (int i = 0; i < m_agent_solver_select->count(); i++) {
		QByteArray agent = m_agent_solver_select->itemData(i).toString().toUtf8();
		indigo_property *process = properties.get(agent.constData(), AGENT_START_PROCESS_PROPERTY_NAME);
		indigo_property *wcs = properties.get(agent.constData(), AGENT_PLATESOLVER_WCS_PROPERTY_NAME);
		if ((process && process->state == INDIGO_BUSY_STATE) || (wcs && wcs->state == INDIGO_BUSY_STATE)) {
			QString message = QString("Can not switch the solver agent, '%1' is busy").arg(QString(agent));
			window_log(message.toUtf8().constData(), INDIGO_ALERT_STATE);
			sync_solver_agent_menu();
			return;
		}
	}

	m_solver_switch_target = type;
	m_solver_switch_succeeded = false;
	for (const QString &server : m_solver_servers) {
		m_solver_switch_succeeded |= request_solver_driver(server);
	}
	if (m_solver_switch_pending.isEmpty()) {
		finish_solver_switch();
	}
}

/* The solver agents refuse to start while the other one is loaded, so the new driver is
   requested only after the server reports the old one is gone - see update_solver_switch().
   Returns true if the wanted driver is already loaded, it is also called when a server connects. */
bool ImagerWindow::request_solver_driver(const QString &server) {
	indigo_property *p = properties.get(server.toUtf8().constData(), "DRIVERS");
	const char *wanted = solver_driver_name(m_solver_switch_target);
	const char *other = solver_driver_name(other_solver(m_solver_switch_target));

	if (p && indigo_get_switch(p, wanted)) return true;

	bool unload_first = p && indigo_get_switch(p, other);
	m_solver_switch_pending.insert(server, !unload_first);
	change_server_driver(server, unload_first ? other : wanted, !unload_first);
	m_solver_switch_timer->start();
	return false;
}

void ImagerWindow::change_server_driver(const QString &server, const char *driver, bool load) const {
	QByteArray device = server.toUtf8();
	QByteArray driver_name(driver);
	indigo_property *p = properties.get(device.constData(), "DRIVERS");
	// a server started with drivers on its command line lists only those, the others can still be loaded by name
	bool listed = p && indigo_get_item(p, driver);
	QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
		if (listed || !load) {
			indigo_change_switch_property_1(nullptr, device.constData(), "DRIVERS", driver_name.constData(), load);
		} else {
			const char *items[] = { "DRIVER" };
			const char *values[] = { driver_name.constData() };
			indigo_change_text_property(NULL, device.constData(), "LOAD", 1, items, values);
		}
	});
}

void ImagerWindow::update_solver_switch(indigo_property *property) {
	QString server(property->device);
	auto pending = m_solver_switch_pending.find(server);
	if (pending == m_solver_switch_pending.end()) return;

	const char *wanted = solver_driver_name(m_solver_switch_target);
	const char *other = solver_driver_name(other_solver(m_solver_switch_target));

	if (!pending.value()) {
		if (indigo_get_switch(property, other)) return;
		pending.value() = true;
		change_server_driver(server, wanted, true);
		m_solver_switch_timer->start();
		return;
	}
	if (!indigo_get_switch(property, wanted)) return;

	m_solver_switch_pending.erase(pending);
	m_solver_switch_succeeded = true;
	if (m_solver_switch_pending.isEmpty()) {
		finish_solver_switch();
	}
}

/* A refused unload or a failed load changes nothing the client can wait for, the server has already sent the reason. */
void ImagerWindow::solver_switch_timeout() {
	const char *other = solver_driver_name(other_solver(m_solver_switch_target));
	for (auto pending = m_solver_switch_pending.constBegin(); pending != m_solver_switch_pending.constEnd(); ++pending) {
		QString message = QString("Failed to switch to %1 on '%2'").arg(solver_agent_name(m_solver_switch_target), pending.key());
		window_log(message.toUtf8().constData(), INDIGO_ALERT_STATE);
		if (pending.value()) {
			// the old driver is already gone, do not leave the server without a solver
			change_server_driver(pending.key(), other, true);
		}
	}
	m_solver_switch_pending.clear();
	finish_solver_switch();
}

void ImagerWindow::solver_server_removed(const QString &server) {
	m_solver_servers.remove(server);
	if (m_solver_switch_pending.remove(server) && m_solver_switch_pending.isEmpty()) {
		finish_solver_switch();
	}
}

void ImagerWindow::finish_solver_switch() {
	m_solver_switch_timer->stop();
	if (m_solver_switch_succeeded || m_solver_servers.isEmpty()) {
		if (conf.solver_agent_type != m_solver_switch_target) {
			conf.solver_agent_type = m_solver_switch_target;
			write_conf();
			QString message = QString("Solver agent changed to %1").arg(solver_agent_name(m_solver_switch_target));
			window_log(message.toUtf8().constData());
			if (m_solver_switch_target == AIN_SOLVER_PLATESOLVER) {
				window_log("Platesolver Agent needs its indexes installed and selected: Platesolver Agent > Index management in INDIGO Control Panel");
			}
		}
	} else {
		m_solver_switch_target = conf.solver_agent_type;
	}
	sync_solver_agent_menu();
}

void ImagerWindow::on_solver_agent_selected(int index) {
	Q_UNUSED(index);
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

void ImagerWindow::on_solver_hints_changed() {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_solver_agent(selected_agent);

		change_solver_agent_hints_property(selected_agent);
	});
}

void ImagerWindow::on_solver_ra_dec_hints_changed(bool clicked) {
	Q_UNUSED(clicked);
	on_solver_hints_changed();
}

void ImagerWindow::on_solver_hints_changed(int value) {
	Q_UNUSED(value);
	on_solver_hints_changed();
}

void ImagerWindow::on_solver_hints_changed(double value) {
	Q_UNUSED(value);
	on_solver_hints_changed();
}

void ImagerWindow::on_trigger_solve() {
	trigger_solve();
}

void ImagerWindow::on_solver_load_coords() {
	indigo_debug("CALLED: %s\n", __FUNCTION__);

	char selected_agent[INDIGO_NAME_SIZE];
	get_selected_solver_agent(selected_agent);

	indigo_property *p = properties.get(selected_agent, AGENT_PLATESOLVER_WCS_PROPERTY_NAME);
	if (p) {
		if (p->state != INDIGO_OK_STATE) {
			return;
		}
		double solved_ra = 0;
		double solved_dec = 0;
		for (int i = 0; i < p->count; i++) {
			if (client_match_item(&p->items[i], AGENT_PLATESOLVER_WCS_RA_ITEM_NAME)) {
				solved_ra = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_PLATESOLVER_WCS_DEC_ITEM_NAME)) {
				solved_dec = p->items[i].number.value;
			}
		}
		set_text(m_mount_ra_input, indigo_dtos(solved_ra, "%d:%02d:%04.1f"));
		set_text(m_mount_dec_input, indigo_dtos(solved_dec, "%d:%02d:%04.1f"));

		char message[255];
		snprintf(
			message, 255, "Push Goto to slew to α = %s, δ = %s",
			indigo_dtos(solved_ra, "%dh %02d' %04.1f\""),
			indigo_dtos(solved_dec, "%+d° %02d' %04.1f\"")
		);
		window_log(message);
	}
}

void ImagerWindow::on_image_source1_selected(int index) {
	QString solver_source = m_solver_source_select1->currentText();
	// cleared because the solver agent went away - keep the choice and the relation for when it is back
	if (index < 0) {
		show_selected_preview_in_solver_tab(solver_source);
		return;
	}
	strncpy(conf.solver_image_source1, solver_source.toUtf8().constData(), INDIGO_NAME_SIZE);
	write_conf();
	show_selected_preview_in_solver_tab(solver_source);
	indigo_debug("%s -> %s\n", __FUNCTION__, conf.solver_image_source1);

	// relate the source now, so that its filters can be chosen before solving
	QtConcurrent::run([=]() {
		m_property_mutex.lock();
		apply_solver_source_relation();
		m_property_mutex.unlock();
	});
}

void ImagerWindow::on_solver_filter_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_solver_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_solver_filter_property(selected_agent);
	});
}
