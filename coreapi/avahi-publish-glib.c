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

struct pubContext *pubctx = NULL;

static void avahi_entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {

    assert(g == pubctx->group || pubctx->group == NULL);
    pubctx->group = g;

    /* Called whenever the entry group state changes */

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            fprintf(stderr, "Service '%s' successfully established.\n", pubctx->name);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name(pubctx->name);
            avahi_free(pubctx->name);
            pubctx->name = n;

            fprintf(stderr, "Service name collision, renaming service to '%s'\n", pubctx->name);

            /* And recreate the services */
            avahi_add_service(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            fprintf(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

			/* Some kind of failure happened while we were registering our services */
			unregister_pub_services(pubctx);
			break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}


// void update_presence(void) {
// 	char txt[128];
// 	if(pubctx->lcore){
// 		snprintf(txt,sizeof(txt),"presence=%d",linphone_core_get_presence_info(pubctx->lcore));
// 		if(pubctx->group) 
// 			avahi_entry_group_update_service_txt(pubctx->group,AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,0, pubctx->name, "_sipuri._udp", NULL, NULL, 651,txt, NULL);
// 	}	
// }

void avahi_add_service (AvahiClient *c) {
        
	char *n;
	int ret;
	assert(c);
// 	char txt[128];
// 	snprintf(txt,sizeof(txt),"presence=%i",(int)linphone_core_get_presence_info(pubctx->lcore));

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!pubctx->group)
        if (!(pubctx->group = avahi_entry_group_new(c, avahi_entry_group_callback, NULL))) {
            fprintf(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(pubctx->group)) {
        fprintf(stderr, "Adding service '%s'\n", pubctx->name);


        /* Add the service for SIPURI*/
        if ((ret = avahi_entry_group_add_service(pubctx->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,0, pubctx->name, "_sipuri._udp", NULL, NULL, 651,NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            fprintf(stderr, "Failed to add _sipuri._udp service: %s\n", avahi_strerror(ret));
            goto fail;
        }


        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(pubctx->group)) < 0) {
            fprintf(stderr, "Failed to commit entry group: %s\n", avahi_strerror(ret));
            goto fail;
        }
    }

    return;

collision:

    /* A service name collision with a local service happened. Let's
     * pick a new name */
    n = avahi_alternative_service_name(pubctx->name);
    avahi_free(pubctx->name);
    pubctx->name = n;

    fprintf(stderr, "Service name collision, renaming service to '%s'\n", pubctx->name);
    avahi_entry_group_reset(pubctx->group);
    avahi_add_service(c);

	return;

fail:
	unregister_pub_services(pubctx);
}


/* Callback for state changes on the Client */
static void
avahi_client_callback (AVAHI_GCC_UNUSED AvahiClient *client, AvahiClientState state, void *userdata)
{

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:

            /* The server has startup successfully and registered its host
             * name on the network, so it's time to create our services */
            avahi_add_service(client);
            break;

        case AVAHI_CLIENT_FAILURE:

            fprintf(stderr, "\nClient failure: %s", avahi_strerror(avahi_client_errno(client)));
	        unregister_pub_services(pubctx);
            break;

        case AVAHI_CLIENT_S_COLLISION:

            /* Let's drop our registered services. When the server is back
             * in AVAHI_SERVER_RUNNING state we will register them
             * again with the new host name. */
			
        case AVAHI_CLIENT_S_REGISTERING:

            /* The server records are now being established. This
             * might be caused by a host name change. We need to wait
             * for our own records to register until the host name is
             * properly esatblished. */

            if (pubctx->group)
                avahi_entry_group_reset(pubctx->group);

            break;

        case AVAHI_CLIENT_CONNECTING:
            ;
    }

    if (state == AVAHI_CLIENT_FAILURE)
    {
        /* We we're disconnected from the Daemon */
        g_message ("Disconnected from the Avahi Daemon: %s", avahi_strerror(avahi_client_errno(client)));

        /* Quit the application */
        unregister_pub_services(pubctx);
    }
}

const char * get_self_uri(LinphoneCore *lc) {
 
   return linphone_core_get_primary_contact(lc);
 //  snprintf(key,sizeof(key),"uri%i",i);
 //  uri = linphone_gtk_get_ui_config(key,NULL);
}

void avahi_thr_poll_pub_main(LinphoneCore *lc)
{
	int error;
	
	// Initialize context
   pubctx = calloc(1,sizeof(struct pubContext));	
   pubctx->client = NULL;
   pubctx->group = NULL;
   pubctx->thr_poll = NULL;
   pubctx->lcore = lc; 	
   pubctx->name = avahi_strdup (get_self_uri ( (LinphoneCore*)lc) );

   //create a new threaded poll
   if (!(pubctx->thr_poll = avahi_threaded_poll_new())) {
     goto fail;
   }

   if (!(pubctx->client = avahi_client_new (avahi_threaded_poll_get(pubctx->thr_poll),       /* AvahiPoll object from above */
                               AVAHI_CLIENT_NO_FAIL,
            avahi_client_callback,					                  /* Callback function for Client state changes */
            NULL,                               				  /* User data */
            &error)))                               				 /* Error return */
   {
     goto fail;
   }

   /* Finally, start the event loop thread */
   if (avahi_threaded_poll_start(pubctx->thr_poll) < 0) {
     goto fail;
   }
   return;
   
fail:
    /* Clean up */

	unregister_pub_services(pubctx);
}

int unregister_pub_services(struct pubContext* pubctx)
{
	if(pubctx->thr_poll)
		avahi_threaded_poll_stop(pubctx->thr_poll);
	avahi_client_free(pubctx->client);
	if(pubctx->thr_poll)
		avahi_threaded_poll_free(pubctx->thr_poll);
	avahi_free(pubctx->name);
	return 0;
}

void avahi_thr_poll_pub_stop()
{
	unregister_pub_services(pubctx);
}
