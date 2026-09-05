/**
 * @file componentdialog.hpp
 * @brief Declaration of the ComponentDialog class for editing circuit component
 * properties in a Qt dialog.
 *
 * This class provides a dialog interface for viewing and editing
 * the properties of a circuit component. The dialog allows users to modify the
 * component type and its parameters using dynamically generated input fields.
 *
 */
#ifndef COMPONENTDIALOG_HPP
#define COMPONENTDIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QMap>

#include "component.hpp"

class ComponentDialog : public QDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Component Dialog object
   *
   * @param comp
   * @param parent
   */
  explicit ComponentDialog(const GuiComponent& comp, QWidget* parent = nullptr);

  QString componentType() const;

  /**
   * @brief Get the parameters of the component.
   * @return QMap<QString, QVariant>
   */

  QMap<QString, QVariant> parameters() const;

 private:
  /**
   * @brief Line edit for the component type.
   *
   */
  QLineEdit* typeLineEdit;

  /**
   * @brief Map of parameter names to their corresponding line edits.
   *
   */
  QMap<QString, QLineEdit*> parameterEdits;
};

#endif  // COMPONENTDIALOG_HPP
