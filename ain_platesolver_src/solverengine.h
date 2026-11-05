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


#ifndef SOLVERENGINE_H
#define SOLVERENGINE_H

#include <QByteArray>
#include <QFutureWatcher>
#include <QMetaType>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>

extern "C" {
#include <indigo_solver.h>
}

struct SolverHints {
	double radius = 0;       // degrees, 0 searches the whole sky
	double ra = 0;           // hours
	double dec = 0;          // degrees
	double epoch = 2000;     // 2000 for J2000, 0 for JNow, also the epoch of the solution
	double scale = 0;        // degrees per pixel, 0 if unknown
	int parity = 0;
	int downsample = 2;
	int depth = 0;
	int cpu_limit = 180;     // seconds, 0 for none
};

struct SolverSolution {
	double ra = 0;           // hours
	double dec = 0;          // degrees
	double epoch = 2000;
	double angle = 0;        // degrees
	double width = 0;        // degrees
	double height = 0;       // degrees
	double scale = 0;        // degrees per pixel
	int parity = 0;
	float index = 0;         // the quad scale of the index, degrees
};

struct SolverIndex {
	QString name;            // INDEX_<scale>DEG_<catalogue>
	QString label;           // "2° Tycho-2 (FOV 2° - 4.5°)"
	QString size_label;      // "2° Tycho-2 (FOV 2° - 4.5°, 358 MB)"
	float fov;
	bool wanted;             // installed, or to be downloaded
	bool listed;             // installed and offered for use
	bool used;
};

Q_DECLARE_METATYPE(SolverSolution)

// The solver and the index management of indigo_agent_platesolver without the agent. The indexes are the same files
// in the same folder. A solve runs on a worker thread, the rest and all signals are on the GUI thread.
class SolverEngine : public QObject {
	Q_OBJECT
public:
	explicit SolverEngine(QObject *parent = nullptr);
	virtual ~SolverEngine();

	const QVector<SolverIndex> &indexes() const { return m_indexes; }
	QString index_folder() const { return m_base_dir; }
	bool index_busy() const { return m_downloading; }
	// The running download: the file name, the MB received and the approximate size of the file
	QString download_name() const;
	int download_received_mb() const;
	int download_size_mb() const { return m_download_size_mb; }
	bool index_failed() const { return m_index_failed; }
	bool solving() const { return m_solving; }
	bool any_index_used() const;

	// Downloads a wanted index that is missing and deletes an installed one that is not wanted
	void set_index_wanted(const QString &name, bool wanted);
	void set_index_used(const QString &name, bool used);
	// The data is copied, false if the solver or the indexes are busy
	bool solve(const unsigned char *data, size_t size, const QString &format, const SolverHints &hints);
	void abort();

signals:
	void indexes_changed();
	void message_received(const QString &message);
	void solve_finished(bool solved, const SolverSolution &solution, const QString &message);

private:
	struct SolveResult {
		bool solved = false;
		SolverSolution solution;
		QString message;
	};

	QString index_path(float fov) const;
	bool index_installed(float fov) const;
	void remove_partial_index_files();
	void free_catalog();
	bool load_catalog(const char *path);
	void rebuild_use_index();
	void sync_installed_indexes();
	void start_download(SolverIndex &index);
	void download_finished(bool downloaded);
	void finish_index_change();
	void unselect_missing_indexes();
	bool limits_exceeded_or_aborted(indigo_solver_limits *limits) const;
	SolveResult run_solve(const QByteArray &data, const QString &format, const SolverHints &hints, const QVector<float> &selected);

	QString m_base_dir;
	QVector<SolverIndex> m_indexes;
	bool m_downloading;
	QProcess *m_download;
	QFutureWatcher<QString> *m_download_watcher;
	QString m_download_part;
	QString m_download_url;
	float m_download_fov;
	int m_download_size_mb;
	std::atomic<qint64> m_download_received;
	bool m_download_aborted;
	bool m_index_failed;
	bool m_solving;
	volatile bool m_abort_requested;
	QFutureWatcher<SolveResult> *m_watcher;
	char m_loaded_index[PATH_MAX];
	indigo_solver_catalog m_catalog;
};

#endif // SOLVERENGINE_H
