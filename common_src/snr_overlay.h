#ifndef SNR_OVERLAY_H
#define SNR_OVERLAY_H

#include <QWidget>
#include <QLabel>
#include "snr_calculator.h"

class SNROverlay : public QWidget {
    Q_OBJECT

public:
    explicit SNROverlay(QWidget *parent = nullptr);
    void setSNRResult(const SNRResult &result);
    void setWidgetOpacity(double opacity);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_info_label;
    SNRResult m_result;
    double m_opacity;
};

#endif // SNR_OVERLAY_H