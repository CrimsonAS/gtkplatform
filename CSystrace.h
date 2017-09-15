/*
 * Copyright (c) 2017 Crimson AS <info@crimson.no>
 * Author: Robin Burchell <robin.burchell@crimson.no>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SYSTRACE_H
#define SYSTRACE_H

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
# if defined(BUILDING_DLL)
#  if defined(__GNUC__)
#   define SYSTRACE_EXPORT __attribute__ ((dllexport))
#  else
#   define SYSTRACE_EXPORT __declspec(dllexport)
#  endif
# else
#  if defined(__GNUC__)
#   define SYSTRACE_EXPORT __attribute__ ((dllimport))
#  else
#   define SYSTRACE_EXPORT __declspec(dllimport)
#  endif
# endif
#else
# define SYSTRACE_EXPORT __attribute__ ((visibility ("default")))
#endif

/*!
 * Perform necessary set up. Should be called before any other functions.
 *
 * \note If your compiler supports the `constructor` attribute (gcc does),
 * then this method will be called for you. Calling it multiple times will not
 * cause trouble, however, so feel free to call it yourself.
 *
 * \sa systrace_deinit()
 */
SYSTRACE_EXPORT void systrace_init();

/*!
 * Perform necessary tear down. Should be called before termination, and no systrace
 * methods should be called after it.
 *
 * \note If your compiler supports the `destructor` attribute (gcc does),
 * then this method will be called for you. Calling it multiple times will not
 * cause trouble, however, so feel free to call it yourself.
 *
 * \sa systrace_init()
 */
SYSTRACE_EXPORT void systrace_deinit();

/*!
 * Determine whether or not a given \a module should be traced.
 * This can be used to avoid expensive setup (such as allocation of data for the
 * trace event).
 *
 * Returns 1 if the event should be traced, 0 otherwise.
 */
SYSTRACE_EXPORT int systrace_should_trace(const char *module);

struct CSystraceEvent;

SYSTRACE_EXPORT void systrace_duration_begin(const char *module, const char *tracepoint);
SYSTRACE_EXPORT void systrace_duration_end(const char *module, const char *tracepoint);
SYSTRACE_EXPORT void systrace_duration_begin(CSystraceEvent &event);
SYSTRACE_EXPORT void systrace_duration_end(CSystraceEvent &event);
SYSTRACE_EXPORT void systrace_record_counter(const char *module, const char *tracepoint, int value, int id = -1);
SYSTRACE_EXPORT void systrace_async_begin(const char *module, const char *tracepoint, const void *cookie);
SYSTRACE_EXPORT void systrace_async_end(const char *module, const char *tracepoint, const void *cookie);
struct SYSTRACE_EXPORT CSystraceEvent
{
public:
    CSystraceEvent(const char *module, const char *tracepoint)
        : m_module(module)
        , m_tracepoint(tracepoint)
    {
        systrace_duration_begin(*this);
    }

    ~CSystraceEvent()
    {
        systrace_duration_end(*this);
    }

    const char *m_module;
    const char *m_tracepoint;
    uint64_t m_begin;
};

struct SYSTRACE_EXPORT CSystraceAsyncEvent
{
public:
    CSystraceAsyncEvent(const char *module, const char *tracepoint, const void *cookie)
        : m_module(module)
        , m_tracepoint(tracepoint)
        , m_cookie(cookie)
    {
        systrace_async_begin(m_module, m_tracepoint, m_cookie);
    }

    ~CSystraceAsyncEvent()
    {
        systrace_async_end(m_module, m_tracepoint, m_cookie);
    }

private:
    const char *m_module;
    const char *m_tracepoint;
    const void *m_cookie;
};

#define COMBINE1(X,Y) X##Y  // helper macros
#define COMBINE(X,Y) COMBINE1(X,Y)

// ### TRACE_STR_COPY
// ### TRACE_EVENT_COPY_XXX

// Records a pair of begin and end events called "name" for the current
// scope, with 0, 1 or 2 associated arguments. If the category is not
// enabled, then this does nothing.
// - category and name strings must have application lifetime (statics or
//   literals). They may not include " chars.
#define TRACE_EVENT0(module, tracepoint) \
    CSystraceEvent COMBINE(ev, __LINE__) (module, tracepoint);
// ### EVENT1, EVENT2

// ### TRACE_EVENT_INSTANT0?

#define TRACE_EVENT_BEGIN0(module, tracepoint) \
    systrace_duration_begin(module, tracepoint);
#define TRACE_EVENT_END0(module, tracepoint) \
    systrace_duration_end(module, tracepoint);
// ### BEGIN & END 1, 2


// ###:
// Pointers can be used for the ID parameter, and they will be mangled
// internally so that the same pointer on two different processes will not
// match.
#define TRACE_EVENT_ASYNC_BEGIN0(module, tracepoint, cookie) \
    systrace_async_begin(module, tracepoint, cookie);
#define TRACE_EVENT_ASYNC_END0(module, tracepoint, cookie) \
    systrace_async_end(module, tracepoint, cookie);
// ### 1 & 2

#define TRACE_COUNTER1(module, tracepoint, value) \
    systrace_record_counter(module, tracepoint, value);
// ### TRACE_COUNTER2

#define TRACE_COUNTER_ID1(module, tracepoint, value, id) \
    systrace_record_counter(module, tracepoint, value, id);

#endif // SYSTRACE_H
