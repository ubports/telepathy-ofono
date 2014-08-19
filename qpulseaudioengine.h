/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file was taken from qt5 and modified by 
** David Henningsson <david.henningsson@canonical.com> for usage in 
** telepathy-ofono.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/

#ifndef QPULSEAUDIOENGINE_H
#define QPULSEAUDIOENGINE_H

#include <QtCore/qmap.h>
#include <QtCore/qbytearray.h>
#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioEngine : public QObject
{
    Q_OBJECT

public:
    enum CallStatus {
        CallRinging,
        CallActive,
        CallEnded
    };

    enum CallMode {
        CallNormal,
        CallSpeaker,
        CallBluetooth
    };

    QPulseAudioEngine(QObject *parent = 0);
    ~QPulseAudioEngine();

    static QPulseAudioEngine *instance();
    pa_threaded_mainloop *mainloop() { return m_mainLoop; }
    pa_context *context() { return m_context; }

    void setupVoiceCall(void);
    void restoreVoiceCall(void);
    void setCallMode(CallStatus callstatus, CallMode callmode);
    void setMicMute(bool muted); /* True if muted, false if unmuted */
    void setSinkVolume(const char *sink_name, const double volume);

    /* These four are only used internally */
    void cardInfoCallback(const pa_card_info *card);
    void sinkInfoCallback(const pa_sink_info *sink);
    void sourceInfoCallback(const pa_source_info *source);
    void serverInfoCallback(const pa_server_info *server);
public Q_SLOTS:
    void plugUnplugCard();
    void plugUnplugSlot();
private:
    pa_mainloop_api *m_mainLoopApi;
    pa_threaded_mainloop *m_mainLoop;
    pa_context *m_context;

    CallStatus m_callstatus;
    CallMode m_callmode;
    bool m_micmute;
    std::string m_nametoset, m_valuetoset;
    std::string m_defaultsink, m_defaultsource;
    std::string m_bt_hsp, m_bt_hsp_a2dp;
    std::string m_voicecallcard, m_voicecallhighest, m_voicecallprofile;

    bool handleOperation(pa_operation *operation, const char *func_name);
 };

QT_END_NAMESPACE

#endif
