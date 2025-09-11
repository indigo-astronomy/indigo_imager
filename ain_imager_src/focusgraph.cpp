#include <QMainWindow>
#include <focusgraph.h>

FocusGraph::FocusGraph(QWidget *parent) : QCustomPlot(parent) {
    addGraph();
    graph(0)->setPen(QPen(Qt::red));
	addGraph();
	graph(1)->setPen(QPen(QColor(3,172,240)));
	setBackground(QBrush(QColor(0,0,0,0)));

    xAxis2->setVisible(true);
    xAxis2->setTickLabels(false);
    yAxis2->setVisible(true);
    yAxis2->setTickLabels(false);

	QPen axis_pen(QColor(150, 150, 150));
	xAxis->setBasePen(axis_pen);
    xAxis->setTickPen(axis_pen);
	xAxis->setSubTickPen(axis_pen);
	yAxis->setBasePen(axis_pen);
    yAxis->setTickPen(axis_pen);
	yAxis->setSubTickPen(axis_pen);
	xAxis2->setBasePen(axis_pen);
    xAxis2->setTickPen(axis_pen);
	xAxis2->setSubTickPen(axis_pen);
	yAxis2->setBasePen(axis_pen);
    yAxis2->setTickPen(axis_pen);
	yAxis2->setSubTickPen(axis_pen);
	xAxis->setAutoTickCount(4);
	yAxis->setAutoTickCount(5);
	xAxis2->setAutoTickCount(4);
	yAxis2->setAutoTickCount(5);
	xAxis->setRange(0, 1);
	xAxis->setTickLabelColor(QColor(255, 255, 255));
	yAxis->setTickLabelColor(QColor(255, 255, 255));

    // make left and bottom axes always transfer their ranges to right and top axes:
    connect(xAxis, SIGNAL(rangeChanged(QCPRange)), xAxis2, SLOT(setRange(QCPRange)));
    connect(yAxis, SIGNAL(rangeChanged(QCPRange)), yAxis2, SLOT(setRange(QCPRange)));
}

void FocusGraph::set_yaxis_range(double min, double max) {
	yAxis->setRange(min, max);
}

void FocusGraph::redraw_data(QVector<double> data) {
    QVector<double> x(data.size());
    for (int i=0; i< x.size(); ++i)
    {
        x[i] = i;
    }
    graph(0)->setData(x, data);
    graph(0)->rescaleKeyAxis();

	//set the range to ensure the last point is visible
	if (!data.isEmpty()) {
		xAxis->setRange(0, data.size() - 0.8);
	} else {
		xAxis->setRange(0, 1);
	}

    replot();
}

void FocusGraph::redraw_data2(QVector<double> data1, QVector<double> data2) {
	if (data1.size() != data2.size()) return;
    QVector<double> x(data1.size());
    for (int i=0; i< x.size(); ++i)
    {
        x[i] = i;
    }
    graph(0)->setData(x, data1);
    graph(0)->rescaleKeyAxis();
	graph(1)->setData(x, data2);
    graph(1)->rescaleKeyAxis();

	// set the range to ensure the last point is visible
	if (!data1.isEmpty()) {
		xAxis->setRange(0, data1.size() - 0.8);
	} else {
		xAxis->setRange(0, 1);
	}

    replot();
}
