/* For terms of usage/redistribution/modification see the LICENSE file */
/* For authors and contributors see the AUTHORS file */

/***

options.c - implements the configuration section of the utility

***/

#include "iptraf-ng-compat.h"

#include "serv.h"
#include "options.h"
#include "deskman.h"
#include "attrs.h"
#include "landesc.h"
#include "promisc.h"
#include "dirs.h"
#include "instances.h"

#define ALLOW_ZERO 1
#define DONT_ALLOW_ZERO 0

static void makeoptionmenu(struct MENU *menu)
{
	tx_initmenu(menu, 20, 40, (LINES - 19) / 2 - 1, (COLS - 40) / 16,
		    BOXATTR, STDATTR, HIGHATTR, BARSTDATTR, BARHIGHATTR,
		    DESCATTR);
	tx_additem(menu, " ^R^everse DNS lookups",
		   "Toggles resolution of IP addresses into host names");
	tx_additem(menu, " TCP/UDP ^s^ervice names",
		   "Displays TCP/UDP service names instead of numeric ports");
	tx_additem(menu, " Force ^p^romiscuous mode",
		   "Toggles capture of all packets by LAN interfaces");
	tx_additem(menu, " ^C^olor",
		   "Turns color on or off (restart IPTraf to effect change)");
	tx_additem(menu, " ^L^ogging",
		   "Toggles logging of traffic to a data file");
	tx_additem(menu, " Acti^v^ity mode",
		   "Toggles activity indicators between kbits/s and kbytes/s");
	tx_additem(menu, " Source ^M^AC addrs in traffic monitor",
		   "Toggles display of source MAC addresses in the IP Traffic Monitor");
	tx_additem(menu, " ^S^how v6-in-v4 traffic as IPv6",
		   "Toggled display of IPv6 tunnel in IPv4 as IPv6 traffic");
	tx_additem(menu, NULL, NULL);
	tx_additem(menu, " ^T^imers...", "Configures timeouts and intervals");
	tx_additem(menu, NULL, NULL);
	tx_additem(menu, " ^A^dditional ports...",
		   "Allows you to add port numbers higher than 1023 for the service stats");
	tx_additem(menu, " ^D^elete port/range...",
		   "Deletes a port or range of ports earlier added");
	tx_additem(menu, NULL, NULL);
	tx_additem(menu, " ^E^thernet/PLIP host descriptions...",
		   "Manages descriptions for Ethernet and PLIP addresses");
	tx_additem(menu, " ^F^DDI/Token Ring host descriptions...",
		   "Manages descriptions for FDDI and FDDI addresses");
	tx_additem(menu, NULL, NULL);
	tx_additem(menu, " E^x^it configuration", "Returns to main menu");
}

static void maketimermenu(struct MENU *menu)
{
	tx_initmenu(menu, 8, 35, (LINES - 19) / 2 + 7, (COLS - 35) / 2, BOXATTR,
		    STDATTR, HIGHATTR, BARSTDATTR, BARHIGHATTR, DESCATTR);

	tx_additem(menu, " TCP ^t^imeout...",
		   "Sets the length of time before inactive TCP entries are considered idle");
	tx_additem(menu, " ^L^ogging interval...",
		   "Sets the time between loggings for interface, host, and service stats");
	tx_additem(menu, " ^S^creen update interval...",
		   "Sets the screen update interval in seconds (set to 0 for fastest updates)");
	tx_additem(menu, " TCP closed/idle ^p^ersistence...",
		   "Determines how long closed/idle/reset entries stay onscreen");
	tx_additem(menu, NULL, NULL);
	tx_additem(menu, " E^x^it menu", "Returns to the configuration menu");
}

static void printoptonoff(unsigned int option, WINDOW * win)
{
	if (option)
		wprintw(win, " On");
	else
		wprintw(win, "Off");
}

