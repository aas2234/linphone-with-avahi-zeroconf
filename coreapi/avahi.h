/*
linphone
Copyright (C) 2010  Abhishek Srivastava (aas2234@columbia.edu ; a.srivastava.800@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef _AVAHI_H
#define _AVAHI_H

#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <avahi-common/alternative.h>
#include <avahi-common/timeval.h>
#include <avahi-client/client.h>
#include <avahi-common/error.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-client/publish.h>
#include <glib.h>
#include <avahi-common/thread-watch.h>
#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>

#include <linphonecore.h>
#include <linphonefriend.h>
#include <intl/hash-string.h>

extern pthread_t avahi_pub_thread, avahi_sb_thread;

pthread_rwlock_t lc_lock;

struct pubContext {
	
	AvahiClient *client;
	char *name;
	AvahiThreadedPoll *thr_poll;
	AvahiEntryGroup *group;
	LinphoneCore *lcore;
};

struct browserContext {
	AvahiClient *client;
	LinphoneCore *lcore;
	AvahiThreadedPoll *thr_poll;
	AvahiServiceBrowser *browser;	
};

extern struct pubContext *pubctx;
extern struct browserContext *brctx;

void* avahi_sb_main(void *);
void* avahi_pub_main(void *);

void avahi_thr_poll_pub_main(LinphoneCore*);
void avahi_thr_poll_pub_stop(void);
int unregister_pub_services(struct pubContext*);

void avahi_thr_poll_browser_main(LinphoneCore*);
void avahi_thr_poll_browser_stop(void);
void avahi_add_service (AvahiClient *c);
int unregister_br_services(struct browserContext*);

int start_avahi(LinphoneCore *lc);
int stop_avahi(LinphoneCore *lc);

#endif