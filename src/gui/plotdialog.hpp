#ifndef PLOTDIALOG_HPP
#define PLOTDIALOG_HPP

#include <QDialog>
#include <QVBoxLayout>
#include <complex>

using dcomplex = std::complex<double>;

#include "qcustomplot.h"

/**
 * @brief Class for displaying sine wave plots
 * for component voltages and currents
 *
 */
class PlotDialog : public QDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Plot Dialog object
   *
   * @param parent Parent widget
   * @param i Complex current of the clicked component
   * @param v Complex voltage of the clicked component
   * @param freq AC frequency of the circuit
   */
  PlotDialog(QWidget* parent, dcomplex i, dcomplex v, double freq);

  /**
   * @brief Destroy the Plot Dialog object
   *
   */
  ~PlotDialog();

  /**
   * @brief Calculate sine wave with RMS value and phase
   *
   * @param mag RMS value of voltage or current
   * @param phase Voltage or current phase
   * @param freq AC frequency
   * @param vec Reference to voltage or current sine vector
   */
  void calculateSineWave(double mag, double phase, double freq,
                         QVector<double>& vec);

  /**
   * @brief Initialize the time vector
   * for sine wave calculations
   *
   * @param freq AC frequency
   */
  void initTimeVector(double freq);

 private:
  QCustomPlot* plot_;
  QVector<double> i_sin;
  QVector<double> v_sin;
  QVector<double> t_vec;
};

#endif  // PLOTDIALOG_HPP