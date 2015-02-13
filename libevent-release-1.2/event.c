/*
 * Copyright (c) 2000-2004 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * libevent的使用流程:
 * ===================
 * 1) 调用event_init()接口初始化libevent
 * 2) 调用event_set()接口设计事件
 * 3) 调用event_add()接口把事件添加到libevent中进行监听
 * 4) 调用event_dispatch()开始等待事件的触发, 并调用事件关联的回调函数
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include "misc.h"
#endif
#include <sys/types.h>
#include <sys/tree.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else 
#include <sys/_time.h>
#endif
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include "event.h"
#include "event-internal.h"
#include "log.h"

#ifdef HAVE_EVENT_PORTS
extern const struct eventop evportops;
#endif
#ifdef HAVE_SELECT
extern const struct eventop selectops;
#endif
#ifdef HAVE_POLL
extern const struct eventop pollops;
#endif
#ifdef HAVE_RTSIG
extern const struct eventop rtsigops;
#endif
#ifdef HAVE_EPOLL
extern const struct eventop epollops;
#endif
#ifdef HAVE_WORKING_KQUEUE
extern const struct eventop kqops;
#endif
#ifdef HAVE_DEVPOLL
extern const struct eventop devpollops;
#endif
#ifdef WIN32
extern const struct eventop win32ops;
#endif

/* In order of preference */
const struct eventop *eventops[] = {
#ifdef HAVE_EVENT_PORTS
	&evportops,
#endif
#ifdef HAVE_WORKING_KQUEUE
	&kqops,
#endif
#ifdef HAVE_EPOLL
	&epollops,
#endif
#ifdef HAVE_DEVPOLL
	&devpollops,
#endif
#ifdef HAVE_RTSIG
	&rtsigops,
#endif
#ifdef HAVE_POLL
	&pollops,
#endif
#ifdef HAVE_SELECT
	&selectops,
#endif
#ifdef WIN32
	&win32ops,
#endif
	NULL
};

/* Global state */
struct event_list signalqueue;

struct event_base *current_base = NULL;

/* Handle signals - This is a deprecated interface */
int (*event_sigcb)(void);		/* Signal callback when gotsig is set */
volatile sig_atomic_t event_gotsig;	/* Set in signal handler */

/* Prototypes */
static void	event_queue_insert(struct event_base *, struct event *, int);
static void	event_queue_remove(struct event_base *, struct event *, int);
static int	event_haveevents(struct event_base *);

static void	event_process_active(struct event_base *);

static int	timeout_next(struct event_base *, struct timeval *);
static void	timeout_process(struct event_base *);
static void	timeout_correct(struct event_base *, struct timeval *);

static int
compare(struct event *a, struct event *b)
{
	if (timercmp(&a->ev_timeout, &b->ev_timeout, <))
		return (-1);
	else if (timercmp(&a->ev_timeout, &b->ev_timeout, >))
		return (1);
	if (a < b)
		return (-1);
	else if (a > b)
		return (1);
	return (0);
}

static int
gettime(struct timeval *tp)
{
#ifdef HAVE_CLOCK_GETTIME
	struct timespec	ts;
	
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
		return (-1);
	tp->tv_sec = ts.tv_sec;
	tp->tv_usec = ts.tv_nsec / 1000;
#else
	gettimeofday(tp, NULL);
#endif

	return (0);
}

RB_PROTOTYPE(event_tree, event, ev_timeout_node, compare);

RB_GENERATE(event_tree, event, ev_timeout_node, compare);


/*
 * 初始化libevent环境
 */
