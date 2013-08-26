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
#ifdef USE_PULSEAUDIO

#include <QtCore/qdebug.h>

#include "qpulseaudioengine.h"
#include <sys/types.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

static void contextStateCallbackInit(pa_context *context, void *userdata)
{
    Q_UNUSED(context);
#ifdef DEBUG_PULSE
    qDebug() << QPulseAudioInternal::stateToQString(pa_context_get_state(context));
#endif
    QPulseAudioEngine *pulseEngine = reinterpret_cast<QPulseAudioEngine*>(userdata);
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void contextStateCallback(pa_context *context, void *userdata)
{
    Q_UNUSED(userdata);
    Q_UNUSED(context);

#ifdef DEBUG_PULSE
    pa_context_state_t state = pa_context_get_state(context);
    qDebug() << QPulseAudioInternal::stateToQString(state);
#endif
}

Q_GLOBAL_STATIC(QPulseAudioEngine, pulseEngine);

QPulseAudioEngine::QPulseAudioEngine(QObject *parent)
    : QObject(parent)
    , m_mainLoopApi(0)
    , m_context(0)

{
    bool keepGoing = true;
    bool ok = true;

    m_mainLoop = pa_threaded_mainloop_new();
    if (m_mainLoop == 0) {
        qWarning("Unable to create pulseaudio mainloop");
        return;
    }

    if (pa_threaded_mainloop_start(m_mainLoop) != 0) {
        qWarning("Unable to start pulseaudio mainloop");
        pa_threaded_mainloop_free(m_mainLoop);
        return;
    }

    m_mainLoopApi = pa_threaded_mainloop_get_api(m_mainLoop);

    pa_threaded_mainloop_lock(m_mainLoop);

    m_context = pa_context_new(m_mainLoopApi, QString(QLatin1String("QtmPulseContext:%1")).arg(::getpid()).toLatin1().constData());
    pa_context_set_state_callback(m_context, contextStateCallbackInit, this);

    if (!m_context) {
        qWarning("Unable to create new pulseaudio context");
        pa_threaded_mainloop_free(m_mainLoop);
        return;
    }

    if (pa_context_connect(m_context, NULL, (pa_context_flags_t)0, NULL) < 0) {
        qWarning("Unable to create a connection to the pulseaudio context");
        pa_context_unref(m_context);
        pa_threaded_mainloop_free(m_mainLoop);
        return;
    }

    pa_threaded_mainloop_wait(m_mainLoop);

    while (keepGoing) {
        switch (pa_context_get_state(m_context)) {
            case PA_CONTEXT_CONNECTING:
            case PA_CONTEXT_AUTHORIZING:
            case PA_CONTEXT_SETTING_NAME:
                break;

            case PA_CONTEXT_READY:
#ifdef DEBUG_PULSE
                qDebug("Connection established.");
#endif
                keepGoing = false;
                break;

            case PA_CONTEXT_TERMINATED:
                qCritical("Context terminated.");
                keepGoing = false;
                ok = false;
                break;

            case PA_CONTEXT_FAILED:
            default:
                qCritical() << QString("Connection failure: %1").arg(pa_strerror(pa_context_errno(m_context)));
                keepGoing = false;
                ok = false;
        }

        if (keepGoing) {
            pa_threaded_mainloop_wait(m_mainLoop);
        }
    }

    if (ok) {
        pa_context_set_state_callback(m_context, contextStateCallback, this);
    } else {
        if (m_context) {
            pa_context_unref(m_context);
            m_context = 0;
        }
    }

    pa_threaded_mainloop_unlock(m_mainLoop);
}

QPulseAudioEngine::~QPulseAudioEngine()
{
    if (m_context) {
        pa_threaded_mainloop_lock(m_mainLoop);
        pa_context_disconnect(m_context);
        pa_threaded_mainloop_unlock(m_mainLoop);
        m_context = 0;
    }

    if (m_mainLoop) {
        pa_threaded_mainloop_stop(m_mainLoop);
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = 0;
    }
}

QPulseAudioEngine *QPulseAudioEngine::instance()
{
    return pulseEngine();
}

void QPulseAudioEngine::sinkInfoCallback(const pa_sink_info *info)
{
    pa_sink_port_info *earpiece = NULL, *speaker = NULL, *highest = NULL;
    pa_sink_port_info *preferred = NULL;

    for (int i = 0; i < info->n_ports; i++) {
        if (!highest || info->ports[i]->priority > highest->priority) {
            if (info->ports[i]->available != PA_PORT_AVAILABLE_NO)
                highest = info->ports[i];
        }
        if (!strcmp(info->ports[i]->name, "[Out] Earpiece"))
            earpiece = info->ports[i];
        if (!strcmp(info->ports[i]->name, "[Out] Speaker"))
            speaker = info->ports[i];
    }

    if (!earpiece)
        return; /* Not the right sink */

    /* TODO: When on ringtone and headphones are plugged in, people want output
       through *both* headphones and speaker, but when on call with speaker mode,
       people want *just* speaker, not including headphones. */
    if (m_speakermode)
        preferred = speaker;
    else if (m_incall)
        preferred = earpiece;
    if (!preferred)
        preferred = highest;

    if (preferred && preferred != info->active_port) {
        m_sinktoset = info->name;
        m_porttoset = preferred->name; 
    }
}

void QPulseAudioEngine::cardInfoCallback(const pa_card_info *info)
{
    pa_card_profile_info *voice_call = NULL, *highest = NULL;

    for (int i = 0; i < info->n_profiles; i++) {
        if (!highest || info->profiles[i].priority > highest->priority)
            highest = &info->profiles[i];
        if (!strcmp(info->profiles[i].name, "Voice Call"))
            voice_call = &info->profiles[i];
    }

    if (!voice_call)
        return; /* Not the right card */

    if (m_incall && (voice_call != info->active_profile)) {
        m_cardtoset = info->name;
        m_profiletoset = voice_call->name;
    }
    else if (!m_incall && (voice_call == info->active_profile)) {
        m_cardtoset = info->name;
        m_profiletoset = highest->name;
    }
}

static void cardinfo_cb(pa_context *context, const pa_card_info *info, int isLast, void *userdata)
{
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);
/*    qDebug("cardinfo_cb: pulseengine = %p, info = %p, isLast = %d", pulseEngine, info, isLast); */
    if (isLast != 0 || !pulseEngine || !info) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }
    pulseEngine->cardInfoCallback(info);
}

