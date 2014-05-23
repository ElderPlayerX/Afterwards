// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_JOINGAME				10
#define ID_STARTSERVER			11
#define ID_SETUP				12
#define ID_DEMOS				13
#define ID_MODS					14
#define ID_EXIT					15

#define MAIN_BACKGROUND_LEFT			"menu/art/mainback1"
#define MAIN_BACKGROUND_RIGHT			"menu/art/mainback2"
#define MAIN_BANNER_MODEL				"models/misc/banner.md3"
#define MAIN_MENU_VERTICAL_SPACING		34


typedef struct {
	menuframework_s	menu;

	menubitmap_s	backleft;
	menubitmap_s	backright;
	menutext_s		joingame;
	menutext_s		startserver;
	menutext_s		setup;
	menutext_s		demos;
	menutext_s		mods;
	menutext_s		exit;

	qhandle_t		bannerModel;
} mainmenu_t;


static mainmenu_t s_main;


/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_JOINGAME:
		UI_ArenaServersMenu();
		break;

	case ID_STARTSERVER:
		UI_StartServerMenu( qtrue );
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_MODS:
		UI_ModsMenu();
		break;

	case ID_EXIT:
		UI_ConfirmMenu( "EXIT GAME?", NULL, MainMenu_ExitAction );
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
	trap_R_RegisterModel( MAIN_BACKGROUND_LEFT );
	trap_R_RegisterModel( MAIN_BACKGROUND_RIGHT );
	s_main.bannerModel = trap_R_RegisterModel( MAIN_BANNER_MODEL );
}


/*
===============
Main_MenuDraw
===============
*/
static void Main_MenuDraw( void ) {
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	float			adjust;
	float			x, y, w, h;
	vec4_t			color = {0.5, 0, 0, 1};

	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	x = 0;
	y = 0;
	w = 640;
	h = 120;
	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	
	adjust = 0; // JDC: Kenneth asked me to stop this  1.0 * sin( (float)uis.realtime / 1000 );
	refdef.fov_x = 70 + adjust;
	refdef.fov_y = 19.6875 + adjust;

	refdef.time = uis.realtime;

	origin[0] = 300;
	origin[1] = 0;
	origin[2] = -32;

	trap_R_ClearScene();

	// add the model

	memset( &ent, 0, sizeof(ent) );

	adjust = 20.0 * sin( (float)uis.realtime / 3000 );
	VectorSet( angles, 0, 180 + adjust, 0 );
	AnglesToAxis( angles, ent.axis );
	ent.hModel = s_main.bannerModel;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	// standard menu drawing
	Menu_Draw( &s_main.menu );

	trap_R_RenderScene( &refdef );

/*	if (uis.demoversion) {
		UI_DrawProportionalString( 320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER|UI_SMALLFONT, color );
		UI_DrawString( 320, 400, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	} else {
		UI_DrawString( 320, 450, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	}*/
}


/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, "missionpack") == 0) {
			return qtrue;
		}
    dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
	qboolean teamArena = qfalse;
	int		style = UI_LEFT | UI_CURSORSHADOW;

	trap_Cvar_Set( "sv_killserver", "1" );

	if( !uis.demoversion && !ui_cdkeychecked.integer ) {
		char	key[17];

		trap_GetCDKey( key, sizeof(key) );
		if( trap_VerifyCDKey( key, NULL ) == qfalse ) {
			UI_CDKeyMenu();
			return;
		}
	}

	memset( &s_main, 0 ,sizeof(mainmenu_t) );

	MainMenu_Cache();

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;

	s_main.backleft.generic.type	= MTYPE_BITMAP;
	s_main.backleft.generic.name	= MAIN_BACKGROUND_LEFT;
	s_main.backleft.generic.flags	= QMF_INACTIVE;
	s_main.backleft.generic.x		= 0;  
	s_main.backleft.generic.y		= 120;
	s_main.backleft.width			= 320;
	s_main.backleft.height			= 360;

	s_main.backright.generic.type	= MTYPE_BITMAP;
	s_main.backright.generic.name	= MAIN_BACKGROUND_RIGHT;
	s_main.backright.generic.flags	= QMF_INACTIVE;
	s_main.backright.generic.x		= 320;  
	s_main.backright.generic.y		= 120;
	s_main.backright.width			= 320;
	s_main.backright.height			= 360;

	y = 160;
	s_main.joingame.generic.type			= MTYPE_PTEXT;
	s_main.joingame.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.joingame.generic.x				= 20;
	s_main.joingame.generic.y				= y;
	s_main.joingame.generic.id				= ID_JOINGAME;
	s_main.joingame.generic.callback		= Main_MenuEvent; 
	s_main.joingame.string					= "JOIN GAME";
	s_main.joingame.color					= color_orange;
	s_main.joingame.style					= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.startserver.generic.type			= MTYPE_PTEXT;
	s_main.startserver.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.startserver.generic.x			= 20;
	s_main.startserver.generic.y			= y;
	s_main.startserver.generic.id			= ID_STARTSERVER;
	s_main.startserver.generic.callback		= Main_MenuEvent; 
	s_main.startserver.string				= "START SERVER";
	s_main.startserver.color				= color_orange;
	s_main.startserver.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= 20;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_orange;
	s_main.setup.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= 20;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= color_orange;
	s_main.demos.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.mods.generic.type				= MTYPE_PTEXT;
	s_main.mods.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.mods.generic.x					= 20;
	s_main.mods.generic.y					= y;
	s_main.mods.generic.id					= ID_MODS;
	s_main.mods.generic.callback			= Main_MenuEvent; 
	s_main.mods.string						= "MODS";
	s_main.mods.color						= color_orange;
	s_main.mods.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= 20;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_orange;
	s_main.exit.style						= style;

	Menu_AddItem( &s_main.menu,	&s_main.backleft );
	Menu_AddItem( &s_main.menu,	&s_main.backright );
	Menu_AddItem( &s_main.menu,	&s_main.joingame );
	Menu_AddItem( &s_main.menu,	&s_main.startserver );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.demos );
	Menu_AddItem( &s_main.menu,	&s_main.mods );
	Menu_AddItem( &s_main.menu,	&s_main.exit );             

	uis.addToFavorites = qfalse;
	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
}