void *
event_init(void)
{
	int i;

	if ((current_base = calloc(1, sizeof(struct event_base))) == NULL)
		event_err(1, "%s: calloc");

	event_sigcb = NULL;
	event_gotsig = 0;
	gettime(&current_base->event_tv); // 获取当前时间

	// 初始化一些队列
	RB_INIT(&current_base->timetree);
	TAILQ_INIT(&current_base->eventqueue);
	TAILQ_INIT(&signalqueue);

	// 选择合适的多路复用IO接口
	current_base->evbase = NULL;
	for (i = 0; eventops[i] && !current_base->evbase; i++) {
		current_base->evsel = eventops[i];
		current_base->evbase = current_base->evsel->init();
	}

	// 如果初始化多路复用IO接口失败
	if (current_base->evbase == NULL)
		event_errx(1, "%s: no event mechanism available", __func__);

	if (getenv("EVENT_SHOW_METHOD")) 
		event_msgx("libevent using: %s\n",
			   current_base->evsel->name);

	/* allocate a single active event queue */
	// 初始化活跃优先队列
	event_base_priority_init(current_base, 1);

	return (current_base);
}

void
event_base_free(struct event_base *base)
{
	int i;

	if (base == NULL && current_base)
		base = current_base;
        if (base == current_base)
		current_base = NULL;

	assert(base);
	assert(TAILQ_EMPTY(&base->eventqueue));
	for (i=0; i < base->nactivequeues; ++i)
		assert(TAILQ_EMPTY(base->activequeues[i]));

	assert(RB_EMPTY(&base->timetree));

	for (i = 0; i < base->nactivequeues; ++i)
		free(base->activequeues[i]);
	free(base->activequeues);

	if (base->evsel->dealloc != NULL)
		base->evsel->dealloc(base->evbase);

	free(base);
}

int
event_priority_init(int npriorities)
{
  return event_base_priority_init(current_base, npriorities);
}

int
event_base_priority_init(struct event_base *base, int npriorities)
{
	int i;

	if (base->event_count_active)
		return (-1);

	if (base->nactivequeues && npriorities != base->nactivequeues) {
		for (i = 0; i < base->nactivequeues; ++i) {
			free(base->activequeues[i]);
		}
		free(base->activequeues);
	}

	/* Allocate our priority queues */
	base->nactivequeues = npriorities;
	/* bug here? */
	base->activequeues = (struct event_list **)calloc(base->nactivequeues,
	    npriorities * sizeof(struct event_list *));
	if (base->activequeues == NULL)
		event_err(1, "%s: calloc", __func__);

	// 创建event_list对象
	for (i = 0; i < base->nactivequeues; ++i) {
		base->activequeues[i] = malloc(sizeof(struct event_list));
		if (base->activequeues[i] == NULL)
			event_err(1, "%s: malloc", __func__);
		TAILQ_INIT(base->activequeues[i]);
	}

	return (0);
}

int
event_haveevents(struct event_base *base)
{
	return (base->event_count > 0);
}

/*
 * Active events are stored in priority queues.  Lower priorities are always
 * process before higher priorities.  Low priority events can starve high
 * priority ones.
 */

static void
event_process_active(struct event_base *base)
{
	struct event *ev;
	struct event_list *activeq = NULL;
	int i;
	short ncalls;

	if (!base->event_count_active)
		return;

	for (i = 0; i < base->nactivequeues; ++i) {
		if (TAILQ_FIRST(base->activequeues[i]) != NULL) {
			activeq = base->activequeues[i];
			break;
		}
	}

	for (ev = TAILQ_FIRST(activeq); ev; ev = TAILQ_FIRST(activeq)) {
		event_queue_remove(base, ev, EVLIST_ACTIVE);
		
		/* Allows deletes to work */
		ncalls = ev->ev_ncalls;
		ev->ev_pncalls = &ncalls;
		while (ncalls) {
			ncalls--;
			ev->ev_ncalls = ncalls;
			(*ev->ev_callback)((int)ev->ev_fd, ev->ev_res, ev->ev_arg);
		}
	}
}

/*
 * Wait continously for events.  We exit only if no events are left.
 */

int
event_dispatch(void)
{
	return (event_loop(0));
}

int
event_base_dispatch(struct event_base *event_base)
{
  return (event_base_loop(event_base, 0));
}

