// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"

#define SERVERINFO_BACK0		"menu/art/back_0"
#define SERVERINFO_BACK1		"menu/art/back_1"
#define SERVERINFO_ADD0			"menu/art/favorite_0"
#define SERVERINFO_ADD1			"menu/art/favorite_1"
#define SERVERINFO_ARROWS0		"menu/art/arrows_vert_0"
#define SERVERINFO_ARROWS_UP	"menu/art/arrows_vert_top"
#define SERVERINFO_ARROWS_DOWN	"menu/art/arrows_vert_bot"
#define SERVERINFO_UNKNOWNMAP	"menu/art/unknownmap"

static char* serverinfo_artlist[] =
{
	SERVERINFO_BACK0,
	SERVERINFO_BACK1,
	SERVERINFO_ADD0,
	SERVERINFO_ADD1,
	SERVERINFO_ARROWS0,
	SERVERINFO_ARROWS_UP,
	SERVERINFO_ARROWS_DOWN,
	NULL
};

#define ID_ADD			100
#define ID_BACK			101
#define ID_SCROLL_UP	102
#define ID_SCROLL_DOWN	103

#define MAX_SERVERSTATUS_LINES	64
#define MAX_SERVERSTATUS_TEXT	1024
#define MAX_ADDRESSLENGTH		64

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menutext_s		waittext;
	menubitmap_s	mappic;
	menubitmap_s	arrows;
	menubitmap_s	up;
	menubitmap_s	down;
	menubitmap_s	back;
	menubitmap_s	add;
	char			address[MAX_ADDRESSLENGTH];
	int				top;
} serverinfo_t;

static serverinfo_t	s_serverinfo;

typedef struct {
	char address[MAX_ADDRESSLENGTH];
	char *lines[MAX_SERVERSTATUS_LINES][4];
	char text[MAX_SERVERSTATUS_TEXT];
	char pings[MAX_CLIENTS * 3];
	int numLines;
	int playerLines;
	int refreshtime;
	qboolean found;
} serverStatusInfo_t;

static serverStatusInfo_t s_serverStatusInfo;

typedef struct
{
	char *name, *altName;
} serverStatusCvar_t;


// EXTENDED_INFO defines which information is "scrollable"
#define	EXTENDED_INFO	7
#define EXTENDED_LINES	14

serverStatusCvar_t serverStatusCvars[] = {
	{"sv_hostname", "Name"},
	{"Address", ""},
	{"gamename", "Game"},
	{"mapname", "Map"},
	{"g_gametype", "Game Type"},
	{"version", ""},
	{"protocol", ""},
	{"timelimit", ""},
	{"fraglimit", ""},
	{"winlimit", ""},
	{NULL, NULL}
};

static char* gametypes[GT_MAX_GAME_TYPE+1] = {
	"Tactical",
	"Deathmatch",
	"Unknown"
};

/*
==================
ServerInfo_SortStatusInfo
==================
*/
static void ServerInfo_SortStatusInfo( serverStatusInfo_t *info ) {
	int i, j, index, gametype;
	char *tmp1, *tmp2;

	index = 0;
	for (i = 0; serverStatusCvars[i].name; i++) {
		for (j = 0; j < info->numLines; j++) {
			if ( !info->lines[j][3] ) {
				continue;
			}
			if ( !Q_stricmp(serverStatusCvars[i].name, info->lines[j][0]) ) {
				// swap lines
				tmp1 = info->lines[index][0];
				tmp2 = info->lines[index][1];
				info->lines[index][0] = info->lines[j][0];
				info->lines[index][1] = info->lines[j][1];
				info->lines[j][0] = tmp1;
				info->lines[j][1] = tmp2;

				// use altName
				if ( strlen(serverStatusCvars[i].altName) ) {
					info->lines[index][0] = serverStatusCvars[i].altName;
				}
				index++;
			}
		}
	}

	// insert gametype name
	if ( !Q_stricmp(info->lines[2][1], GAMEVERSION) ) {
		// gametype name
		gametype = atoi(info->lines[4][1]);
		info->lines[4][1] = gametypes[ (gametype < GT_MAX_GAME_TYPE) ? gametype : GT_MAX_GAME_TYPE ];
	} else {
		// unknown
		info->lines[4][1] = gametypes[ GT_MAX_GAME_TYPE ];
	}

}

