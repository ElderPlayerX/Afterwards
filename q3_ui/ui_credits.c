// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

CREDITS

=======================================================================
*/


#include "ui_local.h"

#define ART_BACKGROUND		"textures/sfx/aw_terra"
#define ART_BANNER_MODEL	"models/misc/banner.md3"
#define ART_ERASER			"eraserShader"

typedef struct {
	menuframework_s	menu;
	menubitmap_s	background;
	qhandle_t		bannerModel;
	qhandle_t		eraserShader;

	int	startTime;
} creditsmenu_t;

static creditsmenu_t	s_credits;

typedef struct textline_s {
	char		*line;
	int			x, y;
	int			deltaStart, deltaEnd;
} textline_t;

static const textline_t text[] = 
{
	// ****************************
	{
		"Code & Visual Effects",
		600,	300, 
		500,	5000
	},
	{
		"Herby",
		600,	330,
		1200,	4000,
	},
	// ****************************
	{
		"Player & Weapon Models",
		600,	300,
		5000,	5000
	},
	{
		"Rastaman_Bey",
		600,	330,
		1200,	4000,
	},
	{
		"Shoukaiser",
		600,	350,
		800,	4000,
	},
	// ****************************
	{
		"Textures, Skins & 2D Art",
		600,	300,
		5000,	5000
	},
	{
		"Liquid",
		600,	330,
		1200,	4000,
	},
	{
		"Rastaman_Bey",
		600,	350,
		800,	4000,
	},
	{
		"SparX",
		600,	370,
		800,	4000,
	},
	// ****************************
	{
		"All Maps",
		600,	300,
		5000,	5000
	},
	{
		"SparX",
		600,	330,
		1200,	4000,
	},
	// ****************************
	{
		"Webdesign",
		600,	300,
		5000,	5000
	},
	{
		"Ww3",
		600,	330,
		1200,	4000
	},
	{
		"SparX",
		600,	350,
		800,	4000
	},
	// ****************************
	{
		"Additional Textures",
		600,	300,
		5000,	5000
	},
	{
		"The Big House, Shaderlab",
		600,	330,
		1200,	4000
	},
	{
		"Evil Lair, Iikka 'Fingers'",
		600,	350,
		800,	4000
	},
	{
		"Robshack, Gonnakillya",
		600,	370,
		800,	4000
	},
	{
		"Diggedagg, Lunaran",
		600,	390,
		800,	4000
	},
	// ****************************
	{
		"Special Thanks To",
		600,	300,
		5000,	5000
	},
	{
		"ID & Planetside Software",
		600,	330,
		1200,	4000
	},
	{
		"The Q3F Team & Neil Toronto",
		600,	350,
		800,	4000
	},
	{
		"Code 3 Arena",
		600,	370,
		800,	4000
	},
	{
		"The Beta Testers",
		600,	390,
		800,	4000
	},
	{
		"All who have supported us",
		600,	410,
		800,	4000
	},
	// ****************************
	{
		"That's all Folks!",
		600,	300,
		10000,	300000
	},
	{
		"Really !!!",
		600,	330,
		60000,	10000
	},
	{
		"You don't believe it, don't you?",
		600,	330,
		60000,	10000
	},
	{
		"This really is a waste of time!",
		600,	330,
		60000,	10000
	},
	{
		"You seem to be one of the",
		600,	330,
		120000,	10000
	},
	{
		"really hard ones.",
		600,	350,
		1000,	10000
	},
	{
		"You now sit in front of that",
		600,	330,
		240000,	10000
	},
	{
		"screen for 10 minutes!",
		600,	350,
		1000,	10000
	},
	{
		"You're crazy!",
		600,	370,
		20000,	10000
	},
	{
		"Shut me down, please !!!",
		600,	330,
		300000,	-1
	},
	// ****************************
	{
		NULL,
		0,	0,
		0,	0
	}
};


/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	return 0;
}

#define FIRST_TIME 300
#define DELTA_TIME 50
#define ERASER_TIME 70