static void
event_loopexit_cb(int fd, short what, void *arg)
{
	struct event_base *base = arg;
	base->event_gotterm = 1;
}

/* not thread safe */

int
event_loopexit(struct timeval *tv)
{
	return (event_once(-1, EV_TIMEOUT, event_loopexit_cb,
		    current_base, tv));
}

int
event_base_loopexit(struct event_base *event_base, struct timeval *tv)
{
	return (event_once(-1, EV_TIMEOUT, event_loopexit_cb,
		    event_base, tv));
}

/* not thread safe */

int
event_loop(int flags)
{
	return event_base_loop(current_base, flags);
}

int
event_base_loop(struct event_base *base, int flags)
{
	const struct eventop *evsel = base->evsel;
	void *evbase = base->evbase;
	struct timeval tv;
	int res, done;

	done = 0;
	while (!done) {
		/* Calculate the initial events that we are waiting for */
		if (evsel->recalc(base, evbase, 0) == -1)
			return (-1);

		/* Terminate the loop if we have been asked to */
		if (base->event_gotterm) { // 结束loop
			base->event_gotterm = 0;
			break;
		}

		/* You cannot use this interface for multi-threaded apps */
		while (event_gotsig) { // 如果收到信号
			event_gotsig = 0;
			if (event_sigcb) {
				res = (*event_sigcb)();
				if (res == -1) {
					errno = EINTR;
					return (-1);
				}
			}
		}

		/* Check if time is running backwards */
		gettime(&tv);
		if (timercmp(&tv, &base->event_tv, <)) { // 如果当前时候比最后一次更新的时间小, 说明服务器时间出错了
			struct timeval off;
			event_debug(("%s: time is running backwards, corrected",
				    __func__));
			// 调整时间
			timersub(&base->event_tv, &tv, &off);
			timeout_correct(base, &off);
		}
		base->event_tv = tv;

		// epoll需要等待的最大时间
		if (!base->event_count_active && !(flags & EVLOOP_NONBLOCK))
			timeout_next(base, &tv);
		else
			timerclear(&tv);

		/* If we have no events, we just exit */
		if (!event_haveevents(base)) {
			event_debug(("%s: no events registered.", __func__));
			return (1);
		}

		res = evsel->dispatch(base, evbase, &tv); // 开始等待事件的发生

		if (res == -1)
			return (-1);

		timeout_process(base); // 把超时事件放进活动队列中

		if (base->event_count_active) {
			event_process_active(base); // 处理活动队列的事件
			if (!base->event_count_active && (flags & EVLOOP_ONCE))
				done = 1;
		} else if (flags & EVLOOP_NONBLOCK)
			done = 1;
	}

	event_debug(("%s: asked to terminate loop.", __func__));
	return (0);
}

/* Sets up an event for processing once */

struct event_once {
	struct event ev;

	void (*cb)(int, short, void *);
	void *arg;
};

/* One-time callback, it deletes itself */

static void
event_once_cb(int fd, short events, void *arg)
{
	struct event_once *eonce = arg;

	(*eonce->cb)(fd, events, eonce->arg);
	free(eonce);
}

/* Schedules an event once */

int
event_once(int fd, short events,
    void (*callback)(int, short, void *), void *arg, struct timeval *tv)
{
	struct event_once *eonce;
	struct timeval etv;

	/* We cannot support signals that just fire once */
	if (events & EV_SIGNAL)
		return (-1);

	if ((eonce = calloc(1, sizeof(struct event_once))) == NULL)
		return (-1);

	eonce->cb = callback;
	eonce->arg = arg;

	if (events == EV_TIMEOUT) {
		if (tv == NULL) {
			timerclear(&etv);
			tv = &etv;
		}

		evtimer_set(&eonce->ev, event_once_cb, eonce);
	} else if (events & (EV_READ|EV_WRITE)) {
		events &= EV_READ|EV_WRITE;

		event_set(&eonce->ev, fd, events, event_once_cb, eonce);
	} else {
		/* Bad event combination */
		free(eonce);
		return (-1);
	}

	event_add(&eonce->ev, tv);

	return (0);
}

