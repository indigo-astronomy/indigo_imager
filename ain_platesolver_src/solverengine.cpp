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


#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QProcessEnvironment>
#include <QtConcurrent/QtConcurrent>
#include <cctype>
#include <chrono>
#include <climits>
#include <cmath>
#include <memory>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigocat/indigocat_precession.h>
#include <conf.h>
#include <imagepreview.h>
#include <solverengine.h>
#if defined(SOLVER_USE_WININET)
#include <wininetdownload.h>
#endif

// the solver needs only this function of indigo_timer.c, which is not in libindigo_client
extern "C" double indigo_monotonic_time(void) {
	return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Quad indexes built by indigo_solver_indexer are downloaded from here as index-<scale>deg-tyc2.isix (Tycho-2) and
// index-<scale>deg-gaia.isix (Gaia DR3, scales below 1 degree), the scale zero padded so the files sort by it
#define AGENT_SOLVER_INDEX_URL "https://github.com/indigo-astronomy/indigo/releases/download/index_release_v2"

#define MAX_IMAGE_STAR_COUNT 500
#define MAX_IMAGE_QUAD_COUNT 1000000
#define MAX_QUAD_MATCH_COUNT 1000000
#define MAX_STAR_MATCH_COUNT 1000
#define IMAGE_GRID_CELLS 10
#define MAX_DEPTH_STAGES 3
#define DOWNLOAD_STOP_TIMEOUT_MS 3000

// approximate sizes in MB of the published index files, in the order of indigo_solver_index_fovs
static const int index_sizes[INDIGO_SOLVER_INDEX_FOV_COUNT] = { 1216, 911, 538, 358, 178, 92, 36 };

// INDEX_<scale>DEG_<CATALOG> as the file name: INDEX_00_2DEG_GAIA and INDEX_00_5DEG_GAIA for the Gaia DR3 indexes of
// scale 0.2 and 0.5, INDEX_01DEG_TYC2 ... INDEX_20DEG_TYC2 for the Tycho-2 indexes, the scale zero padded so the names
// sort by it and with an underscore for the decimal point
static QString index_item_name(float fov) {
	char scale[16], name[64];
	indigo_solver_index_scale(fov, scale, sizeof(scale));
	snprintf(name, sizeof(name), "INDEX_%sDEG_%s", scale, indigo_solver_index_catalog(fov));
	for (char *c = name; *c; c++) {
		*c = *c == '.' ? '_' : toupper(*c);
	}
	return QString(name);
}

// "2° Tycho-2 (FOV 2° - 4.5°)", with the size of the file "2° Tycho-2 (FOV 2° - 4.5°, 358 MB)"
static QString index_item_label(int index, bool with_size) {
	char label[256];
	indigo_solver_index_label(indigo_solver_index_fovs[index], label, sizeof(label));
	QString result = QString::fromUtf8(label);
	if (with_size && result.endsWith(')')) {
		result.chop(1);
		result += QString(", %1 MB)").arg(index_sizes[index]);
	}
	return result;
}

// a downloaded file is accepted only with a valid header for the requested scale
static bool validate_index_file(const char *path, float fov) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return false;
	}
	indigo_solve_index_header header;
	bool valid = indigo_solver_index_read_header(file, &header) && header.fov == fov && header.star_count > 0 && header.quad_count > 0;
	fclose(file);
	return valid;
}

// The image as RAW MONO16, opened by the same code as for the view: a Bayer mosaic with a known pattern is debayered,
// so the colour pattern does not reach the star detector, and colour is reduced with the luminance weights
static inline float luminance(float red, float green, float blue) {
	return 0.299f * red + 0.587f * green + 0.114f * blue;
}

template <typename T> static void integer_plane(const T *in, int channels, size_t pixels, float factor, uint16_t *out) {
	for (size_t i = 0; i < pixels; i++) {
		const T *p = in + i * channels;
		float value = channels == 3 ? luminance(p[0], p[1], p[2]) : p[0];
		out[i] = (uint16_t)(value * factor + 0.5f);
	}
}

