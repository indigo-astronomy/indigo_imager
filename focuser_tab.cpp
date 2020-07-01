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
	focuser_frame_layout->addWidget(label, row, 0);
	m_focuser_select = new QComboBox();
	focuser_frame_layout->addWidget(m_wheel_select, row, 1, 1, 3);
	connect(m_focuser_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_selected);

	// Star Selection
	row++;
	label = new QLabel("Star X:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_star_x = new QSpinBox();
	m_star_x->setMaximum(100000);
	m_star_x->setMinimum(0);
	m_star_x->setValue(0);
	m_star_x->setEnabled(false);
	focuser_frame_layout->addWidget(m_star_x , row, 1);

	label = new QLabel("Star Y:");
	focuser_frame_layout->addWidget(label, row, 2);
	m_star_y = new QSpinBox();
	m_star_y->setMaximum(100000);
	m_star_y->setMinimum(0);
	m_star_y->setValue(0);
	m_star_y->setEnabled(false);
	focuser_frame_layout->addWidget(m_star_y, row, 3);

	// Exposure time
	row++;
	label = new QLabel("Exposure (s):");
	focuser_frame_layout->addWidget(label, row, 0);
	m_focuser_exposure_time = new QDoubleSpinBox();
	m_focuser_exposure_time->setMaximum(10000);
	m_focuser_exposure_time->setMinimum(0);
	m_focuser_exposure_time->setValue(1);
	focuser_frame_layout->addWidget(m_focuser_exposure_time, row, 1);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	focuser_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	QPushButton *button = new QPushButton("Abort");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_abort);

	button = new QPushButton("Start");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_start);

	button = new QPushButton("Preview");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_preview);
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
