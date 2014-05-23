// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

MULTIPLAYER MENU (SERVER BROWSER)

=======================================================================
*/


#include "ui_local.h"


#define MAX_GLOBALSERVERS		2048
#define MAX_PINGREQUESTS		16
#define MAX_ADDRESSLENGTH		64
#define MAX_HOSTNAMELENGTH		32
#define MAX_MAPNAMELENGTH		16
#define MAX_LISTBOXITEMS		2048
#define MAX_LOCALSERVERS		2048
#define MAX_STATUSLENGTH		64
#define MAX_LEAGUELENGTH		28
#define MAX_LISTBOXWIDTH		66

#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"
#define ART_SPECIFY0			"menu/art/specify_0"
#define ART_SPECIFY1			"menu/art/specify_1"
#define ART_REFRESH0			"menu/art/refresh_0"
#define ART_REFRESH1			"menu/art/refresh_1"
#define ART_CONNECT0			"menu/art/fight_0"
#define ART_CONNECT1			"menu/art/fight_1"
#define ART_ARROWS0				"menu/art/arrows_vert_0"
#define ART_ARROWS_UP			"menu/art/arrows_vert_top"
#define ART_ARROWS_DOWN			"menu/art/arrows_vert_bot"
#define ART_UNKNOWNMAP			"menu/art/unknownmap"
#define ART_INFO0				"menu/art/info_0"
#define ART_INFO1				"menu/art/info_1"
#define ART_REMOVE0				"menu/art/delete_0"
#define ART_REMOVE1				"menu/art/delete_1"
#define ART_ADD0				"menu/art/favorite_0"
#define ART_ADD1				"menu/art/favorite_1"
#define GLOBALRANKINGS_LOGO		"menu/art/gr/grlogo"

#define ID_MASTER			10
#define ID_GAMETYPE			11
#define ID_SORTKEY			12
#define ID_SHOW_FULL		13
#define ID_SHOW_EMPTY		14
#define ID_SHOW_AWONLY		15
#define ID_LIST				16
#define ID_SCROLL_UP		17
#define ID_SCROLL_DOWN		18
#define ID_BACK				19
#define ID_REFRESH			20
#define ID_SPECIFY			21
#define ID_CONNECT			22
#define ID_INFO				23
#define ID_FAVORITE			24

#define GR_LOGO				30
#define GR_LETTERS			31

#define AS_LOCAL			0
#define AS_MPLAYER			1
#define AS_GLOBAL			2
#define AS_FAVORITES		3

#define SORT_HOST			0
#define SORT_MAP			1
#define SORT_CLIENTS		2
#define SORT_GAME			3
#define SORT_PING			4

#define GAMES_ALL			0
#define GAMES_DM			1
#define GAMES_TEAMPLAY		2
#define GAMES_TACTICAL		3

static const char *master_items[] = {
	"Local",
	"Mplayer",
	"Internet",
	"Favorites",
	0
};

static const char *servertype_items[] = {
	"All",
	"Tactical",
	"Deathmatch",
	0
};

static const char *sortkey_items[] = {
	"Server Name",
	"Map Name",
	"Open Player Spots",
	"Game Type",
	"Ping Time",
	0
};

static char* gamenames[GT_MAX_GAME_TYPE+1] = {
	"TC",	// tactical
	"DM",	// deathmatch
	"--"
};

static char* netnames[] = {
	"???",
	"UDP",
	"IPX",
	NULL
};

static char quake3worldMessage[] = "Visit our page at www.planetquake.com/afterwards";
static char globalRankingsMessage[] = "Visit our page at www.planetquake.com/afterwards";

typedef struct {
	char	adrstr[MAX_ADDRESSLENGTH];
	int		start;
} pinglist_t;

typedef struct servernode_s {
	char	adrstr[MAX_ADDRESSLENGTH];
	char	hostname[MAX_HOSTNAMELENGTH];
	char	mapname[MAX_MAPNAMELENGTH];
	int		numclients;
	int		maxclients;
	int		pingtime;
	int		gametype;
	char	gamename[12];
	int		nettype;
	int		minPing;
	int		maxPing;

} servernode_t; 

#define MAX_DISPLAY_SERVERS		2048

struct serverStatus_s {
	pinglist_t pingList[MAX_PINGREQUESTS];
	int		numqueriedservers;
	int		currentping;
	int		nextpingtime;
	int		maxservers;
	int		refreshtime;
	int		numServers;
//	int		sortKey;
	int		sortDir;
	int		lastCount;
	qboolean refreshActive;
	qboolean newDisplayList;
	int		currentServer;
	int		displayServers[MAX_DISPLAY_SERVERS];
	int		numRefreshedServers;
	int		numDisplayServers;
	int		numPlayersOnServers;
	int		nextDisplayRefresh;
	int		nextSortTime;
//	qhandle_t currentServerPreview;
//	int		currentServerCinematic;
//	int		motdLen;
//	int		motdWidth;
//	int		motdPaintX;
//	int		motdPaintX2;
//	int		motdOffset;
//	int		motdTime;
//	char	motd[MAX_STRING_CHARS];
} serverStatus;


typedef struct {
	char			buff[MAX_LISTBOXWIDTH+8];
	servernode_t*	servernode;
} table_t;

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;

	menulist_s			master;
	menulist_s			gametype;
	menulist_s			sortkey;
	menuradiobutton_s	showfull;
	menuradiobutton_s	showempty;
	menuradiobutton_s	showawonly;

	menulist_s			list;
	menubitmap_s		mappic;
	menubitmap_s		arrows;
	menubitmap_s		up;
	menubitmap_s		down;
	menutext_s			status;
	menutext_s			statusbar;

	menubitmap_s		info;
	menubitmap_s		favorite;
	menubitmap_s		back;
	menubitmap_s		refresh;
	menubitmap_s		specify;
	menubitmap_s		go;

	pinglist_t			pinglist[MAX_PINGREQUESTS];
	table_t				table[MAX_LISTBOXITEMS];
	char*				items[MAX_LISTBOXITEMS];
	int					numqueriedservers;
	int					*numservers;
	servernode_t		*serverlist;	
	int					currentping;
	int					nextpingtime;
	int					maxservers;
	int					refreshtime;
	char				favoriteaddresses[MAX_FAVORITESERVERS][MAX_ADDRESSLENGTH];
	int					numfavoriteaddresses;


	menubitmap_s	grlogo;
	menubitmap_s	grletters;
	menutext_s		league;
	menutext_s		practice;

} arenaservers_t;