/*
 * 设置事件相关的数据(如: 关注的文件句柄和事件, 回调函数等等)
 */
void
event_set(struct event *ev, int fd, short events,
	  void (*callback)(int, short, void *), void *arg)
{
	/* Take the current base - caller needs to set the real base later */
	ev->ev_base = current_base;

	ev->ev_callback = callback;
	ev->ev_arg = arg;
	ev->ev_fd = fd;
	ev->ev_events = events;
	ev->ev_flags = EVLIST_INIT;
	ev->ev_ncalls = 0;
	ev->ev_pncalls = NULL;

	/* by default, we put new events into the middle priority */
	ev->ev_pri = current_base->nactivequeues/2;
}

int
event_base_set(struct event_base *base, struct event *ev)
{
	/* Only innocent events may be assigned to a different base */
	if (ev->ev_flags != EVLIST_INIT)
		return (-1);

	ev->ev_base = base;
	ev->ev_pri = base->nactivequeues/2;

	return (0);
}

/*
 * Set's the priority of an event - if an event is already scheduled
 * changing the priority is going to fail.
 */

int
event_priority_set(struct event *ev, int pri)
{
	if (ev->ev_flags & EVLIST_ACTIVE)
		return (-1);
	if (pri < 0 || pri >= ev->ev_base->nactivequeues)
		return (-1);

	ev->ev_pri = pri;

	return (0);
}

/*
 * Checks if a specific event is pending or scheduled.
 */

int
event_pending(struct event *ev, short event, struct timeval *tv)
{
	struct timeval	now, res;
	int flags = 0;

	if (ev->ev_flags & EVLIST_INSERTED)
		flags |= (ev->ev_events & (EV_READ|EV_WRITE));
	if (ev->ev_flags & EVLIST_ACTIVE)
		flags |= ev->ev_res;
	if (ev->ev_flags & EVLIST_TIMEOUT)
		flags |= EV_TIMEOUT;
	if (ev->ev_flags & EVLIST_SIGNAL)
		flags |= EV_SIGNAL;

	event &= (EV_TIMEOUT|EV_READ|EV_WRITE|EV_SIGNAL);

	/* See if there is a timeout that we should report */
	if (tv != NULL && (flags & event & EV_TIMEOUT)) {
		gettime(&now);
		timersub(&ev->ev_timeout, &now, &res);
		/* correctly remap to real time */
		gettimeofday(&now, NULL);
		timeradd(&now, &res, tv);
	}

	return (flags & event);
}

/*
 * 把事件对象添加到libevent中进行监听
 */
int
event_add(struct event *ev, struct timeval *tv)
{
	struct event_base *base = ev->ev_base;
	const struct eventop *evsel = base->evsel;
	void *evbase = base->evbase;

	event_debug((
		 "event_add: event: %p, %s%s%scall %p",
		 ev,
		 ev->ev_events & EV_READ ? "EV_READ " : " ",
		 ev->ev_events & EV_WRITE ? "EV_WRITE " : " ",
		 tv ? "EV_TIMEOUT " : " ",
		 ev->ev_callback));

	assert(!(ev->ev_flags & ~EVLIST_ALL));

	// 如果需要设置超时的
	if (tv != NULL) {
		struct timeval now;

		if (ev->ev_flags & EVLIST_TIMEOUT) // 当前事件已经在timeout队列中, 那么把他从timeout队列中删除
			event_queue_remove(base, ev, EVLIST_TIMEOUT);

		/* Check if it is active due to a timeout.  Rescheduling
		 * this timeout before the callback can be executed
		 * removes it from the active list. */
		if ((ev->ev_flags & EVLIST_ACTIVE) && // 如果当前事件已经超时, 那么把此事件从活动队列中删除
		    (ev->ev_res & EV_TIMEOUT)) {
			/* See if we are just active executing this
			 * event in a loop
			 */
			if (ev->ev_ncalls && ev->ev_pncalls) {
				/* Abort loop */
				*ev->ev_pncalls = 0;
			}
			
			event_queue_remove(base, ev, EVLIST_ACTIVE);
		}

		// 计算超时时间
		gettime(&now);
		timeradd(&now, tv, &ev->ev_timeout);

		event_debug((
			 "event_add: timeout in %d seconds, call %p",
			 tv->tv_sec, ev->ev_callback));

		// 把事件添加到超时队列中
		event_queue_insert(base, ev, EVLIST_TIMEOUT);
	}

	// IO事件
	if ((ev->ev_events & (EV_READ|EV_WRITE)) &&
	    !(ev->ev_flags & (EVLIST_INSERTED|EVLIST_ACTIVE))) {

		event_queue_insert(base, ev, EVLIST_INSERTED); // 添加到IO队列中
		return (evsel->add(evbase, ev));

	} else if ((ev->ev_events & EV_SIGNAL) &&
	    !(ev->ev_flags & EVLIST_SIGNAL)) { // 信号事件

		event_queue_insert(base, ev, EVLIST_SIGNAL); // 添加到信号队列中
		return (evsel->add(evbase, ev));
	}

	return (0);
}

