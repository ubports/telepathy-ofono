#include <QtDBus/QtDBus>
#include <QtCore/QObject>

#include "ofonointerface.h"

OfonoInterface::OfonoInterface(const QString& path, const QString& ifname, OfonoGetPropertySetting setting, QObject *parent)
    : QObject(parent) , m_path(path), m_ifname(ifname), m_getpropsetting(setting)
{
}

OfonoInterface::~OfonoInterface()
{
}

void OfonoInterface::setPath(const QString& path)
{
    m_path = path;
}

QVariantMap OfonoInterface::properties() const
{
    return m_properties;
}

void OfonoInterface::resetProperties()
{
    m_properties = QVariantMap();
}

QVariantMap OfonoInterface::getAllPropertiesSync()
{
    QVariantMap map;
    return map;
}

void OfonoInterface::requestProperty(const QString& name)
{
}

void OfonoInterface::getPropertiesAsyncResp(QVariantMap properties)
{
}

void OfonoInterface::getPropertiesAsyncErr(const QDBusError& error)
{
}

void OfonoInterface::onPropertyChanged(QString property, QDBusVariant value)
{
    m_properties[property] = value.variant();
    Q_EMIT propertyChanged(property, value.variant());
}

void OfonoInterface::setProperty(const QString& name, const QVariant& property, const QString& password)
{
    QTimer::singleShot(0, this, SLOT(onPropertyChanged(name, property)));
}

void OfonoInterface::setPropertyResp()
{
}

void OfonoInterface::setPropertyErr(const QDBusError& error)
{
}

void OfonoInterface::setError(const QString& errorName, const QString& errorMessage)
{
    m_errorName = errorName;
    m_errorMessage = errorMessage;
}