/*
==================
ServerInfo_GetStatus
==================
*/
static int ServerInfo_GetStatus( const char *serverAddress, serverStatusInfo_t *info ) {
	char *p, *score, *ping, *name;
	int i, len, rankscore;

	if (!info) {
		trap_LAN_ServerStatus( serverAddress, NULL, 0);
		return qfalse;
	}
//	memset(info, 0, sizeof(*info));

	if ( trap_LAN_ServerStatus( serverAddress, info->text, sizeof(info->text)) ) {
		info->found = qtrue;
		Q_strncpyz(info->address, serverAddress, sizeof(info->address));
		p = info->text;
		info->numLines = 0;
		info->lines[info->numLines][0] = "Address";
		info->lines[info->numLines][1] = info->address;
		info->lines[info->numLines][2] = "";
		info->lines[info->numLines][3] = "";
		info->numLines++;
		// get the cvars
		while (p && *p) {
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			if (*p == '\\')
				break;
			info->lines[info->numLines][0] = p;
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			info->lines[info->numLines][1] = p;
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";

			info->numLines++;
			if (info->numLines >= MAX_SERVERSTATUS_LINES)
				break;
		}
		ServerInfo_SortStatusInfo( info );
		info->playerLines = info->numLines+1;
		// get the player list
		if (info->numLines < MAX_SERVERSTATUS_LINES-3) {
			// empty line
			info->lines[info->numLines][0] = "";
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";
			info->numLines++;
			// header
			info->lines[info->numLines][0] = "num";
			info->lines[info->numLines][1] = "name";
			if ( !Q_stricmp(info->lines[4][1], gametypes[GT_TACTICAL]) ) info->lines[info->numLines][2] = "rank";
			else info->lines[info->numLines][2] = "score";
			info->lines[info->numLines][3] = "ping";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;
			while (p && *p) {
				if (*p == '\\')
					*p++ = '\0';
				if (!p)
					break;
				score = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				ping = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				name = p;
				Com_sprintf(&info->pings[len], sizeof(info->pings)-len, "%d", i);
				info->lines[info->numLines][0] = &info->pings[len];
				len += strlen(&info->pings[len]) + 1;
				info->lines[info->numLines][1] = name;
				if ( !Q_stricmp(info->lines[4][1], gametypes[GT_TACTICAL]) ) {
					sscanf( score, "%i", &rankscore );
					info->lines[info->numLines][2] = bg_ranklist[BG_GetTacticalRank(rankscore)].rankname;
				} else info->lines[info->numLines][2] = score;
				info->lines[info->numLines][3] = ping;
				info->numLines++;
				if (info->numLines >= MAX_SERVERSTATUS_LINES)
					break;
				p = strchr(p, '\\');
				if (!p)
					break;
				*p++ = '\0';
				//
				i++;
			}
		}
		return qtrue;
	}
	return qfalse;
}


/*
=================
Favorites_Add
=================
*/
void Favorites_Add( void )
{
	int res;

	if( !s_serverStatusInfo.found ) 
		return;

	if (strlen(s_serverStatusInfo.lines[0][1]) > 0 && strlen(s_serverinfo.address) > 0) {
		res = trap_LAN_AddServer(AS_FAVORITES, s_serverStatusInfo.lines[0][1], s_serverinfo.address);
		if (res == 0) {
			// server already in the list
			Com_Printf("Favorite already in list\n");
			}
		else if (res == -1) {
			// list full
			Com_Printf("Favorite list full\n");
		}
		else {
			// successfully added
			Com_Printf("Added favorite server %s\n", s_serverinfo.address);
		}
	}
}


