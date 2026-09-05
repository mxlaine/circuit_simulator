#include "componentdialog.hpp"

#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

ComponentDialog::ComponentDialog(const GuiComponent& comp, QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Edit Component");

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  QFormLayout* formLayout = new QFormLayout();

  typeLineEdit = new QLineEdit(comp.type, this);
  formLayout->addRow("Type:", typeLineEdit);

  for (auto it = comp.parameters.constBegin(); it != comp.parameters.constEnd();
       ++it) {
    QLineEdit* lineEdit = new QLineEdit(it.value().toString(), this);
    formLayout->addRow(it.key() + ":", lineEdit);
    parameterEdits.insert(it.key(), lineEdit);
  }

  mainLayout->addLayout(formLayout);

  QHBoxLayout* buttonLayout = new QHBoxLayout();
  QPushButton* okButton = new QPushButton("OK", this);
  QPushButton* cancelButton = new QPushButton("Cancel", this);

  buttonLayout->addWidget(okButton);
  buttonLayout->addWidget(cancelButton);

  mainLayout->addLayout(buttonLayout);

  connect(okButton, &QPushButton::clicked, this, &ComponentDialog::accept);
  connect(cancelButton, &QPushButton::clicked, this, &ComponentDialog::reject);
}

QString ComponentDialog::componentType() const { return typeLineEdit->text(); }

QMap<QString, QVariant> ComponentDialog::parameters() const {
  QMap<QString, QVariant> params;
  for (auto it = parameterEdits.constBegin(); it != parameterEdits.constEnd();
       ++it) {
    params.insert(it.key(), it.value()->text());
  }
  return params;
}
