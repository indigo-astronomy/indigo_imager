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

    m_info_label = new QLabel(this);
    m_info_label->setStyleSheet(
        "QLabel { "
        "  background-color: rgba(0, 0, 0, 180); "
        "  color: rgb(220, 220, 220); "
        "  padding: 8px; "
        "  border-radius: 4px; "
        "  font-family: monospace; "
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

    QString info = QString(
        "<b>📊 SNR Analysis</b><br>"
        "<table cellspacing='2'>"
        "<tr><td><b>SNR:</b></td><td align='right'><font color='#00FF00'>%1</font></td></tr>"
        "<tr><td><b>Signal:</b></td><td align='right'>%2 ± %3</td></tr>"
        "<tr><td><b>Background:</b></td><td align='right'>%4 ± %5</td></tr>"
        "<tr><td><b>Star center:</b></td><td align='right'>(%6, %7)</td></tr>"
        "<tr><td><b>Star radius:</b></td><td align='right'>%8 px</td></tr>"
        "<tr><td><b>Pixels:</b></td><td align='right'>%9 / %10</td></tr>"
        "</table>"
    )
    .arg(result.snr, 0, 'f', 1)
    .arg(result.signal_mean, 0, 'f', 1)
    .arg(result.signal_stddev, 0, 'f', 1)
    .arg(result.background_mean, 0, 'f', 1)
    .arg(result.background_stddev, 0, 'f', 1)
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
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set global opacity for the entire widget
    painter.setOpacity(m_opacity);

    // Draw the background and border for the info box
    QRect labelRect = m_info_label->geometry();

    // Draw a subtle shadow effect
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 60));
    painter.drawRoundedRect(labelRect.adjusted(2, 2, 2, 2), 4, 4);

    // Draw the main background
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.drawRoundedRect(labelRect, 4, 4);

    // Draw a border
    QPen borderPen(QColor(100, 100, 100, 200), 1);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(labelRect, 4, 4);

    // If we have a valid SNR result, draw additional visual indicators
    if (m_result.valid) {
        // Draw a colored bar indicating SNR quality
        int barWidth = labelRect.width() - 16;
        int barHeight = 4;
        int barX = labelRect.x() + 8;
        int barY = labelRect.bottom() - 10;

        // Determine color based on SNR value
        QColor barColor;
        if (m_result.snr < 5.0) {
            barColor = QColor(255, 0, 0);  // Red - poor SNR
        } else if (m_result.snr < 10.0) {
            barColor = QColor(255, 165, 0);  // Orange - fair SNR
        } else if (m_result.snr < 20.0) {
            barColor = QColor(255, 255, 0);  // Yellow - good SNR
        } else if (m_result.snr < 50.0) {
            barColor = QColor(144, 238, 144);  // Light green - very good SNR
        } else {
            barColor = QColor(0, 255, 0);  // Green - excellent SNR
        }

        // Draw SNR quality indicator bar
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(50, 50, 50));
        painter.drawRect(barX, barY, barWidth, barHeight);

        // Fill the bar based on SNR value (logarithmic scale)
        double snr_clamped = std::min(m_result.snr, 100.0);
        double fillRatio = std::log10(snr_clamped + 1) / std::log10(101.0);
        int fillWidth = static_cast<int>(barWidth * fillRatio);

        painter.setBrush(barColor);
        painter.drawRect(barX, barY, fillWidth, barHeight);

        // Draw border around the bar
        painter.setPen(QPen(QColor(80, 80, 80), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(barX, barY, barWidth, barHeight);

        // Add a small indicator showing quality level text
        QString qualityText;
        if (m_result.snr < 5.0) {
            qualityText = "Poor";
        } else if (m_result.snr < 10.0) {
            qualityText = "Fair";
        } else if (m_result.snr < 20.0) {
            qualityText = "Good";
        } else if (m_result.snr < 50.0) {
            qualityText = "Very Good";
        } else {
            qualityText = "Excellent";
        }

        // Draw quality text below the bar
        QFont smallFont = painter.font();
        smallFont.setPointSize(7);
        painter.setFont(smallFont);
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(
            QRect(barX, barY + barHeight + 2, barWidth, 12),
            Qt::AlignCenter,
            qualityText
        );
    }

    // Call base class implementation to ensure proper widget rendering
    QWidget::paintEvent(event);
}