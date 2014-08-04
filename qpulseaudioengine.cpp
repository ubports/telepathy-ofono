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

#include <QtCore/qdebug.h>

#include "qpulseaudioengine.h"
#include <sys/types.h>
#include <unistd.h>

#define PULSEAUDIO_PROFILE_HSP "hsp"
#define PULSEAUDIO_PROFILE_A2DP "a2dp"

QT_BEGIN_NAMESPACE

static void contextStateCallbackInit(pa_context *context, void *userdata)
{
    Q_UNUSED(context);
    QPulseAudioEngine *pulseEngine = reinterpret_cast<QPulseAudioEngine*>(userdata);
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void contextStateCallback(pa_context *context, void *userdata)
{
    Q_UNUSED(userdata);
    Q_UNUSED(context);
}

static void success_cb(pa_context *context, int success, void *userdata)
{
    QPulseAudioEngine *pulseEngine = reinterpret_cast<QPulseAudioEngine*>(userdata);
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void subscribeCallback(pa_context *context, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
    /* For card change events (slot plug/unplug and add/remove card) */
    if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_CARD) {
        if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE)
            QMetaObject::invokeMethod((QPulseAudioEngine *) userdata, "plugUnplugSlot", Qt::QueuedConnection);
        if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW)
            QMetaObject::invokeMethod((QPulseAudioEngine *) userdata, "plugUnplugCard", Qt::QueuedConnection);
        if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
            /* unplug slot is needed because remove gets called after event change */
            QMetaObject::invokeMethod((QPulseAudioEngine *) userdata, "plugUnplugCard", Qt::QueuedConnection);
            QMetaObject::invokeMethod((QPulseAudioEngine *) userdata, "plugUnplugSlot", Qt::QueuedConnection);
        }
    }
}

Q_GLOBAL_STATIC(QPulseAudioEngine, pulseEngine);