static arenaservers_t	g_arenaservers;


static servernode_t		g_globalserverlist[MAX_GLOBALSERVERS];
static int				g_numglobalservers;
static servernode_t		g_localserverlist[MAX_LOCALSERVERS];
static int				g_numlocalservers;
static servernode_t		g_favoriteserverlist[MAX_FAVORITESERVERS];
static int				g_numfavoriteservers;
static servernode_t		g_mplayerserverlist[MAX_GLOBALSERVERS];
static int				g_nummplayerservers;
static int				g_servertype;
static int				g_gametype;
static int				g_sortkey;
static int				g_emptyservers;
static int				g_fullservers;


static void ArenaServers_UpdateMenu();


/*
==================
UI_UpdatePendingPings
==================
*/
static void UI_UpdatePendingPings() { 
	trap_LAN_ResetPings(g_arenaservers.master.curvalue);
	serverStatus.refreshActive = qtrue;
	serverStatus.refreshtime = uis.realtime + 500;

}


/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList(int num, int position) {
	int i;

	if (position < 0 || position > serverStatus.numDisplayServers ) {
		return;
	}
	//
	serverStatus.numDisplayServers++;
	for (i = serverStatus.numDisplayServers; i > position; i--) {
		serverStatus.displayServers[i] = serverStatus.displayServers[i-1];
	}
	serverStatus.displayServers[position] = num;
}


/*
==================
UI_RemoveServerFromDisplayList
==================
*/
static void UI_RemoveServerFromDisplayList(int num) {
	int i, j;

	for (i = 0; i < serverStatus.numDisplayServers; i++) {
		if (serverStatus.displayServers[i] == num) {
			serverStatus.numDisplayServers--;
			for (j = i; j < serverStatus.numDisplayServers; j++) {
				serverStatus.displayServers[j] = serverStatus.displayServers[j+1];
			}
			return;
		}
	}
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion(int num) {
	int mid, offset, res, len;

	// use binary search to insert server
	len = serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;
	while(mid > 0) {
		mid = len >> 1;
		//
		res = trap_LAN_CompareServers( g_arenaservers.master.curvalue, g_sortkey,
					serverStatus.sortDir, num, serverStatus.displayServers[offset+mid]);
		// if equal
		if (res == 0) {
			UI_InsertServerIntoDisplayList(num, offset+mid);
			return;
		}
		// if larger
		else if (res == 1) {
			offset += mid;
			len -= mid;
		}
		// if smaller
		else {
			len -= mid;
		}
	}
	if (res == 1) {
		offset++;
	}
	UI_InsertServerIntoDisplayList(num, offset);
}

/*
==================
ArenaServers_BuildDisplayList
==================
*/
static void ArenaServers_BuildDisplayList(qboolean force) {
	int i, count, clients, maxClients, ping, game, visible;
	char info[MAX_STRING_CHARS];
	qboolean startRefresh = qtrue;
	static int numinvisible;

/*	if (!(force || uis.realtime > serverStatus.nextDisplayRefresh)) {
		return;
	}
	// if we shouldn't reset
	if ( force == 2 ) {
		force = 0;
	}*/

	if (force) {
		numinvisible = 0;
		// clear number of displayed servers
		serverStatus.numDisplayServers = 0;
		serverStatus.numRefreshedServers = 0;
		serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		//Menu_SetFeederSelection(NULL, FEEDER_SERVERS, 0, NULL);
		g_arenaservers.list.numitems = 0;
		g_arenaservers.list.curvalue = 0;
		g_arenaservers.list.top      = 0;
		// mark all servers as visible so we store ping updates for them
		trap_LAN_MarkServerVisible(g_arenaservers.master.curvalue, -1, qtrue);
	}

	// get the server count (comes from the master)
	count = trap_LAN_GetServerCount(g_arenaservers.master.curvalue);
	if (count == -1 || (g_arenaservers.master.curvalue == AS_LOCAL && count == 0) ) {
		// still waiting on a response from the master
		serverStatus.numDisplayServers = 0;
		serverStatus.numRefreshedServers = 0;
		serverStatus.numPlayersOnServers = 0;
		serverStatus.nextDisplayRefresh = uis.realtime + 500;
		return;
	}

	visible = qfalse;
	for (i = 0; i < count; i++) {
		// if we already got info for this server
		if (!trap_LAN_ServerIsVisible(g_arenaservers.master.curvalue, i)) {
			continue;
		}
		visible = qtrue;
		// get the ping for this server
		ping = trap_LAN_GetServerPing(g_arenaservers.master.curvalue, i);
		if (ping > 0 || g_arenaservers.master.curvalue == AS_FAVORITES) {

			trap_LAN_GetServerInfo(g_arenaservers.master.curvalue, i, info, MAX_STRING_CHARS);

			serverStatus.numRefreshedServers++;

			clients = atoi(Info_ValueForKey(info, "clients"));
			maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));

			if (ui_browserShowAwOnly.integer) {
				// CHANGE "awbeta2" back to "afterwards"!!!
				if (Q_stricmp(Info_ValueForKey(info, "game"), "afterwards") != 0 && g_arenaservers.master.curvalue != AS_FAVORITES ) {
					trap_LAN_MarkServerVisible(g_arenaservers.master.curvalue, i, qfalse);
					continue;
				}
			}

			if (ui_browserShowEmpty.integer == 0) {
				if (clients == 0) {
					trap_LAN_MarkServerVisible(g_arenaservers.master.curvalue, i, qfalse);
					continue;
				}
			}

			if (ui_browserShowFull.integer == 0) {
				if (clients == maxClients) {
					trap_LAN_MarkServerVisible(g_arenaservers.master.curvalue, i, qfalse);
					continue;
				}
			}

			if (g_gametype) {
				game = atoi(Info_ValueForKey(info, "gametype"));
				if (game != g_gametype-1) {
					trap_LAN_MarkServerVisible(g_arenaservers.master.curvalue, i, qfalse);
					continue;
				}
			}
				
			// make sure we never add a favorite server twice
			if (g_arenaservers.master.curvalue == AS_FAVORITES) {
				UI_RemoveServerFromDisplayList(i);
			}
			// insert the server into the list
			UI_BinaryServerInsertion(i);
			// increase clients count
			clients = atoi(Info_ValueForKey(info, "clients"));
			serverStatus.numPlayersOnServers += clients;
			// done with this server
			if (ping > 0) {
				trap_LAN_MarkServerVisible(g_arenaservers.master.curvalue, i, qfalse);
				numinvisible++;
			}
		}
	}

