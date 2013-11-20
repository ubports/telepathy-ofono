#include "ofonomodemmanager.h"

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
    return QStringList() << "/OfonoModem";
}

void OfonoModemManager::onModemAdded(const QDBusObjectPath &path, const QVariantMap &map)
{
}

void OfonoModemManager::onModemRemoved(const QDBusObjectPath &path)
{
}
