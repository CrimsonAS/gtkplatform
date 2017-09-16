/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qgtkeventdispatcher.h"
#include <private/qeventdispatcher_unix_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qthread_p.h>

#include "qcoreapplication.h"
#include "qsocketnotifier.h"

#include <QtCore/qlist.h>
#include <QtCore/qpair.h>

#include <glib.h>

QT_BEGIN_NAMESPACE

struct GPollFDWithQSocketNotifier
{
    GPollFD pollfd;
    QSocketNotifier *socketNotifier;
};

struct GSocketNotifierSource
{
    GSource source;
    QList<GPollFDWithQSocketNotifier *> pollfds;
};

static gboolean socketNotifierSourcePrepare(GSource *, gint *timeout)
{
    if (timeout)
        *timeout = -1;
    return false;
}

static gboolean socketNotifierSourceCheck(GSource *source)
{
    GSocketNotifierSource *src = reinterpret_cast<GSocketNotifierSource *>(source);

    bool pending = false;
    for (int i = 0; !pending && i < src->pollfds.count(); ++i) {
        GPollFDWithQSocketNotifier *p = src->pollfds.at(i);

        if (p->pollfd.revents & G_IO_NVAL) {
            // disable the invalid socket notifier
            static const char *t[] = { "Read", "Write", "Exception" };
            qWarning("QSocketNotifier: Invalid socket %d and type '%s', disabling...",
                     p->pollfd.fd, t[int(p->socketNotifier->type())]);
            // ### note, modifies src->pollfds!
            p->socketNotifier->setEnabled(false);
        }

        pending = ((p->pollfd.revents & p->pollfd.events) != 0);
    }

    return pending;
}

static gboolean socketNotifierSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    QEvent event(QEvent::SockAct);

    GSocketNotifierSource *src = reinterpret_cast<GSocketNotifierSource *>(source);
    for (int i = 0; i < src->pollfds.count(); ++i) {
        GPollFDWithQSocketNotifier *p = src->pollfds.at(i);

        if ((p->pollfd.revents & p->pollfd.events) != 0)
            QCoreApplication::sendEvent(p->socketNotifier, &event);
    }

    return true; // ??? don't remove, right?
}

static GSourceFuncs socketNotifierSourceFuncs = {
    socketNotifierSourcePrepare,
    socketNotifierSourceCheck,
    socketNotifierSourceDispatch,
    NULL,
    NULL,
    NULL
};

struct GTimerSource
{
    GSource source;
    QTimerInfoList timerList;
    QEventLoop::ProcessEventsFlags processEventsFlags;
    bool runWithIdlePriority;
};

static gboolean timerSourcePrepareHelper(GTimerSource *src, gint *timeout)
{
    timespec tv = { 0l, 0l };
    if (!(src->processEventsFlags & QEventLoop::X11ExcludeTimers) && src->timerList.timerWait(tv))
        *timeout = (tv.tv_sec * 1000) + ((tv.tv_nsec + 999999) / 1000 / 1000);
    else
        *timeout = -1;

    return (*timeout == 0);
}

static gboolean timerSourceCheckHelper(GTimerSource *src)
{
    if (src->timerList.isEmpty()
        || (src->processEventsFlags & QEventLoop::X11ExcludeTimers))
        return false;

    if (src->timerList.updateCurrentTime() < src->timerList.constFirst()->timeout)
        return false;

    return true;
}

static gboolean timerSourcePrepare(GSource *source, gint *timeout)
{
    gint dummy;
    if (!timeout)
        timeout = &dummy;

    GTimerSource *src = reinterpret_cast<GTimerSource *>(source);
    if (src->runWithIdlePriority) {
        if (timeout)
            *timeout = -1;
        return false;
    }

    return timerSourcePrepareHelper(src, timeout);
}