//	serverStatus.refreshtime = uis.realtime;

	ArenaServers_UpdateMenu( force );

	// if there were no servers visible for ping updates
	if (!visible) {
//		UI_StopServerRefresh();
//		serverStatus.nextDisplayRefresh = 0;
	}
}


/*
=================
ArenaServers_StopRefresh
=================
*/
static void ArenaServers_StopRefresh( void )
{
	int count;

	if (!serverStatus.refreshActive) {
		// not currently refreshing
		return;
	}
	serverStatus.refreshActive = qfalse;
	Com_Printf("%d servers listed in browser with %d players.\n",
					serverStatus.numDisplayServers,
					serverStatus.numPlayersOnServers);
	count = trap_LAN_GetServerCount(g_arenaservers.master.curvalue);
	if (count - serverStatus.numDisplayServers > 0) {
		if ( ui_browserShowAwOnly.integer ) {
			Com_Printf("%d servers not listed because they aren't running Afterwards or have pings over %d\n",
						count - serverStatus.numDisplayServers,
						(int) trap_Cvar_VariableValue("cl_maxPing"));
		} else {
			Com_Printf("%d servers not listed due to packet loss or pings over %d\n",
						count - serverStatus.numDisplayServers,
						(int) trap_Cvar_VariableValue("cl_maxPing"));
		}
	}

	ArenaServers_UpdateMenu( qtrue );
}

/*
=================
ArenaServers_MaxPing
=================
*/
static int ArenaServers_MaxPing( void ) {
	int		maxPing;

	maxPing = (int)trap_Cvar_VariableValue( "cl_maxPing" );
	if( maxPing < 100 ) {
		maxPing = 100;
	}
	return maxPing;
}

/*
=================
ArenaServers_DoRefresh
=================
*/
static void ArenaServers_DoRefresh( void )
{
	qboolean wait = qfalse;

	if (!serverStatus.refreshActive) {
		return;
	}
	if (g_arenaservers.master.curvalue != AS_FAVORITES) {
		if (g_arenaservers.master.curvalue == AS_LOCAL) {
			if (!trap_LAN_GetServerCount(g_arenaservers.master.curvalue)) {
				if ( uis.realtime > serverStatus.refreshtime ) {
					serverStatus.refreshtime = uis.realtime + 3000;
					trap_Cmd_ExecuteText( EXEC_NOW, "localservers\n" );
				}
				wait = qtrue;
			}
		} else {
			if (trap_LAN_GetServerCount(g_arenaservers.master.curvalue) < 0) {
				wait = qtrue;
			}
		}
	}

	if (uis.realtime < serverStatus.refreshtime || wait) {
		return;
	}

//	ArenaServers_BuildDisplayList(qtrue);
//	Com_Printf("Number of Servers: %i\n", serverStatus.numDisplayServers);
//	ArenaServers_UpdateMenu();

	// if still trying to retrieve pings
	if (trap_LAN_UpdateVisiblePings(g_arenaservers.master.curvalue)) {
		serverStatus.refreshtime = uis.realtime + 500;
	} else if (!wait) {
		// get the last servers in the list
		ArenaServers_BuildDisplayList( qfalse );
		// stop the refresh
		ArenaServers_StopRefresh();
	}
	//
	ArenaServers_BuildDisplayList( qfalse );
}

/*
=================
ArenaServers_StartRefresh
=================
*/
static void ArenaServers_StartRefresh(qboolean full)
{
	int		i;
	char	*ptr;

//	qtime_t q;
//	trap_RealTime(&q);
// 	trap_Cvar_Set( va("ui_lastServerRefresh_%i", g_arenaservers.master.curvalue), va("%s-%i, %i at %i:%i", MonthAbbrev[q.tm_mon],q.tm_mday, 1900+q.tm_year,q.tm_hour,q.tm_min));

	if (!full) {
		UI_UpdatePendingPings();
		return;
	}

	serverStatus.refreshActive = qtrue;
	serverStatus.nextDisplayRefresh = uis.realtime + 1000;
	// clear number of displayed servers
	serverStatus.numDisplayServers = 0;
	serverStatus.numRefreshedServers = 0;
	serverStatus.numPlayersOnServers = 0;
	// reset list
	g_arenaservers.list.numitems = serverStatus.numDisplayServers;
	g_arenaservers.list.curvalue = 0;
	g_arenaservers.list.top      = 0;
	// mark all servers as visible so we store ping updates for them
	trap_LAN_MarkServerVisible(g_arenaservers.master.curvalue, -1, qtrue);
	// reset all the pings
	trap_LAN_ResetPings(g_arenaservers.master.curvalue);
	//
	serverStatus.refreshtime = uis.realtime + 1000;
	if( g_arenaservers.master.curvalue == AS_LOCAL ) {
		trap_Cmd_ExecuteText( EXEC_NOW, "localservers\n" );
		// update menu
		ArenaServers_UpdateMenu( qfalse );
		return;
	}

	if( g_arenaservers.master.curvalue == AS_GLOBAL || g_arenaservers.master.curvalue == AS_MPLAYER ) {
		serverStatus.refreshtime = uis.realtime + 4000;
		if( g_arenaservers.master.curvalue == AS_GLOBAL ) {
			i = 0;
		}
		else {
			i = 1;
		}

		ptr = UI_Cvar_VariableString("debug_protocol");
		if (strlen(ptr)) {
			trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %s full empty\n", i, ptr));
		}
		else {
			trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %d full empty\n", i, (int)trap_Cvar_VariableValue( "protocol" ) ) );
		}
	}
	// update menu
	ArenaServers_UpdateMenu( qfalse );
}


/*
=================
ArenaServers_Go
=================
*/
static void ArenaServers_Go( void ) {
	char	info[MAX_STRING_CHARS];

	if( !g_arenaservers.list.numitems ) 
		return;

	trap_LAN_GetServerInfo(g_arenaservers.master.curvalue, serverStatus.displayServers[g_arenaservers.list.curvalue], info, MAX_STRING_CHARS);

	if( Info_ValueForKey(info, "addr") ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", Info_ValueForKey(info, "addr") ) );
	}
}


