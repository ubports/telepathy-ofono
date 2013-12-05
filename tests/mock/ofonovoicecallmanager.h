/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
 *
 * Portions of this file are Copyright (C) 2011 Intel Corporation
 * Contact: Shane Bryan <shane.bryan@linux.intel.com>
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

#ifndef OFONOVOICECALLMANAGER_H
#define OFONOVOICECALLMANAGER_H

#include <QtCore/QObject>
#include <QStringList>
#include <QDBusError>
#include <QDBusObjectPath>

#include "ofonomodeminterface.h"
#include "libofono-qt_global.h"

struct OfonoVoiceCallManagerStruct {
    QDBusObjectPath path;
    QVariantMap properties;
};
typedef QList<OfonoVoiceCallManagerStruct> OfonoVoiceCallManagerList;

Q_DECLARE_METATYPE(OfonoVoiceCallManagerStruct)
Q_DECLARE_METATYPE(OfonoVoiceCallManagerList)

//! This class is used to access oFono voice call manager API
/*!
 * The API is documented in
 * http://git.kernel.org/?p=network/ofono/ofono.git;a=blob_plain;f=doc/voicecallmanager-api.txt
 */
class OFONO_QT_EXPORT OfonoVoiceCallManager : public OfonoModemInterface
{
    Q_OBJECT

    Q_PROPERTY(QStringList emergencyNumbers READ emergencyNumbers NOTIFY emergencyNumbersChanged)

public:
    OfonoVoiceCallManager(OfonoModem::SelectionSetting modemSetting, const QString &modemPath, QObject *parent=0);
    ~OfonoVoiceCallManager();

    /* Properties */
    QStringList emergencyNumbers() const;

    Q_INVOKABLE QStringList getCalls() const;

public Q_SLOTS:
    QDBusObjectPath dial(const QString &number, const QString &callerid_hide, bool &success);
    void hangupAll();
    void sendTones(const QString &tonestring);
    void transfer();
    void swapCalls();
    void releaseAndAnswer();
    void holdAndAnswer();
    void privateChat(const QString &path);
    void createMultiparty();
    void hangupMultiparty();

Q_SIGNALS:
    void emergencyNumbersChanged(const QStringList &numbers);
    void callAdded(const QString &call, const QVariantMap &values);
    void callRemoved(const QString &call);
    void hangupAllComplete(const bool status);
    void sendTonesComplete(const bool status);
    void transferComplete(const bool status);
    void swapCallsComplete(const bool status);
    void releaseAndAnswerComplete(const bool status);
    void holdAndAnswerComplete(const bool status);
    void privateChatComplete(const bool status, const QStringList& calls);
    void createMultipartyComplete(const bool status, const QStringList& calls);
    void hangupMultipartyComplete(const bool status);

    void barringActive(const QString &type);
    void forwarded(const QString &type);

private Q_SLOTS:
    void validityChanged(bool);
    void pathChanged(const QString& path);
    void propertyChanged(const QString &property, const QVariant &value);
    void callAddedChanged(const QDBusObjectPath &call, const QVariantMap &values);
    void callRemovedChanged(const QDBusObjectPath &call);
    void hangupAllResp();
    void hangupAllErr(const QDBusError &error);
    void sendTonesResp();
    void sendTonesErr(const QDBusError &error);
    void transferResp();
    void transferErr(const QDBusError &error);
    void swapCallsResp();
    void swapCallsErr(const QDBusError &error);
    void releaseAndAnswerResp();
    void releaseAndAnswerErr(const QDBusError &error);
    void holdAndAnswerResp();
    void holdAndAnswerErr(const QDBusError &error);
    void privateChatResp(const QList<QDBusObjectPath> &paths);
    void privateChatErr(const QDBusError &error);
    void createMultipartyResp(const QList<QDBusObjectPath> &paths);
    void createMultipartyErr(const QDBusError &error);
    void hangupMultipartyResp();
    void hangupMultipartyErr(const QDBusError &error);

private:
    QStringList getCallList();
    void connectDbusSignals(const QString& path);
private:
    QStringList m_calllist;
};

#endif  /* !OFONOVOICECALLMANAGER_H */
