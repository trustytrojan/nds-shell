#ifndef _PTHREAD_H
#define _PTHREAD_H

#include <errno.h>
#include <stdint.h>
#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef thrd_t pthread_t;
typedef struct {
    int detachstate;
} pthread_attr_t;
typedef int pthread_mutexattr_t;
typedef int pthread_condattr_t;

typedef mtx_t pthread_mutex_t;

typedef struct {
    int dummy;
} pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER 0
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

static inline int pthread_attr_init(pthread_attr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    return 0;
}

static inline int pthread_attr_destroy(pthread_attr_t *attr)
{
    (void)attr;
    return 0;
}

static inline int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if (!attr)
        return EINVAL;
    if ((detachstate != PTHREAD_CREATE_JOINABLE) && (detachstate != PTHREAD_CREATE_DETACHED))
        return EINVAL;
    attr->detachstate = detachstate;
    return 0;
}

static inline int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
    if (!attr || !detachstate)
        return EINVAL;
    *detachstate = attr->detachstate;
    return 0;
}

static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                 void *(*start_routine)(void *), void *arg)
{
    if (!thread || !start_routine)
        return EINVAL;
    if (attr && (attr->detachstate != PTHREAD_CREATE_JOINABLE) &&
        (attr->detachstate != PTHREAD_CREATE_DETACHED))
        return EINVAL;

    cothread_t id = cothread_create((cothread_entrypoint_t)start_routine, arg, 0, 0);
    if (id < 0)
        return (errno != 0) ? errno : EAGAIN;

    if (attr && attr->detachstate == PTHREAD_CREATE_DETACHED)
    {
        if (thrd_detach(id) != thrd_success)
        {
            cothread_delete(id);
            return EINVAL;
        }
    }

    *thread = id;
    return 0;
}

static inline int pthread_join(pthread_t thread, void **retval)
{
    int result = 0;
    if (thrd_join(thread, &result) != thrd_success)
        return (errno != 0) ? errno : EINVAL;
    if (retval)
        *retval = (void *)(intptr_t)result;
    return 0;
}

static inline void pthread_exit(void *retval)
{
    (void)retval;
    for (;;)
        thrd_yield();
}

static inline pthread_t pthread_self(void)
{
    return thrd_current();
}

static inline int pthread_detach(pthread_t thread)
{
    return (thrd_detach(thread) == thrd_success) ? 0 : EINVAL;
}

static inline int pthread_yield(void)
{
    thrd_yield();
    return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    (void)attr;
    return (mtx_init(mutex, mtx_plain) == thrd_success) ? 0 : EINVAL;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    return (mtx_lock(mutex) == thrd_success) ? 0 : EINVAL;
}

static inline int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    return (mtx_trylock(mutex) == thrd_success) ? 0 : EBUSY;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    return (mtx_unlock(mutex) == thrd_success) ? 0 : EINVAL;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    (void)mutex;
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif // _PTHREAD_H