int
event_del(struct event *ev)
{
	struct event_base *base;
	const struct eventop *evsel;
	void *evbase;

	event_debug(("event_del: %p, callback %p",
		 ev, ev->ev_callback));

	/* An event without a base has not been added */
	if (ev->ev_base == NULL)
		return (-1);

	base = ev->ev_base;
	evsel = base->evsel;
	evbase = base->evbase;

	assert(!(ev->ev_flags & ~EVLIST_ALL));

	/* See if we are just active executing this event in a loop */
	if (ev->ev_ncalls && ev->ev_pncalls) {
		/* Abort loop */
		*ev->ev_pncalls = 0;
	}

	if (ev->ev_flags & EVLIST_TIMEOUT) // 如果在timeout队列, 那么删除之
		event_queue_remove(base, ev, EVLIST_TIMEOUT);

	if (ev->ev_flags & EVLIST_ACTIVE) // 如果在active队列, 那么删除之
		event_queue_remove(base, ev, EVLIST_ACTIVE);

	if (ev->ev_flags & EVLIST_INSERTED) { // 如果在IO队列, 那么删除之
		event_queue_remove(base, ev, EVLIST_INSERTED);
		return (evsel->del(evbase, ev));

	} else if (ev->ev_flags & EVLIST_SIGNAL) { // 如果在信号队列, 删除之
		event_queue_remove(base, ev, EVLIST_SIGNAL);
		return (evsel->del(evbase, ev));
	}

	return (0);
}

void
event_active(struct event *ev, int res, short ncalls)
{
	/* We get different kinds of events, add them together */
	if (ev->ev_flags & EVLIST_ACTIVE) {
		ev->ev_res |= res;
		return;
	}

	ev->ev_res = res;
	ev->ev_ncalls = ncalls;
	ev->ev_pncalls = NULL;
	event_queue_insert(ev->ev_base, ev, EVLIST_ACTIVE);
}

int
timeout_next(struct event_base *base, struct timeval *tv)
{
	struct timeval dflt = TIMEOUT_DEFAULT;

	struct timeval now;
	struct event *ev;

	if ((ev = RB_MIN(event_tree, &base->timetree)) == NULL) {
		*tv = dflt;
		return (0);
	}

	if (gettime(&now) == -1)
		return (-1);

	if (timercmp(&ev->ev_timeout, &now, <=)) {
		timerclear(tv);
		return (0);
	}

	timersub(&ev->ev_timeout, &now, tv);

	assert(tv->tv_sec >= 0);
	assert(tv->tv_usec >= 0);

	event_debug(("timeout_next: in %d seconds", tv->tv_sec));
	return (0);
}

