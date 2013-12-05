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

#ifndef OFONOCALLVOLUME_H
#define OFONOCALLVOLUME_H

#include <QtCore/QObject>
#include "ofonomodeminterface.h"
#include "libofono-qt_global.h"

//! This class is used to access oFono call volume API
/*!
 * The API is documented in
 * http://git.kernel.org/?p=network/ofono/ofono.git;a=blob_plain;f=doc/call-volume-api.txt
 */
class OFONO_QT_EXPORT OfonoCallVolume : public OfonoModemInterface
{
    Q_OBJECT

    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(quint8 speakerVolume READ speakerVolume WRITE setSpeakerVolume NOTIFY speakerVolumeChanged)
    Q_PROPERTY(quint8 microphoneVolume READ microphoneVolume WRITE setMicrophoneVolume NOTIFY microphoneVolumeChanged)
    
public:
    OfonoCallVolume(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent=0);
    ~OfonoCallVolume();

    /* Properties */
    bool muted() const;
    quint8 speakerVolume() const;
    quint8 microphoneVolume()const ;

public Q_SLOTS:
    void setMuted(const bool mute);
    void setSpeakerVolume(const quint8 &spvolume);
    void setMicrophoneVolume(const quint8 &mpvolume);

Q_SIGNALS:
    void mutedChanged(const bool &muted);
    void speakerVolumeChanged(const quint8 &volume);
    void microphoneVolumeChanged(const quint8 &mvolume);
    void setMutedFailed();
    void setSpeakerVolumeFailed();
    void setMicrophoneVolumeFailed();

private Q_SLOTS:
    void propertyChanged(const QString& property, const QVariant& value);
    void setPropertyFailed(const QString& property);

};

#endif // OFONOCALLVOLUME_H
