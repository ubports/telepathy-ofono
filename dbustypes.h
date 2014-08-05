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
 * Authors: Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 */

#ifndef DBUSTYPES
#define DBUSTYPES

struct AudioOutput {
    QString id;
    QString type;
    QString name;
};

struct MessageStruct {
    QDBusObjectPath path;
    QVariantMap properties;
};

struct IncomingAttachmentStruct {
    QString id;
    QString contentType;
    QString filePath;
    quint64 offset;
    quint64 length;
};

struct OutgoingAttachmentStruct {
    QString id;
    QString contentType;
    QString filePath;
};

Q_DECLARE_METATYPE(AudioOutput)

typedef QList<AudioOutput> AudioOutputList;
Q_DECLARE_METATYPE(AudioOutputList)

typedef QList<IncomingAttachmentStruct> IncomingAttachmentList;
typedef QList<OutgoingAttachmentStruct> OutgoingAttachmentList;
Q_DECLARE_METATYPE(IncomingAttachmentStruct)
Q_DECLARE_METATYPE(OutgoingAttachmentStruct)
Q_DECLARE_METATYPE(IncomingAttachmentList)
Q_DECLARE_METATYPE(OutgoingAttachmentList)

typedef QList<MessageStruct> MessageList;
Q_DECLARE_METATYPE(MessageStruct)
Q_DECLARE_METATYPE(MessageList)


#endif