QPulseAudioEngine::QPulseAudioEngine(QObject *parent)
    : QObject(parent)
    , m_mainLoopApi(0)
    , m_context(0)
    , m_callstatus(CallEnded)
    , m_callmode(CallNormal)
    , m_micmute(false)
    , m_defaultsink("sink.primary")
    , m_defaultsource("source.primary")
    , m_voicecallcard("")
    , m_voicecallhighest("")
    , m_voicecallprofile("")
    , m_bt_hsp("")
    , m_bt_hsp_a2dp("")

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
                qDebug("Pulseaudio connection established.");
                keepGoing = false;
                break;

            case PA_CONTEXT_TERMINATED:
                qCritical("Pulseaudio context terminated.");
                keepGoing = false;
                ok = false;
                break;

            case PA_CONTEXT_FAILED:
            default:
                qCritical() << QString("Pulseaudio connection failure: %1").arg(pa_strerror(pa_context_errno(m_context)));
                keepGoing = false;
                ok = false;
        }

        if (keepGoing) {
            pa_threaded_mainloop_wait(m_mainLoop);
        }
    }

    if (ok) {
        pa_context_set_state_callback(m_context, contextStateCallback, this);
        pa_context_set_subscribe_callback(m_context, subscribeCallback, this);
        pa_context_subscribe(m_context, PA_SUBSCRIPTION_MASK_CARD, NULL, this);
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

void QPulseAudioEngine::cardInfoCallback(const pa_card_info *info)
{
    pa_card_profile_info *voice_call = NULL, *highest = NULL;
    pa_card_profile_info *hsp = NULL, *a2dp = NULL;

    /* For now we only support one card with the voicecall feature */
    for (int i = 0; i < info->n_profiles; i++) {
        if (!highest || info->profiles[i].priority > highest->priority)
            highest = &info->profiles[i];
        if (!strcmp(info->profiles[i].name, "voicecall"))
            voice_call = &info->profiles[i];
        if (!strcmp(info->profiles[i].name, PULSEAUDIO_PROFILE_HSP))
            hsp = &info->profiles[i];
        if (!strcmp(info->profiles[i].name, PULSEAUDIO_PROFILE_A2DP))
            a2dp = &info->profiles[i];
    }

    /* Record the card that supports voicecall (default one to be used) */
    if (voice_call) {
        qDebug("Found card that supports voicecall: '%s'", info->name);
        m_voicecallcard = info->name;
        m_voicecallhighest = highest->name;
        m_voicecallprofile = voice_call->name;
    }

    /* Handle the use cases needed for bluetooth */
    if (hsp && a2dp) {
        qDebug("Found card that supports hsp and a2dp: '%s'", info->name);
        m_bt_hsp_a2dp = info->name;
    } else if (hsp && a2dp == NULL) {
        /* This card only provides the hsp profile */
        qDebug("Found card that supports only hsp: '%s'", info->name);
        m_bt_hsp = info->name;
    }
}

void QPulseAudioEngine::sinkInfoCallback(const pa_sink_info *info)
{
    pa_sink_port_info *earpiece = NULL, *speaker = NULL;
    pa_sink_port_info *highest = NULL, *preferred = NULL;
    pa_sink_port_info *bluetooth_sco = NULL;

    for (int i = 0; i < info->n_ports; i++) {
        if (!highest || info->ports[i]->priority > highest->priority)
            if (info->ports[i]->available != PA_PORT_AVAILABLE_NO)
                highest = info->ports[i];
        if (!strcmp(info->ports[i]->name, "output-earpiece"))
            earpiece = info->ports[i];
        if (!strcmp(info->ports[i]->name, "output-speaker"))
            speaker = info->ports[i];
        if (!strcmp(info->ports[i]->name, "output-bluetooth_sco"))
            bluetooth_sco = info->ports[i];
    }

    if (!earpiece)
        return; /* Not the right sink */

    if (m_callmode == CallSpeaker)
        preferred = speaker;
    else if (m_callstatus == CallActive) {
        if (bluetooth_sco && ((m_bt_hsp != "") || (m_bt_hsp_a2dp != "")))
            preferred = bluetooth_sco;
        else if (highest == speaker)
            preferred = earpiece;
        else
            preferred = highest;
    }

    if (!preferred)
        preferred = highest;

    m_nametoset = info->name;
    if (info->active_port != preferred)
        m_valuetoset = preferred->name;
}

void QPulseAudioEngine::sourceInfoCallback(const pa_source_info *info)
{
    pa_source_port_info *builtin_mic = NULL, *highest = NULL;
    pa_source_port_info *preferred = NULL;

    if (info->monitor_of_sink != PA_INVALID_INDEX)
        return;  /* Not the right source */

    for (int i = 0; i < info->n_ports; i++) {
        if (!highest || info->ports[i]->priority > highest->priority)
            if (info->ports[i]->available != PA_PORT_AVAILABLE_NO)
                highest = info->ports[i];
        if (!strcmp(info->ports[i]->name, "input-builtin_mic"))
            builtin_mic = info->ports[i];
    }

    if (!builtin_mic)
        return; /* Not the right source */

    if (m_callmode == CallSpeaker)
        preferred = builtin_mic;

    if (!preferred)
        preferred = highest;

    m_nametoset = info->name;
    if (info->active_port != preferred)
        m_valuetoset = preferred->name;
}

void QPulseAudioEngine::serverInfoCallback(const pa_server_info *info)
{
    /* Saving default sink/source to restore after call hangup */
    m_defaultsink = info->default_sink_name;
    m_defaultsource = info->default_source_name;

    /* In the case of a server call back we need to signal the mainloop */
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void cardinfo_cb(pa_context *context, const pa_card_info *info, int isLast, void *userdata)
{
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);
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

static void sourceinfo_cb(pa_context *context, const pa_source_info *info, int isLast, void *userdata)
{
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);
    if (isLast != 0 || !pulseEngine || !info) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }
    pulseEngine->sourceInfoCallback(info);
}

static void serverinfo_cb(pa_context *context, const pa_server_info *info, void *userdata)
{
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);
    if (!pulseEngine || !info) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }
    pulseEngine->serverInfoCallback(info);
}

bool QPulseAudioEngine::handleOperation(pa_operation *operation, const char *func_name)
{
    if (!operation) {
        qCritical("'%s' failed (lost PulseAudio connection?)", func_name);
        pa_threaded_mainloop_unlock(m_mainLoop);
        return false;
    }

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);
    return true;
}