/*
=================
ArenaServers_UpdatePicture
=================
*/
static void ArenaServers_UpdatePicture( void ) {
	char	info[MAX_STRING_CHARS];
	static char		picname[64];

	trap_LAN_GetServerInfo(g_arenaservers.master.curvalue, serverStatus.displayServers[g_arenaservers.list.curvalue], info, MAX_STRING_CHARS);

	if( !g_arenaservers.list.numitems ) {
		g_arenaservers.mappic.generic.name = NULL;
	}
	else {
		Com_sprintf( picname, sizeof(picname), "levelshots/%s.tga", Info_ValueForKey(info, "mapname") );
		g_arenaservers.mappic.generic.name = picname;
	
	}

	// force shader update during draw
	g_arenaservers.mappic.shader = 0;
}


/*
=================
ArenaServers_UpdateMenu
=================
*/
static void ArenaServers_UpdateMenu( qboolean full ) {
	static char info[MAX_STRING_CHARS];
	int			index;
	char*		buff;
	table_t*	tableptr;
	int			ping;
	int			clients;
	int			maxclients;
	int			gametype;
	char		hostname[MAX_HOSTNAMELENGTH];
	char		mapname[MAX_MAPNAMELENGTH];
	char		*pingColor;
	char		*gametypeColor;

	g_arenaservers.numqueriedservers = trap_LAN_GetServerCount(g_arenaservers.master.curvalue);
	if( g_arenaservers.numqueriedservers > 0 ) {
		// servers found
		if( serverStatus.refreshActive ) {
			// show progress
			Com_sprintf( g_arenaservers.status.string, MAX_STATUSLENGTH, "Found %d Servers. Refreshing... (%d)", 
				g_arenaservers.numqueriedservers, serverStatus.numRefreshedServers);
			g_arenaservers.statusbar.string  = "Press SPACE to stop";

			// disable controls during refresh
			g_arenaservers.master.generic.flags		|= QMF_GRAYED;
			g_arenaservers.gametype.generic.flags	|= QMF_GRAYED;
			g_arenaservers.sortkey.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showempty.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showfull.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showawonly.generic.flags	|= QMF_GRAYED;
			g_arenaservers.list.generic.flags		|= QMF_GRAYED;
			g_arenaservers.refresh.generic.flags	|= QMF_GRAYED;
			g_arenaservers.go.generic.flags			|= QMF_GRAYED;
			g_arenaservers.favorite.generic.flags	|= QMF_GRAYED;
			g_arenaservers.info.generic.flags		|= QMF_GRAYED;
		}
		else {
			// all servers pinged - enable controls
			g_arenaservers.master.generic.flags		&= ~QMF_GRAYED;
			g_arenaservers.gametype.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.sortkey.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showempty.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showfull.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showawonly.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.list.generic.flags		&= ~QMF_GRAYED;
			g_arenaservers.refresh.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.go.generic.flags			&= ~QMF_GRAYED;
			g_arenaservers.favorite.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.info.generic.flags		&= ~QMF_GRAYED;

			// update status bar
			if ( ui_browserShowAwOnly.integer ) {
				strcpy(g_arenaservers.status.string, va("%i Servers running Afterwards with %i Players found.", 
					serverStatus.numDisplayServers, serverStatus.numPlayersOnServers));
			} else {
				strcpy(g_arenaservers.status.string, va("%i Servers with %i Players found.", 
					serverStatus.numDisplayServers, serverStatus.numPlayersOnServers));
			}

			if( g_servertype == AS_GLOBAL || g_servertype == AS_MPLAYER ) {
				g_arenaservers.statusbar.string = quake3worldMessage;
			}
			else {
				g_arenaservers.statusbar.string = "";
			}
			if (!(g_arenaservers.grlogo.generic.flags & QMF_HIDDEN)) {
				g_arenaservers.statusbar.string = globalRankingsMessage;
			}

		}
	}
	else {
		// no servers found
		if( serverStatus.refreshActive ) {
			strcpy( g_arenaservers.status.string,"Scanning For Servers." );
			g_arenaservers.statusbar.string = "Press SPACE to stop";

			// disable controls during refresh
			g_arenaservers.master.generic.flags		|= QMF_GRAYED;
			g_arenaservers.gametype.generic.flags	|= QMF_GRAYED;
			g_arenaservers.sortkey.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showempty.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showfull.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showawonly.generic.flags	|= QMF_GRAYED;
			g_arenaservers.list.generic.flags		|= QMF_GRAYED;
			g_arenaservers.refresh.generic.flags	|= QMF_GRAYED;
			g_arenaservers.go.generic.flags			|= QMF_GRAYED;
			g_arenaservers.favorite.generic.flags	|= QMF_GRAYED;
			g_arenaservers.info.generic.flags		|= QMF_GRAYED;
		}
		else {
			if( g_arenaservers.numqueriedservers < 0 ) {
				strcpy(g_arenaservers.status.string,"No Response From Master Server." );
			}
			else {
				if ( ui_browserShowAwOnly.integer ) strcpy(g_arenaservers.status.string,"No Servers running Afterwards found." );
				else strcpy(g_arenaservers.status.string,"No Servers Found." );
			}

			// update status bar
			if( g_servertype == AS_GLOBAL || g_servertype == AS_MPLAYER ) {
				g_arenaservers.statusbar.string = quake3worldMessage;
			}
			else {
				g_arenaservers.statusbar.string = "";
			}

			// end of refresh - set control state
			g_arenaservers.master.generic.flags		&= ~QMF_GRAYED;
			g_arenaservers.gametype.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.sortkey.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showempty.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showfull.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showawonly.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.list.generic.flags		|= QMF_GRAYED;
			g_arenaservers.refresh.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.go.generic.flags			|= QMF_GRAYED;
			g_arenaservers.favorite.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.info.generic.flags		&= ~QMF_GRAYED;
		}

		// zero out list box
		g_arenaservers.list.numitems = 0;
		g_arenaservers.list.curvalue = 0;
		g_arenaservers.list.top      = 0;

		// update picture
		ArenaServers_UpdatePicture();
		return;
	}

	for (index = 0; (index < serverStatus.numDisplayServers && index < MAX_LISTBOXITEMS && (index < 11 || full)); index++ ) {

		tableptr = &g_arenaservers.table[index];
		buff = tableptr->buff;

		trap_LAN_GetServerInfo(g_arenaservers.master.curvalue, serverStatus.displayServers[index], info, MAX_STRING_CHARS);

		Q_strncpyz(mapname, Info_ValueForKey(info, "mapname"), MAX_MAPNAMELENGTH ); 
		clients = atoi(Info_ValueForKey(info, "clients"));
		maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
		
		ping = atoi(Info_ValueForKey(info, "ping"));
		if (ping == -1) {
			// if we ever see a ping that is out of date, do a server refresh
			// UI_UpdatePendingPings();
		}

		if( ping < 200 ) 
			pingColor = S_COLOR_GREEN;
		else if( ping < 400 ) 
			pingColor = S_COLOR_YELLOW;
		else 
			pingColor = S_COLOR_RED;

		if (ping <= 0) {
			Q_strncpyz( hostname, Info_ValueForKey( info, "addr"), MAX_HOSTNAMELENGTH );
		} else {
			if ( g_arenaservers.master.curvalue == AS_LOCAL ) {
				Com_sprintf( hostname, sizeof(hostname), "%s [%s]",
					Info_ValueForKey(info, "hostname"),
					netnames[atoi(Info_ValueForKey(info, "nettype"))] );
			}
			else {
				Q_strncpyz( hostname, Info_ValueForKey( info, "hostname"), MAX_HOSTNAMELENGTH );
			}
		}
		UI_DeleteColorCodes( hostname );

		gametype = atoi(Info_ValueForKey(info, "gametype"));
		if ( gametype > GT_MAX_GAME_TYPE ) gametype = GT_MAX_GAME_TYPE;		

		// CHANGE "awbeta2" back to "afterwards"!!!
		if ( Q_stricmp(Info_ValueForKey(info, "game"), "afterwards") == 0 ) {
			// afterwards game -> show color code and gametype short
			switch (gametype) {
				case 0: gametypeColor = ""; break; // orange
				case 1: gametypeColor = S_COLOR_RED; break;
				default: gametypeColor = S_COLOR_YELLOW;
			}
		} else {
			// not an afterwards game -> yellow and "--" for gametype
			gametype = GT_MAX_GAME_TYPE;
			gametypeColor = S_COLOR_YELLOW;
		}

		Com_sprintf( buff, MAX_LISTBOXWIDTH+8, "%s%s  %-32.32s  %-16.16s  %2d/%2d  %s%3d", 
			gametypeColor, gamenames[gametype], hostname, mapname, clients, maxclients, pingColor, ping );

	}

	// Com_sprintf( g_arenaservers.status.string, MAX_STATUSLENGTH, "%d of %d Arena Servers.", serverStatus.numDisplayServers, *g_arenaservers.numservers );

	g_arenaservers.list.numitems = (serverStatus.numDisplayServers < MAX_LISTBOXITEMS) ? serverStatus.numDisplayServers : MAX_LISTBOXITEMS ;
	g_arenaservers.list.curvalue = 0;
	g_arenaservers.list.top      = 0;

	// update picture
	ArenaServers_UpdatePicture();
}