static gboolean timerSourceCheck(GSource *source)
{
    GTimerSource *src = reinterpret_cast<GTimerSource *>(source);
    if (src->runWithIdlePriority)
        return false;
    return timerSourceCheckHelper(src);
}

static gboolean timerSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    GTimerSource *timerSource = reinterpret_cast<GTimerSource *>(source);
    if (timerSource->processEventsFlags & QEventLoop::X11ExcludeTimers)
        return true;
    timerSource->runWithIdlePriority = true;
    (void) timerSource->timerList.activateTimers();
    return true; // ??? don't remove, right again?
}

static GSourceFuncs timerSourceFuncs = {
    timerSourcePrepare,
    timerSourceCheck,
    timerSourceDispatch,
    NULL,
    NULL,
    NULL
};

struct GIdleTimerSource
{
    GSource source;
    GTimerSource *timerSource;
};

static gboolean idleTimerSourcePrepare(GSource *source, gint *timeout)
{
    GIdleTimerSource *idleTimerSource = reinterpret_cast<GIdleTimerSource *>(source);
    GTimerSource *timerSource = idleTimerSource->timerSource;
    if (!timerSource->runWithIdlePriority) {
        // Yield to the normal priority timer source
        if (timeout)
            *timeout = -1;
        return false;
    }

    return timerSourcePrepareHelper(timerSource, timeout);
}

static gboolean idleTimerSourceCheck(GSource *source)
{
    GIdleTimerSource *idleTimerSource = reinterpret_cast<GIdleTimerSource *>(source);
    GTimerSource *timerSource = idleTimerSource->timerSource;
    if (!timerSource->runWithIdlePriority) {
        // Yield to the normal priority timer source
        return false;
    }
    return timerSourceCheckHelper(timerSource);
}

static gboolean idleTimerSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    GTimerSource *timerSource = reinterpret_cast<GIdleTimerSource *>(source)->timerSource;
    (void) timerSourceDispatch(&timerSource->source, 0, 0);
    return true;
}

static GSourceFuncs idleTimerSourceFuncs = {
    idleTimerSourcePrepare,
    idleTimerSourceCheck,
    idleTimerSourceDispatch,
    NULL,
    NULL,
    NULL
};

struct GPostEventSource
{
    GSource source;
    QAtomicInt serialNumber;
    int lastSerialNumber;
    QGtkEventDispatcherPrivate *d;
};

static gboolean postEventSourcePrepare(GSource *s, gint *timeout)
{
    QThreadData *data = reinterpret_cast<QThreadPrivate*>(QObjectPrivate::get(QThread::currentThread()))->data;
    if (!data)
        return false;

    gint dummy;
    if (!timeout)
        timeout = &dummy;
    const bool canWait = data->canWaitLocked();
    *timeout = canWait ? -1 : 0;

    GPostEventSource *source = reinterpret_cast<GPostEventSource *>(s);
    return (!canWait
            || (source->serialNumber.load() != source->lastSerialNumber));
}

static gboolean postEventSourceCheck(GSource *source)
{
    return postEventSourcePrepare(source, 0);
}

static gboolean postEventSourceDispatch(GSource *s, GSourceFunc, gpointer)
{
    GPostEventSource *source = reinterpret_cast<GPostEventSource *>(s);
    source->lastSerialNumber = source->serialNumber.load();
    QCoreApplication::sendPostedEvents();
    source->d->runTimersOnceWithNormalPriority();
    return true; // i dunno, george...
}

static GSourceFuncs postEventSourceFuncs = {
    postEventSourcePrepare,
    postEventSourceCheck,
    postEventSourceDispatch,
    NULL,
    NULL,
    NULL
};

struct GUserEventSource
{
    GSource source;
    QGtkEventDispatcherPrivate *d;
};

static gboolean userEventSourcePrepare(GSource *s, gint *timeout)
{
    Q_UNUSED(s);
    Q_UNUSED(timeout);

    return QWindowSystemInterface::windowSystemEventsQueued() > 0;
}

