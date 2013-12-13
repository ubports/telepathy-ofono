/**
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: 
 *  Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *  Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 */

#ifndef OFONOMOCKCONTROLLER_H
#define OFONOMOCKCONTROLLER_H

#include <QObject>
#include <QDBusInterface>

class OfonoMockController : public QObject
{
    Q_OBJECT
public:
    static OfonoMockController *instance();

Q_SIGNALS:
    void MessageAdded(QDBusObjectPath, QVariantMap);
    void CallAdded(QDBusObjectPath, QVariantMap);
    void TonesReceived(QString);
    void CallVolumeMuteChanged(bool muted);

public Q_SLOTS:
    void NetworkRegistrationSetStatus(const QString &status);
    void MessageManagerSendMessage(const QString &from, const QString &text);
    void MessageManagerStatusReport(const QString &message, bool success);
    void MessageMarkFailed(const QString &objPath);
    void MessageMarkSent(const QString &objPath);
    void MessageCancel(const QString &objPath);
    void VoiceCallManagerIncomingCall(const QString &from);
    void VoiceCallHangup(const QString &objPath);
    void VoiceCallAnswer(const QString &objPath);
    void VoiceCallSetAlerting(const QString &objPath);

private Q_SLOTS:
    void onCallVolumePropertyChanged(const QString &name, const QDBusVariant &value);

private:
    explicit OfonoMockController(QObject *parent = 0);
    QDBusInterface mNetworkRegistrationInterface;
    QDBusInterface mMessageManagerInterface;
    QDBusInterface mVoiceCallManagerInterface;
};

#endif // OFONOMOCKCONTROLLER_H
