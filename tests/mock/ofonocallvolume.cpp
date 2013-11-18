/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <QtDBus/QtDBus>
#include <QtCore/QObject>

#include "ofonocallvolume.h"
#include "ofonointerface.h"

OfonoCallVolume::OfonoCallVolume(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent)
    : OfonoModemInterface(modemSetting, modemPath, "org.ofono.CallVolume", OfonoGetAllOnStartup, parent)
{
    connect(m_if, SIGNAL(propertyChanged(const QString&, const QVariant&)),
            this, SLOT(propertyChanged(const QString&, const QVariant&)));

    connect(m_if, SIGNAL(setPropertyFailed(const QString&)),
            this, SLOT(setPropertyFailed(const QString&)));

}

OfonoCallVolume::~OfonoCallVolume()
{
}

void OfonoCallVolume::propertyChanged(const QString &property, const QVariant &value)
{
    if (property == "SpeakerVolume") {
        Q_EMIT speakerVolumeChanged(value.value<quint8>());
    } else if (property == "MicrophoneVolume") {
        Q_EMIT microphoneVolumeChanged(value.value<quint8>());
    } else if (property == "Muted") {
        Q_EMIT mutedChanged(value.value<bool>());
    }
}

quint8 OfonoCallVolume::speakerVolume() const
{
    return m_if->properties()["SpeakerVolume"].value<quint8>();
}

quint8 OfonoCallVolume::microphoneVolume() const
{
    return m_if->properties()["MicrophoneVolume"].value<quint8>();
}

bool OfonoCallVolume::muted() const
{
    return m_if->properties()["Muted"].value<bool>();
}

void OfonoCallVolume::setMuted(const bool value)
{
    m_if->setProperty("Muted",QVariant(value));
}

void OfonoCallVolume::setPropertyFailed(const QString &property)
{
    if (property == "SpeakerVolume") {
        Q_EMIT setSpeakerVolumeFailed();
    } else if (property == "MicrophoneVolume") {
        Q_EMIT setMicrophoneVolumeFailed();
    } else if (property == "Muted") {
        Q_EMIT setMutedFailed();
    }
}

void OfonoCallVolume::setSpeakerVolume(const quint8 &spvolume)
{
    m_if->setProperty("SpeakerVolume",qVariantFromValue(spvolume));
}

void OfonoCallVolume::setMicrophoneVolume(const quint8 &mpvolume)
{
    m_if->setProperty("MicrophoneVolume",qVariantFromValue(mpvolume));
}