// 32 bit samples have no fixed range, floating point ones are often 0 to 1
template <typename T> static void scaled_plane(const T *in, int channels, size_t pixels, uint16_t *out) {
	std::unique_ptr<float[]> values(new float[pixels]);
	float min = 0, max = 0;
	bool first = true;
	for (size_t i = 0; i < pixels; i++) {
		const T *p = in + i * channels;
		float value = channels == 3 ? luminance(p[0], p[1], p[2]) : p[0];
		if (!std::isfinite(value)) {
			value = 0;
		}
		values[i] = value;
		if (first || value < min) min = value;
		if (first || value > max) max = value;
		first = false;
	}
	float scale = max > min ? 65535 / (max - min) : 1;
	for (size_t i = 0; i < pixels; i++) {
		out[i] = (uint16_t)((values[i] - min) * scale + 0.5f);
	}
}

static indigo_raw_header *load_image(const QByteArray &data, const QString &format, int downsample, int *bin, int *full_width, int *full_height, QString *message) {
	const stretch_config_t config = {STRETCH_NONE, CD_NONE, BAYER_PAT_AUTO};
	const QByteArray format_name = format.toUtf8();
	std::unique_ptr<preview_image> preview(create_preview((unsigned char *)data.constData(), data.size(), format_name.constData(), config));
	if (preview == nullptr) {
		*message = "Unsupported image format";
		return NULL;
	}
	const int width = preview->width();
	const int height = preview->height();
	const size_t pixels = (size_t)width * height;
	*bin = downsample > 1 ? downsample : 1;
	*full_width = width;
	*full_height = height;
	if (width / *bin == 0 || height / *bin == 0) {
		*message = "Image is too small for downsampling";
		return NULL;
	}
	indigo_raw_header *image = (indigo_raw_header *)indigo_safe_malloc(sizeof(indigo_raw_header) + pixels * sizeof(uint16_t));
	image->signature = INDIGO_RAW_MONO16;
	image->width = width;
	image->height = height;
	uint16_t *out = (uint16_t *)(image + 1);
	const char *raw = preview->m_raw_data;
	if (raw == nullptr) {
		// the formats decoded only for the view
		const QImage gray = preview->convertToFormat(QImage::Format_RGB32);
		for (int y = 0; y < height; y++) {
			const QRgb *line = (const QRgb *)gray.constScanLine(y);
			for (int x = 0; x < width; x++) {
				out[(size_t)y * width + x] = (uint16_t)(luminance(qRed(line[x]), qGreen(line[x]), qBlue(line[x])) * 257 + 0.5f);
			}
		}
	} else {
		switch (preview->m_pix_format) {
			case PIX_FMT_Y8: integer_plane((const uint8_t *)raw, 1, pixels, 257, out); break;
			case PIX_FMT_Y16: memcpy(out, raw, pixels * sizeof(uint16_t)); break;
			case PIX_FMT_Y32: scaled_plane((const uint32_t *)raw, 1, pixels, out); break;
			case PIX_FMT_F32: scaled_plane((const float *)raw, 1, pixels, out); break;
			case PIX_FMT_RGB24: integer_plane((const uint8_t *)raw, 3, pixels, 257, out); break;
			case PIX_FMT_RGB48: integer_plane((const uint16_t *)raw, 3, pixels, 1, out); break;
			case PIX_FMT_RGB96: scaled_plane((const uint32_t *)raw, 3, pixels, out); break;
			case PIX_FMT_RGBF: scaled_plane((const float *)raw, 3, pixels, out); break;
			default:
				indigo_safe_free(image);
				*message = "Unsupported pixel format";
				return NULL;
		}
	}
	indigo_solver_bin_image(image, *bin);
	return image;
}

