/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGTKREFPTR_H
#define QGTKREFPTR_H

#include <QtCore/qglobal.h>
#include <gtk/gtk.h>

QT_BEGIN_NAMESPACE

template<typename T>
class QGtkRefPtr
{
public:
    QGtkRefPtr()
        : m_obj(0)
    {

    }

    QGtkRefPtr(gpointer obj)
        : m_obj(obj)
    {
        if (m_obj)
            g_object_ref_sink(m_obj);
    }

    QGtkRefPtr(const QGtkRefPtr& other)
        : m_obj(other.m_obj)
    {
        if (m_obj)
            g_object_ref(m_obj);
    }

    ~QGtkRefPtr()
    {
        if (m_obj)
            g_object_unref(m_obj);
    }

    QGtkRefPtr<T>& operator=(const QGtkRefPtr &other)
    {
        reset(other.get());
        return *this;
    }

    bool operator==(const QGtkRefPtr &other)
    {
        return this->m_obj == other.m_obj;
    }

    bool operator!=(const QGtkRefPtr &other)
    {
        return !(*this == other);
    }

    bool operator==(gpointer other)
    {
        return this->m_obj == other;
    }

    bool operator!=(gpointer other)
    {
        return !(*this == other);
    }

    // ### consider moves

    T* get() const { return static_cast<T*>(m_obj); }

    operator bool() const { return m_obj != nullptr; }

    void reset(T* newObj)
    {
        if (m_obj)
            g_object_unref(m_obj);
        m_obj = newObj;
        if (m_obj)
            g_object_ref_sink(m_obj);
    }

private:
    gpointer m_obj;
};

QT_END_NAMESPACE

#endif // QGTKREFPTR_H
