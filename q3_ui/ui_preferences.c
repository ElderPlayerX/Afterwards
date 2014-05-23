// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

GAME OPTIONS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"

#define PREFERENCES_X_POS		360

#define ID_CROSSHAIR			127
#define ID_FEWMODELS			129
#define ID_EJECTINGBRASS		130
#define ID_WALLMARKS			131
#define ID_DYNAMICLIGHTS		132
#define ID_ATMOSPHERICEFFECTS	133
#define ID_DETAILEFFECTS		134
#define ID_PREDICTWEAPONS		135
#define ID_IDENTIFYTARGET		136
#define ID_SYNCEVERYFRAME		137
#define ID_FORCEMODEL			138
#define ID_DRAWTEAMOVERLAY		139
#define ID_ALLOWDOWNLOAD		140
#define ID_BACK					141

#define	NUM_CROSSHAIRS			10


typedef struct {
	menuframework_s		menu;

	menutext_s			banner;

	menulist_s			crosshair;
	menuradiobutton_s	fewmodels;
	menuradiobutton_s	brass;
	menuradiobutton_s	wallmarks;
	menuradiobutton_s	dynamiclights;
	menulist_s			atmosphericeffects;
	menulist_s			detaileffects;
	menulist_s			predictweapons;
	menuradiobutton_s	identifytarget;
	menuradiobutton_s	synceveryframe;
	menuradiobutton_s	forcemodel;
	menulist_s			drawteamoverlay;
	menuradiobutton_s	allowdownload;
	menubitmap_s		back;

	qhandle_t			crosshairShader[NUM_CROSSHAIRS];
} preferences_t;

static preferences_t s_preferences;

static const char *atmosphericeffects_names[] =
{
	"off",
	"low detail",
	"high detail",
	0
};

static const char *detaileffects_names[] =
{
	"high",
	"low",
	0
};

static const char *predictweapons_names[] =
{
	"no",
	"partially",
	"full",
	0
};

static const char *teamoverlay_names[] =
{
	"off",
	"upper right",
	"lower right",
	"lower left",
	0
};

static void Preferences_SetMenuItems( void ) {
	s_preferences.crosshair.curvalue			= (int)trap_Cvar_VariableValue( "cg_drawCrosshair" ) % NUM_CROSSHAIRS;
	s_preferences.fewmodels.curvalue			= trap_Cvar_VariableValue( "cg_fewModels" ) != 0;
	s_preferences.brass.curvalue				= trap_Cvar_VariableValue( "cg_brassTime" ) != 0;
	s_preferences.wallmarks.curvalue			= trap_Cvar_VariableValue( "cg_marks" ) != 0;
	s_preferences.identifytarget.curvalue		= trap_Cvar_VariableValue( "cg_drawCrosshairNames" ) != 0;
	s_preferences.dynamiclights.curvalue		= trap_Cvar_VariableValue( "r_dynamiclight" ) != 0;
	s_preferences.atmosphericeffects.curvalue	= Com_Clamp( 0, 2, trap_Cvar_VariableValue( "cg_atmosphericEffects" ) );
	s_preferences.detaileffects.curvalue		= Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_lowDetailEffects" ) );
	s_preferences.predictweapons.curvalue		= Com_Clamp( 0, 2, trap_Cvar_VariableValue( "cg_predictWeapons" ) );
	s_preferences.synceveryframe.curvalue		= trap_Cvar_VariableValue( "r_finish" ) != 0;
	s_preferences.forcemodel.curvalue			= trap_Cvar_VariableValue( "cg_forcemodel" ) != 0;
	s_preferences.drawteamoverlay.curvalue		= Com_Clamp( 0, 3, trap_Cvar_VariableValue( "cg_drawTeamOverlay" ) );
	s_preferences.allowdownload.curvalue		= trap_Cvar_VariableValue( "cl_allowDownload" ) != 0;
}


