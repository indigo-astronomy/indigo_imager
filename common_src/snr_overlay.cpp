#include "snr_overlay.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QPaintEvent>
#include <cmath>  // Add this line for std::log10, std::min, etc.

SNROverlay::SNROverlay(QWidget *parent)
    : QWidget(parent)
    , m_opacity(0.85)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);  // Don't block mouse events

    m_info_label = new QLabel(this);
    m_info_label->setStyleSheet(
        "QLabel { "
        "  background-color: rgba(0, 0, 0, 40%); "  // Match histogram transparency
        "  color: rgb(200, 200, 200); "
        "  padding: 6px; "
        "  border-radius: 3px; "
        "  font-family: monospace; "
        "  font-size: 10px; "
        "}"
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
    QString qualityText;
    QString qualityColor;
    if (result.snr < 5.0) {
        qualityText = "Poor";
        qualityColor = "#FF0000";  // Red
    } else if (result.snr < 10.0) {
        qualityText = "Fair";
        qualityColor = "#FFA500";  // Orange
    } else if (result.snr < 20.0) {
        qualityText = "Good";
        qualityColor = "#FFFF00";  // Yellow
    } else if (result.snr < 50.0) {
        qualityText = "Very Good";
        qualityColor = "#90EE90";  // Light green
    } else {
        qualityText = "Excellent";
        qualityColor = "#00FF00";  // Green
    }

    QString info = QString(
        "<b>SNR: <font color='%1'>%2</font> (%3)</b><br>"
        "Signal: %4±%5<br>"
        "Background: %6±%7<br>"
        "HFD: %8 px<br>"
        "Center: (%9,%10)<br>"
        "Radius: %11px (%12/%13)"
    )
    .arg(qualityColor)
    .arg(result.snr, 0, 'f', 1)
    .arg(qualityText)
    .arg(result.signal_mean, 0, 'f', 0)
    .arg(result.signal_stddev, 0, 'f', 0)
    .arg(result.background_mean, 0, 'f', 0)
    .arg(result.background_stddev, 0, 'f', 0)
    .arg(result.hfd, 0, 'f', 2)
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
    // Simplified paint event - let the label's stylesheet handle the rendering
    QWidget::paintEvent(event);
}