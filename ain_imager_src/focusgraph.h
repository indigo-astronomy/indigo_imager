#ifndef FOCUSGRAPH_H
#define FOCUSGRAPH_H

#include <QMainWindow>
#include <qcustomplot/qcustomplot.h>

class FocusGraph : public QCustomPlot
{
	Q_OBJECT
public:
	explicit FocusGraph(QWidget *parent = 0);

signals:

public slots:
	 void set_yaxis_range(double min, double max);
	 void redraw_data(QVector<double> data);
	 void redraw_data2(QVector<double> data1, QVector<double> data2);
};

#endif // FOCUSGRAPH_H
