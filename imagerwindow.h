// Copyright (c) 2019 Rumen G.Bogdanovski & David Hulse
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


#ifndef IMAGERWINDOW_H
#define IMAGERWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QComboBox>
#include <indigo/indigo_bus.h>
#include "image-viewer.h"

class QPlainTextEdit;
class QFrame;
class QPushButton;
class QServiceModel;
class QItemSelection;
class QVBoxLayout;
class QDoubleSpinBox;
class QSpinBox;
class QProgressBar;
class QScrollArea;
class QIndigoServers;
class QLabel;


class ImagerWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit ImagerWindow(QWidget *parent = nullptr);
	virtual ~ImagerWindow();
	void property_define_delete(indigo_property* property, char *message, bool action_deleted);

	bool get_selected_agent(char * selected_agent) {
		if (!selected_agent || !m_camera_select) return false;
		strncpy(selected_agent, m_camera_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
		return true;
	};

signals:
	void enable_blobs(bool on);
	void rebuild_blob_previews();

public slots:
	void on_start(bool clicked);
	void on_abort(bool clicked);
	void on_pause(bool clicked);
	void on_window_log(indigo_property* property, char *message);
	void on_property_define(indigo_property* property, char *message);
	void on_property_change(indigo_property* property, char *message);
	void on_property_delete(indigo_property* property, char *message);
	void on_message_sent(indigo_property* property, char *message);
	void on_blobs_changed(bool status);
	void on_bonjour_changed(bool status);
	void on_use_suffix_changed(bool status);
	void on_use_state_icons_changed(bool status);
	void on_use_system_locale_changed(bool status);
	void on_log_error();
	void on_log_info();
	void on_log_debug();
	void on_log_trace();
	void on_servers_act();
	void on_exit_act();
	void on_about_act();
	void on_no_stretch();
	void on_normal_stretch();
	void on_hard_stretch();
	void on_create_preview(indigo_property *property, indigo_item *item);
	void on_obsolete_preview(indigo_property *property, indigo_item *item);
	void on_remove_preview(indigo_property *property, indigo_item *item);

	void on_camera_selected(int index);
	void on_ccd_mode_selected(int index);
	void on_frame_type_selected(int index);

private:
	QPlainTextEdit* mLog;
	QLabel* mSelectionLine;
	//QVBoxLayout* mFormLayout;

	QComboBox *m_camera_select;
	QComboBox *m_frame_type_select;
	QComboBox *m_frame_size_select;
	QSpinBox  *m_roi_x, *m_roi_w;
	QSpinBox  *m_roi_y, *m_roi_h;
	QDoubleSpinBox *m_exposure_time;
	QSpinBox *m_frame_count;
	QSpinBox *m_frame_delay;
	QPushButton *m_pause_button;

	QProgressBar *m_exposure_progress;
	QProgressBar *m_process_progress;
	QScrollArea *mScrollArea;
	//QLabel *mImage;
	pal::ImageViewer *m_viewer;
	//QImage *m_image;
	QIndigoServers *mIndigoServers;
	QServiceModel *mServiceModel;

	void clear_window();
};

#endif // IMAGERWINDOW_H