/*
=================
ArenaServers_Addfavorite
=================
*/
void ArenaServers_AddRemoveFavorite( void )
{
	char info[MAX_STRING_CHARS];
	char name[MAX_NAME_LENGTH];
	char addr[MAX_NAME_LENGTH];

	if (g_arenaservers.master.curvalue == AS_FAVORITES) {
		trap_LAN_GetServerInfo(g_arenaservers.master.curvalue, serverStatus.displayServers[g_arenaservers.list.curvalue], info, MAX_STRING_CHARS);
		addr[0] = '\0';
		Q_strncpyz(addr, 	Info_ValueForKey(info, "addr"), MAX_NAME_LENGTH);
		if (strlen(addr) > 0) {
			trap_LAN_RemoveServer(AS_FAVORITES, addr);
		}
		ArenaServers_BuildDisplayList(qtrue);
	}
	else {
		int res;

		if( !g_arenaservers.list.numitems ) 
			return;

		trap_LAN_GetServerInfo(g_arenaservers.master.curvalue, serverStatus.displayServers[g_arenaservers.list.curvalue], info, MAX_STRING_CHARS);
		name[0] = addr[0] = '\0';
		Q_strncpyz(name, 	Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH);
		Q_strncpyz(addr, 	Info_ValueForKey(info, "addr"), MAX_NAME_LENGTH);
		if (strlen(name) > 0 && strlen(addr) > 0) {
			res = trap_LAN_AddServer(AS_FAVORITES, name, addr);
			if (res == 0) {
				// server already in the list
				strcpy(g_arenaservers.status.string, "Favorite already in list\n");
			}
			else if (res == -1) {
				// list full
				strcpy(g_arenaservers.status.string, "Favorite list full\n");
			}
			else {
				// successfully added
				strcpy(g_arenaservers.status.string, va("Added favorite server %s\n", addr));
			}
		}
	}
}


/*
=================
ArenaServers_SetType
=================
*/
void ArenaServers_SetType( int type )
{
	if (g_servertype == type)
		return;

	g_servertype = type;

	if ( type == AS_FAVORITES ) {
		g_arenaservers.favorite.generic.flags &= ~QMF_HIDDEN;
		g_arenaservers.favorite.generic.name = ART_REMOVE0;
		g_arenaservers.favorite.focuspic = ART_REMOVE1;
		g_arenaservers.favorite.focusshader = 0;
		g_arenaservers.favorite.shader = 0;
	} else if ( type == AS_LOCAL ) {
		g_arenaservers.favorite.generic.flags |= QMF_HIDDEN;
	} else {
		g_arenaservers.favorite.generic.flags &= ~QMF_HIDDEN;
		g_arenaservers.favorite.generic.name = ART_ADD0;
		g_arenaservers.favorite.focuspic = ART_ADD1;
		g_arenaservers.favorite.focusshader = 0;
		g_arenaservers.favorite.shader = 0;
	}

	ArenaServers_BuildDisplayList(qtrue);

	if( !serverStatus.numRefreshedServers ) {
		ArenaServers_StartRefresh( qtrue );
		return;
	}
/*	else {
		// avoid slow operation, use existing results
		g_arenaservers.currentping       = *g_arenaservers.numservers;
		g_arenaservers.numqueriedservers = *g_arenaservers.numservers; 
		ArenaServers_BuildDisplayList(qtrue);
	}*/
}


