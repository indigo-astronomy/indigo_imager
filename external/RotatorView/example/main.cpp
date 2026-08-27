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

/* A stand alone visualization of the RotatorView widget - every value the
   widget takes through its API is wired to a control here. "Slew" turns the
   rotator to the target the way a driver would, so the busy blink and the
   target marker can be watched. */

#include "RotatorView.h"

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	QWidget window;
	window.setWindowTitle("RotatorView");
	QHBoxLayout *layout = new QHBoxLayout(&window);

	RotatorView *view = new RotatorView;
	view->setTitle("Rotator");
	view->setPosition(0);
	layout->addWidget(view, 1);

	QWidget *panel = new QWidget;
	panel->setMaximumWidth(300);
	QVBoxLayout *panelLayout = new QVBoxLayout(panel);
	layout->addWidget(panel);

	QGroupBox *stateBox = new QGroupBox("Rotator");
	QFormLayout *stateForm = new QFormLayout(stateBox);
	panelLayout->addWidget(stateBox);

	QDoubleSpinBox *position = new QDoubleSpinBox;
	position->setRange(-720, 720);
	position->setDecimals(2);
	position->setSingleStep(1);
	stateForm->addRow("Position (°)", position);
	QObject::connect(position, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		[view](double value) { view->setPosition(value); });

	QDoubleSpinBox *target = new QDoubleSpinBox;
	target->setRange(-720, 720);
	target->setDecimals(2);
	target->setSingleStep(1);
	stateForm->addRow("Target (°)", target);
	QObject::connect(target, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		[view](double value) { view->setTarget(value); });

	QDoubleSpinBox *minimum = new QDoubleSpinBox;
	minimum->setRange(-720, 720);
	minimum->setValue(0);
	stateForm->addRow("Limit min (°)", minimum);

	QDoubleSpinBox *maximum = new QDoubleSpinBox;
	maximum->setRange(-720, 720);
	maximum->setValue(360);
	stateForm->addRow("Limit max (°)", maximum);

	auto applyLimits = [view, minimum, maximum]() {
		view->setLimits(minimum->value(), maximum->value());
	};
	QObject::connect(minimum, QOverload<double>::of(&QDoubleSpinBox::valueChanged), applyLimits);
	QObject::connect(maximum, QOverload<double>::of(&QDoubleSpinBox::valueChanged), applyLimits);

	QCheckBox *busy = new QCheckBox("Busy (blinks)");
	stateForm->addRow(busy);
	QObject::connect(busy, &QCheckBox::toggled, [view](bool on) {
		if (on) {
			view->setBusy();
		} else {
			view->setOK();
		}
	});

	QCheckBox *reversed = new QCheckBox("Reverse direction");
	stateForm->addRow(reversed);
	QObject::connect(reversed, &QCheckBox::toggled, [view](bool on) { view->setReversed(on); });

	QCheckBox *pick = new QCheckBox("Pick by dragging");
	pick->setChecked(true);
	stateForm->addRow(pick);
	QObject::connect(pick, &QCheckBox::toggled, [view](bool on) { view->setPickEnabled(on); });

	QGroupBox *lookBox = new QGroupBox("Appearance");
	QFormLayout *lookForm = new QFormLayout(lookBox);
	panelLayout->addWidget(lookBox);

	QCheckBox *readout = new QCheckBox("Show readout");
	readout->setChecked(true);
	lookForm->addRow(readout);
	QObject::connect(readout, &QCheckBox::toggled, [view](bool on) { view->setShowReadout(on); });

	QCheckBox *labels = new QCheckBox("Show labels");
	labels->setChecked(true);
	lookForm->addRow(labels);
	QObject::connect(labels, &QCheckBox::toggled, [view](bool on) { view->setShowLabels(on); });

	QLineEdit *title = new QLineEdit("Rotator");
	lookForm->addRow("Title", title);
	QObject::connect(title, &QLineEdit::textChanged, [view](const QString &text) { view->setTitle(text); });

	/* Turn the rotator to the target, one degree at a time, the short way
	   round - just enough movement to watch the view work. */
	QTimer *slew = new QTimer(&window);
	slew->setInterval(30);
	QObject::connect(slew, &QTimer::timeout, [view, position, busy, slew]() {
		double from = view->position();
		double to = view->target();
		double delta = fmod(to - from + 540.0, 360.0) - 180.0;
		if (fabs(delta) < 1.0) {
			position->setValue(to);
			busy->setChecked(false);
			slew->stop();
			return;
		}
		position->setValue(from + (delta > 0 ? 1.0 : -1.0));
	});

	QPushButton *go = new QPushButton("Slew to target");
	panelLayout->addWidget(go);
	QObject::connect(go, &QPushButton::clicked, [busy, slew]() {
		busy->setChecked(true);
		slew->start();
	});

	QLabel *picked = new QLabel("Picked: -");
	panelLayout->addWidget(picked);
	QObject::connect(view, &RotatorView::targetPicked, [picked, target](double angle) {
		picked->setText(QString("Picked: %1°").arg(angle, 0, 'f', 2));
		target->blockSignals(true);
		target->setValue(angle);
		target->blockSignals(false);
	});

	panelLayout->addStretch();

	window.resize(760, 460);
	window.show();
	return app.exec();
}
