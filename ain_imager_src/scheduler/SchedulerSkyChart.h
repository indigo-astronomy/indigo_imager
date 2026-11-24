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

#ifndef __SCHEDULERSKYCHART_H
#define __SCHEDULERSKYCHART_H

#include <QWidget>
#include <QList>
#include <QStringList>
#include <QPair>
#include <QPointer>
#include <QMap>

class QTimer;

/* Low precision positions for a chart, good to a fraction of a degree: the Sun and
   the Moon after the Astronomical Almanac, J2000 targets with the annual precession. Times
   are UTC in seconds since the epoch, RA in hours, declination and altitude in degrees. */
namespace SchedulerSky {
	void sun(qint64 t, double *ra, double *dec);
	/* the parallax of the Moon in degrees, for its topocentric altitude */
	void moon(qint64 t, double *ra, double *dec, double *parallax);
	double altitude(double ra, double dec, qint64 t, double latitude, double longitude);
	/* the altitude of J2000 coordinates, precessed to the date */
	double targetAltitude(double ra, double dec, qint64 t, double latitude, double longitude);
	double sunAltitude(qint64 t, double latitude, double longitude);
	double moonAltitude(qint64 t, double latitude, double longitude);
	/* the night shown: from an hour before sunset to an hour after sunrise, the night in
	   progress or the next one; a day without night shows the next 24 hours */
	QPair<qint64, qint64> nightWindow(qint64 now, double latitude, double longitude);
}

/* The altitude of the targets of the tasks over the night, with the twilight, the Moon,
   the runs of the night plan and the present time. */
class SchedulerSkyChart : public QWidget {
	Q_OBJECT
public:
	struct Target {
		QString name;
		double ra = 0;
		double dec = 0;
		double minAltitude = -1;	// @altitude, -1 without
	};
	struct Run {
		QString name;
		qint64 start = 0;
		qint64 end = 0;
		// the light of a finished run in the history, SchedulerState; a planned run has the color of its task
		int state = 0;
	};

	explicit SchedulerSkyChart(QWidget *parent = nullptr);

	void setSite(bool known, double latitude = 0, double longitude = 0);
	bool siteKnown() const { return m_site_known; }
	double latitude() const { return m_latitude; }
	double longitude() const { return m_longitude; }
	void setTargets(const QList<Target>& targets);
	/* the global altitude limits of the agent, 0 and 90 without */
	void setAltitudeLimits(double minimum, double maximum);
	void setRuns(const QList<Run>& runs);
	/* the finished runs of the history, in the color of their result */
	void setHistoryRuns(const QList<Run>& runs);
	/* the lights of the tasks by name, SchedulerState: the runs of a running task are yellow, of a done
	   one green, of a failed one red, the others have the color of the curve */
	void setTaskStates(const QMap<QString, int>& states);
	/* the selected task is drawn over the others */
	void setHighlighted(const QStringList& names);
	/* the theme sets its accent in the style sheet: the selection color of this widget is used */
	void setAccentSource(QWidget *widget);
	/* for tests: a fixed present time, 0 for the clock */
	void setNow(qint64 now);
	/* for tests: what the chart shows, as text */
	Q_INVOKABLE QString describe() const;

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	bool event(QEvent *event) override;

private:
	qint64 now() const;
	QRect plotRect() const;
	/* the time, the altitudes and the runs under the mouse */
	void showInfo(const QPoint& pos, const QPoint& global);

	bool m_site_known = false;
	double m_latitude = 0;
	double m_longitude = 0;
	QList<Target> m_targets;
	double m_min_altitude = 0;
	double m_max_altitude = 90;
	QList<Run> m_runs;
	QList<Run> m_history_runs;
	QMap<QString, int> m_task_states;
	QStringList m_highlighted;
	QPointer<QWidget> m_accent_source;
	qint64 m_now = 0;
	QTimer *m_timer;
};

#endif // __SCHEDULERSKYCHART_H