static void indicatesetting(int row, struct OPTIONS *options, WINDOW * win)
{
	wmove(win, row, 30);
	wattrset(win, HIGHATTR);

	switch (row) {
	case 1:
		printoptonoff(options->revlook, win);
		break;
	case 2:
		printoptonoff(options->servnames, win);
		break;
	case 3:
		printoptonoff(options->promisc, win);
		break;
	case 4:
		printoptonoff(options->color, win);
		break;
	case 5:
		printoptonoff(options->logging, win);
		break;
	case 6:
		wmove(win, row, 25);
		if (options->actmode == KBITS)
			wprintw(win, " kbits/s");
		else
			wprintw(win, "kbytes/s");
		break;
	case 7:
		printoptonoff(options->mac, win);
		break;
	case 8:
		printoptonoff(options->v6inv4asv6, win);
	}

}

void saveoptions(struct OPTIONS *options)
{
	int fd;
	int bw;

	fd = open(CONFIGFILE, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

	if (fd < 0) {
		tui_error(ANYKEY_MSG, "Cannot create config file: %s %s",
			  CONFIGFILE, strerror(errno));
		return;
	}
	bw = write(fd, options, sizeof(struct OPTIONS));

	if (bw < 0)
		tui_error(ANYKEY_MSG, "Unable to write config file");

	close(fd);
}

static void setdefaultopts(struct OPTIONS *options)
{
	options->revlook = 0;
	options->promisc = 0;
	options->servnames = 0;
	options->color = 1;
	options->logging = 0;
	options->actmode = KBITS;
	options->mac = 0;
	options->timeout = 15;
	options->logspan = 3600;
	options->updrate = 0;
	options->closedint = 0;
	options->v6inv4asv6 = 1;
}

void loadoptions(struct OPTIONS *options)
{
	int fd;

	setdefaultopts(options);
	fd = open(CONFIGFILE, O_RDONLY);

	if (fd < 0)
		return;

	read(fd, options, sizeof(struct OPTIONS));

	close(fd);
}

static void updatetimes(struct OPTIONS *options, WINDOW *win)
{
	wattrset(win, HIGHATTR);
	mvwprintw(win, 10, 25, "%3u mins", options->timeout);
	mvwprintw(win, 11, 25, "%3u mins", options->logspan / 60);
	mvwprintw(win, 12, 25, "%3u secs", options->updrate);
	mvwprintw(win, 13, 25, "%3u mins", options->closedint);
}

static void showoptions(struct OPTIONS *options, WINDOW *win)
{
	int i;

	for (i = 1; i <= 8; i++)
		indicatesetting(i, options, win);

	updatetimes(options, win);
}

static void settimeout(time_t *value, const char *units, int allow_zero,
		       int *aborted)
{
	WINDOW *dlgwin;
	PANEL *dlgpanel;
	struct FIELDLIST field;
	time_t tmval = 0;

	dlgwin = newwin(7, 40, (LINES - 7) / 2, (COLS - 40) / 4);
	dlgpanel = new_panel(dlgwin);

	wattrset(dlgwin, DLGBOXATTR);
	tx_colorwin(dlgwin);
	tx_box(dlgwin, ACS_VLINE, ACS_HLINE);

	wattrset(dlgwin, DLGTEXTATTR);
	wmove(dlgwin, 2, 2);
	wprintw(dlgwin, "Enter value in %s", units);
	wmove(dlgwin, 5, 2);
	stdkeyhelp(dlgwin);

	tx_initfields(&field, 1, 10, (LINES - 7) / 2 + 3, (COLS - 40) / 4 + 2,
		      DLGTEXTATTR, FIELDATTR);
	tx_addfield(&field, 3, 0, 0, "");

	do {
		tx_fillfields(&field, aborted);

		if (!(*aborted)) {
			unsigned int tm;

			tmval = 0;
			int ret = strtoul_ui(field.list->buf, 10, &tm);
			if ((ret == -1) || (!allow_zero && (tm == 0)))
				tui_error(ANYKEY_MSG, "Invalid timeout value");
			else
				tmval = tm;
		}
	} while (((!allow_zero) && (tmval == 0)) && (!(*aborted)));

	if (!(*aborted))
		*value = tmval;

	del_panel(dlgpanel);
	delwin(dlgwin);

	tx_destroyfields(&field);
	update_panels();
	doupdate();
}

void setoptions(struct OPTIONS *options, struct porttab **ports)
{
	int row = 1;
	int trow = 1;		/* row for timer submenu */
	int aborted;

	struct MENU menu;
	struct MENU timermenu;

	WINDOW *statwin;
	PANEL *statpanel;

	if (!is_first_instance) {
		tui_error(ANYKEY_MSG,
			  "Only the first instance of ipraf-ng"
			  " can configure");
		return;
	}
	makeoptionmenu(&menu);

	statwin = newwin(15, 35, (LINES - 19) / 2 - 1, (COLS - 40) / 16 + 40);
	statpanel = new_panel(statwin);

	wattrset(statwin, BOXATTR);
	tx_colorwin(statwin);
	tx_box(statwin, ACS_VLINE, ACS_HLINE);
	wmove(statwin, 9, 1);
	whline(statwin, ACS_HLINE, 33);
	mvwprintw(statwin, 0, 1, " Current Settings ");
	wattrset(statwin, STDATTR);
	mvwprintw(statwin, 1, 2, "Reverse DNS lookups:");
	mvwprintw(statwin, 2, 2, "Service names:");
	mvwprintw(statwin, 3, 2, "Promiscuous:");
	mvwprintw(statwin, 4, 2, "Color:");
	mvwprintw(statwin, 5, 2, "Logging:");
	mvwprintw(statwin, 6, 2, "Activity mode:");
	mvwprintw(statwin, 7, 2, "MAC addresses:");
	mvwprintw(statwin, 8, 2, "v6-in-v4 as IPv6:");
	mvwprintw(statwin, 10, 2, "TCP timeout:");
	mvwprintw(statwin, 11, 2, "Log interval:");
	mvwprintw(statwin, 12, 2, "Update interval:");
	mvwprintw(statwin, 13, 2, "Closed/idle persist:");
	showoptions(options, statwin);

	do {
		tx_showmenu(&menu);
		tx_operatemenu(&menu, &row, &aborted);

		switch (row) {
		case 1:
			options->revlook = ~(options->revlook);
			break;
		case 2:
			options->servnames = ~(options->servnames);
			break;
		case 3:
			options->promisc = ~(options->promisc);
			break;
		case 4:
			options->color = ~(options->color);
			break;
		case 5:
			options->logging = ~(options->logging);
			break;
		case 6:
			options->actmode = ~(options->actmode);
			break;
		case 7:
			options->mac = ~(options->mac);
			break;
		case 8:
			options->v6inv4asv6 = ~(options->v6inv4asv6);
			break;
		case 10:
			maketimermenu(&timermenu);
			trow = 1;
			do {
				tx_showmenu(&timermenu);
				tx_operatemenu(&timermenu, &trow, &aborted);

				switch (trow) {
				case 1:
					settimeout(&(options->timeout),
						   "minutes", DONT_ALLOW_ZERO,
						   &aborted);
					if (!aborted)
						updatetimes(options, statwin);
					break;
				case 2:
					settimeout(&(options->logspan),
						   "minutes", DONT_ALLOW_ZERO,
						   &aborted);
					if (!aborted) {
						options->logspan =
						    options->logspan * 60;
						updatetimes(options, statwin);
					}
					break;
				case 3:
					settimeout(&options->updrate, "seconds",
						   ALLOW_ZERO, &aborted);
					if (!aborted)
						updatetimes(options, statwin);
					break;
				case 4:
					settimeout(&options->closedint,
						   "minutes", ALLOW_ZERO,
						   &aborted);
					if (!aborted)
						updatetimes(options, statwin);
					break;
				}
			} while (trow != 6);

			tx_destroymenu(&timermenu);
			update_panels();
			doupdate();
			break;
		case 12:
			addmoreports(ports);
			break;
		case 13:
			removeaport(ports);
			break;
		case 15:
			manage_eth_desc(ARPHRD_ETHER);
			break;
		case 16:
			manage_eth_desc(ARPHRD_FDDI);
			break;
		}

		indicatesetting(row, options, statwin);
	} while (row != 18);

	tx_destroymenu(&menu);
	del_panel(statpanel);
	delwin(statwin);
	update_panels();
	doupdate();
}