/*
===============
UI_DrawTextLines
===============
*/
static void UI_DrawTextLines( void ) {
	const textline_t *curLine;
	int time;
	int startTime, endTime;

	UI_DrawProportionalString( 320, 100, "Thanks for playing Afterwards. Visit us at:", UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW, color_orange );
	UI_DrawProportionalString( 320, 125, "http://www.planetquake.com/afterwards", UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW, color_orange );
	
	time = uis.realtime - s_credits.startTime;
	curLine = &text[0];
	startTime = 0;
	endTime = 0;
	while ( curLine->line ) {

		startTime += curLine->deltaStart;
		endTime = startTime + curLine->deltaEnd;

		if ( time > startTime && (time < endTime || endTime == -1 ) ) {
			const char *s;
			char drawstr[2];
			int ch;
			int i;
			float x,y;
		
			// draw one string
			x = curLine->x - UI_ProportionalStringWidth( curLine->line ) * PROP_SMALL_SIZE_SCALE;
			i = 0;

			drawstr[1] = *"\0";
			s = curLine->line;
			while ( *s ) {
				ch = *s & 127;		
				drawstr[0] = *s;

				if ( time < startTime + FIRST_TIME + i*DELTA_TIME) {
					y = curLine->y + (500-curLine->y) * ( FIRST_TIME - (time - startTime - i*DELTA_TIME) ) / FIRST_TIME;
				} else if ( time < endTime - (strlen(curLine->line)-i) * ERASER_TIME || endTime == -1 ) {
					y = curLine->y;
				} else {
					x += (propMap[ch][2] + PROP_GAP_WIDTH) * PROP_SMALL_SIZE_SCALE;
					s++;
					i++;
					continue;
				}

				UI_DrawProportionalString( (int)x, (int)y, drawstr, UI_SMALLFONT|UI_DROPSHADOW, color_orange );

				x += (propMap[ch][2] + PROP_GAP_WIDTH) * PROP_SMALL_SIZE_SCALE;
				s++;
				i++;
			}
			// draw eraserShader
			if ( time > endTime - strlen(curLine->line)*ERASER_TIME ) {				
				x = curLine->x - UI_ProportionalStringWidth( curLine->line ) * PROP_SMALL_SIZE_SCALE;
				x += ( time - endTime + strlen(curLine->line)*ERASER_TIME ) * UI_ProportionalStringWidth( curLine->line ) * PROP_SMALL_SIZE_SCALE / (strlen(curLine->line)*ERASER_TIME);
				y = curLine->y;
				trap_R_DrawStretchPic( (x-20)*uis.scale, (y-20)*uis.scale, 60*uis.scale, 60*uis.scale, 0, 0, 1, 1, s_credits.eraserShader );
			}
		}

		curLine++;
	}
}

/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_CreditMenu_Draw( void ) {
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
	h = 100;
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
	ent.hModel = s_credits.bannerModel;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	// standard menu drawing
	Menu_Draw( &s_credits.menu );
	trap_R_RenderScene( &refdef );

	UI_DrawTextLines();
	

//	UI_DrawProportionalString( (int)(620 - UI_ProportionalStringWidth( "SparX" ) * PROP_SMALL_SIZE_SCALE), 370, "SparX", UI_SMALLFONT, color_orange );
}

/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu( void ) {
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
//	s_credits.menu.showlogo = LOGO_AFTERWARDS;
	s_credits.startTime = uis.realtime;

	trap_R_RegisterShaderNoMip( ART_BACKGROUND );
	s_credits.bannerModel = trap_R_RegisterModel( ART_BANNER_MODEL );
	s_credits.eraserShader = trap_R_RegisterShaderNoMip( ART_ERASER );

	s_credits.background.generic.type		= MTYPE_BITMAP;
	s_credits.background.generic.name		= ART_BACKGROUND;
	s_credits.background.generic.flags		= QMF_INACTIVE;
	s_credits.background.generic.x			= 0;
	s_credits.background.generic.y			= 0;
	s_credits.background.width  			= 640;
	s_credits.background.height  			= 480;	

	Menu_AddItem( &s_credits.menu, &s_credits.background );

	UI_PushMenu ( &s_credits.menu );
}