/*
=================
ServerInfo_Event
=================
*/
static void ServerInfo_Event( void* ptr, int event )
{
	switch (((menucommon_s*)ptr)->id)
	{
		case ID_ADD:
			if (event != QM_ACTIVATED)
				break;
		
			Favorites_Add();
			UI_PopMenu();
			break;

		case ID_BACK:
			if (event != QM_ACTIVATED)
				break;

			UI_PopMenu();
			break;

		case ID_SCROLL_DOWN:
			if (event != QM_ACTIVATED)
				break;

			if ( s_serverinfo.top + EXTENDED_INFO + EXTENDED_LINES < s_serverStatusInfo.numLines ) {
				s_serverinfo.top++;
			}
			break;

		case ID_SCROLL_UP:
			if (event != QM_ACTIVATED)
				break;

			if ( s_serverinfo.top > 0 ) {
				s_serverinfo.top--;
			}
			break;
	}
}

/*
=================
ServerInfo_MenuDraw
=================
*/
static void ServerInfo_MenuDraw( void )
{
	static char		picname[64];
	int		y;
	int		i;
	
	if ( !s_serverStatusInfo.found && uis.realtime > s_serverStatusInfo.refreshtime ) {
		s_serverStatusInfo.refreshtime = uis.realtime + 500;
		ServerInfo_GetStatus( s_serverinfo.address, &s_serverStatusInfo );

		if ( s_serverStatusInfo.found ) {
			// found the server this time -> update pic
			Com_sprintf( picname, sizeof(picname), "levelshots/%s.tga", s_serverStatusInfo.lines[3][1] );
			s_serverinfo.mappic.generic.name = picname;
			s_serverinfo.mappic.generic.flags &= ~QMF_HIDDEN;
			// force shader update during draw
			s_serverinfo.mappic.shader = 0;
			// show arrows
			s_serverinfo.arrows.generic.flags &= ~QMF_HIDDEN;
			s_serverinfo.up.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
			s_serverinfo.down.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
			// hide wait message
			s_serverinfo.waittext.generic.flags |= QMF_HIDDEN;
		}
	}

	if ( s_serverStatusInfo.found ) {
		int		start;

		y = 64;

		for ( i = 0; i < EXTENDED_INFO; i++ ) {
			UI_DrawString(40,y,s_serverStatusInfo.lines[i][0],UI_LEFT|UI_SMALLFONT,color_red);
			UI_DrawString(120,y,s_serverStatusInfo.lines[i][1],UI_LEFT|UI_SMALLFONT,text_color_normal);
			y += SMALLCHAR_HEIGHT;
		}

		y += SMALLCHAR_HEIGHT;

		start = s_serverinfo.top + EXTENDED_INFO;
		for ( i = start; ( i < s_serverStatusInfo.numLines && i < start + EXTENDED_LINES ); i++ ) {

			if ( i < s_serverStatusInfo.playerLines ) {
				UI_DrawString(200,y,s_serverStatusInfo.lines[i][0],UI_RIGHT|UI_SMALLFONT,color_red);
				UI_DrawString(210,y,s_serverStatusInfo.lines[i][1],UI_LEFT|UI_SMALLFONT,text_color_normal);
			} 
			else if ( i == s_serverStatusInfo.playerLines ) {
				UI_DrawString( 40,y,s_serverStatusInfo.lines[i][0],UI_LEFT|UI_SMALLFONT,color_red);
				UI_DrawString(100,y,s_serverStatusInfo.lines[i][1],UI_LEFT|UI_SMALLFONT,color_red);
				UI_DrawString(300,y,s_serverStatusInfo.lines[i][2],UI_LEFT|UI_SMALLFONT,color_red);
				UI_DrawString(450,y,s_serverStatusInfo.lines[i][3],UI_LEFT|UI_SMALLFONT,color_red);
			} else {
				UI_DrawString( 40,y,s_serverStatusInfo.lines[i][0],UI_LEFT|UI_SMALLFONT,text_color_normal);
				UI_DrawString(100,y,s_serverStatusInfo.lines[i][1],UI_LEFT|UI_SMALLFONT,text_color_normal);
				UI_DrawString(300,y,s_serverStatusInfo.lines[i][2],UI_LEFT|UI_SMALLFONT,text_color_normal);
				UI_DrawString(450,y,s_serverStatusInfo.lines[i][3],UI_LEFT|UI_SMALLFONT,text_color_normal);
			}

			y += SMALLCHAR_HEIGHT;
		}
	}

	Menu_Draw( &s_serverinfo.menu );
}

