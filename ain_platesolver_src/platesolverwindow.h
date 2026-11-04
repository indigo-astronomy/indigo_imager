// Copyright (c) 2026 Rumen G.Bogdanovski
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


#ifndef PLATESOLVERWINDOW_H
#define PLATESOLVERWINDOW_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <viewerwindow.h>
#include <solverclient.h>
#include <objectoverlay.h>

class PlateSolverWindow : public ViewerWindow {
	Q_OBJECT
public:
	explicit PlateSolverWindow(QWidget *parent = nullptr);
	virtual ~PlateSolverWindow();

private slots:
	void on_property_defined(const SolverProperty &property);
	void on_property_updated(const SolverProperty &property);
	void on_property_deleted(const QString &name);
	void on_message(const QString &message);
	void on_image_changed();
	void on_solve_clicked();
	void on_abort_clicked();
	void on_index_toggled(const QString &name, bool checked);
	void on_use_index_toggled();
	void on_hints_edited();
	void on_show_objects(bool show);
	void on_download_timer();

private:
	QWidget *create_solve_tab();
	QWidget *create_index_tab();
	QWidget *create_hints_tab();
	void update_hints(const SolverProperty &property);
	void update_wcs(const SolverProperty &property);
	void update_index(const SolverProperty &property);
	void update_use_index(const SolverProperty &property);
	void clear_solution();
	void apply_wcs_to_image();
	void remove_wcs_from_image();
	void update_objects();
	void update_controls();
	void log_message(const QString &message);
	bool any_use_index_selected() const;
	QString index_folder() const;

	SolverClient *m_client;
	bool m_agent_loaded;
	bool m_solve_requested;
	bool m_wcs_busy;
	bool m_index_busy;
	QString m_current_image;
	QString m_solving_image;
	QString m_wcs_image;
	bool m_wcs_valid;
	bool m_showing_stack;
	double m_wcs_ra;
	double m_wcs_dec;
	double m_wcs_angle;
	double m_wcs_scale;
	int m_wcs_parity;
	bool m_wcs_jnow;
	ObjectOverlay *m_objects;

	QTabWidget *m_tabs;
	QLabel *m_agent_status;
	QLabel *m_image_label;
	QPushButton *m_solve_button;
	QPushButton *m_abort_button;
	QCheckBox *m_show_objects;
	QVBoxLayout *m_use_index_layout;
	QLabel *m_use_index_empty;
	QMap<QString, QCheckBox *> m_use_index_boxes;
	QLineEdit *m_hint_ra;
	QLineEdit *m_hint_dec;
	QDoubleSpinBox *m_hint_radius;
	QComboBox *m_hint_epoch;
	QDoubleSpinBox *m_hint_scale;
	QComboBox *m_hint_parity;
	QSpinBox *m_hint_downsample;
	QSpinBox *m_hint_depth;
	QSpinBox *m_hint_cpu_limit;
	QMap<QString, QLabel *> m_wcs_labels;
	QLabel *m_solution_image;
	QVBoxLayout *m_index_layout;
	QMap<QString, QCheckBox *> m_index_boxes;
	QLabel *m_download_status;
	QProgressBar *m_download_progress;
	QPushButton *m_download_abort;
	QTimer *m_download_timer;
	QPlainTextEdit *m_log;
};

#endif // PLATESOLVERWINDOW_H
