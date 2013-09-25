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
#ifdef USE_PULSEAUDIO

#include <QtCore/qmap.h>
#include <QtCore/qbytearray.h>
#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioEngine : public QObject
{
    Q_OBJECT

public:
    QPulseAudioEngine(QObject *parent = 0);
    ~QPulseAudioEngine();

    static QPulseAudioEngine *instance();
    pa_threaded_mainloop *mainloop() { return m_mainLoop; }
    pa_context *context() { return m_context; }

    void setCallMode(bool inCall, bool speakerMode);

    /* These two are only used internally */
    void cardInfoCallback(const pa_card_info *card);
    void sinkInfoCallback(const pa_sink_info *sink);
private:
    pa_mainloop_api *m_mainLoopApi;
    pa_threaded_mainloop *m_mainLoop;
    pa_context *m_context;

    bool m_incall, m_speakermode;
    std::string m_cardtoset, m_profiletoset;
    std::string m_sinktoset, m_porttoset;
 };

QT_END_NAMESPACE

#endif
#endif