static void Preferences_Event( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_CROSSHAIR:
		s_preferences.crosshair.curvalue++;
		if( s_preferences.crosshair.curvalue == NUM_CROSSHAIRS ) {
			s_preferences.crosshair.curvalue = 0;
		}
		trap_Cvar_SetValue( "cg_drawCrosshair", s_preferences.crosshair.curvalue );
		break;

	case ID_FEWMODELS:
		trap_Cvar_SetValue( "cg_fewModels", s_preferences.fewmodels.curvalue );
		break;

	case ID_EJECTINGBRASS:
		if ( s_preferences.brass.curvalue )
			trap_Cvar_Reset( "cg_brassTime" );
		else
			trap_Cvar_SetValue( "cg_brassTime", 0 );
		break;

	case ID_WALLMARKS:
		trap_Cvar_SetValue( "cg_marks", s_preferences.wallmarks.curvalue );
		break;

	case ID_DYNAMICLIGHTS:
		trap_Cvar_SetValue( "r_dynamiclight", s_preferences.dynamiclights.curvalue );
		break;		

	case ID_ATMOSPHERICEFFECTS:
		trap_Cvar_SetValue( "cg_atmosphericEffects", s_preferences.atmosphericeffects.curvalue );
		break;

	case ID_DETAILEFFECTS:
		trap_Cvar_SetValue( "cg_lowDetailEffects", s_preferences.detaileffects.curvalue );
		break;

	case ID_PREDICTWEAPONS:
		trap_Cvar_SetValue( "cg_predictWeapons", s_preferences.predictweapons.curvalue );
		break;

	case ID_IDENTIFYTARGET:
		trap_Cvar_SetValue( "cg_drawCrosshairNames", s_preferences.identifytarget.curvalue );
		break;

	case ID_SYNCEVERYFRAME:
		trap_Cvar_SetValue( "r_finish", s_preferences.synceveryframe.curvalue );
		break;

	case ID_FORCEMODEL:
		trap_Cvar_SetValue( "cg_forcemodel", s_preferences.forcemodel.curvalue );
		break;

	case ID_DRAWTEAMOVERLAY:
		trap_Cvar_SetValue( "cg_drawTeamOverlay", s_preferences.drawteamoverlay.curvalue );
		break;

	case ID_ALLOWDOWNLOAD:
		trap_Cvar_SetValue( "cl_allowDownload", s_preferences.allowdownload.curvalue );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
=================
Crosshair_Draw
=================
*/
static void Crosshair_Draw( void *self ) {
	menulist_s	*s;
	float		*color;
	int			x, y;
	int			style;
	qboolean	focus;

	s = (menulist_s *)self;
	x = s->generic.x;
	y =	s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if ( s->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus )
	{
		color = text_color_highlight;
		style |= UI_PULSE;
	}
	else if ( s->generic.flags & QMF_BLINK )
	{
		color = text_color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if ( focus )
	{
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, listbar_color ); 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, style|UI_RIGHT, color );
	if( !s->curvalue ) {
		return;
	}
	UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y - 4, 24, 24, s_preferences.crosshairShader[s->curvalue] );
}


