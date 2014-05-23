// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"

/*
================
CG_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CG_AdjustFrom640( float *x, float *y, float *w, float *h ) {
#if 0
	// adjust for wide screens
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
		*x += 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * 640 / 480 ) );
	}
#endif
	// scale for screen sizes
	*x *= cgs.screenXScale;
	*y *= cgs.screenYScale;
	*w *= cgs.screenXScale;
	*h *= cgs.screenYScale;
}

/*
================
CG_GetCvarColor

Converts a cvar rgb value to a vec4_t color
=================
*/
#define COLOR_TEXTBUF_LENGTH	20

void CG_GetCvarColor( const char *var_name, vec4_t *color ) {
	char	textbuf[COLOR_TEXTBUF_LENGTH];

	// get cvar value
	trap_Cvar_VariableStringBuffer( var_name, textbuf, COLOR_TEXTBUF_LENGTH );
	sscanf( textbuf, "%f %f %f", &(*color)[0], &(*color)[1], &(*color)[2] );
	(*color)[3] = 1.0f;
}


/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenXScale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenYScale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}


/*
================
CG_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  CG_DrawTopBottom(x, y, width, height, size);
  CG_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}


/*
================
CG_DrawFrame

Coordinates are 640*480 virtual values
=================
*/

#define HUD_EDGESIZE	6
#define HUD_BRIGHTSIZE	3

void CG_DrawFrame( float x, float y, float width, float height ) {
	float	left, top, right, bottom, dewidth, deheight, dlwidth, dlheight;
	vec4_t	color;

	trap_R_SetColor( NULL );

	left   = x + HUD_EDGESIZE;
	top    = y + HUD_EDGESIZE;
	right  = x + width - HUD_EDGESIZE;
	bottom = y + height - HUD_EDGESIZE;
	dewidth  = HUD_EDGESIZE;
	deheight = HUD_EDGESIZE;
	dlwidth  = width  - 2 * HUD_EDGESIZE;
	dlheight = height - 2 * HUD_EDGESIZE;
	
	CG_AdjustFrom640( &x, &y, &width, &height );
	CG_AdjustFrom640( &left, &top, &right, &bottom );
	CG_AdjustFrom640( &dewidth, &deheight, &dlwidth, &dlheight );

	// draw edges
	trap_R_DrawStretchPic( x,     y,      dewidth, deheight, 0, 0, 1, 1, cgs.media.hudEdge[0] );
	trap_R_DrawStretchPic( right, y,      dewidth, deheight, 0, 0, 1, 1, cgs.media.hudEdge[1] );
	trap_R_DrawStretchPic( right, bottom, dewidth, deheight, 0, 0, 1, 1, cgs.media.hudEdge[2] );
	trap_R_DrawStretchPic( x,     bottom, dewidth, deheight, 0, 0, 1, 1, cgs.media.hudEdge[3] );

	// draw frame lines
	trap_R_DrawStretchPic( left,  y,      dlwidth, deheight, 0, 0, 1, 1, cgs.media.hudLine[0] );
	trap_R_DrawStretchPic( right, top,    dewidth, dlheight, 0, 0, 1, 1, cgs.media.hudLine[1] );
	trap_R_DrawStretchPic( left,  bottom, dlwidth, deheight, 0, 0, 1, 1, cgs.media.hudLine[2] );
	trap_R_DrawStretchPic( x,     top,    dewidth, dlheight, 0, 0, 1, 1, cgs.media.hudLine[3] );

	// draw bright lines
	color[0] = cg.color0[0];
	color[1] = cg.color0[1];
	color[2] = cg.color0[2];
	color[3] = 0.70f;
	trap_R_SetColor( color );

	right -= HUD_BRIGHTSIZE * cgs.screenXScale;
	bottom -= HUD_BRIGHTSIZE * cgs.screenYScale;
	dlheight -= 2 * HUD_BRIGHTSIZE * cgs.screenYScale;
	dewidth = HUD_BRIGHTSIZE * cgs.screenXScale;
	deheight = HUD_BRIGHTSIZE * cgs.screenYScale;

	trap_R_DrawStretchPic( left,  top,    dlwidth, deheight, 0, 0, 1, 1, cgs.media.hudFill );
	trap_R_DrawStretchPic( left,  bottom, dlwidth, deheight, 0, 0, 1, 1, cgs.media.hudFill );
	top += HUD_BRIGHTSIZE * cgs.screenYScale;
	trap_R_DrawStretchPic( left,  top,    dewidth, dlheight, 0, 0, 1, 1, cgs.media.hudFill );
	trap_R_DrawStretchPic( right, top,    dewidth, dlheight, 0, 0, 1, 1, cgs.media.hudFill );
	left += HUD_BRIGHTSIZE * cgs.screenXScale;
	dlwidth -= 2 * HUD_BRIGHTSIZE * cgs.screenXScale;

	// fill frame
	color[3] = 0.30f;
	trap_R_SetColor( color );

	trap_R_DrawStretchPic( left, top, dlwidth, dlheight, 0, 0, 1, 1, cgs.media.hudFill );

	// reset color
	trap_R_SetColor( NULL );
}