/*
=================
ServerInfo_MenuKey
=================
*/
static sfxHandle_t ServerInfo_MenuKey( int key )
{
	if( key == K_DOWNARROW ) {
		if ( s_serverinfo.top + EXTENDED_INFO + EXTENDED_LINES < s_serverStatusInfo.numLines ) {
			s_serverinfo.top++;
			return menu_move_sound;
		} else {
			return menu_buzz_sound;
		}
	}

	if( key == K_UPARROW ) {
		if ( s_serverinfo.top > 0 ) {
			s_serverinfo.top--;
			return menu_move_sound;
		} else {
			return menu_buzz_sound;
		}
	}

	return ( Menu_DefaultKey( &s_serverinfo.menu, key ) );
}

/*
=================
ServerInfo_Cache
=================
*/
void ServerInfo_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!serverinfo_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(serverinfo_artlist[i]);
	}
}


/*
=================
UI_ServerInfoMenu
=================
*/
void UI_ServerInfoMenu( const char *serverAddress )
{
	// zero set all our globals
	memset( &s_serverinfo, 0 ,sizeof(serverinfo_t) );
	memset( &s_serverStatusInfo, 0 ,sizeof(serverStatusInfo_t) );

	ServerInfo_Cache();

	s_serverinfo.menu.draw       = ServerInfo_MenuDraw;
	s_serverinfo.menu.key        = ServerInfo_MenuKey;
	s_serverinfo.menu.wrapAround = qtrue;
	s_serverinfo.menu.fullscreen = qtrue;
	s_serverinfo.menu.showlogo   = LOGO_AFTERWARDS;

	s_serverinfo.banner.generic.type  = MTYPE_BTEXT;
	s_serverinfo.banner.generic.x	  = 320;
	s_serverinfo.banner.generic.y	  = 16;
	s_serverinfo.banner.string		  = "SERVER INFO";
	s_serverinfo.banner.color	      = color_white;
	s_serverinfo.banner.style	      = UI_CENTER;

	s_serverinfo.waittext.generic.type	= MTYPE_PTEXT;
	s_serverinfo.waittext.generic.x		= 320;
	s_serverinfo.waittext.generic.y		= 200;
	s_serverinfo.waittext.string		= "Retrieving Server Information...";
	s_serverinfo.waittext.color			= color_orange;
	s_serverinfo.waittext.style			= UI_CENTER;

	s_serverinfo.mappic.generic.type	= MTYPE_BITMAP;
	s_serverinfo.mappic.generic.flags	= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_serverinfo.mappic.generic.x		= 448;
	s_serverinfo.mappic.generic.y		= 64;
	s_serverinfo.mappic.width			= 128;
	s_serverinfo.mappic.height			= 96;
	s_serverinfo.mappic.errorpic		= SERVERINFO_UNKNOWNMAP;
	s_serverinfo.mappic.generic.flags	|= QMF_HIDDEN;

	s_serverinfo.arrows.generic.type		= MTYPE_BITMAP;
	s_serverinfo.arrows.generic.name		= SERVERINFO_ARROWS0;
	s_serverinfo.arrows.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE|QMF_HIDDEN;
	s_serverinfo.arrows.generic.callback	= ServerInfo_Event;
	s_serverinfo.arrows.generic.x			= 512+48;
	s_serverinfo.arrows.generic.y			= 240-16;
	s_serverinfo.arrows.width				= 64;
	s_serverinfo.arrows.height				= 128;

	s_serverinfo.up.generic.type			= MTYPE_BITMAP;
	s_serverinfo.up.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY|QMF_INACTIVE|QMF_HIDDEN;
	s_serverinfo.up.generic.callback		= ServerInfo_Event;
	s_serverinfo.up.generic.id				= ID_SCROLL_UP;
	s_serverinfo.up.generic.x				= 512+48;
	s_serverinfo.up.generic.y				= 240-16;
	s_serverinfo.up.width					= 64;
	s_serverinfo.up.height					= 64;
	s_serverinfo.up.focuspic				= SERVERINFO_ARROWS_UP;

	s_serverinfo.down.generic.type			= MTYPE_BITMAP;
	s_serverinfo.down.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY|QMF_INACTIVE|QMF_HIDDEN;
	s_serverinfo.down.generic.callback		= ServerInfo_Event;
	s_serverinfo.down.generic.id			= ID_SCROLL_DOWN;
	s_serverinfo.down.generic.x				= 512+48;
	s_serverinfo.down.generic.y				= 240+48;
	s_serverinfo.down.width					= 64;
	s_serverinfo.down.height				= 64;
	s_serverinfo.down.focuspic				= SERVERINFO_ARROWS_DOWN;

	s_serverinfo.add.generic.type		= MTYPE_BITMAP;
	s_serverinfo.add.generic.name		= SERVERINFO_ADD0;
	s_serverinfo.add.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_serverinfo.add.generic.callback	= ServerInfo_Event;
	s_serverinfo.add.generic.id			= ID_ADD;
	s_serverinfo.add.generic.x			= 640-128;
	s_serverinfo.add.generic.y			= 480-64;
	s_serverinfo.add.width				= 128;
	s_serverinfo.add.height				= 64;
	s_serverinfo.add.focuspic			= SERVERINFO_ADD1;
	if( trap_Cvar_VariableValue( "sv_running" ) ) {
		s_serverinfo.add.generic.flags |= QMF_GRAYED;
	}

	s_serverinfo.back.generic.type	   = MTYPE_BITMAP;
	s_serverinfo.back.generic.name     = SERVERINFO_BACK0;
	s_serverinfo.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_serverinfo.back.generic.callback = ServerInfo_Event;
	s_serverinfo.back.generic.id	   = ID_BACK;
	s_serverinfo.back.generic.x		   = 0;
	s_serverinfo.back.generic.y		   = 480-64;
	s_serverinfo.back.width  		   = 128;
	s_serverinfo.back.height  		   = 64;
	s_serverinfo.back.focuspic         = SERVERINFO_BACK1;

	s_serverinfo.top = 0;
	// reset requests
	trap_LAN_ServerStatus( NULL, NULL, 0);
	// set new request        
	Q_strncpyz( s_serverinfo.address, serverAddress, MAX_ADDRESSLENGTH );
	// Com_Printf("Getting ServerStatus on Address: %s\n", s_serverinfo.address); 
	ServerInfo_GetStatus( s_serverinfo.address, &s_serverStatusInfo);

	s_serverStatusInfo.found = qfalse;
	s_serverStatusInfo.refreshtime = uis.realtime + 200;

	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.banner );
	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.waittext );
	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.mappic );
	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.arrows );
	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.up );
	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.down );
	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.add );
	Menu_AddItem( &s_serverinfo.menu, (void*) &s_serverinfo.back );

	UI_PushMenu( &s_serverinfo.menu );
}


