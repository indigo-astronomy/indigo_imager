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

#include "pewindowbase.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include "peanalysis.h"
#include "verticallabel.h"

#include <simpleplot.h>

PEWindowBase::PEWindowBase(const QString &title, QWidget *parent)
    : QMainWindow(parent, Qt::Window) {
	setWindowTitle(title);
	resize(1000, 620);
	setWindowIcon(QIcon(":/resource/ain_guidelog_viewer.png"));

	QFile f(":/resource/control_panel.qss");
	if (f.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream ts(&f);
		setStyleSheet(ts.readAll());
		f.close();
	}

	m_central = new QWidget(this);
	setCentralWidget(m_central);

	m_rootLayout = new QVBoxLayout(m_central);
	m_rootLayout->setContentsMargins(6, 6, 6, 6);
	m_rootLayout->setSpacing(6);
}

void PEWindowBase::addSummaryRow(QWidget *trailing) {
	m_summaryLabel = new QLabel(m_central);
	m_summaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_summaryLabel->setTextFormat(Qt::RichText);
	m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	QFont summaryFont = m_summaryLabel->font();
	if (summaryFont.pointSizeF() > 0.0) {
		summaryFont.setPointSizeF(summaryFont.pointSizeF() + 2.0);
	} else {
		summaryFont.setPixelSize(summaryFont.pixelSize() + 2);
	}
	m_summaryLabel->setFont(summaryFont);

	if (trailing) {
		QHBoxLayout *summaryRow = new QHBoxLayout();
		summaryRow->addWidget(m_summaryLabel, 1);
		summaryRow->addWidget(trailing, 0, Qt::AlignRight);
		m_rootLayout->addLayout(summaryRow);
	} else {
		m_rootLayout->addWidget(m_summaryLabel);
	}
}

void PEWindowBase::addPlotRow() {
	m_plot = new SimplePlot(SimplePlot::Graph, m_central);
	m_plot->setPlotMargins(56, 12, 16, 28);
	m_plot->xAxis2->setVisible(true);
	m_plot->yAxis2->setVisible(true);
	m_plot->xAxis2->setTickLabels(false);
	m_plot->yAxis2->setTickLabels(false);

	// SimplePlot's Graph mode does not paint axis captions, so draw them as
	// separate widgets: a rotated label to the left of the plot for Y, and a
	// centered label beneath it for X.
	m_yCaptionLabel = new VerticalLabel(m_central);

	QHBoxLayout *plotRow = new QHBoxLayout();
	plotRow->setContentsMargins(0, 0, 0, 0);
	plotRow->setSpacing(2);
	plotRow->addWidget(m_yCaptionLabel);
	plotRow->addWidget(m_plot, 1);
	m_rootLayout->addLayout(plotRow, 1);

	m_xCaptionLabel = new QLabel(m_central);
	m_xCaptionLabel->setAlignment(Qt::AlignHCenter);
	m_rootLayout->addWidget(m_xCaptionLabel);

	// Keep both captions visually identical (the rotated one otherwise inherits
	// a different effective font than the styled QLabel).
	m_yCaptionLabel->setFont(m_xCaptionLabel->font());
}

void PEWindowBase::setSession(const std::shared_ptr<PEAnalysis> &analysis,
                              double calibrationPxPerS,
                              double mountDecDeg) {
	m_analysis = analysis;
	applyLogDefaults(calibrationPxPerS, mountDecDeg);
	recompute();
}

void PEWindowBase::applyLogDefaults(double, double) {
}

void PEWindowBase::showPlaceholder(const QString &message) {
	if (m_summaryLabel) {
		m_summaryLabel->setText(message);
	}
	if (m_plot) {
		m_plot->clearGraphs();
		m_plot->clearCustomXAxisTicks();
		m_plot->xAxis->setRange(0, 1);
		m_plot->yAxis->setRange(-1, 1);
		m_plot->replot();
	}
}

QString PEWindowBase::askForCsvPath(const QString &dialogTitle) {
	const QString location = QDir::toNativeSeparators(QDir::homePath());
	QString fileName = QFileDialog::getSaveFileName(this, dialogTitle, location,
	                                                tr("CSV files (*.csv);;All files (*)"));
	if (fileName.isEmpty()) {
		return QString();
	}
	if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
		fileName += ".csv";
	}
	return fileName;
}

QString PEWindowBase::number(double value, int precision) {
	return QString::number(value, 'f', precision);
}

QString PEWindowBase::separator() {
	return QStringLiteral(" &nbsp;&nbsp;&middot;&nbsp;&nbsp; ");
}

QString PEWindowBase::twoLines(const QString &first, const QString &second) {
	// Rendered as two blocks rather than joined with <br> so the gap between
	// them is ours to set.
	return QStringLiteral("<div style='margin:0'>") + first +
	       QStringLiteral("</div><div style='margin-top:7px'>") + second +
	       QStringLiteral("</div>");
}