/*
================
CG_DrawFrameActiveRect

Coordinates are 640*480 virtual values
=================
*/

void CG_DrawFrameActiveRect( float x, float y, float width, float height ) {
	float	left, top, right, bottom, dewidth, deheight, dlwidth, dlheight;
	vec4_t	color;

	trap_R_SetColor( NULL );

	left     = x + HUD_EDGESIZE;
	top      = y + HUD_EDGESIZE;
	right    = x + width - HUD_EDGESIZE - HUD_BRIGHTSIZE;
	bottom   = y + height - HUD_EDGESIZE - HUD_BRIGHTSIZE;
	dewidth  = HUD_BRIGHTSIZE;
	deheight = HUD_BRIGHTSIZE;
	dlwidth  = width  - 2 * HUD_EDGESIZE;
	dlheight = height - 2 * HUD_EDGESIZE - 2 * HUD_BRIGHTSIZE;
	
	CG_AdjustFrom640( &x, &y, &width, &height );
	CG_AdjustFrom640( &left, &top, &right, &bottom );
	CG_AdjustFrom640( &dewidth, &deheight, &dlwidth, &dlheight );

	// draw bright lines
	color[0] = color[1] = color[2] = 1.0f;
	color[3] = 0.70f;
	trap_R_SetColor( color );

	trap_R_DrawStretchPic( left,  top,    dlwidth, deheight,  0, 0, 1, 1, cgs.media.hudFill );
	trap_R_DrawStretchPic( left,  bottom, dlwidth, deheight,  0, 0, 1, 1, cgs.media.hudFill );
	top += HUD_BRIGHTSIZE * cgs.screenYScale;
	trap_R_DrawStretchPic( left,  top,    dewidth, dlheight, 0, 0, 1, 1, cgs.media.hudFill );
	trap_R_DrawStretchPic( right, top,    dewidth, dlheight, 0, 0, 1, 1, cgs.media.hudFill );
	left += HUD_BRIGHTSIZE * cgs.screenXScale;
	dlwidth -= 2 * HUD_BRIGHTSIZE * cgs.screenXScale;

	// fill frame
	color[3] = 0.20f;
	trap_R_SetColor( color );

	trap_R_DrawStretchPic( left, top, dlwidth, dlheight, 0, 0, 1, 1, cgs.media.hudFill );

	// reset color
	trap_R_SetColor( NULL );
}


/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}



/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
void CG_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	ax = x;
	ay = y;
	aw = width;
	ah = height;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	trap_R_DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cgs.media.charsetShader );
}


/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, 
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars ) {
	vec4_t		color;
	const char	*s;
	int			xx;
	int			cnt;

	if (maxChars <= 0)
		maxChars = 32767; // do them all!

	// draw the drop shadow
	if (shadow) {
		color[0] = color[1] = color[2] = 0;
		color[3] = setColor[3];
		trap_R_SetColor( color );
		s = string;
		xx = x;
		cnt = 0;
		while ( *s && cnt < maxChars) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			}
			CG_DrawChar( xx + 2, y + 2, charWidth, charHeight, *s );
			cnt++;
			xx += charWidth;
			s++;
		}
	}

	// draw the colored text
	s = string;
	xx = x;
	cnt = 0;
	trap_R_SetColor( setColor );
	while ( *s && cnt < maxChars) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				trap_R_SetColor( color );
			}
			s += 2;
			continue;
		}
		CG_DrawChar( xx, y, charWidth, charHeight, *s );
		xx += charWidth;
		cnt++;
		s++;
	}
	trap_R_SetColor( NULL );
}