static void
timeout_correct(struct event_base *base, struct timeval *off)
{
	struct event *ev;

	/*
	 * We can modify the key element of the node without destroying
	 * the key, beause we apply it to all in the right order.
	 */
	RB_FOREACH(ev, event_tree, &base->timetree)
		timersub(&ev->ev_timeout, off, &ev->ev_timeout);
}

/*
 * 处理超时事件
 */
void
timeout_process(struct event_base *base)
{
	struct timeval now;
	struct event *ev, *next;

	gettime(&now);

	// 从红黑树中取得最快超时的事件
	for (ev = RB_MIN(event_tree, &base->timetree); ev; ev = next) {
		if (timercmp(&ev->ev_timeout, &now, >))
			break;
		next = RB_NEXT(event_tree, &base->timetree, ev);

		event_queue_remove(base, ev, EVLIST_TIMEOUT);

		/* delete this event from the I/O queues */
		event_del(ev);

		event_debug(("timeout_process: call %p", ev->ev_callback));

		event_active(ev, EV_TIMEOUT, 1); // 把事件添加到活动队列中
	}
}

void
event_queue_remove(struct event_base *base, struct event *ev, int queue)
{
	int docount = 1;

	if (!(ev->ev_flags & queue))
		event_errx(1, "%s: %p(fd %d) not on queue %x", __func__,
			   ev, ev->ev_fd, queue);

	if (ev->ev_flags & EVLIST_INTERNAL)
		docount = 0;

	if (docount)
		base->event_count--;

	ev->ev_flags &= ~queue;

	switch (queue) {
	case EVLIST_ACTIVE:
		if (docount)
			base->event_count_active--;
		TAILQ_REMOVE(base->activequeues[ev->ev_pri],
		    ev, ev_active_next);
		break;

	case EVLIST_SIGNAL:
		TAILQ_REMOVE(&signalqueue, ev, ev_signal_next);
		break;

	case EVLIST_TIMEOUT:
		RB_REMOVE(event_tree, &base->timetree, ev);
		break;

	case EVLIST_INSERTED:
		TAILQ_REMOVE(&base->eventqueue, ev, ev_next);
		break;

	default:
		event_errx(1, "%s: unknown queue %x", __func__, queue);
	}
}

/*
 * 把事件对象添加到相应的队列中
 */
void
event_queue_insert(struct event_base *base, struct event *ev, int queue)
{
	int docount = 1;

	if (ev->ev_flags & queue) {
		/* Double insertion is possible for active events */
		if (queue & EVLIST_ACTIVE)
			return;

		event_errx(1, "%s: %p(fd %d) already on queue %x", __func__,
			   ev, ev->ev_fd, queue);
	}

	if (ev->ev_flags & EVLIST_INTERNAL)
		docount = 0;

	if (docount)
		base->event_count++;

	ev->ev_flags |= queue; // 添加旗标位

	switch (queue) {
	case EVLIST_ACTIVE: // 添加到活动列表
		if (docount)
			base->event_count_active++;
		TAILQ_INSERT_TAIL(base->activequeues[ev->ev_pri],
		    ev,ev_active_next);
		break;

	case EVLIST_SIGNAL: // 添加信号列表
		TAILQ_INSERT_TAIL(&signalqueue, ev, ev_signal_next);
		break;

	case EVLIST_TIMEOUT: { // 添加到timeout列表
		struct event *tmp = RB_INSERT(event_tree, &base->timetree, ev);
		assert(tmp == NULL);
		break;
	}

	case EVLIST_INSERTED: // 添加到IO列表
		TAILQ_INSERT_TAIL(&base->eventqueue, ev, ev_next);
		break;

	default:
		event_errx(1, "%s: unknown queue %x", __func__, queue);
	}
}

/* Functions for debugging */

const char *
event_get_version(void)
{
	return (VERSION);
}

/* 
 * No thread-safe interface needed - the information should be the same
 * for all threads.
 */

const char *
event_get_method(void)
{
	return (current_base->evsel->name);
}
