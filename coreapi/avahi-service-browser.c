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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "avahi.h"

struct browserContext *brctx = NULL;

static void resolve_callback(
    AvahiServiceResolver *r,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    AVAHI_GCC_UNUSED void* userdata) {

// 	char *presence=NULL;
    assert(r);
	
    /* Called whenever a service has been resolved successfully or timed out */

    switch (event) {
        case AVAHI_RESOLVER_FAILURE:
            fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
            break;

        case AVAHI_RESOLVER_FOUND: {
            char a[AVAHI_ADDRESS_STR_MAX], *t;
            LinphoneFriend *lf = NULL;
            fprintf(stderr, "Service '%s' of type '%s' in domain '%s':\n", name, type, domain);
	    
	    /* Filter out seeing ourself */
	    if (flags & AVAHI_LOOKUP_RESULT_LOCAL) {
		  avahi_service_resolver_free (r);
		  return;
	    }
	    
            avahi_address_snprint(a, sizeof(a), address);
            t = avahi_string_list_to_string(txt);
            fprintf(stderr,
                    "\t%s:%u (%s)\n"
                    "\tTXT=%s\n",
                    host_name, port, a,t);

			lf = linphone_friend_new_zeroconf_with_uri(name);
			if(lf!=NULL)
				linphone_core_add_zeroconf_friend(brctx->lcore,lf);	// Need to add the friend to Users Nearby GTK List
			avahi_free(t);
        }
    }

    avahi_service_resolver_free(r);
}

static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    void* userdata) {

    AvahiClient *c = userdata;
	LinphoneFriend *lf = NULL;
    assert(b);

    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */

    switch (event) {
        case AVAHI_BROWSER_FAILURE:

            fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
			unregister_br_services(brctx);
            return;

        case AVAHI_BROWSER_NEW:
            fprintf(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);

            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */

            if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, c)))
                fprintf(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));

            break;

        case AVAHI_BROWSER_REMOVE:
			lf=linphone_core_get_zeroconf_friend_by_address(brctx->lcore,name);
			fprintf(stderr, "\n name = %s",name);
			if(lf!=NULL)
				linphone_core_remove_zeroconf_friend(brctx->lcore,lf);
			linphone_core_zeroconf_refresh(brctx->lcore);
            fprintf(stderr, "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
            break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            break;
    }
}

static void
avahi_client_callback (AVAHI_GCC_UNUSED AvahiClient *client, AvahiClientState state, void *userdata)
{

    if (state == AVAHI_CLIENT_FAILURE)
    {
        /* We we're disconnected from the Daemon */
        g_message ("Disconnected from the Avahi Daemon: %s", avahi_strerror(avahi_client_errno(client)));

        /* Quit the application */
        unregister_br_services(brctx);
    }
}


void avahi_thr_poll_browser_main(LinphoneCore *lcore)
{

	int error;
	brctx = calloc(1,sizeof(struct browserContext));	
	brctx->client = NULL;
	brctx->lcore = lcore;
	brctx->browser = NULL;
	brctx->thr_poll = NULL;
    brctx->lcore = lcore;


        /* Create a new threaded poll */
    if (!(brctx->thr_poll = avahi_threaded_poll_new())) {
     goto fail;
   }


    /* Allocate a new client */
       brctx->client = avahi_client_new (avahi_threaded_poll_get(brctx->thr_poll),            /* AvahiPoll object from above */
                               AVAHI_CLIENT_NO_FAIL,
            avahi_client_callback,                  /* Callback function for Client state changes */
            NULL,                                   /* User data */
            &error);                                /* Error return */


    /* Check wether creating the client object succeeded */
    if (!brctx->client) {
        fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
        goto fail;
    }

    /* Create the service browser */
    if (!(brctx->browser = avahi_service_browser_new(brctx->client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_sipuri._udp", NULL, 0, browse_callback, brctx->client))) {
        fprintf(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(brctx->client)));
        goto fail;
    }

    /* Finally, start the event loop thread */
   if (avahi_threaded_poll_start(brctx->thr_poll) < 0) {
     goto fail;
   }
   return;
 
fail:

	unregister_br_services(brctx);

}

void avahi_thr_poll_browser_stop()
{
	unregister_br_services(brctx);
}

int unregister_br_services(struct browserContext* brctx)
{
	if(brctx->thr_poll)
		avahi_threaded_poll_stop(brctx->thr_poll);
	if(brctx->browser)
		avahi_service_browser_free(brctx->browser);

	avahi_client_free(brctx->client);
	if(brctx->thr_poll)
		avahi_threaded_poll_free(brctx->thr_poll);
	
	return 0;
}