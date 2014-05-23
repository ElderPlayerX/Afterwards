// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

static int			loadingPlayerIconCount;
static int			loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];


/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int		n;
	int		x, y;

	for( n = 0; n < loadingPlayerIconCount; n++ ) {
		x = 16 + n * 58;
		y = 300;
		CG_DrawPic( x, y, 48, 48, loadingPlayerIcons[n] );
	}

	for( n = 0; n < loadingItemIconCount; n++ ) {
		y = 360 + 40 * (int)(n/8);
		x = 16 + n % 8 * 76;
		CG_DrawPic( x, y, 64, 32, loadingItemIcons[n] );
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t		*item;

	item = &bg_itemlist[itemNum];
	
	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip( item->icon );
	}

	CG_LoadingString( item->pickup_name );
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			*skin;
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = Q_strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/aw_players/%s/icon_%s.tga", model, skin );
		
		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/aw_players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	Q_CleanStr( personality );

	CG_LoadingString( personality );
}

/*
===================
CG_LoadingAwModel
===================
*/
void CG_LoadingAwModel( int modelNum ) {
	char			personality[MAX_QPATH];
	char			iconName[MAX_QPATH];
	modelslist_t	*ml;

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {

		ml = &(bg_modelslist[modelNum]);

		if( ml->head[0] == '*' ) {
			Com_sprintf( iconName, sizeof( iconName ), "models/aw_players/heads/%s/icon_%s.tga", &(ml->head[1]), ml->headSkin );
		} else if( ml->head[0] == '&' ) {
			Com_sprintf( iconName, sizeof( iconName ), "models/aw_players/%s/icon_%s.tga", ml->model, ml->headSkin );
		} else {
			Com_sprintf( iconName, sizeof( iconName ), "models/aw_players/%s/icon_%s.tga", ml->head, ml->headSkin );
		}

		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/aw_players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( personality, bg_modelslist[modelNum].name, MAX_QPATH );

	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	const char	*mapname;
	int			y;
	int			value;
	qhandle_t	background;
	qhandle_t	levelshot;
	qhandle_t	rahmen;
	char		buf[1024];
	qboolean	local;

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	trap_R_SetColor( NULL );
	// draw background
	background = trap_R_RegisterShaderNoMip( "menubacklandscape" );
	CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, background );

	// draw level screenshot
	mapname = Info_ValueForKey( info, "mapname" );
	levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", mapname ) );
	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}
	rahmen = trap_R_RegisterShaderNoMip( "gfx/2d/select" );
	CG_DrawPic( 397, 54, 222, 166, rahmen );
	CG_DrawPic( 400, 57, 216, 160, levelshot );

	// blend a detail texture over it
	// detail = trap_R_RegisterShader( "levelShotDetail" );
	// trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 2.5, 2, detail );

	// draw the icons of things as they are loaded
	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		UI_DrawProportionalString( 20, 54, "Loading...",
			UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
		UI_DrawProportionalString( 20, 80, cg.infoScreenText,
			UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
	} else {
		UI_DrawProportionalString( 20, 54, "Awaiting snapshot...",
			UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
	}

	// draw info string information
	y = 118;

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		UI_DrawProportionalString( 20, y, s,
			UI_TINYFONT, colorWhite );
		y += 18;
	}

	// game type
	switch ( cgs.gametype ) {
	case GT_TACTICAL:
		s = "Tactical";
		break;
	case GT_DM:
		s = "Deathmatch";
		break;
	default:
		s = "Unknown Gametype";
		break;
	}
	UI_DrawProportionalString( 20, y, s,
		UI_TINYFONT, colorWhite );
	y += 18;

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		UI_DrawProportionalString( 20, y, "CHEATS ARE ENABLED",
			UI_TINYFONT, colorWhite );
		y += 18;
	}

	// friendly fire setting
	if ( cgs.gametype == GT_TACTICAL ) {
		s = Info_ValueForKey( info, "g_friendlyFire" );
		if ( s[0] == '0' ) {
			UI_DrawProportionalString( 20, y, "Friendly Fire - OFF",
				UI_TINYFONT, colorWhite );
		} else if ( s[0] == '1' ) {
			UI_DrawProportionalString( 20, y, "Friendly Fire - LIMITED",
				UI_TINYFONT, colorWhite );
		} else {
			UI_DrawProportionalString( 20, y, "Friendly Fire - FULL",
				UI_TINYFONT, colorWhite );
		}
		y += 18;
	}

	value = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( value ) {
		UI_DrawProportionalString( 20, y, va( "timelimit %i", value ),
			UI_TINYFONT, colorWhite );
		y += 18;
	}

	if ( cgs.gametype == GT_DM ) {
		value = atoi( Info_ValueForKey( info, "fraglimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 20, y, va( "fraglimit %i", value ),
				UI_TINYFONT, colorWhite );
			y += 18;
		}
	}

	if ( cgs.gametype == GT_TACTICAL ) {
		value = atoi( Info_ValueForKey( info, "winlimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 20, y, va( "winlimit %i", value ),
				UI_TINYFONT, colorWhite );
			y += 18;
		}
	}

	// extra space
	y += 10;		

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	local = !atoi( buf );
	if ( local ) {
		// pure server
		s = Info_ValueForKey( sysInfo, "sv_pure" );
		if ( s[0] == '1' ) {
			UI_DrawProportionalString( 20, y, "Pure Server",
				UI_TINYFONT, colorWhite );
			y += 18;
		}

		s = Info_ValueForKey( info, "g_delagWeapons" );
		if ( s[0] == '1' ) {
			UI_DrawProportionalString( 20, y, "Lag Compensated",
				UI_TINYFONT, colorWhite );
			y += 18;
		}

		// server hostname
		Q_strncpyz(buf, Info_ValueForKey( info, "sv_hostname" ), 1024);
		Q_CleanStr(buf);
		UI_DrawProportionalString( 20, y, buf,
			UI_TINYFONT, colorWhite );
		y += 18;

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			UI_DrawProportionalString( 20, y, s,
				UI_TINYFONT, colorWhite );
			y += 18;
		}

		// some extra space after hostname and motd
		//y += 10;
	}
}

