#include "ProtocolSettingsWidget.hpp"

#include "ProtocolFactory.hpp"
#include "SpecificSettings.hpp"

#include <Library/LibrarySettings.hpp>
#include <State/Widgets/AddressFragmentLineEdit.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/model/tree/TreeNodeItemModel.hpp>
#include <score/tools/FindStringInFile.hpp>

#include <ossia/detail/config.hpp>

#include <ossia/detail/flat_map.hpp>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDirIterator>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVariant>

#include <wobjectimpl.h>

W_OBJECT_IMPL(SpatGRIS::ProtocolSettingsWidget)

namespace SpatGRIS
{
ProtocolSettingsWidget::ProtocolSettingsWidget(QWidget* parent)
    : Device::ProtocolSettingsWidget(parent)
{
  m_deviceNameEdit = new State::AddressFragmentLineEdit{this};
  m_deviceNameEdit->setText("SpatGRIS");

  m_host = new QLineEdit(this);
  m_host->setText("127.0.0.1");
  m_host->setWhatsThis(
      tr("IP address of the computer SpatGRIS is running on."));

  m_port = new QSpinBox(this);
  m_port->setRange(0, 65535);
  m_port->setValue(18032);
  m_port->setWhatsThis(
      tr("On which port SpatGRIS is expecting OSC messages."));

  m_control = new QSpinBox{this};
  m_control->setRange(1, 256);
  m_control->setValue(32);

  auto layout = new QFormLayout;
  layout->addRow(tr("Name"), m_deviceNameEdit);
  layout->addRow(tr("Host"), m_host);
  layout->addRow(tr("Port"), m_port);
  layout->addRow(tr("Source count"), m_control);

  setLayout(layout);
}

ProtocolSettingsWidget::~ProtocolSettingsWidget() { }

Device::DeviceSettings ProtocolSettingsWidget::getSettings() const
{
  // TODO should be = m_settings to follow the other patterns.
  Device::DeviceSettings s;
  s.name = m_deviceNameEdit->text();
  s.protocol = ProtocolFactory::static_concreteKey();

  SpecificSettings settings{};
  settings.host = this->m_host->text();
  settings.port = this->m_port->value();
  settings.sources = this->m_control->value();
  s.deviceSpecificSettings = QVariant::fromValue(settings);

  return s;
}

void ProtocolSettingsWidget::setSettings(
    const Device::DeviceSettings& settings)
{
  m_deviceNameEdit->setText(settings.name);
  const auto& specif
      = settings.deviceSpecificSettings.value<SpecificSettings>();
  m_host->setText(specif.host);
  m_port->setValue(specif.port);
  m_control->setValue(specif.sources);
}
}