/*
=================
ArenaServers_Event
=================
*/
static void ArenaServers_Event( void* ptr, int event ) {
	char info[MAX_STRING_CHARS];
	int		id;

	id = ((menucommon_s*)ptr)->id;

	if( event != QM_ACTIVATED && id != ID_LIST ) {
		return;
	}

	switch( id ) {
	case ID_MASTER:
		trap_Cvar_SetValue( "ui_browserMaster", g_arenaservers.master.curvalue );
		ArenaServers_SetType( g_arenaservers.master.curvalue );
		break;

	case ID_GAMETYPE:
		trap_Cvar_SetValue( "ui_browserGameType", g_arenaservers.gametype.curvalue );
		g_gametype = g_arenaservers.gametype.curvalue;
		serverStatus.newDisplayList = qtrue;
		break;

	case ID_SORTKEY:
		trap_Cvar_SetValue( "ui_browserSortKey", g_arenaservers.sortkey.curvalue );
		g_sortkey = g_arenaservers.sortkey.curvalue;
		serverStatus.newDisplayList = qtrue;
		break;

	case ID_SHOW_FULL:
		trap_Cvar_SetValue( "ui_browserShowFull", g_arenaservers.showfull.curvalue );
		g_fullservers = g_arenaservers.showfull.curvalue;
		serverStatus.newDisplayList = qtrue;
		break;

	case ID_SHOW_EMPTY:
		trap_Cvar_SetValue( "ui_browserShowEmpty", g_arenaservers.showempty.curvalue );
		g_emptyservers = g_arenaservers.showempty.curvalue;
		serverStatus.newDisplayList = qtrue;
		break;

	case ID_SHOW_AWONLY:
		trap_Cvar_SetValue( "ui_browserShowAwOnly", g_arenaservers.showawonly.curvalue );
//		g_emptyservers = g_arenaservers.showawonly.curvalue;
		serverStatus.newDisplayList = qtrue;
		break;

	case ID_LIST:
		if( event == QM_GOTFOCUS ) {
			ArenaServers_UpdatePicture();
		}
		break;

	case ID_SCROLL_UP:
		ScrollList_Key( &g_arenaservers.list, K_UPARROW );
		break;

	case ID_SCROLL_DOWN:
		ScrollList_Key( &g_arenaservers.list, K_DOWNARROW );
		break;

	case ID_BACK:
		ArenaServers_StopRefresh();
		UI_PopMenu();
		break;

	case ID_REFRESH:
		ArenaServers_StartRefresh( qtrue );
		break;

	case ID_SPECIFY:
		UI_SpecifyServerMenu();
		break;

	case ID_CONNECT:
		ArenaServers_Go();
		break;

	case ID_INFO:
		trap_LAN_GetServerInfo(g_arenaservers.master.curvalue, serverStatus.displayServers[g_arenaservers.list.curvalue], info, MAX_STRING_CHARS);
		if ( serverStatus.numDisplayServers > 0 && strlen(Info_ValueForKey(info, "addr")) > 0 ) {
			UI_ServerInfoMenu( Info_ValueForKey(info, "addr") );
		}
		break;

	case ID_FAVORITE:
		ArenaServers_AddRemoveFavorite();
		break;
	}
}


/*
=================
ArenaServers_MenuDraw
=================
*/
static void ArenaServers_MenuDraw( void )
{
	if (serverStatus.refreshActive)
		ArenaServers_DoRefresh();
	if (serverStatus.newDisplayList) {
		serverStatus.newDisplayList = qfalse;
		ArenaServers_BuildDisplayList( qtrue );
	}

	Menu_Draw( &g_arenaservers.menu );
}


/*
=================
ArenaServers_MenuKey
=================
*/
static sfxHandle_t ArenaServers_MenuKey( int key ) {
	if( key == K_SPACE  && serverStatus.refreshActive ) {
		ArenaServers_StopRefresh();	
		return menu_move_sound;
	}

	if( ( key == K_DEL || key == K_KP_DEL ) && ( g_servertype == AS_FAVORITES ) &&
		( Menu_ItemAtCursor( &g_arenaservers.menu) == &g_arenaservers.list ) ) {
		ArenaServers_AddRemoveFavorite();
		return menu_move_sound;
	}

/*	if( ( key == 70 ) && ( g_servertype != AS_FAVORITES ) &&
		( Menu_ItemAtCursor( &g_arenaservers.menu) == &g_arenaservers.list ) ) {
		ArenaServers_AddRemoveFavorite();
		return menu_move_sound;
	}*/

	if( key == K_MOUSE2 || key == K_ESCAPE ) {
		ArenaServers_StopRefresh();
//		ArenaServers_SaveChanges();
	}


	return Menu_DefaultKey( &g_arenaservers.menu, key );
}