// Orders the selected indexes: the one fitting the field size first, when the scale is known, then the others
static int select_candidate_fovs(const QString &base_dir, const QVector<float> &selected, double scale, int width, int height, float *candidates) {
	const int selected_count = selected.size();
	int count = 0;
	if (scale > 0 && selected_count > 0) {
		char path[PATH_MAX];
		float fov;
		if (indigo_solver_select_index(base_dir.toUtf8().constData(), selected.constData(), selected_count, width * scale * DEG_TO_RAD, height * scale * DEG_TO_RAD, path, sizeof(path), &fov)) {
			candidates[count++] = fov;
		}
	}
	// the other selected indexes follow, the nearest to the field size first when the scale is known, otherwise the
	// largest index first
	float target = scale > 0 ? INDIGO_SOLVER_INDEX_FIELD_RATIO * sqrt((double)width * height) * scale : 0;
	while (true) {
		float best = 0, best_distance = 0;
		for (int i = 0; i < selected_count; i++) {
			bool listed = false;
			for (int j = 0; j < count; j++) {
				listed = listed || candidates[j] == selected[i];
			}
			if (listed) {
				continue;
			}
			float distance = target > 0 ? fabs(log(selected[i] / target)) : -selected[i];
			if (best == 0 || distance < best_distance) {
				best = selected[i];
				best_distance = distance;
			}
		}
		if (best == 0) {
			break;
		}
		candidates[count++] = best;
	}
	return count;
}

// The image star counts of the progressive stages: the DEPTH hint first and then all stars, or the built-in
// progression when the hint is 0; the stages above the detected count collapse into the final one
static int depth_stages(int depth, int star_count, int *stages) {
	static const int progression[] = { 40, 100 };
	int count = 0;
	if (depth > 0) {
		if (depth < star_count) {
			stages[count++] = depth;
		}
	} else {
		for (unsigned i = 0; i < sizeof(progression) / sizeof(progression[0]); i++) {
			if (progression[i] < star_count) {
				stages[count++] = progression[i];
			}
		}
	}
	stages[count++] = star_count;
	return count;
}

SolverEngine::SolverEngine(QObject *parent) :
	QObject(parent),
	m_downloading(false),
	m_download(nullptr),
	m_download_watcher(nullptr),
	m_download_fov(0),
	m_download_size_mb(0),
	m_download_received(0),
	m_download_aborted(false),
	m_index_failed(false),
	m_solving(false),
	m_abort_requested(false)
{
	qRegisterMetaType<SolverSolution>("SolverSolution");
	memset(&m_catalog, 0, sizeof(m_catalog));
	m_loaded_index[0] = 0;

	const QString base = QProcessEnvironment::systemEnvironment().value("INDIGO_CACHE_BASE");
	m_base_dir = base.isEmpty() ? QDir::homePath() + "/.indigo/solver" : base + "/solver";
	QDir().mkpath(m_base_dir);
	remove_partial_index_files();

	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		const float fov = indigo_solver_index_fovs[i];
		const bool present = index_installed(fov);
		m_indexes.append({index_item_name(fov), index_item_label(i, false), index_item_label(i, true), fov, present, present, false});
	}

#if defined(SOLVER_USE_WININET)
	m_download_watcher = new QFutureWatcher<QString>(this);
	connect(m_download_watcher, &QFutureWatcher<QString>::finished, this, [this]() {
		const QString error = m_download_watcher->result();
		if (!error.isEmpty() && !m_download_aborted) {
			emit message_received(error);
		}
		download_finished(error.isEmpty());
	});
#endif

	m_watcher = new QFutureWatcher<SolveResult>(this);
	connect(m_watcher, &QFutureWatcher<SolveResult>::finished, this, [this]() {
		const SolveResult result = m_watcher->result();
		m_solving = false;
		emit solve_finished(result.solved, result.solution, result.message);
	});
}

SolverEngine::~SolverEngine() {
	m_abort_requested = true;
	m_watcher->disconnect(this);
	m_watcher->waitForFinished();
#if defined(SOLVER_USE_WININET)
	m_download_aborted = true;
	m_download_watcher->disconnect(this);
	wininet_cancel();
	m_download_watcher->waitForFinished();
#else
	if (m_download != nullptr) {
		m_download->disconnect(this);
		m_download->kill();
		m_download->waitForFinished(DOWNLOAD_STOP_TIMEOUT_MS);
	}
#endif
	if (m_downloading) {
		QFile::remove(m_download_part);
	}
	free_catalog();
}

