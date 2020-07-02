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
	focuser_frame_layout->addWidget(m_focuser_select, row, 1, 1, 3);
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

	label = new QLabel("Method:");
	focuser_frame_layout->addWidget(label, row, 2);
	QComboBox *focus_method = new QComboBox();
	focus_method->addItem("Manual");
	focus_method->addItem("Auto");
	focuser_frame_layout->addWidget(focus_method, row, 3);
	//connect(focus_method, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_selected);

	row++;
	label = new QLabel("Autofocus setings:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	label = new QLabel("Initial Step:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_initial_step = new QSpinBox();
	m_initial_step->setMaximum(1000000);
	m_initial_step->setMinimum(0);
	m_initial_step->setValue(0);
	m_initial_step->setEnabled(false);
	focuser_frame_layout->addWidget(m_initial_step , row, 1);

	label = new QLabel("Final step:");
	focuser_frame_layout->addWidget(label, row, 2);
	m_final_step = new QSpinBox();
	m_final_step->setMaximum(100000);
	m_final_step->setMinimum(0);
	m_final_step->setValue(0);
	m_final_step->setEnabled(false);
	focuser_frame_layout->addWidget(m_final_step, row, 3);

	row++;
	label = new QLabel("Backlash:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_focus_backlash = new QSpinBox();
	m_focus_backlash->setMaximum(1000000);
	m_focus_backlash->setMinimum(0);
	m_focus_backlash->setValue(0);
	m_focus_backlash->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_backlash, row, 1);

	label = new QLabel("Stacking:");
	focuser_frame_layout->addWidget(label, row, 2);
	m_focus_stack = new QSpinBox();
	m_focus_stack->setMaximum(100000);
	m_focus_stack->setMinimum(0);
	m_focus_stack->setValue(0);
	m_focus_stack->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_stack, row, 3);

	row++;
	label = new QLabel("Manual focuser control:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	label = new QLabel("Absoute Position:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 2);
	m_focus_position = new QSpinBox();
	m_focus_position->setMaximum(1000000);
	m_focus_position->setMinimum(0);
	m_focus_position->setValue(0);
	m_focus_position->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_position, row, 2, 1, 2);

	row++;
	label = new QLabel("Move:");
	focuser_frame_layout->addWidget(label, row, 0);
	m_focus_steps = new QSpinBox();
	m_focus_steps->setMaximum(100000);
	m_focus_steps->setMinimum(0);
	m_focus_steps->setValue(0);
	m_focus_steps->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_steps, row, 1);

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

	button = new QPushButton("Focus");
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
