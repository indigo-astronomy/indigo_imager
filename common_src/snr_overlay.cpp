#include "snr_overlay.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QPaintEvent>
#include <cmath>

SNROverlay::SNROverlay(QWidget *parent)
	: QWidget(parent)
	, m_opacity(0.85)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);  // Don't block mouse events

	m_info_label = new QLabel(this);
	m_info_label->setStyleSheet(
		"background-color: rgba(0,0,0,40%); "
		"color: rgba(200,200,200,100%); "
		"font-family: monospace; "
		"font-size: 12px; "
		"padding: 6px;"
	);
	m_info_label->setTextFormat(Qt::RichText);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_info_label);
	layout->setContentsMargins(0, 0, 0, 0);
}

void SNROverlay::setSNRResult(const SNRResult &result) {
	m_result = result;

	if (!result.valid) {
		m_info_label->setText("<b>No star detected</b>");
		adjustSize();
		return;
	}

	// Determine quality text and color based on SNR value
	// Scale based on typical astronomical requirements:
	// SNR < 3: Barely detectable (3-sigma detection limit)
	// SNR 3-5: Detection but unreliable photometry
	// SNR 5-10: Usable for basic photometry
	// SNR 10-20: Good photometry
	// SNR 20-50: Very good photometry (< 5% error)
	// SNR 50+: Excellent precision (< 2% error)
	QString qualityText;
	QString qualityColor;
	if (result.is_saturated) {
		qualityText = "SATURATED";
		qualityColor = "#FF00FF";  // Magenta - warning color
	} else if (result.snr < 3.0) {
		qualityText = "Noisy";
		qualityColor = "#FF0000";  // Red
	} else if (result.snr < 5.0) {
		qualityText = "Weak";
		qualityColor = "#FF6600";  // Orange-red
	} else if (result.snr < 10.0) {
		qualityText = "Fair";
		qualityColor = "#FFA500";  // Orange
	} else if (result.snr < 20.0) {
		qualityText = "Good";
		qualityColor = "#FFFF00";  // Yellow
	} else if (result.snr < 50.0) {
		qualityText = "Very Good";
		qualityColor = "#9ABD32";  // Yellow-green
	} else {
		qualityText = "Excellent";
		qualityColor = "#00DD00";  // Bright green
	}

	QString info = QString(
		"<table cellspacing='0' cellpadding='0' style='border-collapse: collapse;'>"
		"<tr><td><b>SNR:</b></td><td align='right'><b><font color='%1'>%2</font> (%3)</b></td></tr>"
		"<tr><td>Peak:</td><td align='right'>%4 ADU</td></tr>"
		"<tr><td>Total Flux:</td><td align='right'>%5 ADU</td></tr>"
		"<tr><td>Signal mean:</td><td align='right'>%6±%7 ADU</td></tr>"
		"<tr><td>Background mean:</td><td align='right'>%8±%9 ADU</td></tr>"
		"<tr><td>HFD:</td><td align='right'>%10 px</td></tr>"
		"<tr><td>Eccentricity:</td><td align='right'>%11</td></tr>"
		"<tr><td>Center:</td><td align='right'>(%12,%13)</td></tr>"
		"<tr><td>Radius:</td><td align='right'>%14px (%15/%16)</td></tr>"
		"</table>"
	)
	.arg(qualityColor)
	.arg(result.snr, 0, 'f', 1)
	.arg(qualityText)
	.arg(result.peak_value, 0, 'f', 0)
	.arg(result.total_flux, 0, 'f', 0)
	.arg(result.signal_mean, 0, 'f', 0)
	.arg(result.signal_stddev, 0, 'f', 0)
	.arg(result.background_mean, 0, 'f', 0)
	.arg(result.background_stddev, 0, 'f', 0)
	.arg(result.hfd, 0, 'f', 2)
	.arg(result.eccentricity, 0, 'f', 3)
	.arg(result.star_x, 0, 'f', 1)
	.arg(result.star_y, 0, 'f', 1)
	.arg(result.star_radius, 0, 'f', 1)
	.arg(result.star_pixels)
	.arg(result.background_pixels);

	m_info_label->setText(info);
	adjustSize();
	update();
}

void SNROverlay::setWidgetOpacity(double opacity) {
	m_opacity = opacity;
	update();
}

void SNROverlay::paintEvent(QPaintEvent *event) {
	QWidget::paintEvent(event);
}