// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

uiStatic_t		uis;
qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound
vec4_t			cg_color;

qboolean bkInitialized = qfalse;


// these are here so the functions in q_shared.c can link
#ifndef UI_HARD_LINKED

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	trap_Error( va("%s", text) );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	trap_Print( va("%s", text) );
}

#endif

/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

/*
=================
UI_StartDemoLoop
=================
*/
void UI_StartDemoLoop( void ) {
	trap_Cmd_ExecuteText( EXEC_APPEND, "d1\n" );
}

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu( menuframework_s *menu )
{
	int				i;
	menucommon_s*	item;

	// avoid stacking menus invoked by hotkeys
	for (i=0 ; i<uis.menusp ; i++)
	{
		if (uis.stack[i] == menu)
		{
			uis.menusp = i;
			break;
		}
	}

	if (i == uis.menusp)
	{
		if (uis.menusp >= MAX_MENUDEPTH)
			trap_Error("UI_PushMenu: menu stack overflow");

		uis.stack[uis.menusp++] = menu;
	}

	uis.activemenu = menu;

	// default cursor position
	menu->cursor      = 0;
	menu->cursor_prev = 0;

	m_entersound = qtrue;

	trap_Key_SetCatcher( KEYCATCH_UI );

	// force first available item to have focus
	for (i=0; i<menu->nitems; i++)
	{
		item = (menucommon_s *)menu->items[i];
		if (!(item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE)))
		{
			menu->cursor_prev = -1;
			Menu_SetCursor( menu, i );
			break;
		}
	}

	uis.firstdraw = qtrue;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu (void)
{
	trap_S_StartLocalSound( menu_out_sound, CHAN_LOCAL_SOUND );

	uis.menusp--;

	if (uis.menusp < 0)
		trap_Error ("UI_PopMenu: menu stack underflow");

	if (uis.menusp) {
		uis.activemenu = uis.stack[uis.menusp-1];
		uis.firstdraw = qtrue;
	}
	else {
		UI_ForceMenuOff ();
	}
}

void UI_ForceMenuOff (void)
{
	uis.menusp     = 0;
	uis.activemenu = NULL;

	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
	trap_Key_ClearStates();
	trap_Cvar_Set( "cl_paused", "0" );
	trap_Cvar_Set( "cg_frameWidth", "0" );
	trap_Cvar_Set( "cg_frameHeight", "0" );
}

/*
=================
UI_LerpColor
=================
*/
void UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
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
	
	ax = x * uis.scale + uis.bias;
	ay = y * uis.scale;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* uis.scale;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2] * uis.scale;
			ah = (float)PROPB_HEIGHT * uis.scale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, uis.charsetPropB );
			ax += (aw + (float)PROPB_GAP_WIDTH * uis.scale);
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

