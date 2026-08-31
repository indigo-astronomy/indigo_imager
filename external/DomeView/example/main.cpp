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

/* A stand alone visualization of the DomeView widget - every value the widget
   takes through its API is wired to a control here. */

#include "DomeView.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>

namespace {

struct Row {
	QSlider *slider;
	QDoubleSpinBox *spin;

	double value() const { return spin->value(); }
	void setValue(double value) { spin->setValue(value); }
};

Row *addRow(
	QFormLayout *form,
	const QString &label,
	double min,
	double max,
	double step,
	int decimals,
	double value,
	const std::function<void(double)> &apply
) {
	Row *row = new Row;
	row->slider = new QSlider(Qt::Horizontal);
	row->spin = new QDoubleSpinBox();

	double factor = pow(10.0, decimals);
	row->slider->setRange(qRound(min * factor), qRound(max * factor));
	row->slider->setSingleStep(qMax(1, qRound(step * factor)));
	row->spin->setRange(min, max);
	row->spin->setSingleStep(step);
	row->spin->setDecimals(decimals);
	row->spin->setValue(value);
	row->slider->setValue(qRound(value * factor));

	QObject::connect(row->slider, &QSlider::valueChanged, row->spin, [row, factor, apply](int position) {
		double value = position / factor;
		QSignalBlocker blocker(row->spin);
		row->spin->setValue(value);
		apply(row->spin->value());
	});
	QObject::connect(row->spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), row->slider,
		[row, factor, apply](double value) {
			QSignalBlocker blocker(row->slider);
			row->slider->setValue(qRound(value * factor));
			apply(value);
		}
	);

	QWidget *container = new QWidget();
	QHBoxLayout *layout = new QHBoxLayout(container);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(row->slider, 1);
	layout->addWidget(row->spin, 0);
	form->addRow(label, container);
	return row;
}