static gboolean userEventSourceCheck(GSource *source)
{
    return userEventSourcePrepare(source, 0);
}

static gboolean userEventSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    GUserEventSource *userEventSource = reinterpret_cast<GUserEventSource*>(source);
    QGtkEventDispatcherPrivate *dispatcher = userEventSource->d;
    QWindowSystemInterface::sendWindowSystemEvents(dispatcher->m_flags);
    return true;
}

static GSourceFuncs userEventSourceFuncs = {
    userEventSourcePrepare,
    userEventSourceCheck,
    userEventSourceDispatch,
    NULL,
    NULL,
    NULL
};

QGtkEventDispatcherPrivate::QGtkEventDispatcherPrivate(GMainContext *context)
    : mainContext(context)
{
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 32
    if (qEnvironmentVariableIsEmpty("QT_NO_THREADED_GLIB")) {
        static QBasicMutex mutex;
        QMutexLocker locker(&mutex);
        if (!g_thread_supported())
            g_thread_init(NULL);
    }
#endif

    if (mainContext) {
        g_main_context_ref(mainContext);
    } else {
        QCoreApplication *app = QCoreApplication::instance();
        if (app && QThread::currentThread() == app->thread()) {
            mainContext = g_main_context_default();
            g_main_context_ref(mainContext);
        } else {
            mainContext = g_main_context_new();
        }
    }

#if GLIB_CHECK_VERSION (2, 22, 0)
    g_main_context_push_thread_default (mainContext);
#endif

    // ### RB: modified from upstream
    // We must ensure that gtk's priority settings are above us, otherwise we
    // might exhaust gtk, meaning that windows won't show etc.
    int QT_BASE_PRIORITY = G_PRIORITY_LOW - 1;

    // user event source
    userEventSource = reinterpret_cast<GUserEventSource *>(g_source_new(&userEventSourceFuncs,
                                                                        sizeof(GUserEventSource)));
    userEventSource->d = this;
    g_source_set_priority(&userEventSource->source, QT_BASE_PRIORITY - 2);
    g_source_set_can_recurse(&userEventSource->source, true);
    g_source_attach(&userEventSource->source, mainContext);

    // setup post event source
    postEventSource = reinterpret_cast<GPostEventSource *>(g_source_new(&postEventSourceFuncs,
                                                                        sizeof(GPostEventSource)));
    g_source_set_priority(&postEventSource->source, QT_BASE_PRIORITY - 1);
    postEventSource->serialNumber.store(1);
    postEventSource->d = this;
    g_source_set_can_recurse(&postEventSource->source, true);
    g_source_attach(&postEventSource->source, mainContext);

    // setup socketNotifierSource
    socketNotifierSource =
        reinterpret_cast<GSocketNotifierSource *>(g_source_new(&socketNotifierSourceFuncs,
                                                               sizeof(GSocketNotifierSource)));
    g_source_set_priority(&socketNotifierSource->source, QT_BASE_PRIORITY - 2);
    (void) new (&socketNotifierSource->pollfds) QList<GPollFDWithQSocketNotifier *>();
    g_source_set_can_recurse(&socketNotifierSource->source, true);
    g_source_attach(&socketNotifierSource->source, mainContext);

    // setup normal and idle timer sources
    timerSource = reinterpret_cast<GTimerSource *>(g_source_new(&timerSourceFuncs,
                                                                sizeof(GTimerSource)));
    g_source_set_priority(&timerSource->source, QT_BASE_PRIORITY - 2);
    (void) new (&timerSource->timerList) QTimerInfoList();
    timerSource->processEventsFlags = QEventLoop::AllEvents;
    timerSource->runWithIdlePriority = false;
    g_source_set_can_recurse(&timerSource->source, true);
    g_source_attach(&timerSource->source, mainContext);

    idleTimerSource = reinterpret_cast<GIdleTimerSource *>(g_source_new(&idleTimerSourceFuncs,
                                                                        sizeof(GIdleTimerSource)));
    g_source_set_priority(&idleTimerSource->source, QT_BASE_PRIORITY - 3);
    idleTimerSource->timerSource = timerSource;
    g_source_set_can_recurse(&idleTimerSource->source, true);
    g_source_attach(&idleTimerSource->source, mainContext);
}

