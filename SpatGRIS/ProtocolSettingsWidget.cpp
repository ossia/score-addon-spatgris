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
  checkForChanges(m_deviceNameEdit);

  m_host = new QLineEdit(this);
  m_host->setText("127.0.0.1");
  m_host->setWhatsThis(
      tr("IP address of the computer the app is running on."));

  m_port = new QSpinBox(this);
  m_port->setRange(0, 65535);
  m_port->setValue(18032);
  m_port->setWhatsThis(tr("On which port the app is expecting OSC messages."));

  m_control = new QSpinBox{this};
  m_control->setRange(1, 256);
  m_control->setValue(32);
  
  m_format = new QComboBox{this};
  m_format->addItem("SpatGRIS");
  m_format->addItem("ADM-OSC");
  m_format->addItem("SPAT Revolution");
  m_format->setCurrentIndex(0);
  
  m_programs = new QSpinBox{this};
  m_programs->setRange(0, 16);
  m_programs->setValue(1);
  m_programs->setEnabled(false);

  connect(
      m_format,
      QOverload<int>::of(&QComboBox::currentIndexChanged),
      this,
      [this](int index)
      {
        switch (index)
        {
          case 0:
            m_deviceNameEdit->setText("SpatGRIS");
            m_port->setValue(18032);
            m_programs->setEnabled(false);
            break;
          case 1:
            m_deviceNameEdit->setText("ADM-OSC");
            m_port->setValue(4001);
            m_programs->setEnabled(false);
            break;
          case 2:
            m_deviceNameEdit->setText("SPAT");
            m_port->setValue(9000);
            m_programs->setEnabled(true);
            m_programs->setToolTip(tr("Number of rooms (SPAT Revolution)"));
            break;
        }
      });

  auto layout = new QFormLayout;
  layout->addRow(tr("Name"), m_deviceNameEdit);
  layout->addRow(tr("Format"), m_format);
  layout->addRow(tr("Host"), m_host);
  layout->addRow(tr("Port"), m_port);
  layout->addRow(tr("Source/Object count"), m_control);
  layout->addRow(tr("Program/Room count"), m_programs);

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
  settings.format = static_cast<SpatFormat>(this->m_format->currentIndex());
  settings.programs = this->m_programs->value();
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
  m_format->setCurrentIndex(static_cast<int>(specif.format));
  m_programs->setValue(specif.programs);
  m_programs->setEnabled(specif.format == SpatFormat::ADMOSC || specif.format == SpatFormat::SPAT);
}
}
