#include "ofonomodemmanager.h"
#include "modemprivate.h"

// mock implementation of ofono-qt
OfonoModemManager::OfonoModemManager(QObject *parent)
: QObject(parent)
{
}

OfonoModemManager::~OfonoModemManager()
{
}

QStringList OfonoModemManager::modems() const
{
    return QStringList() << OFONO_MOCK_MODEM_OBJECT;
}

void OfonoModemManager::onModemAdded(const QDBusObjectPath &path, const QVariantMap &map)
{
}

void OfonoModemManager::onModemRemoved(const QDBusObjectPath &path)
{
}