void CG_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawBigNarrowString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qtrue, BIGCHAR_WIDTH-6, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawSmallString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float	s1, t1, s2, t2;

	s1 = x/64.0;
	t1 = y/64.0;
	s2 = (x+w)/64.0;
	t2 = (y+h)/64.0;
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}



/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void ) {
	int		top, bottom, left, right;
	int		w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( cg.refdef.x == 0 && cg.refdef.y == 0 && 
		cg.refdef.width == w && cg.refdef.height == h ) {
		return;		// full screen rendering
	}

	top = cg.refdef.y;
	bottom = top + cg.refdef.height-1;
	left = cg.refdef.x;
	right = left + cg.refdef.width-1;

	// clear above view screen
	CG_TileClearBox( 0, 0, w, top, cgs.media.backTileShader );

	// clear below view screen
	CG_TileClearBox( 0, bottom, w, h - bottom, cgs.media.backTileShader );

	// clear left of view screen
	CG_TileClearBox( 0, top, left, bottom - top + 1, cgs.media.backTileShader );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );
}



/*
================
CG_FadeColor
================
*/
float *CG_FadeColor( int startMsec, int totalMsec ) {
	static vec4_t		color;
	int			t;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME ) {
		color[3] = ( totalMsec - t ) * 1.0/FADE_TIME;
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	return color;
}


/*
================
CG_TeamColor
================
*/
float *CG_TeamColor( int team ) {
	static vec4_t	red = {1, 0.2f, 0.2f, 1};
	static vec4_t	blue = {0.2f, 0.2f, 1, 1};
	static vec4_t	other = {1, 1, 1, 1};
	static vec4_t	spectator = {0.7f, 0.7f, 0.7f, 1};

	switch ( team ) {
	case TEAM_RED:
		return red;
	case TEAM_BLUE:
		return blue;
	case TEAM_SPECTATOR:
		return spectator;
	case TEAM_RED_SPECTATOR:
		return red;
	case TEAM_BLUE_SPECTATOR:
		return blue;
	default:
		return other;
	}
}



/*
=================
CG_GetColorForHealth
=================
*/
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor ) {
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if ( health <= 0 ) {
		VectorClear( hcolor );	// black
		hcolor[3] = 1;
		return;
	}
	count = armor;
	max = health * ARMOR_PROTECTION / ( 1.0 - ARMOR_PROTECTION );
	if ( max < count ) {
		count = max;
	}
	health += count;

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if ( health >= 100 ) {
		hcolor[2] = 1.0;
	} else if ( health < 66 ) {
		hcolor[2] = 0;
	} else {
		hcolor[2] = ( health - 66 ) / 33.0;
	}

	if ( health > 60 ) {
		hcolor[1] = 1.0;
	} else if ( health < 30 ) {
		hcolor[1] = 0;
	} else {
		hcolor[1] = ( health - 30 ) / 30.0;
	}
}



/*
=================
UI_DrawProportionalString2
=================
*/
static int	propMap[128][3] = {
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, PROP_SPACE_WIDTH},		// SPACE
{11, 122, 7},	// !
{154, 181, 14},	// "
{55, 122, 17},	// #
{79, 122, 18},	// $
{101, 122, 23},	// %
{153, 122, 18},	// &
{9, 93, 7},		// '
{207, 122, 8},	// (
{230, 122, 9},	// )
{177, 122, 18},	// *
{30, 152, 18},	// +
{85, 181, 7},	// ,
{34, 93, 11},	// -
{110, 181, 6},	// .
{130, 152, 14},	// /

