/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Contact: https://www.crimson.no
**
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
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
****************************************************************************/

#ifndef QEVENTDISPATCHER_GLIB_P_H
#define QEVENTDISPATCHER_GLIB_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qabstracteventdispatcher.h>
#include <private/qabstracteventdispatcher_p.h>

typedef struct _GMainContext GMainContext;

QT_BEGIN_NAMESPACE

class QGtkEventDispatcherPrivate;

class Q_CORE_EXPORT QGtkEventDispatcher : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGtkEventDispatcher)

public:
    explicit QGtkEventDispatcher(QObject *parent = 0);
    explicit QGtkEventDispatcher(GMainContext *context, QObject *parent = 0);
    ~QGtkEventDispatcher();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) Q_DECL_OVERRIDE;
    bool hasPendingEvents() Q_DECL_OVERRIDE;

    void registerSocketNotifier(QSocketNotifier *socketNotifier) Q_DECL_FINAL;
    void unregisterSocketNotifier(QSocketNotifier *socketNotifier) Q_DECL_FINAL;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_FINAL;
    bool unregisterTimer(int timerId) Q_DECL_FINAL;
    bool unregisterTimers(QObject *object) Q_DECL_FINAL;
    QList<TimerInfo> registeredTimers(QObject *object) const Q_DECL_FINAL;

    int remainingTime(int timerId) Q_DECL_FINAL;

    void wakeUp() Q_DECL_FINAL;
    void interrupt() Q_DECL_FINAL;
    void flush() Q_DECL_FINAL;

    static bool versionSupported();

protected:
    QGtkEventDispatcher(QGtkEventDispatcherPrivate &dd, QObject *parent);
};

struct GPostEventSource;
struct GSocketNotifierSource;
struct GTimerSource;
struct GIdleTimerSource;
struct GUserEventSource;

class Q_CORE_EXPORT QGtkEventDispatcherPrivate : public QAbstractEventDispatcherPrivate
{

public:
    QGtkEventDispatcherPrivate(GMainContext *context = 0);
    GMainContext *mainContext;
    GPostEventSource *postEventSource;
    GSocketNotifierSource *socketNotifierSource;
    GTimerSource *timerSource;
    GIdleTimerSource *idleTimerSource;
    GUserEventSource *userEventSource;

    void runTimersOnceWithNormalPriority();

    QEventLoop::ProcessEventsFlags m_flags = QEventLoop::AllEvents;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_GLIB_P_H
