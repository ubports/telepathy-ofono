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
#ifndef OFONOVOICECALL_H
#define OFONOVOICECALL_H

#include <QtCore/QObject>
#include <QVariant>
#include <QStringList>
#include <QDBusError>

#include "libofono-qt_global.h"

class OfonoInterface;

//! This class is used to access oFono voice call API
/*!
 * The API is documented in
 * http://git.kernel.org/?p=network/ofono/ofono.git;a=blob;f=doc/voicecall-api.txt
 */
class OFONO_QT_EXPORT OfonoVoiceCall : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QString path READ path)
    Q_PROPERTY(QString errorName READ errorName)
    Q_PROPERTY(QString errorMessage READ errorMessage)
    
    Q_PROPERTY(QString lineIdentification READ lineIdentification NOTIFY lineIdentificationChanged)
    Q_PROPERTY(QString incomingLine READ incomingLine NOTIFY incomingLineChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString startTime READ startTime NOTIFY startTimeChanged)
    Q_PROPERTY(QString information READ information NOTIFY informationChanged)
    Q_PROPERTY(bool multiparty READ multiparty NOTIFY multipartyChanged)
    Q_PROPERTY(bool emergency READ emergency NOTIFY emergencyChanged)
    Q_PROPERTY(quint8 icon READ icon NOTIFY iconChanged)

public:
    OfonoVoiceCall(const QString &callId, QObject *parent=0);
    OfonoVoiceCall(const OfonoVoiceCall &op);
    ~OfonoVoiceCall();

    OfonoVoiceCall operator=(const OfonoVoiceCall &op);
    bool operator==(const OfonoVoiceCall &op);

    //! Returns the D-Bus object path of the voice call object
    QString path() const;

    //! Get the D-Bus error name of the last operation.
    /*!
     * Returns the D-Bus error name of the last operation (setting a property
     * or calling a method) if it has failed
     */
    QString errorName() const;

    //! Get the D-Bus error message of the last operation.
    /*!
     * Returns the D-Bus error message of the last operation (setting a property
     * or calling a method) if it has failed
     */
    QString errorMessage() const;

    QString lineIdentification() const;
    QString incomingLine() const;
    QString name() const;
    QString state() const;
    QString startTime() const;
    QString information() const;
    bool multiparty() const;
    bool emergency() const;
    quint8 icon() const;
    bool remoteHeld() const;
    bool remoteMultiparty() const;

public Q_SLOTS:
    void answer();
    void hangup();
    void deflect(const QString &number);

Q_SIGNALS:
    void answerComplete(bool status);
    void hangupComplete(bool status);
    void deflectComplete(bool status);
    void lineIdentificationChanged(const QString &name);
    void nameChanged(const QString &name);
    void stateChanged(const QString &state);
    void startTimeChanged(const QString &time);
    void informationChanged(const QString &mcc);
    void incomingLineChanged(const QString &line);
    void disconnectReason(const QString &reason);
    void multipartyChanged(const bool multiparty);
    void iconChanged(const quint8 &icon);
    void emergencyChanged(const bool emergency);
    void remoteHeldChanged(const bool remoteHeld);
    void remoteMultipartyChanged(const bool remoteMultiparty);

private Q_SLOTS:
    void propertyChanged(const QString &property, const QVariant &value);
    void answerResp();
    void answerErr(const QDBusError &error);
    void hangupResp();
    void hangupErr(const QDBusError &error);
    void deflectResp();
    void deflectErr(const QDBusError &error);

private:
    OfonoInterface *m_if;

};

#endif // OFONOVOICECALL_H