static void sinkinfo_cb(pa_context *context, const pa_sink_info *info, int isLast, void *userdata)
{
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);
    if (isLast != 0 || !pulseEngine || !info) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }
    pulseEngine->sinkInfoCallback(info);
}

void QPulseAudioEngine::setCallMode(bool inCall, bool speakerMode)
{
    pa_operation *operation;

    m_incall = inCall;
    m_speakermode = speakerMode;
    m_cardtoset = "";
    m_sinktoset = "";

    pa_threaded_mainloop_lock(m_mainLoop);

    operation = pa_context_get_card_info_list(m_context, cardinfo_cb, this);
    if (!operation) {
        qDebug("pa_context_get_card_info_list failed (lost PulseAudio connection?)");
        /* TODO: It would be nice if we could restart the connection here. Or use RAII to unlock the mainloop. */
        pa_threaded_mainloop_unlock(m_mainLoop);
        return;
    }
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);

    if (m_cardtoset != "") {
        qDebug("Setting PulseAudio card '%s' profile '%s'", m_cardtoset.c_str(), m_profiletoset.c_str());
        operation = pa_context_set_card_profile_by_name(m_context, 
            m_cardtoset.c_str(), m_profiletoset.c_str(), NULL, NULL);
        /* This one will be finished before PA processes the next command. */
        if (!operation) {
            qDebug("pa_context_set_card_profile_by_name failed (lost PulseAudio connection?)");
            pa_threaded_mainloop_unlock(m_mainLoop);
            return;
        }
        pa_operation_unref(operation);
    }

    operation = pa_context_get_sink_info_list(m_context, sinkinfo_cb, this);
    if (!operation) {
        qDebug("pa_context_get_sink_info_list failed (lost PulseAudio connection?)");
        pa_threaded_mainloop_unlock(m_mainLoop);
        return;
    }
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);

    if (m_sinktoset != "") {
        qDebug("Setting PulseAudio sink '%s' port '%s'", m_sinktoset.c_str(), m_porttoset.c_str());
        operation = pa_context_set_sink_port_by_name(m_context, 
            m_sinktoset.c_str(), m_porttoset.c_str(), NULL, NULL);
        if (!operation) {
            qDebug("pa_context_set_sink_port_by_name failed (lost PulseAudio connection?)");
            pa_threaded_mainloop_unlock(m_mainLoop);
            return;
        }
        /* We can have this operation running in parallel. */
        pa_operation_unref(operation);
    }

    pa_threaded_mainloop_unlock(m_mainLoop);    
}

QT_END_NAMESPACE

#endif
