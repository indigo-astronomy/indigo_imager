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