void QPulseAudioEngine::setupVoiceCall()
{
    pa_operation *o;

    qDebug("Setting up pulseaudio for voice call");

    pa_threaded_mainloop_lock(m_mainLoop);

    /* Get and set the default sink/source to be restored later */
    o = pa_context_get_server_info(m_context, serverinfo_cb, this);
    if (!handleOperation(o, "pa_context_get_server_info"))
        return;

    qDebug("Recorded default sink: %s default source: %s", m_defaultsink.c_str(), m_defaultsource.c_str());

    /* Walk through the list of devices, find the voice call capable card and
       identify if we have bluetooth capable devices (hsp and a2dp) */
    m_voicecallcard = m_voicecallhighest = m_voicecallprofile = "";
    m_bt_hsp = m_bt_hsp_a2dp = "";
    o = pa_context_get_card_info_list(m_context, cardinfo_cb, this);
    if (!handleOperation(o, "pa_context_get_card_info_list"))
        return;
    /* In case we have only one bt device that provides hsp and a2dp, we need
     * to make sure we switch the default profile for that card (to hsp) */
    if ((m_bt_hsp_a2dp != "") && (m_bt_hsp == "")) {
        qDebug("Setting PulseAudio card '%s' profile '%s'", m_bt_hsp_a2dp.c_str(), PULSEAUDIO_PROFILE_HSP);
        o = pa_context_set_card_profile_by_name(m_context,
            m_bt_hsp_a2dp.c_str(), PULSEAUDIO_PROFILE_HSP, success_cb, this);
        if (!handleOperation(o, "pa_context_set_card_profile_by_name"))
            return;
    }

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void QPulseAudioEngine::restoreVoiceCall()
{
    pa_operation *o;

    qDebug("Restoring pulseaudio previous state");

    /* Then restore previous settings */
    pa_threaded_mainloop_lock(m_mainLoop);

    /* See if we need to restore any HSP+AD2P device state */
    if ((m_bt_hsp_a2dp != "") && (m_bt_hsp == "")) {
        qDebug("Restoring PulseAudio card '%s' to profile '%s'", m_bt_hsp_a2dp.c_str(), PULSEAUDIO_PROFILE_A2DP);
        o = pa_context_set_card_profile_by_name(m_context,
            m_bt_hsp_a2dp.c_str(), PULSEAUDIO_PROFILE_A2DP, success_cb, this);
        if (!handleOperation(o, "pa_context_set_card_profile_by_name"))
            return;
    }

    /* Restore default sink/source */
    if (m_defaultsink != "") {
        qDebug("Restoring PulseAudio default sink to '%s'", m_defaultsink.c_str());
        o = pa_context_set_default_sink(m_context, m_defaultsink.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_default_sink"))
            return;
    }
    if (m_defaultsource != "") {
        qDebug("Restoring PulseAudio default source to '%s'", m_defaultsource.c_str());
        o = pa_context_set_default_source(m_context, m_defaultsource.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_default_source"))
            return;
    }

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void QPulseAudioEngine::setCallMode(QPulseAudioEngine::CallStatus callstatus, QPulseAudioEngine::CallMode callmode)
{
    pa_operation *o;

    /* Check if we need to save the current pulseaudio state (e.g. when starting a call) */
    if ((callstatus != CallEnded) && (m_callstatus == CallEnded)) {
        setupVoiceCall();
    }

    /* If we have an active call, update internal state (used later when updating sink/source ports */
    m_callstatus = callstatus;
    m_callmode = callmode;

    /* Switch the virtual card mode when call is active and not active
     * This needs to be done before sink/source gets updated, because after changing mode
     * it will automatically move to input/output-parking */
    if ((m_callstatus == CallActive) && (m_voicecallcard != "") && (m_voicecallprofile != "")) {
        qDebug("Setting PulseAudio card '%s' profile '%s'", m_voicecallcard.c_str(), m_voicecallprofile.c_str());
        o = pa_context_set_card_profile_by_name(m_context,
                m_voicecallcard.c_str(), m_voicecallprofile.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_card_profile_by_name"))
            return;
    } else if ((m_callstatus == CallEnded) && (m_voicecallcard != "") && (m_voicecallhighest != "")) {
        /* If using droid, make sure to restore to the profile that has the highest score */
        qDebug("Restoring PulseAudio card '%s' to profile '%s'", m_voicecallcard.c_str(), m_voicecallhighest.c_str());
        o = pa_context_set_card_profile_by_name(m_context,
            m_voicecallcard.c_str(), m_voicecallhighest.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_card_profile_by_name"))
            return;
    }

    /* Update sink/source ports */
    pa_threaded_mainloop_lock(m_mainLoop);

    /* Find highest compatible sink/source elements from the voicecall
       compatible card (on touch this means the pulse droid element) */
    m_nametoset = m_valuetoset = "";
    o = pa_context_get_sink_info_list(m_context, sinkinfo_cb, this);
    if (!handleOperation(o, "pa_context_get_sink_info_list"))
        return;
    if ((m_nametoset != "") && (m_nametoset != m_defaultsink)) {
        qDebug("Setting PulseAudio default sink to '%s'", m_nametoset.c_str());
        o = pa_context_set_default_sink(m_context, m_nametoset.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_default_sink"))
            return;
    }
    if (m_valuetoset != "") {
        qDebug("Setting PulseAudio sink '%s' port '%s'", m_nametoset.c_str(), m_valuetoset.c_str());
        o = pa_context_set_sink_port_by_name(m_context, m_nametoset.c_str(), m_valuetoset.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_sink_port_by_name"))
            return;
    }

    /* Same for source */
    m_nametoset = m_valuetoset = "";
    o = pa_context_get_source_info_list(m_context, sourceinfo_cb, this);
    if (!handleOperation(o, "pa_context_get_source_info_list"))
        return;
    if ((m_nametoset != "") && (m_nametoset != m_defaultsource)) {
        qDebug("Setting PulseAudio default source to '%s'", m_nametoset.c_str());
        o = pa_context_set_default_source(m_context, m_nametoset.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_default_source"))
            return;
    }
    if (m_valuetoset != "") {
        qDebug("Setting PulseAudio source '%s' port '%s'", m_nametoset.c_str(), m_valuetoset.c_str());
        o = pa_context_set_source_port_by_name(m_context, m_nametoset.c_str(), m_valuetoset.c_str(), success_cb, this);
        if (!handleOperation(o, "pa_context_set_source_port_by_name"))
            return;
    }

    pa_threaded_mainloop_unlock(m_mainLoop);

    /* If no more active voicecall, restore previous saved pulseaudio state */
    if (callstatus == CallEnded) {
        restoreVoiceCall();
    }

    /* In case the app had set mute when the call wasn't active, make sure we reflect it here */
    if (m_callstatus != CallEnded)
        setMicMute(m_micmute);
}

void QPulseAudioEngine::setMicMute(bool muted)
{
    m_micmute = muted;

    if (m_callstatus == CallEnded)
        return;

    pa_threaded_mainloop_lock(m_mainLoop);

    m_nametoset = "";
    pa_operation *o = pa_context_get_source_info_list(m_context, sourceinfo_cb, this);
    if (!handleOperation(o, "pa_context_get_source_info_list"))
        return;

    if (m_nametoset != "") {
        int m = m_micmute ? 1 : 0;
        qDebug("Setting PulseAudio source '%s' muted '%d'", m_nametoset.c_str(), m);
        o = pa_context_set_source_mute_by_name(m_context, 
            m_nametoset.c_str(), m, success_cb, this);
        if (!handleOperation(o, "pa_context_set_source_mute_by_name"))
            return;
    }

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void QPulseAudioEngine::plugUnplugCard()
{
    qDebug("Notified about card add/removed event from PulseAudio");

    if (m_callstatus != CallEnded)
        setupVoiceCall();
}

void QPulseAudioEngine::plugUnplugSlot()
{
    qDebug("Notified about card changes (port) event from PulseAudio");

    if (m_callstatus == CallActive)
        setCallMode(m_callstatus, m_callmode);
}


QT_END_NAMESPACE

