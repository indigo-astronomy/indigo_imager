// Copyright (c) 2021 Rumen G.Bogdanovski
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


#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include <stdio.h>
#include <QApplication>
#include <QMainWindow>
#include <QComboBox>
#include <indigo/indigo_bus.h>
#include <imageviewer.h>
#include <imagepreview.h>

#include <conf.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QThread>
#include <QtConcurrentRun>


class ViewerWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit ViewerWindow(QWidget *parent = nullptr);
	virtual ~ViewerWindow();
	void open_image(QString file_name);

public slots:
	void on_use_system_locale_changed(bool status);
	void on_image_open_act();
	void on_image_close_act();
	void on_exit_act();
	void on_about_act();

	void on_stretch_changed(int level);
	void on_antialias_view(bool status);

private:
	// Image viewer
	ImageViewer *m_imager_viewer;
	preview_image *m_preview_image;
	unsigned char *m_image_data;
	size_t m_image_size;
	char m_image_path[PATH_LEN];
	char *m_image_formrat;
};

#endif // VIEWERWINDOW_H