/* Shortest signed way from one azimuth to another, in degrees. */
double azimuthDelta(double from, double to) {
	double delta = fmod(to - from + 540.0, 360.0) - 180.0;
	return delta;
}

}  // namespace

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	app.setApplicationName("DomeView demo");

	QWidget window;
	window.setWindowTitle("DomeView demo");

	DomeView *dome = new DomeView();
	dome->setTitle("Observatory dome");
	dome->setDomeDimensions(2.5, 1.0, 0.0, 0.0, 0.6, 0.5);
	dome->setLatitude(43.0);
	dome->setTubeLength(1.2);
	dome->setApertureDiameter(0.25);
	dome->setDomeAzimuth(120.0);
	dome->setShutterPosition(1.0);
	dome->setTelescopeCoordinates(120.0, 45.0);
	dome->setMinimumSize(420, 420);

	QWidget *panel = new QWidget();
	panel->setMaximumWidth(390);
	QVBoxLayout *panelLayout = new QVBoxLayout(panel);

	QGroupBox *dimensions = new QGroupBox("DOME_DIMENSION (m)");
	QFormLayout *dimensionsForm = new QFormLayout(dimensions);
	addRow(dimensionsForm, "Radius", 0.5, 12.0, 0.1, 2, dome->domeRadius(),
		[dome](double value) { dome->setDomeRadius(value); });
	addRow(dimensionsForm, "Shutter width", 0.1, 8.0, 0.1, 2, dome->shutterWidth(),
		[dome](double value) { dome->setShutterWidth(value); });
	addRow(dimensionsForm, "Pivot N/S", -5.0, 5.0, 0.1, 2, dome->mountPivotOffsetNS(),
		[dome](double value) { dome->setMountPivotOffsetNS(value); });
	addRow(dimensionsForm, "Pivot E/W", -5.0, 5.0, 0.1, 2, dome->mountPivotOffsetEW(),
		[dome](double value) { dome->setMountPivotOffsetEW(value); });
	addRow(dimensionsForm, "Pivot vertical", -3.0, 5.0, 0.1, 2, dome->mountPivotVerticalOffset(),
		[dome](double value) { dome->setMountPivotVerticalOffset(value); });
	addRow(dimensionsForm, "OTA offset", 0.0, 3.0, 0.05, 2, dome->mountPivotOtaOffset(),
		[dome](double value) { dome->setMountPivotOtaOffset(value); });
	addRow(dimensionsForm, "Latitude", -90.0, 90.0, 1.0, 1, dome->latitude(),
		[dome](double value) { dome->setLatitude(value); });
	panelLayout->addWidget(dimensions);

	QGroupBox *domeState = new QGroupBox("Dome");
	QFormLayout *domeForm = new QFormLayout(domeState);
	QComboBox *typeSelect = new QComboBox();
	typeSelect->addItem("Classic dome", DomeView::DomeTypeClassic);
	typeSelect->addItem("Half dome", DomeView::DomeTypeHalfDome);
	typeSelect->addItem("Clamshell", DomeView::DomeTypeClamshell);
	QObject::connect(typeSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), dome, [dome, typeSelect](int) {
		dome->setDomeType(static_cast<DomeView::DomeType>(typeSelect->currentData().toInt()));
	});
	domeForm->addRow("Type", typeSelect);
	Row *domeAz = addRow(domeForm, "Azimuth", 0.0, 360.0, 1.0, 1, dome->domeAzimuth(),
		[dome](double value) { dome->setDomeAzimuth(value); });
	QCheckBox *domeBusy = new QCheckBox("Rotation busy (wall blinks)");
	QObject::connect(domeBusy, &QCheckBox::toggled, dome, [dome](bool on) {
		if (on) {
			dome->setDomeBusy();
		} else {
			dome->setDomeOK();
		}
	});
	domeForm->addRow(QString(), domeBusy);
	Row *shutter = addRow(domeForm, "Shutter", 0.0, 1.0, 0.05, 2, dome->shutterPosition(),
		[dome](double value) { dome->setShutterPosition(value); });
	QPushButton *toggle = new QPushButton("Open / close shutter");
	domeForm->addRow(QString(), toggle);
	QCheckBox *busy = new QCheckBox("Shutter busy (blinks)");
	QObject::connect(busy, &QCheckBox::toggled, dome, [dome](bool on) {
		if (on) {
			dome->setShutterBusy();
		} else {
			dome->setShutterOK();
		}
	});
	domeForm->addRow(QString(), busy);
	panelLayout->addWidget(domeState);

	/* Where the shutter is being driven to - the timer walks it there. */
	double *shutterTarget = new double(dome->shutterPosition());
	QObject::connect(toggle, &QPushButton::clicked, dome, [shutterTarget, shutter]() {
		*shutterTarget = (shutter->value() > 0.5) ? 0.0 : 1.0;
	});

	QGroupBox *scopeState = new QGroupBox("Telescope");
	QFormLayout *scopeForm = new QFormLayout(scopeState);
	Row *scopeAz = addRow(scopeForm, "Azimuth", 0.0, 360.0, 1.0, 1, dome->telescopeAzimuth(),
		[dome](double value) { dome->setTelescopeAzimuth(value); });
	Row *scopeAlt = addRow(scopeForm, "Altitude", 0.0, 90.0, 1.0, 1, dome->telescopeAltitude(),
		[dome](double value) { dome->setTelescopeAltitude(value); });
	addRow(scopeForm, "Tube length", 0.1, 4.0, 0.05, 2, dome->tubeLength(),
		[dome](double value) { dome->setTubeLength(value); });
	addRow(scopeForm, "Lens / mirror", 0.03, 1.5, 0.01, 2, dome->apertureDiameter(),
		[dome](double value) { dome->setApertureDiameter(value); });
	QComboBox *side = new QComboBox();
	side->addItem("Auto (counterweight down)", DomeView::SideOfPierAuto);
	side->addItem("East", DomeView::SideOfPierEast);
	side->addItem("West", DomeView::SideOfPierWest);
	QObject::connect(side, QOverload<int>::of(&QComboBox::currentIndexChanged), dome, [dome, side](int) {
		dome->setSideOfPier(static_cast<DomeView::SideOfPier>(side->currentData().toInt()));
	});
	scopeForm->addRow("Pier side", side);
	panelLayout->addWidget(scopeState);

	QGroupBox *animation = new QGroupBox("Animation");
	QVBoxLayout *animationLayout = new QVBoxLayout(animation);
	QCheckBox *follow = new QCheckBox("Dome follows the telescope");
	follow->setChecked(true);
	QCheckBox *drift = new QCheckBox("Telescope tracks the sky");
	QPushButton *sync = new QPushButton("Sync dome now");
	animationLayout->addWidget(follow);
	animationLayout->addWidget(drift);
	animationLayout->addWidget(sync);
	panelLayout->addWidget(animation);

	QGroupBox *look = new QGroupBox("Appearance");
	QFormLayout *lookForm = new QFormLayout(look);
	addRow(lookForm, "Dome opacity", 0.0, 1.0, 0.05, 2, dome->domeOpacity(),
		[dome](double value) { dome->setDomeOpacity(value); });
	QComboBox *background = new QComboBox();
	background->addItem("Transparent", QColor(0, 0, 0, 0));
	background->addItem("Dark", QColor(28, 30, 34));
	background->addItem("Black", QColor(0, 0, 0));
	background->addItem("Light", QColor(232, 232, 236));
	QObject::connect(background, QOverload<int>::of(&QComboBox::currentIndexChanged), dome, [dome, background](int) {
		dome->setBackgroundColor(background->currentData().value<QColor>());
	});
	lookForm->addRow("Background", background);

	QCheckBox *compass = new QCheckBox("Compass");
	compass->setChecked(true);
	QObject::connect(compass, &QCheckBox::toggled, dome, [dome](bool on) { dome->setShowCompass(on); });
	QCheckBox *labels = new QCheckBox("Title");
	labels->setChecked(true);
	QObject::connect(labels, &QCheckBox::toggled, dome, [dome](bool on) { dome->setShowLabels(on); });
	QCheckBox *status = new QCheckBox("Status line");
	status->setChecked(true);
	QObject::connect(status, &QCheckBox::toggled, dome, [dome](bool on) { dome->setShowStatus(on); });
	lookForm->addRow(QString(), compass);
	lookForm->addRow(QString(), labels);
	lookForm->addRow(QString(), status);
	panelLayout->addWidget(look);

	QLabel *readout = new QLabel();
	readout->setWordWrap(true);
	panelLayout->addWidget(readout);
	panelLayout->addStretch(1);

	QObject::connect(sync, &QPushButton::clicked, dome, [dome, domeAz]() {
		domeAz->setValue(dome->requiredDomeAzimuth());
	});

	QHBoxLayout *mainLayout = new QHBoxLayout(&window);
	mainLayout->addWidget(dome, 1);
	mainLayout->addWidget(panel, 0);

	/* 25 frames per second - the dome slews at 6 deg/s and the telescope
	   drifts a bit faster, so the dome is seen chasing the slit position. */
	QTimer timer;
	QObject::connect(&timer, &QTimer::timeout, dome, [=]() {
		const double interval = 0.04;
		if (drift->isChecked()) {
			scopeAz->setValue(fmod(scopeAz->value() + 4.0 * interval, 360.0));
			double alt = scopeAlt->value() + 1.5 * interval;
			scopeAlt->setValue(alt > 88.0 ? 10.0 : alt);
		}
		double travel = *shutterTarget - shutter->value();
		if (fabs(travel) > 0.005) {
			/* The leaves take about four seconds end to end. The busy state is
			   set on its own, it does not follow the travel. */
			shutter->setValue(shutter->value() + qBound(-0.25 * interval, travel, 0.25 * interval));
		}
		if (follow->isChecked()) {
			double delta = azimuthDelta(dome->domeAzimuth(), dome->requiredDomeAzimuth());
			if (fabs(delta) > 0.2) {
				double stepSize = qBound(-6.0 * interval, delta, 6.0 * interval);
				domeAz->setValue(fmod(dome->domeAzimuth() + stepSize + 360.0, 360.0));
			}
		}
		readout->setText(QString("Required dome azimuth: %1°\nLine of sight: %2")
			.arg(dome->requiredDomeAzimuth(), 0, 'f', 1)
			.arg(dome->isTelescopeBlocked() ? "blocked" : "clear"));
	});
	timer.start(40);

	window.resize(980, 640);
	window.show();
	return app.exec();
}