QString SolverEngine::index_path(float fov) const {
	char path[PATH_MAX];
	indigo_solver_index_path(path, sizeof(path), m_base_dir.toUtf8().constData(), fov);
	return QString::fromUtf8(path);
}

bool SolverEngine::index_installed(float fov) const {
	return QFileInfo(index_path(fov)).isReadable();
}

// an interrupted transfer is never taken for an installed index, so partial files are removed on start
void SolverEngine::remove_partial_index_files() {
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		QFile::remove(index_path(indigo_solver_index_fovs[i]) + ".part");
	}
}

void SolverEngine::free_catalog() {
	indigo_solver_free_catalog(&m_catalog);
	m_loaded_index[0] = 0;
}

// the last used index stays loaded, the next solve with the same index skips the file
bool SolverEngine::load_catalog(const char *path) {
	if (!strcmp(m_loaded_index, path)) {
		indigo_debug("Index %s is already loaded", path);
		return true;
	}
	free_catalog();
	if (!indigo_solver_load_index(path, &m_catalog)) {
		return false;
	}
	snprintf(m_loaded_index, sizeof(m_loaded_index), "%s", path);
	return true;
}

bool SolverEngine::any_index_used() const {
	for (const SolverIndex &index : m_indexes) {
		if (index.listed && index.used) return true;
	}
	return false;
}

void SolverEngine::unselect_missing_indexes() {
	for (SolverIndex &index : m_indexes) {
		if (index.wanted && !index_installed(index.fov)) {
			index.wanted = false;
		}
	}
}

// every installed index is listed, so an index not listed yet was just downloaded and is selected for use
void SolverEngine::rebuild_use_index() {
	for (SolverIndex &index : m_indexes) {
		const bool installed = index_installed(index.fov);
		if (installed && !index.listed) {
			index.used = true;
		}
		index.listed = installed;
	}
}

// Downloads the first missing wanted index and removes every unwanted one; the next download starts when this one is
// finished. Changes made during a download are picked up then.
void SolverEngine::sync_installed_indexes() {
	for (SolverIndex &index : m_indexes) {
		const QString path = index_path(index.fov);
		if (index.wanted) {
			if (index_installed(index.fov)) {
				continue;
			}
			start_download(index);
			return;
		} else if (QFileInfo::exists(path)) {
			if (path == QString::fromUtf8(m_loaded_index)) {
				free_catalog();
			}
			QFile file(path);
			if (!file.remove()) {
				m_index_failed = true;
				index.wanted = true;
				emit message_received(file.errorString());
				break;
			}
		}
	}
	finish_index_change();
}

void SolverEngine::finish_index_change() {
	rebuild_use_index();
	emit indexes_changed();
}

// Fetches the index into the partial file as the agent does: with WinInet on Windows, otherwise with curl
void SolverEngine::start_download(SolverIndex &index) {
	const QString path = index_path(index.fov);
	const QString file_name = QFileInfo(path).fileName();
	m_download_part = path + ".part";
	m_download_url = QString("%1/%2").arg(AGENT_SOLVER_INDEX_URL, file_name);
	m_download_fov = index.fov;
	m_download_size_mb = 0;
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		if (indigo_solver_index_fovs[i] == index.fov) {
			m_download_size_mb = index_sizes[i];
		}
	}
	m_download_received = 0;
	emit message_received(QString("Downloading %1...").arg(file_name));
	m_downloading = true;
#if defined(SOLVER_USE_WININET)
	const QString url = m_download_url, part = m_download_part;
	m_download_watcher->setFuture(QtConcurrent::run([this, url, part]() {
		return wininet_download(url, part, &m_download_aborted, &m_download_received);
	}));
#else
	m_download = new QProcess(this);
	connect(m_download, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exit_code, QProcess::ExitStatus status) {
		download_finished(status == QProcess::NormalExit && exit_code == 0);
	});
	connect(m_download, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
		if (error == QProcess::FailedToStart) {
			emit message_received("curl is not available");
			download_finished(false);
		}
	});
	m_download->start("curl", QStringList() << "-L" << "-s" << "-f" << "-o" << m_download_part << m_download_url);
