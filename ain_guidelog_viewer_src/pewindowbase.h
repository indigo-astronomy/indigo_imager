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

#ifndef PEWINDOWBASE_H
#define PEWINDOWBASE_H

#include <QMainWindow>
#include <QString>

#include <memory>

class QLabel;
class QVBoxLayout;
class QWidget;
class SimplePlot;
class PEAnalysis;

// Common chrome for the two periodic-error windows: the dark stylesheet, the
// rich-text summary line above the plot, the plot itself, and the shared
// PEAnalysis they both read.
//
// Subclasses build their layout in their constructor by calling, in order, any
// controls they need on rootLayout(), then addSummaryRow() and addPlotRow().
// They implement recompute() to fill the plot in.
class PEWindowBase : public QMainWindow {
	Q_OBJECT

public:
	// Points the window at the analysis shared with the rest of the tool and
	// supplies the calibration / declination the log carried, so a window with
	// its own entry fields can pre-fill them. Use this on open / session change.
	void setSession(const std::shared_ptr<PEAnalysis> &analysis,
	                double calibrationPxPerS,
	                double mountDecDeg);

	// The rows behind the shared analysis changed (e.g. the graph's visible
	// window moved); redraw from it, leaving any user entry alone.
	void refresh() { recompute(); }

protected:
	explicit PEWindowBase(const QString &title, QWidget *parent = nullptr);

	// Layout construction, called by the subclass in this order.
	QVBoxLayout *rootLayout() const { return m_rootLayout; }
	QWidget *centralPanel() const { return m_central; }
	// Adds the summary line; trailing, when given, is placed right-aligned on
	// the same row (the PE curve window puts its export button there).
	void addSummaryRow(QWidget *trailing = nullptr);
	// Adds the plot, taking the remaining height. Its axis captions come from
	// SimpleAxis::setLabel(), so the subclass sets them with the rest of the axes.
	void addPlotRow();

	// Redraws everything from the current analysis and control values.
	virtual void recompute() = 0;
	// Called by setSession() before recompute(), for subclasses that pre-fill
	// entry fields from the log. The base implementation does nothing.
	virtual void applyLogDefaults(double calibrationPxPerS, double mountDecDeg);

	// Clears the plot and shows msg in place of the usual statistics. Used for
	// every "nothing to show" path so they all leave the window in one state.
	void showPlaceholder(const QString &message);

	// Asks the user where to save a CSV, appending the extension if they left it
	// off. Returns an empty string when the dialog was cancelled.
	QString askForCsvPath(const QString &dialogTitle);

	// Rich-text helpers shared by both summary lines.
	static QString number(double value, int precision);
	static QString separator();
	static QString twoLines(const QString &first, const QString &second);

	PEAnalysis *analysis() const { return m_analysis.get(); }
	bool hasAnalysis() const { return static_cast<bool>(m_analysis); }

	QLabel *m_summaryLabel = nullptr;
	SimplePlot *m_plot = nullptr;

private:
	QWidget *m_central = nullptr;
	QVBoxLayout *m_rootLayout = nullptr;
	std::shared_ptr<PEAnalysis> m_analysis;
};

#endif // PEWINDOWBASE_H