static void UI_DrawProportionalString2( float x, float y, const char* str, vec4_t color, float sizeScale, qboolean glow )
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
		glowColor[3] = 0.5 + 0.5 * sin( (float)uis.realtime / PULSE_DIVISOR );
	} else {
		fontColor[0] = color[0];
		fontColor[1] = color[1];
		fontColor[2] = color[2];
		fontColor[3] = color[3];
	}

	ax = x * uis.scale + uis.bias;
	ay = y * uis.scale;
	sx = ax;
	gap = (float)PROP_GAP_WIDTH * uis.scale * sizeScale;

	if ( glow ) {
		trap_R_SetColor( glowColor );
		trap_R_DrawStretchPic( ax - (float)PROPB_SPACE_WIDTH * uis.scale * sizeScale, ay, 
			(float)PROPB_SPACE_WIDTH * uis.scale * sizeScale, (float)PROP_HEIGHT * uis.scale * sizeScale, 0, 0, 1, 1, uis.fontGlowEnd );
	}

	trap_R_SetColor( fontColor );

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			aw = (float)PROP_SPACE_WIDTH * uis.scale * sizeScale;
			ah = (float)PROP_HEIGHT * uis.scale * sizeScale;
		}
		else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * uis.scale * sizeScale;
			ah = (float)PROP_HEIGHT * uis.scale * sizeScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, uis.charsetProp );
		}

		ax += (aw + gap);
		s++;
	}

	if ( glow ) {
		trap_R_SetColor( glowColor );
		trap_R_DrawStretchPic( sx, ay, ax - sx, (float)PROP_HEIGHT * uis.scale * sizeScale, 0, 0, 1, 1, uis.fontGlow );
		trap_R_DrawStretchPic( ax, ay, (float)PROPB_SPACE_WIDTH * uis.scale * sizeScale, (float)PROP_HEIGHT * uis.scale * sizeScale, 1, 0, 0, 1, uis.fontGlowEnd );
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

#define		LIGHT_Z				100.0f
#define		FONT_Z				3.0f

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
		drawcolor[3] = color[3] * 0.5;
		UI_DrawProportionalString2( x+2, y+2, str, drawcolor, sizeScale, qfalse );
	}

	if ( style & UI_CURSORSHADOW ) {
		float	stretch;
		float	newX, newY;

		stretch = LIGHT_Z / (LIGHT_Z - FONT_Z);
		newX = (float)uis.cursorx + (x - uis.cursorx) * stretch;
		newY = (float)uis.cursory + (y - uis.cursory) * stretch;

		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0.0f;
		drawcolor[3] = color[3] * 0.5;
		UI_DrawProportionalString2( newX, newY, str, drawcolor, sizeScale * stretch, qfalse );
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

/*
=================
UI_DeleteColorCodes
=================
*/
static void UI_DeleteColorCodes( char* str )
{
	char* sFrom;
	char* sTo;

	sFrom = str;
	sTo = str;
	while ( *sFrom )
	{
		if ( Q_IsColorString( sFrom ) )
		{
			sFrom += 2;
			continue;
		}

		*sTo = *sFrom;
		
		sFrom++;
		sTo++;
	}

	*sTo = *sFrom;
}

/*
=================
UI_DrawString2
=================
*/
static void UI_DrawString2( float x, float y, const char* str, vec4_t color, int charw, int charh )
{
	const char* s;
	char	ch;
	int forceColor = qfalse; //APSFIXME;
	vec4_t	tempcolor;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;

	if (y < -charh)
		// offscreen
		return;

	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x * uis.scale + uis.bias;
	ay = y * uis.scale;
	aw = charw * uis.scale;
	ah = charh * uis.scale;

	s = str;
	while ( *s )
	{
		if ( Q_IsColorString( s ) )
		{
			if ( !forceColor )
			{
				memcpy( tempcolor, g_color_table[ColorIndex(s[1])], sizeof( tempcolor ) );
				tempcolor[3] = color[3];
				trap_R_SetColor( tempcolor );
			}
			s += 2;
			continue;
		}

		ch = *s & 255;
		if (ch != ' ')
		{
			frow = (ch>>4)*0.0625;
			fcol = (ch&15)*0.0625;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charset );
		}

		ax += aw;
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, const char* str, int style, vec4_t color )
{
	int		len;
	int		charw;
	int		charh;
	vec4_t	newcolor;
	vec4_t	lowlight;
	float	*drawcolor;
	vec4_t	dropcolor;

	if( !str ) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

	if (style & UI_SMALLFONT)
	{
		charw =	SMALLCHAR_WIDTH;
		charh =	SMALLCHAR_HEIGHT;
	}
	else if (style & UI_GIANTFONT)
	{
		charw =	GIANTCHAR_WIDTH;
		charh =	GIANTCHAR_HEIGHT;
	}
	else
	{
		charw =	BIGCHAR_WIDTH;
		charh =	BIGCHAR_HEIGHT;
	}

	if (style & UI_PULSE)
	{
		lowlight[0] = 0.8*color[0]; 
		lowlight[1] = 0.8*color[1];
		lowlight[2] = 0.8*color[2];
		lowlight[3] = 0.8*color[3];
		UI_LerpColor(color,lowlight,newcolor,0.5+0.5*sin((float)uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}	
	else
		drawcolor = color;

	switch (style & UI_FORMATMASK)
	{
		case UI_CENTER:
			// center justify at x
			len = strlen(str);
			x   = x - len*charw/2;
			break;

		case UI_RIGHT:
			// right justify at x
			len = strlen(str);
			x   = x - len*charw;
			break;

		default:
			// left justify at x
			break;
	}

	if ( style & UI_DROPSHADOW )
	{
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2(x+2,y+2,str,dropcolor,charw,charh);
	}

	if ( style & UI_CURSORSHADOW ) {
		float	stretch;
		float	newX, newY;

		stretch = LIGHT_Z / (LIGHT_Z - FONT_Z);
		newX = (float)uis.cursorx + (x - uis.cursorx) * stretch;
		newY = (float)uis.cursory + (y - uis.cursory) * stretch;

		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0.0f;
		dropcolor[3] = drawcolor[3] * 0.5;
		UI_DrawString2( newX, newY, str, dropcolor, charw*stretch, charh*stretch );
	}

	UI_DrawString2( x, y, str, drawcolor, charw, charh );
}

/*
=================
UI_DrawChar
=================
*/
void UI_DrawChar( int x, int y, int ch, int style, vec4_t color )
{
	char	buff[2];

	buff[0] = ch;
	buff[1] = '\0';

	UI_DrawString( x, y, buff, style, color );
}

qboolean UI_IsFullscreen( void ) {
	if ( !uis.memoryOK ) {
		return qtrue;
	}

	if ( uis.activemenu && ( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return uis.activemenu->fullscreen;
	}

	return qfalse;
}

static void NeedCDAction( qboolean result ) {
	if ( !result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
}

static void NeedCDKeyAction( qboolean result ) {
	if ( !result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
}


void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	Menu_Cache();

	switch ( menu ) {
	case UIMENU_NONE:
		UI_ForceMenuOff();
		return;
	case UIMENU_MAIN:
		UI_MainMenu();
		return;
	case UIMENU_NEED_CD:
		UI_ConfirmMenu( "Insert the CD", NULL, NeedCDAction );
		return;
	case UIMENU_BAD_CD_KEY:
		UI_ConfirmMenu( "Bad CD Key", NULL, NeedCDKeyAction );
		return;
	case UIMENU_INGAME:
		trap_Cvar_Set( "cl_paused", "1" );
		trap_Cvar_Set( "cg_frameWidth", "300" );
		trap_Cvar_Set( "cg_frameHeight", "340" );
		UI_GetCvarColor( "cg_color1", &cg_color );
		UI_InGameMenu();
		return;
	}
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, int down ) {
	sfxHandle_t		s;

	if (!uis.memoryOK) {
		// quit if on memory info screen
		trap_Cvar_Set( "fs_game", "baseq3" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "writeconfig q3config.cfg;vid_restart;exec q3config.cfg;writeconfig q3config.cfg;quit;" );
		return;
	}

	if (!uis.activemenu) {
		return;
	}

	if (!down) {
		return;
	}

	if (uis.activemenu->key)
		s = uis.activemenu->key( key );
	else
		s = Menu_DefaultKey( uis.activemenu, key );

	if ((s > 0) && (s != menu_null_sound))
		trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int dx, int dy )
{
	int				i;
	menucommon_s*	m;

	if (!uis.activemenu)
		return;

	// update mouse screen position
	uis.cursorx += dx;
	if (uis.cursorx < 0)
		uis.cursorx = 0;
	else if (uis.cursorx > SCREEN_WIDTH)
		uis.cursorx = SCREEN_WIDTH;

	uis.cursory += dy;
	if (uis.cursory < 0)
		uis.cursory = 0;
	else if (uis.cursory > SCREEN_HEIGHT)
		uis.cursory = SCREEN_HEIGHT;

	// region test the active menu items
	for (i=0; i<uis.activemenu->nitems; i++)
	{
		m = (menucommon_s*)uis.activemenu->items[i];

		if (m->flags & (QMF_GRAYED|QMF_INACTIVE))
			continue;

		if ((uis.cursorx < m->left) ||
			(uis.cursorx > m->right) ||
			(uis.cursory < m->top) ||
			(uis.cursory > m->bottom))
		{
			// cursor out of item bounds
			continue;
		}

		// set focus to item at cursor
		if (uis.activemenu->cursor != i)
		{
			Menu_SetCursor( uis.activemenu, i );
			((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if ( !(((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT ) ) {
				trap_S_StartLocalSound( menu_move_sound, CHAN_LOCAL_SOUND );
			}
		}

		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}  

	if (uis.activemenu->nitems > 0) {
		// out of any region
		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
	}
}

char *UI_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}


/*
=================
UI_Cache
=================
*/
void UI_Cache_f( void ) {
	MainMenu_Cache();
	InGame_Cache();
	ConfirmMenu_Cache();
	PlayerModel_Cache();
	PlayerSettings_Cache();
	Controls_Cache();
	Demos_Cache();
	UI_CinematicsMenu_Cache();
	Preferences_Cache();
	ServerInfo_Cache();
	SpecifyServer_Cache();
	ArenaServers_Cache();
	StartServer_Cache();
	ServerOptions_Cache();
	DriverInfo_Cache();
	GraphicsOptions_Cache();
	UI_DisplayOptionsMenu_Cache();
	UI_SoundOptionsMenu_Cache();
	UI_NetworkOptionsMenu_Cache();
	UI_SPLevelMenu_Cache();
	UI_SPSkillMenu_Cache();
	UI_SPPostgameMenu_Cache();
	TeamMain_Cache();
	UI_AddBots_Cache();
	UI_RemoveBots_Cache();
	UI_SetupMenu_Cache();
//	UI_LoadConfig_Cache();
//	UI_SaveConfigMenu_Cache();
	UI_BotSelectMenu_Cache();
	UI_CDKeyMenu_Cache();
	UI_ModsMenu_Cache();

}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}


/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	char	*cmd;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	Menu_Cache();

	if ( Q_stricmp (cmd, "levelselect") == 0 ) {
		UI_SPLevelMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "postgame") == 0 ) {
		UI_SPPostgameMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cinematics") == 0 ) {
		UI_CinematicsMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) {
		UI_TeamOrdersMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "iamacheater") == 0 ) {
		UI_SPUnlock_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "iamamonkey") == 0 ) {
		UI_SPUnlockMedals_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cdkey") == 0 ) {
		UI_CDKeyMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "addfavorite") == 0 ) {
		char *name;
		char *addr;		
		int res;

		name = ConcatArgs( 1 );
		addr = ConcatArgs( 2 );
		res = trap_LAN_AddServer(AS_FAVORITES, name, addr);

		if ( res > 0 ) Com_Printf( "Added favorite server %s\n", addr);
		else Com_Printf( "Failed to add server %s to favorites\n", addr);

		return qtrue;
	}

	return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
	trap_LAN_SaveCachedServers();
}

/*
=================
UI_CheckMemorySettings
=================
*/
void UI_CheckMemorySettings( void ) {
	float		hunkMegs, zoneMegs, soundMegs;

	// get values
	hunkMegs = trap_Cvar_VariableValue( "com_hunkMegs" );
	zoneMegs = trap_Cvar_VariableValue( "com_zoneMegs" );
	soundMegs = trap_Cvar_VariableValue( "com_soundMegs" );
	uis.memoryOK = qtrue;

	if ( hunkMegs < MIN_HUNKMEGS ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "set com_hunkMegs %i\n", MIN_HUNKMEGS ) );
		Com_Printf( S_COLOR_YELLOW "com_hunkMegs changed from %i to %i\n", (int)hunkMegs, MIN_HUNKMEGS );
		uis.memoryOK = qfalse;
	}
	if ( zoneMegs < MIN_ZONEMEGS ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "set com_zoneMegs %i\n", MIN_ZONEMEGS ) );
		Com_Printf( S_COLOR_YELLOW "com_zoneMegs changed from %i to %i\n", (int)zoneMegs, MIN_ZONEMEGS );
		uis.memoryOK = qfalse;
	}
	if ( soundMegs < MIN_SOUNDMEGS ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "set com_soundMegs %i\n", MIN_SOUNDMEGS ) );
		Com_Printf( S_COLOR_YELLOW "com_soundMegs changed from %i to %i\n", (int)soundMegs, MIN_SOUNDMEGS );
		uis.memoryOK = qfalse;
	}

	if ( !uis.memoryOK ) {
		Com_Printf( S_COLOR_YELLOW "Memory configuration changed! Please restart the game.\n" );

		// cache some art for the info screen
		uis.charset			= trap_R_RegisterShaderNoMip( "gfx/2d/bigchars" );
		uis.menuBackLandscape = trap_R_RegisterShaderNoMip( "textures/sfx/aw_terra" );
	}
}