#endif
	emit indexes_changed();
}

void SolverEngine::download_finished(bool downloaded) {
	m_downloading = false;
	if (m_download != nullptr) {
		m_download->disconnect(this);
		m_download->deleteLater();
		m_download = nullptr;
	}
	const QString path = index_path(m_download_fov);
	if (m_download_aborted) {
		QFile::remove(m_download_part);
		unselect_missing_indexes();
		m_index_failed = true;
		emit message_received("Index download aborted");
		finish_index_change();
		return;
	}
	QFile::remove(path);
	if (!downloaded || !validate_index_file(m_download_part.toUtf8().constData(), m_download_fov) || !QFile::rename(m_download_part, path)) {
		QFile::remove(m_download_part);
		for (SolverIndex &index : m_indexes) {
			if (index.fov == m_download_fov) {
				index.wanted = false;
			}
		}
		m_index_failed = true;
		emit message_received(QString("Index download failed: '%1'").arg(m_download_url));
		finish_index_change();
		return;
	}
	emit message_received("Done");
	sync_installed_indexes();
}

QString SolverEngine::download_name() const {
	return QFileInfo(m_download_part).completeBaseName();
}

int SolverEngine::download_received_mb() const {
#if defined(SOLVER_USE_WININET)
	// the size of a file open for writing is not up to date on Windows
	return (int)(m_download_received / 1000000);
#else
	return (int)(QFileInfo(m_download_part).size() / 1000000);
#endif
}

void SolverEngine::set_index_wanted(const QString &name, bool wanted) {
	// the agent refuses this during a solve too, the indexes are in use
	if (m_solving) {
		return;
	}
	for (SolverIndex &index : m_indexes) {
		if (index.name == name) {
			index.wanted = wanted;
		}
	}
	m_download_aborted = false;
	m_index_failed = false;
	// a running download continues, the change is picked up when it is finished
	if (!m_downloading) {
		sync_installed_indexes();
	}
}

void SolverEngine::set_index_used(const QString &name, bool used) {
	for (SolverIndex &index : m_indexes) {
		if (index.name == name) {
			index.used = used;
		}
	}
}

void SolverEngine::abort() {
	m_abort_requested = true;
	m_download_aborted = true;
#if defined(SOLVER_USE_WININET)
	wininet_cancel();
#else
	if (m_download != nullptr) {
		m_download->kill();
	}
#endif
}

bool SolverEngine::solve(const unsigned char *data, size_t size, const QString &format, const SolverHints &hints) {
	if (m_solving || m_downloading) {
		return false;
	}
	QVector<float> selected;
	for (const SolverIndex &index : m_indexes) {
		if (index.listed && index.used && index_installed(index.fov)) {
			selected.append(index.fov);
		}
	}
	m_abort_requested = false;
	m_solving = true;
	emit message_received("Solving started");
	const QByteArray copy((const char *)data, (int)size);
	m_watcher->setFuture(QtConcurrent::run([this, copy, format, hints, selected]() {
		return run_solve(copy, format, hints, selected);
	}));
	return true;
}

// The matching loops stop at the CPU limit or an abort; the abort also stops the index loop before any matching
bool SolverEngine::limits_exceeded_or_aborted(indigo_solver_limits *limits) const {
	return limits->exceeded || m_abort_requested;
}