void QGtkEventDispatcherPrivate::runTimersOnceWithNormalPriority()
{
    timerSource->runWithIdlePriority = false;
}

QGtkEventDispatcher::QGtkEventDispatcher(QObject *parent)
    : QAbstractEventDispatcher(*(new QGtkEventDispatcherPrivate), parent)
{
}

QGtkEventDispatcher::QGtkEventDispatcher(GMainContext *mainContext, QObject *parent)
    : QAbstractEventDispatcher(*(new QGtkEventDispatcherPrivate(mainContext)), parent)
{ }

QGtkEventDispatcher::~QGtkEventDispatcher()
{
    Q_D(QGtkEventDispatcher);

    // destroy user event source
    g_source_destroy(&d->userEventSource->source);
    g_source_unref(&d->userEventSource->source);
    d->userEventSource = 0;

    // destroy all timer sources
    qDeleteAll(d->timerSource->timerList);
    d->timerSource->timerList.~QTimerInfoList();
    g_source_destroy(&d->timerSource->source);
    g_source_unref(&d->timerSource->source);
    d->timerSource = 0;
    g_source_destroy(&d->idleTimerSource->source);
    g_source_unref(&d->idleTimerSource->source);
    d->idleTimerSource = 0;

    // destroy socket notifier source
    for (int i = 0; i < d->socketNotifierSource->pollfds.count(); ++i) {
        GPollFDWithQSocketNotifier *p = d->socketNotifierSource->pollfds[i];
        g_source_remove_poll(&d->socketNotifierSource->source, &p->pollfd);
        delete p;
    }
    d->socketNotifierSource->pollfds.~QList<GPollFDWithQSocketNotifier *>();
    g_source_destroy(&d->socketNotifierSource->source);
    g_source_unref(&d->socketNotifierSource->source);
    d->socketNotifierSource = 0;

    // destroy post event source
    g_source_destroy(&d->postEventSource->source);
    g_source_unref(&d->postEventSource->source);
    d->postEventSource = 0;

    Q_ASSERT(d->mainContext != 0);
#if GLIB_CHECK_VERSION (2, 22, 0)
    g_main_context_pop_thread_default (d->mainContext);
#endif
    g_main_context_unref(d->mainContext);
    d->mainContext = 0;
}

bool QGtkEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QGtkEventDispatcher);

    d->m_flags = flags;

    const bool canWait = (flags & QEventLoop::WaitForMoreEvents);
    if (canWait)
        Q_EMIT aboutToBlock();
    else
        Q_EMIT awake();

    // tell postEventSourcePrepare() and timerSource about any new flags
    QEventLoop::ProcessEventsFlags savedFlags = d->timerSource->processEventsFlags;
    d->timerSource->processEventsFlags = flags;

    if (!(flags & QEventLoop::EventLoopExec)) {
        // force timers to be sent at normal priority
        d->timerSource->runWithIdlePriority = false;
    }

    bool result = g_main_context_iteration(d->mainContext, canWait);
    while (!result && canWait)
        result = g_main_context_iteration(d->mainContext, canWait);

    d->timerSource->processEventsFlags = savedFlags;

    if (canWait)
        Q_EMIT awake();

    return result;
}

bool QGtkEventDispatcher::hasPendingEvents()
{
    Q_D(QGtkEventDispatcher);
    return g_main_context_pending(d->mainContext);
}

void QGtkEventDispatcher::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int sockfd = notifier->socket();
    int type = notifier->type();