/*
=================
ArenaServers_MenuInit
=================
*/
static void ArenaServers_MenuInit( void ) {
	int			i;
	int			type;
	int			y;
	static char	statusbuffer[MAX_STATUSLENGTH];
	static char leaguebuffer[MAX_LEAGUELENGTH];

	// zero set all our globals
	memset( &g_arenaservers, 0 ,sizeof(arenaservers_t) );

	ArenaServers_Cache();

	g_arenaservers.menu.fullscreen = qtrue;
	g_arenaservers.menu.wrapAround = qtrue;
	g_arenaservers.menu.showlogo   = LOGO_AFTERWARDS;
    g_arenaservers.menu.draw       = ArenaServers_MenuDraw;
	g_arenaservers.menu.key        = ArenaServers_MenuKey;

	g_arenaservers.banner.generic.type  = MTYPE_BTEXT;
	g_arenaservers.banner.generic.flags = QMF_CENTER_JUSTIFY;
	g_arenaservers.banner.generic.x	    = 320;
	g_arenaservers.banner.generic.y	    = 16;
	g_arenaservers.banner.string  		= "JOIN GAME";
	g_arenaservers.banner.style  	    = UI_CENTER;
	g_arenaservers.banner.color  	    = color_white;

	g_arenaservers.grlogo.generic.type  = MTYPE_BITMAP;
	g_arenaservers.grlogo.generic.name  = GLOBALRANKINGS_LOGO;
	g_arenaservers.grlogo.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	g_arenaservers.grlogo.generic.x		= 530;
	g_arenaservers.grlogo.generic.y		= 40;
	g_arenaservers.grlogo.width			= 32;
	g_arenaservers.grlogo.height		= 64;
		
	g_arenaservers.league.generic.type		= MTYPE_TEXT;
	g_arenaservers.league.generic.flags		= QMF_HIDDEN;
	g_arenaservers.league.generic.x			= g_arenaservers.grlogo.generic.x +
											  (g_arenaservers.grlogo.width / 2);
	g_arenaservers.league.generic.y			= g_arenaservers.grlogo.generic.y +
											  g_arenaservers.grlogo.height + 2;
	g_arenaservers.league.string			= leaguebuffer;
	g_arenaservers.league.style				= UI_CENTER|UI_SMALLFONT;
	g_arenaservers.league.color				= menu_text_color;
	

	g_arenaservers.practice.generic.type	= MTYPE_TEXT;
	g_arenaservers.practice.generic.flags	= QMF_HIDDEN;
	g_arenaservers.practice.generic.x		= g_arenaservers.grlogo.generic.x +
											  (g_arenaservers.grlogo.width / 2);
	g_arenaservers.practice.generic.y		= g_arenaservers.grlogo.generic.y + 6;
	g_arenaservers.practice.string			= "practice";
	g_arenaservers.practice.style			= UI_CENTER|UI_SMALLFONT;
	g_arenaservers.practice.color			= menu_text_color;
	
	
	y = 80;
	g_arenaservers.master.generic.type			= MTYPE_SPINCONTROL;
	g_arenaservers.master.generic.name			= "Servers:";
	g_arenaservers.master.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.master.generic.callback		= ArenaServers_Event;
	g_arenaservers.master.generic.id			= ID_MASTER;
	g_arenaservers.master.generic.x				= 320;
	g_arenaservers.master.generic.y				= y;
	g_arenaservers.master.itemnames				= master_items;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.gametype.generic.type		= MTYPE_SPINCONTROL;
	g_arenaservers.gametype.generic.name		= "Game Type:";
	g_arenaservers.gametype.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.gametype.generic.callback	= ArenaServers_Event;
	g_arenaservers.gametype.generic.id			= ID_GAMETYPE;
	g_arenaservers.gametype.generic.x			= 320;
	g_arenaservers.gametype.generic.y			= y;
	g_arenaservers.gametype.itemnames			= servertype_items;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.sortkey.generic.type			= MTYPE_SPINCONTROL;
	g_arenaservers.sortkey.generic.name			= "Sort By:";
	g_arenaservers.sortkey.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.sortkey.generic.callback		= ArenaServers_Event;
	g_arenaservers.sortkey.generic.id			= ID_SORTKEY;
	g_arenaservers.sortkey.generic.x			= 320;
	g_arenaservers.sortkey.generic.y			= y;
	g_arenaservers.sortkey.itemnames			= sortkey_items;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.showfull.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.showfull.generic.name		= "Show Full:";
	g_arenaservers.showfull.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.showfull.generic.callback	= ArenaServers_Event;
	g_arenaservers.showfull.generic.id			= ID_SHOW_FULL;
	g_arenaservers.showfull.generic.x			= 320;
	g_arenaservers.showfull.generic.y			= y;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.showempty.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.showempty.generic.name		= "Show Empty:";
	g_arenaservers.showempty.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.showempty.generic.callback	= ArenaServers_Event;
	g_arenaservers.showempty.generic.id			= ID_SHOW_EMPTY;
	g_arenaservers.showempty.generic.x			= 320;
	g_arenaservers.showempty.generic.y			= y;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.showawonly.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.showawonly.generic.name		= "AW Only:";
	g_arenaservers.showawonly.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.showawonly.generic.callback	= ArenaServers_Event;
	g_arenaservers.showawonly.generic.id		= ID_SHOW_AWONLY;
	g_arenaservers.showawonly.generic.x			= 320;
	g_arenaservers.showawonly.generic.y			= y;

	y += 2 * SMALLCHAR_HEIGHT;
	g_arenaservers.list.generic.type			= MTYPE_SCROLLLIST;
	g_arenaservers.list.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.list.generic.id				= ID_LIST;
	g_arenaservers.list.generic.callback		= ArenaServers_Event;
	g_arenaservers.list.generic.x				= 32;
	g_arenaservers.list.generic.y				= y;
	g_arenaservers.list.width					= MAX_LISTBOXWIDTH;
	g_arenaservers.list.height					= 11;
	g_arenaservers.list.itemnames				= (const char **)g_arenaservers.items;
	for( i = 0; i < MAX_LISTBOXITEMS; i++ ) {
		g_arenaservers.items[i] = g_arenaservers.table[i].buff;
	}

	g_arenaservers.mappic.generic.type			= MTYPE_BITMAP;
	g_arenaservers.mappic.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	g_arenaservers.mappic.generic.x				= 72;
	g_arenaservers.mappic.generic.y				= 80;
	g_arenaservers.mappic.width					= 128;
	g_arenaservers.mappic.height				= 96;
	g_arenaservers.mappic.errorpic				= ART_UNKNOWNMAP;

	g_arenaservers.arrows.generic.type			= MTYPE_BITMAP;
	g_arenaservers.arrows.generic.name			= ART_ARROWS0;
	g_arenaservers.arrows.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	g_arenaservers.arrows.generic.callback		= ArenaServers_Event;
	g_arenaservers.arrows.generic.x				= 512+48;
	g_arenaservers.arrows.generic.y				= 240-16;
	g_arenaservers.arrows.width					= 64;
	g_arenaservers.arrows.height				= 128;

	g_arenaservers.up.generic.type				= MTYPE_BITMAP;
	g_arenaservers.up.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	g_arenaservers.up.generic.callback			= ArenaServers_Event;
	g_arenaservers.up.generic.id				= ID_SCROLL_UP;
	g_arenaservers.up.generic.x					= 512+48;
	g_arenaservers.up.generic.y					= 240-16;
	g_arenaservers.up.width						= 64;
	g_arenaservers.up.height					= 64;
	g_arenaservers.up.focuspic					= ART_ARROWS_UP;

	g_arenaservers.down.generic.type			= MTYPE_BITMAP;
	g_arenaservers.down.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	g_arenaservers.down.generic.callback		= ArenaServers_Event;
	g_arenaservers.down.generic.id				= ID_SCROLL_DOWN;
	g_arenaservers.down.generic.x				= 512+48;
	g_arenaservers.down.generic.y				= 240+48;
	g_arenaservers.down.width					= 64;
	g_arenaservers.down.height					= 64;
	g_arenaservers.down.focuspic				= ART_ARROWS_DOWN;

	y = 376;
	g_arenaservers.status.generic.type		= MTYPE_TEXT;
	g_arenaservers.status.generic.x			= 320;
	g_arenaservers.status.generic.y			= y;
	g_arenaservers.status.string			= statusbuffer;
	g_arenaservers.status.style				= UI_CENTER|UI_SMALLFONT;
	g_arenaservers.status.color				= menu_text_color;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.statusbar.generic.type   = MTYPE_TEXT;
	g_arenaservers.statusbar.generic.x	    = 320;
	g_arenaservers.statusbar.generic.y	    = y;
	g_arenaservers.statusbar.string	        = "";
	g_arenaservers.statusbar.style	        = UI_CENTER|UI_SMALLFONT;
	g_arenaservers.statusbar.color	        = text_color_normal;

	g_arenaservers.info.generic.type		= MTYPE_BITMAP;
	g_arenaservers.info.generic.name		= ART_INFO0;
	g_arenaservers.info.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.info.generic.callback	= ArenaServers_Event;
	g_arenaservers.info.generic.id			= ID_INFO;
	g_arenaservers.info.generic.x			= 450;
	g_arenaservers.info.generic.y			= 80;
	g_arenaservers.info.width				= 96;
	g_arenaservers.info.height				= 48;
	g_arenaservers.info.focuspic			= ART_INFO1;

	g_arenaservers.favorite.generic.type		= MTYPE_BITMAP;
	g_arenaservers.favorite.generic.name		= ART_REMOVE0;
	g_arenaservers.favorite.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.favorite.generic.callback	= ArenaServers_Event;
	g_arenaservers.favorite.generic.id			= ID_FAVORITE;
	g_arenaservers.favorite.generic.x			= 450;
	g_arenaservers.favorite.generic.y			= 130;
	g_arenaservers.favorite.width				= 96;
	g_arenaservers.favorite.height				= 48;
	g_arenaservers.favorite.focuspic			= ART_REMOVE1;

	g_arenaservers.back.generic.type		= MTYPE_BITMAP;
	g_arenaservers.back.generic.name		= ART_BACK0;
	g_arenaservers.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.back.generic.callback	= ArenaServers_Event;
	g_arenaservers.back.generic.id			= ID_BACK;
	g_arenaservers.back.generic.x			= 0;
	g_arenaservers.back.generic.y			= 480-64;
	g_arenaservers.back.width				= 128;
	g_arenaservers.back.height				= 64;
	g_arenaservers.back.focuspic			= ART_BACK1;

	g_arenaservers.specify.generic.type	    = MTYPE_BITMAP;
	g_arenaservers.specify.generic.name		= ART_SPECIFY0;
	g_arenaservers.specify.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.specify.generic.callback = ArenaServers_Event;
	g_arenaservers.specify.generic.id	    = ID_SPECIFY;
	g_arenaservers.specify.generic.x		= 170;
	g_arenaservers.specify.generic.y		= 480-64;
	g_arenaservers.specify.width  		    = 128;
	g_arenaservers.specify.height  		    = 64;
	g_arenaservers.specify.focuspic         = ART_SPECIFY1;

	g_arenaservers.refresh.generic.type		= MTYPE_BITMAP;
	g_arenaservers.refresh.generic.name		= ART_REFRESH0;
	g_arenaservers.refresh.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.refresh.generic.callback	= ArenaServers_Event;
	g_arenaservers.refresh.generic.id		= ID_REFRESH;
	g_arenaservers.refresh.generic.x		= 340;
	g_arenaservers.refresh.generic.y		= 480-64;
	g_arenaservers.refresh.width			= 128;
	g_arenaservers.refresh.height			= 64;
	g_arenaservers.refresh.focuspic			= ART_REFRESH1;

	g_arenaservers.go.generic.type			= MTYPE_BITMAP;
	g_arenaservers.go.generic.name			= ART_CONNECT0;
	g_arenaservers.go.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.go.generic.callback		= ArenaServers_Event;
	g_arenaservers.go.generic.id			= ID_CONNECT;
	g_arenaservers.go.generic.x				= 640;
	g_arenaservers.go.generic.y				= 480-64;
	g_arenaservers.go.width					= 128;
	g_arenaservers.go.height				= 64;
	g_arenaservers.go.focuspic				= ART_CONNECT1;

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.banner );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.grlogo );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.league );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.practice );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.master );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.gametype );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.sortkey );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.showfull);
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.showempty );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.showawonly );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.mappic );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.list );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.status );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.statusbar );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.arrows );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.up );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.down );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.info );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.favorite );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.back );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.specify );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.refresh );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.go );

