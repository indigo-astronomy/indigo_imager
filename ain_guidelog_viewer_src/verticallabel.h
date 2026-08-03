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

#ifndef VERTICALLABEL_H
#define VERTICALLABEL_H

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QWidget>

// A thin widget that draws a single line of text rotated 90° (reading
// bottom-to-top), used for a plot's vertical Y-axis caption. No signals/slots,
// so no Q_OBJECT / moc needed.
class VerticalLabel : public QWidget {
public:
	explicit VerticalLabel(QWidget *parent = nullptr) : QWidget(parent) {}

	void setText(const QString &text) {
		m_text = text;
		updateGeometry();
		update();
	}

	QSize sizeHint() const override {
		const QFontMetrics fm(font());
		return QSize(fm.height() + 4, fm.horizontalAdvance(m_text) + 12);
	}
	QSize minimumSizeHint() const override {
		return QSize(sizeHint().width(), 0);
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::TextAntialiasing, true);
		p.setPen(QColor(0xef, 0xf0, 0xf1)); // match the qdarkstyle caption text
		p.setFont(font());
		p.translate(0, height());
		p.rotate(-90.0);
		p.drawText(QRect(0, 0, height(), width()), Qt::AlignCenter, m_text);
	}

private:
	QString m_text;
};

#endif // VERTICALLABEL_H
