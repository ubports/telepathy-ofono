/*
 * This file is part of ofono-qt
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OFONO_QT_GLOBAL_H
#define OFONO_QT_GLOBAL_H

#include <QtCore/QtGlobal>

#ifdef BUILD_OFONO_QT_LIBRARY
#  define OFONO_QT_EXPORT Q_DECL_EXPORT
#else
#  define OFONO_QT_EXPORT Q_DECL_IMPORT
#endif

#endif // OFONO_QT_GLOBAL_H