//	ArenaServers_LoadFavorites();

	g_servertype = Com_Clamp( 0, 3, ui_browserMaster.integer );
	g_arenaservers.master.curvalue = g_servertype;

	g_gametype = Com_Clamp( 0, 4, ui_browserGameType.integer );
	g_arenaservers.gametype.curvalue = g_gametype;

	g_sortkey = Com_Clamp( 0, 4, ui_browserSortKey.integer );
	g_arenaservers.sortkey.curvalue = g_sortkey;

	g_fullservers = Com_Clamp( 0, 1, ui_browserShowFull.integer );
	g_arenaservers.showfull.curvalue = g_fullservers;

	g_emptyservers = Com_Clamp( 0, 1, ui_browserShowEmpty.integer );
	g_arenaservers.showempty.curvalue = g_emptyservers;

	g_arenaservers.showawonly.curvalue = Com_Clamp( 0, 1, ui_browserShowAwOnly.integer );

	// force to initial state and refresh
	memset( &serverStatus, 0, sizeof( serverStatus ) );
	type = g_servertype;
	g_servertype = -1;
	ArenaServers_SetType( type );

	trap_Cvar_Register(NULL, "debug_protocol", "", 0 );
}


/*
=================
ArenaServers_Cache
=================
*/
void ArenaServers_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_INFO0 );
	trap_R_RegisterShaderNoMip( ART_INFO1 );
	trap_R_RegisterShaderNoMip( ART_REMOVE0 );
	trap_R_RegisterShaderNoMip( ART_REMOVE1 );
	trap_R_RegisterShaderNoMip( ART_ADD0 );
	trap_R_RegisterShaderNoMip( ART_ADD1 );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_SPECIFY0 );
	trap_R_RegisterShaderNoMip( ART_SPECIFY1 );
	trap_R_RegisterShaderNoMip( ART_REFRESH0 );
	trap_R_RegisterShaderNoMip( ART_REFRESH1 );
	trap_R_RegisterShaderNoMip( ART_CONNECT0 );
	trap_R_RegisterShaderNoMip( ART_CONNECT1 );
	trap_R_RegisterShaderNoMip( ART_ARROWS0  );
	trap_R_RegisterShaderNoMip( ART_ARROWS_UP );
	trap_R_RegisterShaderNoMip( ART_ARROWS_DOWN );
	trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );

	trap_R_RegisterShaderNoMip( GLOBALRANKINGS_LOGO );
}


/*
=================
UI_ArenaServersMenu
=================
*/
void UI_ArenaServersMenu( void ) {
	ArenaServers_MenuInit();
	UI_PushMenu( &g_arenaservers.menu );
}						  
