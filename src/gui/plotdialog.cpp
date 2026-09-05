#include "plotdialog.hpp"

#include <QPen>

PlotDialog::PlotDialog(QWidget* parent, dcomplex i, dcomplex v, double freq)
    : QDialog(parent) {
  resize(600, 400);

  double i_rms = std::abs(i);
  double i_arg = std::arg(i);

  double v_rms = std::abs(v);
  double v_arg = std::arg(v);

  plot_ = new QCustomPlot(this);

  plot_->xAxis->setLabel("t (s)");
  plot_->xAxis->setRange(0, 3 / freq);

  plot_->yAxis->setLabel("Voltage (V)");
  plot_->yAxis->setRange(-2 * v_rms, 2 * v_rms);

  plot_->yAxis2->setVisible(true);
  plot_->yAxis2->setTickLabels(true);
  plot_->yAxis2->setLabel("Current (A)");
  plot_->yAxis2->setRange(-2 * i_rms, 2 * i_rms);

  initTimeVector(freq);

  calculateSineWave(v_rms, v_arg, freq, v_sin);
  calculateSineWave(i_rms, i_arg, freq, i_sin);

  plot_->addGraph(plot_->xAxis, plot_->yAxis);
  plot_->graph(0)->setPen(QPen(Qt::red));
  plot_->graph(0)->setData(t_vec, v_sin);
  plot_->graph(0)->setName("Voltage");

  plot_->addGraph(plot_->xAxis, plot_->yAxis2);
  plot_->graph(1)->setPen(QPen(Qt::blue));
  plot_->graph(1)->setData(t_vec, i_sin);
  plot_->graph(1)->setName("Current");

  plot_->replot();

  plot_->legend->setVisible(true);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(plot_);
  setLayout(layout);

  setWindowTitle("Component current and voltage");
}

void PlotDialog::calculateSineWave(double rms, double phase, double freq,
                                   QVector<double>& vec) {
  int total_samples = t_vec.size();
  double omega = 2.0 * M_PI * freq;

  vec.reserve(total_samples);
  for (auto t : t_vec) {
    double val = std::sqrt(2.0) * rms * std::sin(omega * t + phase);
    vec.push_back(val);
  }
}

void PlotDialog::initTimeVector(double freq) {
  double sample_rate = 30 * freq;
  double duration = 3 / freq;
  int total_samples = sample_rate * duration;

  t_vec.reserve(total_samples);
  for (int n = 0; n < total_samples; ++n) {
    t_vec.push_back(n / sample_rate);
  }
}

PlotDialog::~PlotDialog() { delete plot_; }