{74, 64, 16},	// 0
{94, 64, 5},	// 1
{103, 64, 16},	// 2
{123, 64, 16},	// 3
{142, 64, 17},	// 4
{161, 64, 16},	// 5
{180, 64, 16},	// 6
{199, 64, 16},	// 7
{220, 64, 16},	// 8
{239, 64, 17},	// 9
{59, 181, 7},	// :
{35,181, 7},	// ;
{203, 152, 14},	// <
{56, 93, 14},	// =
{228, 152, 14},	// >
{177, 181, 18},	// ?

{28, 122, 22},	// @
{6, 4, 22},		// A
{32, 4, 18},	// B
{54, 4, 18},	// C
{75, 4, 18},	// D
{97, 4, 18},	// E
{118, 4, 18},	// F
{139, 4, 19},	// G
{162, 4, 18},	// H
{185, 4, 5},	// I
{194, 4, 12},	// J
{211, 4, 17},	// K
{231, 4, 17},	// L

{2, 34, 23},	// M
{29, 34, 21},	// N
{54, 34, 21},	// O
{77, 34, 18},	// P
{99, 34, 20},	// Q
{123, 34, 18},	// R
{143, 34, 19},	// S
{164, 34, 21},	// T
{187, 34, 18},	// U
{207, 34, 20},	// V
{228, 34, 27},	// W

{4, 64, 20},	// X
{27, 64, 21},	// Y
{50, 64, 21},	// Z

{60, 152, 7},	// [
{106, 151, 13},	// '\'
{83, 152, 7},	// ]
{128, 122, 17},	// ^
{4, 152, 21},	// _

{134, 181, 5},	// '
{6, 4, 22},		// A
{32, 4, 18},	// B
{54, 4, 18},	// C
{75, 4, 18},	// D
{97, 4, 18},	// E
{118, 4, 18},	// F
{139, 4, 19},	// G
{162, 4, 18},	// H
{185, 4, 5},	// I
{194, 4, 12},	// J
{211, 4, 17},	// K
{231, 4, 17},	// L

{2, 34, 23},	// M
{29, 34, 21},	// N
{54, 34, 21},	// O
{77, 34, 18},	// P
{99, 34, 20},	// Q
{123, 34, 18},	// R
{143, 34, 19},	// S
{164, 34, 21},	// T
{187, 34, 18},	// U
{207, 34, 20},	// V
{228, 34, 27},	// W

{4, 64, 20},	// X
{27, 64, 21},	// Y
{50, 64, 21},	// Z
{153, 152, 13},	// {
{11, 181, 5},	// |
{180, 152, 13},	// }
{79, 93, 17},	// ~
{0, 0, -1}		// DEL
};

static int propMapB[26][3] = {
{11, 12, 33},
{49, 12, 31},
{85, 12, 31},
{120, 12, 30},
{156, 12, 21},
{183, 12, 21},
{207, 12, 32},

{13, 55, 30},
{49, 55, 13},
{66, 55, 29},
{101, 55, 31},
{135, 55, 21},
{158, 55, 40},
{204, 55, 32},

{12, 97, 31},
{48, 97, 31},
{82, 97, 30},
{118, 97, 30},
{153, 97, 30},
{185, 97, 25},
{213, 97, 30},

{11, 139, 32},
{42, 139, 51},
{93, 139, 32},
{126, 139, 31},
{158, 139, 25},
};

#define PROPB_GAP_WIDTH		2
#define PROPB_SPACE_WIDTH	12
#define PROPB_HEIGHT		36

/*
=================
UI_DrawBannerString
=================
*/
static void UI_DrawBannerString2( int x, int y, const char* str, vec4_t color )
{
	const char* s;
	char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x * cgs.screenXScale + cgs.screenXBias;
	ay = y * cgs.screenXScale;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* cgs.screenXScale;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2] * cgs.screenXScale;
			ah = (float)PROPB_HEIGHT * cgs.screenXScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, cgs.media.charsetPropB );
			ax += (aw + (float)PROPB_GAP_WIDTH * cgs.screenXScale);
		}
		s++;
	}

	trap_R_SetColor( NULL );
}

void UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color ) {
	const char *	s;
	int				ch;
	int				width;
	vec4_t			drawcolor;

	// find the width of the drawn text
	s = str;
	width = 0;
	while ( *s ) {
		ch = *s;
		if ( ch == ' ' ) {
			width += PROPB_SPACE_WIDTH;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
		}
		s++;
	}
	width -= PROPB_GAP_WIDTH;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			x -= width / 2;
			break;

		case UI_RIGHT:
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawBannerString2( x+2, y+2, str, drawcolor );
	}

	UI_DrawBannerString2( x, y, str, color );
}


int UI_ProportionalStringWidth( const char* str ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;

	s = str;
	width = 0;
	while ( *s ) {
		ch = *s & 127;
		charWidth = propMap[ch][2];
		if ( charWidth != -1 ) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
		s++;
	}

	width -= PROP_GAP_WIDTH;
	return width;
}

static void UI_DrawProportionalString2( int x, int y, const char* str, vec4_t color, float sizeScale, qboolean glow )
{
	const char* s;
	char	ch;
	float	sx;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;
	float	gap;
	vec4_t	fontColor;
	vec4_t	glowColor;

	if ( glow ) {
		fontColor[0] = color[0] * 0.5f;
		fontColor[1] = color[1] * 0.5f;
		fontColor[2] = color[2] * 0.5f;
		fontColor[3] = color[3];

		glowColor[0] = color[0];
		glowColor[1] = color[1];
		glowColor[2] = color[2];
		glowColor[3] = 0.5 + 0.5 * sin( (float)cg.time / PULSE_DIVISOR );
	} else {
		fontColor[0] = color[0];
		fontColor[1] = color[1];
		fontColor[2] = color[2];
		fontColor[3] = color[3];
	}

	ax = x * cgs.screenXScale + cgs.screenXBias;
	ay = y * cgs.screenXScale;
	sx = ax;
	gap = (float)PROP_GAP_WIDTH * cgs.screenXScale * sizeScale;

	if ( glow ) {
		trap_R_SetColor( glowColor );
		trap_R_DrawStretchPic( ax - (float)PROPB_SPACE_WIDTH * cgs.screenXScale * sizeScale, ay, 
			(float)PROPB_SPACE_WIDTH * cgs.screenXScale * sizeScale, (float)PROP_HEIGHT * cgs.screenXScale * sizeScale, 0, 0, 1, 1, cgs.media.fontGlowEnd );
	}

	trap_R_SetColor( fontColor );

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			aw = (float)PROP_SPACE_WIDTH * cgs.screenXScale * sizeScale;
			ah = (float)PROP_HEIGHT * cgs.screenXScale * sizeScale;
		}
		else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * cgs.screenXScale * sizeScale;
			ah = (float)PROP_HEIGHT * cgs.screenXScale * sizeScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, cgs.media.charsetProp );
		} else aw = 0;

		ax += (aw + gap);
		s++;
	}

	if ( glow ) {
		trap_R_SetColor( glowColor );
		trap_R_DrawStretchPic( sx, ay, ax - sx, (float)PROP_HEIGHT * cgs.screenXScale * sizeScale, 0, 0, 1, 1, cgs.media.fontGlow );
		trap_R_DrawStretchPic( ax - gap * 0.5, ay, (float)PROPB_SPACE_WIDTH * cgs.screenXScale * sizeScale, (float)PROP_HEIGHT * cgs.screenXScale * sizeScale, 1, 0, 0, 1, cgs.media.fontGlowEnd );
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale( int style ) {
	if ( style & UI_SMALLFONT ) {
		return PROP_SMALL_SIZE_SCALE;
	} else if ( style & UI_TINYFONT ) {
		return PROP_TINY_SIZE_SCALE;
	}


	return 1.00;
}

#define		LIGHT_Z		100.0f
#define		FONT_Z		6.0f

/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color ) {
	vec4_t	drawcolor;
	int		width;
	float	sizeScale;

	sizeScale = UI_ProportionalSizeScale( style );

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0.0f;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x+2, y+2, str, drawcolor, sizeScale, qfalse );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, qfalse );
		return;
	}

	UI_DrawProportionalString2( x, y, str, color, sizeScale, style & UI_PULSE );
}