#ifndef QT_NO_DEBUG
    if (sockfd < 0) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread()
               || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be enabled from another thread");
        return;
    }
#endif

    Q_D(QGtkEventDispatcher);


    GPollFDWithQSocketNotifier *p = new GPollFDWithQSocketNotifier;
    p->pollfd.fd = sockfd;
    switch (type) {
    case QSocketNotifier::Read:
        p->pollfd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
        break;
    case QSocketNotifier::Write:
        p->pollfd.events = G_IO_OUT | G_IO_ERR;
        break;
    case QSocketNotifier::Exception:
        p->pollfd.events = G_IO_PRI | G_IO_ERR;
        break;
    }
    p->socketNotifier = notifier;

    d->socketNotifierSource->pollfds.append(p);

    g_source_add_poll(&d->socketNotifierSource->source, &p->pollfd);
}

void QGtkEventDispatcher::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
#ifndef QT_NO_DEBUG
    int sockfd = notifier->socket();
    if (sockfd < 0) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread()
               || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be disabled from another thread");
        return;
    }
#endif

    Q_D(QGtkEventDispatcher);

    for (int i = 0; i < d->socketNotifierSource->pollfds.count(); ++i) {
        GPollFDWithQSocketNotifier *p = d->socketNotifierSource->pollfds.at(i);
        if (p->socketNotifier == notifier) {
            // found it
            g_source_remove_poll(&d->socketNotifierSource->source, &p->pollfd);

            d->socketNotifierSource->pollfds.removeAt(i);
            delete p;

            return;
        }
    }
}

void QGtkEventDispatcher::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QGtkEventDispatcher::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QGtkEventDispatcher::registerTimer: timers cannot be started from another thread");
        return;
    }
#endif

    Q_D(QGtkEventDispatcher);
    d->timerSource->timerList.registerTimer(timerId, interval, timerType, object);
}

bool QGtkEventDispatcher::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QGtkEventDispatcher::unregisterTimer: invalid argument");
        return false;
    } else if (thread() != QThread::currentThread()) {
        qWarning("QGtkEventDispatcher::unregisterTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QGtkEventDispatcher);
    return d->timerSource->timerList.unregisterTimer(timerId);
}

bool QGtkEventDispatcher::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QGtkEventDispatcher::unregisterTimers: invalid argument");
        return false;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QGtkEventDispatcher::unregisterTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QGtkEventDispatcher);
    return d->timerSource->timerList.unregisterTimers(object);
}

QList<QGtkEventDispatcher::TimerInfo> QGtkEventDispatcher::registeredTimers(QObject *object) const
{
    if (!object) {
        qWarning("QGtkEventDispatcher:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }

    Q_D(const QGtkEventDispatcher);
    return d->timerSource->timerList.registeredTimers(object);
}

int QGtkEventDispatcher::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QGtkEventDispatcher::remainingTimeTime: invalid argument");
        return -1;
    }
#endif

    Q_D(QGtkEventDispatcher);
    return d->timerSource->timerList.timerRemainingTime(timerId);
}

void QGtkEventDispatcher::interrupt()
{
    wakeUp();
}

void QGtkEventDispatcher::wakeUp()
{
    Q_D(QGtkEventDispatcher);
    d->postEventSource->serialNumber.ref();
    g_main_context_wakeup(d->mainContext);
}

void QGtkEventDispatcher::flush()
{
}

bool QGtkEventDispatcher::versionSupported()
{
#if !defined(GLIB_MAJOR_VERSION) || !defined(GLIB_MINOR_VERSION) || !defined(GLIB_MICRO_VERSION)
    return false;
#else
    return ((GLIB_MAJOR_VERSION << 16) + (GLIB_MINOR_VERSION << 8) + GLIB_MICRO_VERSION) >= 0x020301;
#endif
}

QGtkEventDispatcher::QGtkEventDispatcher(QGtkEventDispatcherPrivate &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{
}

QT_END_NAMESPACE
