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
#include <indigo/indigo_bus.h>

class QPlainTextEdit;
class QTreeView;
class QServiceModel;
class QItemSelection;
class QVBoxLayout;
class QScrollArea;
class QIndigoServers;
class QLabel;

class ImagerWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit ImagerWindow(QWidget *parent = nullptr);
	virtual ~ImagerWindow();
	void property_define_delete(indigo_property* property, char *message, bool action_deleted);

signals:
	void enable_blobs(bool on);
	void rebuild_blob_previews();

public slots:
	void on_window_log(indigo_property* property, char *message);
	void on_property_define(indigo_property* property, char *message);
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

private:
	QPlainTextEdit* mLog;
	QTreeView* mProperties;
	QScrollArea* mScrollArea;
	QLabel* mSelectionLine;
	QVBoxLayout* mFormLayout;

	QIndigoServers *mIndigoServers;
	QServiceModel* mServiceModel;

	void clear_window();
};

#endif // IMAGERWINDOW_H