static void Preferences_MenuInit( void ) {
	int				y;

	memset( &s_preferences, 0 ,sizeof(preferences_t) );

	Preferences_Cache();

	s_preferences.menu.wrapAround = qtrue;
	s_preferences.menu.fullscreen = qtrue;
	s_preferences.menu.showlogo = LOGO_AFTERWARDS;

	s_preferences.banner.generic.type  = MTYPE_BTEXT;
	s_preferences.banner.generic.x	   = 320;
	s_preferences.banner.generic.y	   = 16;
	s_preferences.banner.string		   = "GAME OPTIONS";
	s_preferences.banner.color         = color_white;
	s_preferences.banner.style         = UI_CENTER;

	y = 144;
	s_preferences.crosshair.generic.type		= MTYPE_TEXT;
	s_preferences.crosshair.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NODEFAULTINIT|QMF_OWNERDRAW;
	s_preferences.crosshair.generic.x			= PREFERENCES_X_POS;
	s_preferences.crosshair.generic.y			= y;
	s_preferences.crosshair.generic.name		= "Crosshair:";
	s_preferences.crosshair.generic.callback	= Preferences_Event;
	s_preferences.crosshair.generic.ownerdraw	= Crosshair_Draw;
	s_preferences.crosshair.generic.id			= ID_CROSSHAIR;
	s_preferences.crosshair.generic.top			= y - 4;
	s_preferences.crosshair.generic.bottom		= y + 20;
	s_preferences.crosshair.generic.left		= PREFERENCES_X_POS - ( ( strlen(s_preferences.crosshair.generic.name) + 1 ) * SMALLCHAR_WIDTH );
	s_preferences.crosshair.generic.right		= PREFERENCES_X_POS + 48;

	y += BIGCHAR_HEIGHT+2+4;
	s_preferences.fewmodels.generic.type          = MTYPE_RADIOBUTTON;
	s_preferences.fewmodels.generic.name	      = "Save Memory:";
	s_preferences.fewmodels.generic.flags	      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.fewmodels.generic.callback      = Preferences_Event;
	s_preferences.fewmodels.generic.id            = ID_FEWMODELS;
	s_preferences.fewmodels.generic.x	          = PREFERENCES_X_POS;
	s_preferences.fewmodels.generic.y	          = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.wallmarks.generic.type          = MTYPE_RADIOBUTTON;
	s_preferences.wallmarks.generic.name	      = "Marks on Walls:";
	s_preferences.wallmarks.generic.flags	      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.wallmarks.generic.callback      = Preferences_Event;
	s_preferences.wallmarks.generic.id            = ID_WALLMARKS;
	s_preferences.wallmarks.generic.x	          = PREFERENCES_X_POS;
	s_preferences.wallmarks.generic.y	          = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.brass.generic.type              = MTYPE_RADIOBUTTON;
	s_preferences.brass.generic.name	          = "Ejecting Brass:";
	s_preferences.brass.generic.flags	          = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.brass.generic.callback          = Preferences_Event;
	s_preferences.brass.generic.id                = ID_EJECTINGBRASS;
	s_preferences.brass.generic.x	              = PREFERENCES_X_POS;
	s_preferences.brass.generic.y	              = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.dynamiclights.generic.type      = MTYPE_RADIOBUTTON;
	s_preferences.dynamiclights.generic.name	  = "Dynamic Lights:";
	s_preferences.dynamiclights.generic.flags     = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.dynamiclights.generic.callback  = Preferences_Event;
	s_preferences.dynamiclights.generic.id        = ID_DYNAMICLIGHTS;
	s_preferences.dynamiclights.generic.x	      = PREFERENCES_X_POS;
	s_preferences.dynamiclights.generic.y	      = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.atmosphericeffects.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.atmosphericeffects.generic.name		= "Atmospheric Effects:";
	s_preferences.atmosphericeffects.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.atmosphericeffects.generic.callback	= Preferences_Event;
	s_preferences.atmosphericeffects.generic.id			= ID_ATMOSPHERICEFFECTS;
	s_preferences.atmosphericeffects.generic.x			= PREFERENCES_X_POS;
	s_preferences.atmosphericeffects.generic.y			= y;
	s_preferences.atmosphericeffects.itemnames			= atmosphericeffects_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.detaileffects.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.detaileffects.generic.name		= "Effects Detail:";
	s_preferences.detaileffects.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.detaileffects.generic.callback	= Preferences_Event;
	s_preferences.detaileffects.generic.id			= ID_DETAILEFFECTS;
	s_preferences.detaileffects.generic.x			= PREFERENCES_X_POS;
	s_preferences.detaileffects.generic.y			= y;
	s_preferences.detaileffects.itemnames			= detaileffects_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.predictweapons.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.predictweapons.generic.name		= "Weapon Prediction:";
	s_preferences.predictweapons.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.predictweapons.generic.callback	= Preferences_Event;
	s_preferences.predictweapons.generic.id			= ID_PREDICTWEAPONS;
	s_preferences.predictweapons.generic.x			= PREFERENCES_X_POS;
	s_preferences.predictweapons.generic.y			= y;
	s_preferences.predictweapons.itemnames			= predictweapons_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.identifytarget.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.identifytarget.generic.name	  = "Identify Target:";
	s_preferences.identifytarget.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.identifytarget.generic.callback = Preferences_Event;
	s_preferences.identifytarget.generic.id       = ID_IDENTIFYTARGET;
	s_preferences.identifytarget.generic.x	      = PREFERENCES_X_POS;
	s_preferences.identifytarget.generic.y	      = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.synceveryframe.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.synceveryframe.generic.name	  = "Sync Every Frame:";
	s_preferences.synceveryframe.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.synceveryframe.generic.callback = Preferences_Event;
	s_preferences.synceveryframe.generic.id       = ID_SYNCEVERYFRAME;
	s_preferences.synceveryframe.generic.x	      = PREFERENCES_X_POS;
	s_preferences.synceveryframe.generic.y	      = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.forcemodel.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.forcemodel.generic.name	  = "Force Player Models:";
	s_preferences.forcemodel.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.forcemodel.generic.callback = Preferences_Event;
	s_preferences.forcemodel.generic.id       = ID_FORCEMODEL;
	s_preferences.forcemodel.generic.x	      = PREFERENCES_X_POS;
	s_preferences.forcemodel.generic.y	      = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.drawteamoverlay.generic.type     = MTYPE_SPINCONTROL;
	s_preferences.drawteamoverlay.generic.name	   = "Draw Team Overlay:";
	s_preferences.drawteamoverlay.generic.flags	   = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.drawteamoverlay.generic.callback = Preferences_Event;
	s_preferences.drawteamoverlay.generic.id       = ID_DRAWTEAMOVERLAY;
	s_preferences.drawteamoverlay.generic.x	       = PREFERENCES_X_POS;
	s_preferences.drawteamoverlay.generic.y	       = y;
	s_preferences.drawteamoverlay.itemnames			= teamoverlay_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.allowdownload.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.allowdownload.generic.name	   = "Automatic Downloading:";
	s_preferences.allowdownload.generic.flags	   = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.allowdownload.generic.callback = Preferences_Event;
	s_preferences.allowdownload.generic.id       = ID_ALLOWDOWNLOAD;
	s_preferences.allowdownload.generic.x	       = PREFERENCES_X_POS;
	s_preferences.allowdownload.generic.y	       = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.back.generic.type	    = MTYPE_BITMAP;
	s_preferences.back.generic.name     = ART_BACK0;
	s_preferences.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_preferences.back.generic.callback = Preferences_Event;
	s_preferences.back.generic.id	    = ID_BACK;
	s_preferences.back.generic.x		= 0;
	s_preferences.back.generic.y		= 480-64;
	s_preferences.back.width  		    = 128;
	s_preferences.back.height  		    = 64;
	s_preferences.back.focuspic         = ART_BACK1;

	Menu_AddItem( &s_preferences.menu, &s_preferences.banner );

	Menu_AddItem( &s_preferences.menu, &s_preferences.crosshair );
	Menu_AddItem( &s_preferences.menu, &s_preferences.fewmodels );
	Menu_AddItem( &s_preferences.menu, &s_preferences.wallmarks );
	Menu_AddItem( &s_preferences.menu, &s_preferences.brass );
	Menu_AddItem( &s_preferences.menu, &s_preferences.dynamiclights );
	Menu_AddItem( &s_preferences.menu, &s_preferences.atmosphericeffects );
	Menu_AddItem( &s_preferences.menu, &s_preferences.detaileffects );
	Menu_AddItem( &s_preferences.menu, &s_preferences.predictweapons );
	Menu_AddItem( &s_preferences.menu, &s_preferences.identifytarget );
	Menu_AddItem( &s_preferences.menu, &s_preferences.synceveryframe );
	Menu_AddItem( &s_preferences.menu, &s_preferences.forcemodel );
	Menu_AddItem( &s_preferences.menu, &s_preferences.drawteamoverlay );
	Menu_AddItem( &s_preferences.menu, &s_preferences.allowdownload );

	Menu_AddItem( &s_preferences.menu, &s_preferences.back );

	Preferences_SetMenuItems();
}


/*
===============
Preferences_Cache
===============
*/
void Preferences_Cache( void ) {
	int		n;

	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	for( n = 0; n < NUM_CROSSHAIRS; n++ ) {
		s_preferences.crosshairShader[n] = trap_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + n ) );
	}
}


/*
===============
UI_PreferencesMenu
===============
*/
void UI_PreferencesMenu( void ) {
	Preferences_MenuInit();
	UI_PushMenu( &s_preferences.menu );
}
