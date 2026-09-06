#pragma once
#include <QString>
#include <QVariant>
#include <algorithm>
#include <cmath>

// Display only: saved values and simulator inputs remain in SI base units.
inline QString engineeringValue(double value, const QString& unit) {
  static const char* prefixes[] = {"p", "n", "µ", "m", "", "k", "M", "G"};
  int group = 0;
  if (value != 0.0 && std::isfinite(value)) {
    group = static_cast<int>(std::floor(std::log10(std::abs(value)) / 3.0));
    group = std::max(-4, std::min(3, group));
  }
  double scaled = value / std::pow(1000.0, group);
  if (std::abs(scaled) >= 999.5 && group < 3) { scaled /= 1000.0; ++group; }
  return QString::number(scaled, 'g', 3) + " " + QString::fromUtf8(prefixes[group + 4]) + unit;
}
inline QString parameterDisplay(const QString& key, const QVariant& value) {
  QString unit;
  if (key == "Resistance" || key == "Transresistance") unit = QString::fromUtf8("Ω");
  else if (key == "Capacitance") unit = "F";
  else if (key == "Inductance") unit = "H";
  else if (key == "Voltage") unit = "V";
  else if (key == "Current") unit = "A";
  else if (key == "Transconductance") unit = "S";
  else return value.toString();
  bool ok = false;
  const double number = value.toDouble(&ok);
  return ok ? engineeringValue(number, unit) : value.toString();
}
