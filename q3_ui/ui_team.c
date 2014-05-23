// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// ui_team.c
//

#include "ui_local.h"

#define ID_JOINGAME		100
#define ID_SPECTATE		101


typedef struct
{
	menuframework_s	menu;
	menutext_s		joingame;
	menutext_s		spectate;
} teammain_t;

static teammain_t	s_teammain;

static menuframework_s	s_teammain_menu;
static menuaction_s		s_teammain_orders;
static menuaction_s		s_teammain_voice;
static menuaction_s		s_teammain_joingame;
static menuaction_s		s_teammain_spectate;


/*
===============
TeamMain_MenuEvent
===============
*/
static void TeamMain_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_JOINGAME:
		trap_Cmd_ExecuteText( EXEC_APPEND, "cmd team free\n" );
		UI_ForceMenuOff();
		break;

	case ID_SPECTATE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "cmd team spectator\n" );
		UI_ForceMenuOff();
		break;
	}
}


/*
===============
UI_TeamMain_MenuDraw
===============
*/
void UI_TeamMain_MenuDraw( void ) {
	// set frame dimensions
	trap_Cvar_Set( "cg_frameWidth", "320" );
	trap_Cvar_Set( "cg_frameHeight", "210" );

	// standard menu drawing
	Menu_Draw( &s_teammain.menu );
}


/*
===============
TeamMain_MenuInit
===============
*/
void TeamMain_MenuInit( void ) {
	int		y;

	memset( &s_teammain, 0, sizeof(s_teammain) );

	TeamMain_Cache();

	s_teammain.menu.wrapAround = qtrue;
	s_teammain.menu.fullscreen = qfalse;
	s_teammain.menu.draw = UI_TeamMain_MenuDraw;

	y = 194;

	s_teammain.joingame.generic.type     = MTYPE_PTEXT;
	s_teammain.joingame.generic.flags    = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_teammain.joingame.generic.id       = ID_JOINGAME;
	s_teammain.joingame.generic.callback = TeamMain_MenuEvent;
	s_teammain.joingame.generic.x        = 320;
	s_teammain.joingame.generic.y        = y;
	s_teammain.joingame.string           = "JOIN GAME";
	s_teammain.joingame.style            = UI_CENTER|UI_SMALLFONT;
	s_teammain.joingame.color            = cg_color;
	y += 20;

	s_teammain.spectate.generic.type     = MTYPE_PTEXT;
	s_teammain.spectate.generic.flags    = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_teammain.spectate.generic.id       = ID_SPECTATE;
	s_teammain.spectate.generic.callback = TeamMain_MenuEvent;
	s_teammain.spectate.generic.x        = 320;
	s_teammain.spectate.generic.y        = y;
	s_teammain.spectate.string           = "SPECTATE";
	s_teammain.spectate.style            = UI_CENTER|UI_SMALLFONT;
	s_teammain.spectate.color            = cg_color;
	y += 20;

	Menu_AddItem( &s_teammain.menu, (void*) &s_teammain.joingame );
	Menu_AddItem( &s_teammain.menu, (void*) &s_teammain.spectate );
}


/*
===============
TeamMain_Cache
===============
*/
void TeamMain_Cache( void ) {
}


/*
===============
UI_TeamMainMenu
===============
*/
void UI_TeamMainMenu( void ) {
	TeamMain_MenuInit();
	UI_PushMenu ( &s_teammain.menu );
}
