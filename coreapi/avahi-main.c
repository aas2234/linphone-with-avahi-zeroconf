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

#include "avahi.h"

/**
 * Start Avahi Service Browsing and Publishing Threads
 * 
**/

int start_avahi(LinphoneCore *lc)
{	
	/* Start the publishing and browsing modules as avahi threads*/
	avahi_thr_poll_pub_main(lc);	
	avahi_thr_poll_browser_main(lc);
	return TRUE;
}


/**
 * Stop Avahi Service Browsing and Publishing Threads
 * 
**/
int stop_avahi(LinphoneCore *lc)
{
	avahi_thr_poll_browser_stop();
	avahi_thr_poll_pub_stop();
	linphone_core_remove_zeroconf_friends(lc); /* Clear list of zeroconf_friends in lc->zeroconf_friends */
	linphone_core_zeroconf_refresh(lc);
	return TRUE;
}