// The solve of indigo_agent_platesolver.c, runs on the worker thread
SolverEngine::SolveResult SolverEngine::run_solve(const QByteArray &data, const QString &format, const SolverHints &hints, const QVector<float> &selected) {
	SolveResult result;
	QString message;
	indigo_raw_header *image = NULL;
	indigo_solver_star *image_stars = NULL;
	indigo_solver_quad *image_quads = NULL;
	indigo_solver_quad_match *quad_matches = NULL;
	indigo_solver_star_match *star_matches = NULL;
	// the CPU limit and the abort flag bound every matching loop; the deadline starts now, so index loading counts
	indigo_solver_limits limits = { 0, NULL, false, NULL };
	limits.abort = &m_abort_requested;
	if (hints.cpu_limit > 0) {
		limits.deadline = indigo_monotonic_time() + hints.cpu_limit;
	}
	const int quad_cells = MAX_IMAGE_QUAD_COUNT;
	float candidates[INDIGO_SOLVER_INDEX_FOV_COUNT];
	int stages[MAX_DEPTH_STAGES];
	float ra = 0, dec = 0, rotation = 0, pixel = 0, fov = 0, rms = 0;
	int parity = 0, matched = 0, star_match_count = 0;
	bool solved = false, wrong_parity = false;
	int bin = 1, full_width = 0, full_height = 0, width, height, star_count, grid, stage_count, candidate_count, hint_parity;
	double scale, hint_radius, hint_ra, hint_dec;
	// 1. the input image as RAW MONO16
	image = load_image(data, format, hints.downsample, &bin, &full_width, &full_height, &message);
	if (image == NULL) {
		goto cleanup;
	}
	width = image->width;
	height = image->height;
	// 2. image stars, brightest first
	image_stars = (indigo_solver_star *)indigo_safe_malloc(MAX_IMAGE_STAR_COUNT * sizeof(indigo_solver_star));
	star_count = indigo_solver_image_stars(image, image_stars, MAX_IMAGE_STAR_COUNT);
	if (star_count < 4) {
		message = "Not enough stars detected";
		goto cleanup;
	}
	if (m_abort_requested) {
		message = "Aborted";
		goto cleanup;
	}
	image_quads = (indigo_solver_quad *)indigo_safe_malloc(quad_cells * sizeof(indigo_solver_quad));
	grid = (width > height ? width : height) / IMAGE_GRID_CELLS;
	stage_count = depth_stages(hints.depth, star_count, stages);
	// 3. indexes to try, the one fitting the expected field size first, then the nearest other ones
	scale = hints.scale > 0 ? hints.scale : 0;
	candidate_count = select_candidate_fovs(m_base_dir, selected, scale, full_width, full_height, candidates);
	if (candidate_count == 0) {
		message = "No index files are selected, select at least one";
		goto cleanup;
	}
	// the position hint, in the hints epoch, as a J2000 cone around which the catalog is restricted
	hint_radius = hints.radius;
	hint_ra = hints.ra;
	hint_dec = hints.dec;
	if (hint_radius > 0 && hints.epoch == 0) {
		indigocat_jnow_to_j2k(&hint_ra, &hint_dec);
	}
	hint_parity = hints.parity;
	quad_matches = (indigo_solver_quad_match *)indigo_safe_malloc(MAX_QUAD_MATCH_COUNT * sizeof(indigo_solver_quad_match));
	star_matches = (indigo_solver_star_match *)indigo_safe_malloc(MAX_STAR_MATCH_COUNT * sizeof(indigo_solver_star_match));
	// The indexes are searched stage by stage: the first stage on every index, then the next stage on every index, so
	// an index that does not fit the field costs only its cheap first stage before the others are tried. With the
	// scale every stage starts with the index fitting the field; without it the first stage starts with the largest
	// index, as the larger indexes are the smaller files and fail fast, and every next stage runs in the opposite
	// order, so the index loaded last is used again without loading it
	for (int pass = 0; pass < stage_count && !solved && !limits_exceeded_or_aborted(&limits); pass++) {
		int first_stage = pass;
		int last_stage = pass + 1;
		for (int k = 0; k < candidate_count && !solved && !limits_exceeded_or_aborted(&limits); k++) {
			int i = scale == 0 && pass % 2 ? candidate_count - 1 - k : k;
			char label[256];
			indigo_solver_index_label(candidates[i], label, sizeof(label));
			indigo_debug("Solving with index %s...", label);
			const QByteArray path = index_path(candidates[i]).toUtf8();
			if (!load_catalog(path.constData())) {
				indigo_error("Failed to load %s", path.constData());
				continue;
			}
			// the catalog restricted to the hint cone: the expected field centre lies near the hint, its stars within the
			// radius plus half the field diagonal (or the index scale when the pixel scale is unknown); a quad needs
			// four stars within the cone
			const indigo_solver_catalog *catalog = &m_catalog;
			indigo_solver_catalog cone;
			memset(&cone, 0, sizeof(cone));
			bool usable = catalog->quad_count > 0;
			if (hint_radius > 0) {
				double extent = scale > 0 ? 0.5 * sqrt((double)full_width * full_width + (double)full_height * full_height) * scale : candidates[i];
				usable = usable && indigo_solver_cone_catalog(catalog, hint_ra * 15 * DEG_TO_RAD, hint_dec * DEG_TO_RAD, (hint_radius + extent) * DEG_TO_RAD, &cone) >= 4;
				catalog = &cone;
			}
			int catalog_quad_count = catalog->quad_count;
			for (int stage = first_stage; stage < last_stage && !solved && usable && !limits_exceeded_or_aborted(&limits); stage++) {
				int used_star_count = 0;
				int quad_count = indigo_solver_build_image_quads(image_stars, stages[stage], width, height, grid, &used_star_count, image_quads, MAX_IMAGE_QUAD_COUNT);
				if (quad_count == 0) {
					continue;
				}
				indigo_debug("Stage %d: %d brightest stars, %d quads, %d catalog quads", stage + 1, stages[stage], quad_count, catalog_quad_count);
				int quad_match_count = indigo_solver_match_quads(image_quads, quad_count, catalog, quad_matches, MAX_QUAD_MATCH_COUNT, &limits);
				star_match_count = indigo_solver_match_stars(quad_matches, quad_match_count, star_matches, MAX_STAR_MATCH_COUNT, &limits);
				if (limits.exceeded) {
					break;
				}
				if (indigo_solver_compute_center(star_matches, star_match_count, width, height, &ra, &dec, &rotation, &pixel, &parity, &matched, &rms)) {
					if (hint_parity != 0 && parity != hint_parity) {
						indigo_debug("Solution with parity %d rejected by the parity hint %d", parity, hint_parity);
						wrong_parity = true;
						continue;
					}
					fov = candidates[i];
					solved = true;
				}
			}
			indigo_solver_free_catalog(&cone);
		}
	}
	if (!solved) {
		message = limits.exceeded ? QString(limits.reason) : m_abort_requested ? QString("Aborted") : wrong_parity ? QString("Solution has the wrong parity") : QString("No solution found");
		goto cleanup;
	}
	// 4. WCS result: RA in hours, Dec and angles in degrees, scale in degrees per unbinned pixel
	{
		double ra_hours = ra * RAD_TO_DEG / 15;
		double dec_degrees = dec * RAD_TO_DEG;
		result.solution.ra_j2000 = ra_hours;
		result.solution.dec_j2000 = dec_degrees;
		if (hints.epoch == 0) {
			indigocat_j2k_to_jnow_pm(&ra_hours, &dec_degrees, 0, 0);
			result.solution.epoch = 0;
		} else {
			result.solution.epoch = 2000;
		}
		result.solution.ra = ra_hours;
		result.solution.dec = dec_degrees;
		result.solution.scale = pixel * RAD_TO_DEG / bin;
		result.solution.width = full_width * result.solution.scale;
		result.solution.height = full_height * result.solution.scale;
		result.solution.image_width = full_width;
		result.solution.image_height = full_height;
		result.solution.angle = rotation * RAD_TO_DEG;
		result.solution.parity = parity;
		result.solution.index = fov;
		result.solved = true;
		message = QString("Solved: %1 of %2 stars matched, rms %3\"").arg(matched).arg(star_match_count).arg(rms * RAD_TO_DEG * 3600, 0, 'f', 2);
	}
cleanup:
	indigo_safe_free(star_matches);
	indigo_safe_free(quad_matches);
	indigo_safe_free(image_quads);
	indigo_safe_free(image_stars);
	indigo_safe_free(image);
	result.message = message;
	return result;
}
