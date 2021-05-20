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
#include "widget_state.h"

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

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	solver_frame_layout->addItem(spacer, row, 0, 1, 4);

	row++;
	label = new QLabel("WCS solution:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	solver_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	label = new QLabel("Frame center RA (hours):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_ra_solution = new QLabel("0° 00' 00.0\"");
	set_ok(m_solver_ra_solution);
	solver_frame_layout->addWidget(m_solver_ra_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Frame center Dec (°):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_dec_solution = new QLabel("0° 00' 00.0\"");
	set_ok(m_solver_dec_solution);
	solver_frame_layout->addWidget(m_solver_dec_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Rotation angle (° E of N):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_angle_solution = new QLabel("00");
	set_idle(m_solver_angle_solution);
	solver_frame_layout->addWidget(m_solver_angle_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Frame height (°):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_fheight_solution = new QLabel("0° 00' 00.0\"");
	set_ok(m_solver_fheight_solution);
	solver_frame_layout->addWidget(m_solver_fheight_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Frame width (°):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_fwidth_solution = new QLabel("0° 00' 00.0\"");
	set_ok(m_solver_fwidth_solution);
	solver_frame_layout->addWidget(m_solver_fwidth_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Frame Scale (\"/px):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_scale_solution = new QLabel("0");
	set_ok(m_solver_scale_solution);
	solver_frame_layout->addWidget(m_solver_scale_solution, row, 2, 1, 2);

	row++;
	label = new QLabel("Parity (-1,1):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_parity_solution = new QLabel("0");
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
	label = new QLabel("RA / Dec:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	solver_frame_layout->addWidget(label, row, 0, 1, 2);

	m_solver_ra_hint = new QLineEdit();
	m_solver_ra_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_ra_hint, row, 2);

	m_solver_dec_hint = new QLineEdit();
	m_solver_dec_hint->setEnabled(false);
	solver_frame_layout->addWidget(m_solver_dec_hint, row, 3);

	row++;
	label = new QLabel("Search radius (°):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_radius_hint = new QDoubleSpinBox();
	m_solver_radius_hint->setMaximum(180);
	m_solver_radius_hint->setMinimum(0);
	m_solver_radius_hint->setValue(0);
	solver_frame_layout->addWidget(m_solver_radius_hint, row, 3);
	//connect(m_solver_radius_hint, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	row++;
	label = new QLabel("Downsample:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_ds_hint = new QSpinBox();
	m_solver_ds_hint->setMaximum(16);
	m_solver_ds_hint->setMinimum(0);
	m_solver_ds_hint->setValue(0);
	solver_frame_layout->addWidget(m_solver_ds_hint, row, 3);
	//connect(m_solver_radius_hint, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	row++;
	label = new QLabel("Parity:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_parity_hint = new QSpinBox();
	m_solver_parity_hint->setMaximum(16);
	m_solver_parity_hint->setMinimum(0);
	m_solver_parity_hint->setValue(0);
	solver_frame_layout->addWidget(m_solver_parity_hint, row, 3);
	//connect(m_solver_radius_hint, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	row++;
	label = new QLabel("Depth:");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_depth_hint = new QSpinBox();
	m_solver_depth_hint->setMaximum(16);
	m_solver_depth_hint->setMinimum(0);
	m_solver_depth_hint->setValue(0);
	solver_frame_layout->addWidget(m_solver_depth_hint, row, 3);
	//connect(m_solver_radius_hint, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	row++;
	label = new QLabel(" Time Limit (s):");
	solver_frame_layout->addWidget(label, row, 0, 1, 2);
	m_solver_tlimit_hint = new QSpinBox();
	m_solver_tlimit_hint->setMaximum(16);
	m_solver_tlimit_hint->setMinimum(0);
	m_solver_tlimit_hint->setValue(0);
	solver_frame_layout->addWidget(m_solver_tlimit_hint, row, 3);
	//connect(m_solver_radius_hint, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

/*
	// Solver selection
	row++;
	QLabel *label = new QLabel("Focuser:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	solver_frame_layout->addWidget(label, row, 0);
	m_focuser_select = new QComboBox();
	focuser_frame_layout->addWidget(m_focuser_select, row, 1, 1, 3);
	connect(m_focuser_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_selected);
*/

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