/*
=================
UI_Init
=================
*/
void UI_Init( void ) {
	// cache redundant calulations
	trap_GetGlconfig( &uis.glconfig );

	// prepare uis structure
	// memset( &uis, 0, sizeof(uiStatic_t) );

	// for 640x480 virtualized screen
	uis.scale = uis.glconfig.vidHeight * (1.0/480.0);
	if ( uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640 ) {
		// wide screen
		uis.bias = 0.5 * ( uis.glconfig.vidWidth - ( uis.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else {
		// no wide screen
		uis.bias = 0;
	}

	// check memory settings first
	UI_CheckMemorySettings();
	if ( !uis.memoryOK ) {
		return;
	}

	// init UI
	UI_RegisterCvars();

	UI_InitGameinfo();

	// for high detail athmosperic effects
	trap_Cvar_Set("r_maxpolys","3000");
	trap_Cvar_Set("r_maxpolyverts","15000");
	
	// initialize the menu system
	Menu_Cache();

	trap_LAN_LoadCachedServers();

	uis.activemenu = NULL;
	uis.menusp     = 0;
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// expect valid pointers
	*x = *x * uis.scale + uis.bias;
	*y *= uis.scale;
	*w *= uis.scale;
	*h *= uis.scale;
}

void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	hShader = trap_R_RegisterShaderNoMip( picname );
	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );

	trap_R_DrawStretchPic( x, y, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y, 1, height, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y + height - 1, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x + width - 1, y, 1, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

void UI_SetColor( const float *rgba ) {
	trap_R_SetColor( rgba );
}

void UI_UpdateScreen( void ) {
	trap_UpdateScreen();
}


/*
=================
Particle Effects Defs
=================
*/
#define P_SIZE				64
#define CLIP_DIST			100
#define	MAX_BKPARTICLES		200
#define MIN_GUSTTIME		500
#define MAX_GUSTTIME		8000
#define MIN_GUSTFORCE		20.0f
#define	MAX_GUSTFORCE		280.0f
#define MIN_RESISTANCE		1.0f
#define MAX_RESISTANCE		5.5f
#define MIN_CURSORDIST		15.0f
#define MAX_CURSORFORCE		360.0f
#define START_DRIFT			15.0f
#define START_SPEED			40.0f

typedef struct {
	int		type;
	float	x, y;
	float	dx, dy;
	float	driftx, drifty;
	float	resistance;
} bkParticle_t;

bkParticle_t	bkParticle[MAX_BKPARTICLES];
int		newGustTime, oldGustTime;
float	newGustAngle, oldGustAngle;
float	newGustForce, oldGustForce;
float	gustAngle, gustForce;

/*
=================
UI_ResetGust
=================
*/
void UI_ResetGust( void ) {
	// save old values
	oldGustTime = uis.realtime;
	oldGustForce = newGustForce;
	oldGustAngle = newGustAngle;

	// initialize new gust
	newGustTime = uis.realtime + MIN_GUSTTIME + (MAX_GUSTTIME - MIN_GUSTTIME) * random();
	newGustForce = MIN_GUSTFORCE + (MAX_GUSTFORCE - MIN_GUSTFORCE) * random();
	newGustAngle = 2 * M_PI * random();
}


/*
=================
UI_InitBackground
=================
*/
void UI_InitBackground( void ) {
	bkParticle_t	*p;
	int		i;

	// initialize
	bkInitialized = qtrue;
	UI_ResetGust();
	UI_ResetGust();

	// reset particles
	for ( i=0; i < MAX_BKPARTICLES; i++ ) {
		p = &bkParticle[i];

		p->x = (640+2*P_SIZE) * random() - P_SIZE;
		p->y = (480+2*P_SIZE) * random() - P_SIZE;
		p->dx = START_SPEED * crandom();
		p->dy = START_SPEED * crandom();
		p->driftx = START_DRIFT * crandom();
		p->drifty = START_DRIFT * crandom();
		p->resistance = MIN_RESISTANCE + (MAX_RESISTANCE - MIN_RESISTANCE) * random();
	}
}


/*
=================
UI_InterpolateGust
=================
*/
void UI_InterpolateGust( void ) {
	float	factor;

	factor = (float)(uis.realtime - oldGustTime) / (newGustTime - oldGustTime);
	gustAngle = oldGustAngle + (newGustAngle - oldGustAngle) * factor;
	gustForce = oldGustForce + (newGustForce - oldGustForce) * factor;
}


/*
=================
UI_ResetParticle
=================
*/
void UI_ResetParticle( bkParticle_t *p ) {
	float	posRandom;
	
	// set a random monitor edge position
	posRandom = (2*640+2*480+8*P_SIZE) * random();
	if ( posRandom < 640+2*P_SIZE ) {
		p->x = posRandom - P_SIZE;
		p->y = - P_SIZE;
	} else if ( posRandom < 2*640+4*P_SIZE ) {
		p->x = posRandom - P_SIZE - (640+2*P_SIZE);
		p->y = 480 + P_SIZE;
	} else if ( posRandom < 480+2*P_SIZE + 2*640+4*P_SIZE ) {
		p->x = - P_SIZE;
		p->y = posRandom - P_SIZE - (2*640+4*P_SIZE);
	} else {
		p->x = 640 + P_SIZE;
		p->y = posRandom - P_SIZE - (480+2*P_SIZE + 2*640+4*P_SIZE);
	}

	p->dx = START_SPEED * crandom();
	p->dy = START_SPEED * crandom();

	p->driftx = START_DRIFT * crandom();
	p->drifty = START_DRIFT * crandom();

	p->resistance = MIN_RESISTANCE + (MAX_RESISTANCE - MIN_RESISTANCE) * random();
}


/*
=================
UI_DrawBackground
=================
*/
void UI_DrawBackground( void ) {
	bkParticle_t	*p;
	float	time;
	float	dist;
	int		i;

	// draw the background
	UI_DrawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackStd1 );
	trap_R_DrawStretchPic( 0, 0, 640 * uis.scale, 480 * uis.scale, 0, 0, 8, 6, uis.menuBackStd2 );

	// gust management
	if ( uis.realtime - uis.lastBkDrawTime > 500 || !bkInitialized ) {
		UI_InitBackground();
	} else if ( uis.realtime > newGustTime ) {
		UI_ResetGust();
	}
	UI_InterpolateGust();

	uis.lastBkDrawTime = uis.realtime;
	time = uis.frametime / 1000.0f;
	
	// move and draw the particles
	for ( i=0; i < MAX_BKPARTICLES; i++ ) {
		p = &bkParticle[i];

		// check for leaving view
		if ( p->x < - CLIP_DIST || p->y < - CLIP_DIST || p->x > 640 + CLIP_DIST || p->y > 480 + CLIP_DIST ) {
			UI_ResetParticle( p );
		}

		// calculate new speed
		p->dx -= p->dx * p->resistance * time;
		p->dy -= p->dy * p->resistance * time;
		p->dx += gustForce * cos( gustAngle ) * time;
		p->dy += gustForce * sin( gustAngle ) * time;

		dist = MIN_CURSORDIST + (p->x-uis.cursorx)*(p->x-uis.cursorx) + (p->y-uis.cursory)*(p->y-uis.cursory);
		p->dx += (p->x-uis.cursorx) * MAX_CURSORFORCE / dist * MIN_CURSORDIST * time;
		p->dy += (p->y-uis.cursory) * MAX_CURSORFORCE / dist * MIN_CURSORDIST * time;

		// calculate new pos
		p->x += (p->dx + p->driftx) * time;
		p->y += (p->dy + p->drifty) * time;

		// draw particle
		UI_DrawHandlePic( p->x - P_SIZE, p->y - P_SIZE, 2*P_SIZE, 2*P_SIZE, uis.menuBackFog );
	}
}


/*
=================
UI_Refresh
=================
*/
void UI_Refresh( int realtime )
{
	if ( !uis.memoryOK ) {
		// check for exit of infoscreen
		if ( !uis.realtime ) uis.realtime = realtime;
		if ( realtime > uis.realtime + 10000 ) {
			trap_Cvar_Set( "fs_game", "baseq3" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "writeconfig q3config.cfg;vid_restart;exec q3config.cfg;writeconfig q3config.cfg;quit;" );
			return;
		}

		// draw info screen		
		UI_DrawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackLandscape );

		UI_DrawProportionalString( 320, 100, "Memory Configuration changed!", UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW, color_orange );
		UI_DrawProportionalString( 320, 125, "Please restart the game.", UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW, color_orange );
		return;
	}

	uis.frametime = realtime - uis.realtime;
	uis.realtime  = realtime;

	if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return;
	}

	UI_UpdateCvars();

	if ( uis.activemenu )
	{
		if (uis.activemenu->fullscreen)
		{
			// draw the background
			switch (uis.activemenu->showlogo) {
			case LOGO_NONE:
				UI_DrawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackNoLogoShader );
				break;
			case LOGO_AFTERWARDS:
				UI_DrawBackground();
				break;
			case LOGO_LANDSCAPE:
				UI_DrawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackLandscape );
				break;
			}
		}

		if (uis.activemenu->draw)
			uis.activemenu->draw();
		else
			Menu_Draw( uis.activemenu );

		if( uis.firstdraw ) {
			UI_MouseEvent( 0, 0 );
			uis.firstdraw = qfalse;
		}
	}

	// draw cursor
	UI_SetColor( NULL );
	UI_DrawHandlePic( uis.cursorx-2, uis.cursory-2, 4, 4, uis.cursor );
	UI_DrawHandlePic( uis.cursorx-32, uis.cursory-32, 64, 64, uis.cursorCorona );

#ifndef NDEBUG
	if (uis.debug)
	{
		// cursor coordinates
		UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		trap_S_StartLocalSound( menu_in_sound, CHAN_LOCAL_SOUND );
		m_entersound = qfalse;
	}
}

void UI_DrawTextBox (int x, int y, int width, int lines)
{
	UI_FillRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorBlack );
	UI_DrawRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorWhite );
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uis.cursorx < x ||
		uis.cursory < y ||
		uis.cursorx > x+width ||
		uis.cursory > y+height)
		return qfalse;

	return qtrue;
}


#define COLOR_TEXTBUF_LENGTH	20
void UI_GetCvarColor( const char *var_name, vec4_t *color ) {
	char	textbuf[COLOR_TEXTBUF_LENGTH];

	// get cvar value
	trap_Cvar_VariableStringBuffer( var_name, textbuf, COLOR_TEXTBUF_LENGTH );
	sscanf( textbuf, "%f %f %f", &(*color)[0], &(*color)[1], &(*color)[2] );
	(*color)[3] = 1.0f;
}
