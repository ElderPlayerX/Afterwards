//
// cg_awmenu.c -- afterwards tactical ingame menus
//

#include "cg_local.h"


#define SPACER_LASTLINE		0
#define SPACER_STANDARD		40

// design flags
#define DF_LINE_TOP			0x00000001
#define DF_LINE_BOTTOM		0x00000002
#define DF_BRIGHT			0x00000010

// to change submenu definitions: look at cg_local.h

typedef enum {
	AM_NONE,
	AM_SPACER,
	AM_TEXT,
	AM_SUBMENU,
	AM_SPECTATOR,
	AM_AUTOJOIN,
	AM_EXIT,
	AM_MODEL,
	AM_WEAPON,
	AM_WP_DEACTIVATED
} item_type_t;

typedef struct awMenu_s {
	int				select_num;
	item_type_t		item_type;
	int				item_num;
	int				design;
	char			*item_text;
	char			*caption;
} awMenu_t;

awMenu_t	awMenu[10];

int	InitializedSubMenu = 0;


#define MI_FILELENGTH	10000
#define MI_FRAMES		10
#define MI_SHADERS		10
#define MI_LINES		20
#define MI_LINELENGTH	60

typedef enum {
	L_TEXT,
	L_IMG,
	L_BKIMG
} line_type_t;

typedef struct miFrame_s {
	int		x, y, w, h, ypos;
	int		num_lines;
	char	line[MI_LINES][MI_LINELENGTH];
	int		line_type[MI_LINES];
	int		line_tag[MI_LINES];
	int		line_tx[MI_LINES];
	int		line_ty[MI_LINES];
	int		line_tw[MI_LINES];
	int		line_th[MI_LINES];
} miFrame_t;

miFrame_t	miFrame[MI_FRAMES];
qhandle_t	miShader[MI_SHADERS];


char	text[MI_FILELENGTH];
const char	*info;


/*
==========================
CG_ReadText
==========================
*/

int CG_ReadText( int z, int len, int fn ) {
	int	ln;
	int	lz;
	int lc;

	// leave the first blank or newline
	while ( z <= len && text[z] != '<' ) {
		if ( text[z] == '[' ) return z;
		z++;
	}
	z++;

	ln = miFrame[fn].num_lines;
	miFrame[fn].line_type[ln] = L_TEXT;
	lz = 0;
	lc = 0;

	// CG_Printf( "%i : %i : %i : %i\n", miFrame[fn].x1, miFrame[fn].y1, miFrame[fn].x2, miFrame[fn].y2 );
	while ( z <= len && ( text[z] != '>' || text[z+1] == '>' ) ) {
		// double '>>' prints as '>'
		if ( text[z] == '>' && text[z+1] == '>' ) z++;

		// new line?
		if ( text[z] < ' ' || (lz+1+lc) * 10 > miFrame[fn].w ) {
			lc = 0;
			if ( text[z] < ' ' ) lc++;
			if ( text[z+1] < ' ' ) lc++;
			z += lc;
			miFrame[fn].line[ln][lz] = 0;
			miFrame[fn].num_lines++;
			miFrame[fn].ypos += 16;
			if ( miFrame[fn].num_lines >= MI_LINES ) {
				// report error
				return -1;
			}
			ln++;
			miFrame[fn].line_type[ln] = L_TEXT;
			lz = 0;
			lc = 0;
		} else {
			if ( miFrame[fn].ypos > miFrame[fn].h ) {
				// report error
				return -1;
			}

			if ( Q_IsColorString ( &text[z] ) ) {
				miFrame[fn].line[ln][lz] = text[z];
				lz++;
				z++;
				lc -= 2;
			}

			miFrame[fn].line[ln][lz] = text[z];
			lz++;
			z++;
		}
	}

	miFrame[fn].num_lines++;
	miFrame[fn].ypos += 16;

	return z;
}


/*
==========================
CG_AwMenuLoadMapInfo
==========================
*/

qboolean CG_AwMenuLoadMapInfo( void ) {
	const char	*info;
	char		*mapname;
	fileHandle_t	f;
	char		instruction[10];
	char		parameters[20];
	char		strings[50];
	int			sz;
	int			len;
	int			z;
	int			ln;
	int			shadernum;
	int			framenum;

	// get map name
	info = CG_ConfigString( CS_SERVERINFO );
	mapname = Info_ValueForKey( info, "mapname" );
	
	// load the file
	len = trap_FS_FOpenFile( va( "mapinfos/%s.nfo", mapname ) , &f, FS_READ );
	if ( len <= 0 ) {
		CG_Printf( S_COLOR_YELLOW "MapInfo File '%s.nfo' doesn't exist\n", mapname );
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( S_COLOR_YELLOW "MapInfo File '%s.nfo' too long\n", mapname );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );


	// clear miFrame structure
	for ( z = 0; z < MI_FRAMES; z++ ) {
		miFrame[z].num_lines = 0;
	}
	// head if no other is specified
	Q_strncpyz( miFrame[0].line[0], va( "Map Information: %s", mapname ), MI_LINELENGTH-1 );

	framenum = 0;
	shadernum = 0;
	z = 0;

	do {
		// find instruction
		while ( z <= len && text[z] != '[' ) {
			z++;
		}
		z++;
		// read instruction
		sz = 0;
		while ( z <= len && text[z] != ']' && text[z] != ' ' ) {
			instruction[sz] = text[z];
			sz++;
			z++;
		}
		instruction[sz] = 0;
		if ( text[z] == ' ' ) z++;
		// read numeric parameters
		sz = 0;
		while ( z <= len && ( text[z] >= '0' && text[z] <= '9' || text[z] == ' ' ) ) {
			parameters[sz] = text[z];
			sz++;
			z++;
		}
		parameters[sz] = 0;		
		// read strings
		sz = 0;
		while ( z <= len && text[z] != ']' ) {
			strings[sz] = text[z];
			sz++;
			z++;
		}
		strings[sz] = 0;		
		z++;

		// analyze instruction
		if ( !Q_stricmp( instruction, "HEAD" ) ) {
			// Header (Title)
			miFrame[0].x = 0; miFrame[0].w = 560;
			miFrame[0].y = 0; miFrame[0].h = 16;
			z = CG_ReadText( z, len, 0 );
		} else 
		if ( !Q_stricmp( instruction, "FRAME" ) ) {
			// Frame start
			if ( framenum >= MI_FRAMES-1 ) {
				CG_Printf( S_COLOR_YELLOW "Too many frames in MapInfo '%s.nfo'\n", mapname );
				return qfalse;
			}
			framenum++;
			sscanf( parameters, "%i %i %i %i", 
				&miFrame[framenum].x, &miFrame[framenum].y,
				&miFrame[framenum].w, &miFrame[framenum].h );

			if ( miFrame[framenum].w < 0 || miFrame[framenum].h < 0 || miFrame[framenum].x < 0 || miFrame[framenum].y < 0 ||
				miFrame[framenum].x + miFrame[framenum].w > 560 || miFrame[framenum].y + miFrame[framenum].h > 290 ) {
				CG_Printf( S_COLOR_YELLOW "Frame %i has impossible dimensions in MapInfo '%s.nfo'\n", framenum, mapname );
				return qfalse;
			}
			
			miFrame[framenum].ypos = 0;
			z = CG_ReadText( z, len, framenum );
		} else
			if ( !Q_stricmp( instruction, "IMG" ) || !Q_stricmp( instruction, "BKIMG" ) ) {
			// Image
			if ( shadernum >= MI_SHADERS-1 ) {
				CG_Printf( S_COLOR_YELLOW "Too many media files in MapInfo '%s.nfo'\n", mapname );
				return qfalse;
			}
			if ( miFrame[framenum].num_lines >= MI_LINES-1 ) {
				CG_Printf( S_COLOR_YELLOW "Too many lines in frame %i in MapInfo '%s.nfo'\n", framenum, mapname );
				return qfalse;
			}

			miFrame[framenum].num_lines++;
			ln = miFrame[framenum].num_lines-1;
			
			sscanf( parameters, "%i %i %i %i",
				&miFrame[framenum].line_tx[ln], 
				&miFrame[framenum].line_ty[ln],
				&miFrame[framenum].line_tw[ln], 
				&miFrame[framenum].line_th[ln] );
			Q_strncpyz( miFrame[framenum].line[ln], strings, 50 );

			if ( miFrame[framenum].line_tx[ln] >= 0 && miFrame[framenum].line_ty[ln] >= 0 &&
				miFrame[framenum].line_tw[ln] > 0 && miFrame[framenum].line_th[ln] > 0 &&
				miFrame[framenum].line_tx[ln] + miFrame[framenum].line_tw[ln] <= miFrame[framenum].w &&
				miFrame[framenum].line_ty[ln] + miFrame[framenum].line_th[ln] + miFrame[framenum].ypos <= miFrame[framenum].h ) {

				// dimensions are OK -> load shader
				if ( !Q_stricmp( instruction, "IMG" ) ) {
					miFrame[framenum].ypos += miFrame[framenum].line_ty[ln] + miFrame[framenum].line_th[ln] + 8;
					miFrame[framenum].line_type[ln] = L_IMG;
				} else {
					miFrame[framenum].line_type[ln] = L_BKIMG;
				}

				miShader[shadernum] = trap_R_RegisterShaderNoMip( miFrame[framenum].line[ln] );
				miFrame[framenum].line_tag[ln] = shadernum;
				shadernum++;
				z = CG_ReadText( z, len, framenum );
			} else {
				CG_Printf( S_COLOR_YELLOW "Image in frame %i in MapInfo '%s.nfo' has impossible dimensions\n", framenum, mapname );
				return qfalse;
			}
		} else {
			if ( instruction[0] != 0 ) {
				CG_Printf( S_COLOR_YELLOW "Unknown instruction in MapInfo '%s.nfo'\n", mapname );
				return qfalse;
			}
		}

		if ( z == -1 ) {
			CG_Printf( S_COLOR_YELLOW "Too many lines in Frame %i in MapInfo '%s.nfo'\n", framenum, mapname );
			return qfalse;
		}
	} while ( z <= len );

	return qtrue;
}


/*
==========================
CG_AddWeaponToMenu
==========================
*/
void CG_AddWeaponToMenu( int weapon, int place ) {
	awMenu[place].select_num = place;
	if ( info[ weapon ] == '1' ) awMenu[place].item_type = AM_WEAPON;
	else awMenu[place].item_type = AM_WP_DEACTIVATED;
	awMenu[place].item_num = weapon;
	awMenu[place].caption = BG_FindItemForWeapon( weapon )->pickup_name;
}


/*
==========================
CG_AwInitMenu
==========================
*/

void CG_AwInitMenu( int num ) {
	int i;
	int z;

	info = CG_ConfigString( CS_ITEMS );
	
	for ( i = 0; i < 10; i++ ) {
		awMenu[i].select_num = -1;
		awMenu[i].item_type = AM_NONE;
		awMenu[i].item_num = 0;
		awMenu[i].design = 0;
		awMenu[i].item_text = NULL;
		awMenu[i].caption = NULL;
	}
	
	InitializedSubMenu = num;

	switch ( num ) {

	case SM_NONE:
		// somebody wants to get the menu away
		break;

	case SM_TEAM_SELECT:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Team Selection:";
		
		awMenu[1].select_num = 1;
		awMenu[1].item_type = AM_SUBMENU;
		awMenu[1].item_num = SM_MODELS_RED;
		awMenu[1].caption = "Wasteland Tribes";

		awMenu[2].select_num = 2;
		awMenu[2].item_type = AM_SUBMENU;
		awMenu[2].item_num = SM_MODELS_BLUE;
		awMenu[2].caption = "Australian Army";

		awMenu[3].select_num = 3;
		awMenu[3].item_type = AM_SPECTATOR;
		awMenu[3].caption = "Spectator";

		awMenu[4].item_type = AM_SPACER;
		awMenu[4].item_num = SPACER_STANDARD;

		awMenu[5].select_num = 4;
		awMenu[5].item_type = AM_AUTOJOIN;
		awMenu[5].caption = "Auto Join";

		awMenu[6].item_type = AM_SPACER;
		awMenu[6].item_num = SPACER_LASTLINE;

		awMenu[7].select_num = 0;
		awMenu[7].item_type = AM_EXIT;
		awMenu[7].design = DF_LINE_TOP;
		awMenu[7].caption = "Cancel";
		break;
	
	case SM_MODELS_RED:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Model Selection (Wasteland Tribes):";
		
		z = 1;
		for ( i = 0; i < MAX_TACTICAL_MODELS; i++ )
			if ( bg_modelslist[i].team == TEAM_RED ) {
				awMenu[z].select_num = z;
				awMenu[z].item_type = AM_MODEL;
				awMenu[z].item_num = i;
				awMenu[z].caption = bg_modelslist[i].name;
				z++;
			}

		awMenu[z].item_type = AM_SPACER;
		awMenu[z].item_num = SPACER_LASTLINE;

		awMenu[z+1].select_num = 0;
		awMenu[z+1].item_type = AM_SUBMENU;
		awMenu[z+1].design = DF_LINE_TOP;
		awMenu[z+1].item_num = SM_TEAM_SELECT;
		awMenu[z+1].caption = "Cancel";
		break;
	
	case SM_MODELS_BLUE:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Model Selection (Australians):";
		
		z = 1;
		for ( i = 0; i < MAX_TACTICAL_MODELS; i++ )
			if ( bg_modelslist[i].team == TEAM_BLUE ) {
				awMenu[z].select_num = z;
				awMenu[z].item_type = AM_MODEL;
				awMenu[z].item_num = i;
				awMenu[z].caption = bg_modelslist[i].name;
				z++;
			}

		awMenu[z].item_type = AM_SPACER;
		awMenu[z].item_num = SPACER_LASTLINE;

		awMenu[z+1].select_num = 0;
		awMenu[z+1].item_type = AM_SUBMENU;
		awMenu[z+1].design = DF_LINE_TOP;
		awMenu[z+1].item_num = SM_TEAM_SELECT;
		awMenu[z+1].caption = "Cancel";
		break;
	
	case SM_WEAPONS_MAIN:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Weapon Selection:";
		
		awMenu[1].select_num = 1;
		awMenu[1].item_type = AM_SUBMENU;
		awMenu[1].item_num = SM_WEAPONS_SIDEARMS;
		awMenu[1].caption = "Sidearms";

		awMenu[2].select_num = 2;
		awMenu[2].item_type = AM_SUBMENU;
		awMenu[2].item_num = SM_WEAPONS_SMALL;
		awMenu[2].caption = "Small Guns";

		awMenu[3].select_num = 3;
		awMenu[3].item_type = AM_SUBMENU;
		awMenu[3].item_num = SM_WEAPONS_BIG;
		awMenu[3].caption = "Big Guns";

		awMenu[4].select_num = 4;
		awMenu[4].item_type = AM_SUBMENU;
		awMenu[4].item_num = SM_WEAPONS_EXPLOSIVES;
		awMenu[4].caption = "Explosives";

		awMenu[5].item_type = AM_SPACER;
		awMenu[5].item_num = SPACER_LASTLINE;

		awMenu[6].select_num = 0;
		awMenu[6].item_type = AM_EXIT;
		awMenu[6].design = DF_LINE_TOP;
		awMenu[6].caption = "Cancel";
		break;

	case SM_WEAPONS_SIDEARMS:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Weapon Selection (Sidearms):";
		
		CG_AddWeaponToMenu( WP_STANDARD, 1 );
		CG_AddWeaponToMenu( WP_HANDGUN, 2 );
		CG_AddWeaponToMenu( WP_MICRO, 3 );

		awMenu[4].item_type = AM_SPACER;
		awMenu[4].item_num = SPACER_LASTLINE;

		awMenu[5].select_num = 0;
		awMenu[5].item_type = AM_SUBMENU;
		awMenu[5].design = DF_LINE_TOP;
		awMenu[5].item_num = SM_WEAPONS_MAIN;
		awMenu[5].caption = "Cancel";
		break;

	case SM_WEAPONS_SMALL:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Weapon Selection (Small Guns):";
		
		CG_AddWeaponToMenu( WP_SHOTGUN, 1 );
		CG_AddWeaponToMenu( WP_SUB, 2 );
		CG_AddWeaponToMenu( WP_A_RIFLE, 3 );
		CG_AddWeaponToMenu( WP_SNIPER, 4 );
		CG_AddWeaponToMenu( WP_PHOENIX, 5 );
		CG_AddWeaponToMenu( WP_TRIPACT, 6 );

		awMenu[7].item_type = AM_SPACER;
		awMenu[7].item_num = SPACER_LASTLINE;

		awMenu[8].select_num = 0;
		awMenu[8].item_type = AM_SUBMENU;
		awMenu[8].design = DF_LINE_TOP;
		awMenu[8].item_num = SM_WEAPONS_MAIN;
		awMenu[8].caption = "Cancel";
		break;

	case SM_WEAPONS_BIG:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Weapon Selection (Big Guns):";
		
		CG_AddWeaponToMenu( WP_GRENADE_LAUNCHER, 1 );
		CG_AddWeaponToMenu( WP_MINIGUN, 2 );
		CG_AddWeaponToMenu( WP_FLAMETHROWER, 3 );
		CG_AddWeaponToMenu( WP_DBT, 4 );
		CG_AddWeaponToMenu( WP_OMEGA, 5 );
		CG_AddWeaponToMenu( WP_RPG, 6 );
		CG_AddWeaponToMenu( WP_THERMO, 7 );

		awMenu[8].item_type = AM_SPACER;
		awMenu[8].item_num = SPACER_LASTLINE;

		awMenu[9].select_num = 0;
		awMenu[9].item_type = AM_SUBMENU;
		awMenu[9].design = DF_LINE_TOP;
		awMenu[9].item_num = SM_WEAPONS_MAIN;
		awMenu[9].caption = "Cancel";
		break;

	case SM_WEAPONS_EXPLOSIVES:
		awMenu[0].item_type = AM_TEXT;
		awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
		awMenu[0].caption = "Weapon Selection (Explosives):";
		
		CG_AddWeaponToMenu( WP_FRAG_GRENADE, 1 );
		CG_AddWeaponToMenu( WP_FIRE_GRENADE, 2 );

		awMenu[3].item_type = AM_SPACER;
		awMenu[3].item_num = SPACER_LASTLINE;

		awMenu[4].select_num = 0;
		awMenu[4].item_type = AM_SUBMENU;
		awMenu[4].design = DF_LINE_TOP;
		awMenu[4].item_num = SM_WEAPONS_MAIN;
		awMenu[4].caption = "Cancel";
		break;

	case SM_MAPINFO_ENTER:
	case SM_MAPINFO:
		if ( cgs.loadedMapInfo ) {
			awMenu[0].item_type = AM_TEXT;
			awMenu[0].design = DF_LINE_BOTTOM | DF_BRIGHT;
			awMenu[0].caption = miFrame[0].line[0];
		
			awMenu[1].item_type = AM_SPACER;
			awMenu[1].item_num = SPACER_LASTLINE;

			awMenu[2].select_num = 0;
			awMenu[2].design = DF_LINE_TOP;
			awMenu[2].caption = "Close";
			if ( num == SM_MAPINFO_ENTER ) {
				awMenu[2].item_type = AM_SUBMENU;
				awMenu[2].item_num = SM_TEAM_SELECT;
			} else {
				awMenu[2].item_type = AM_EXIT;
			}
		} else {
			// the MapInfo wasn't loaded -> don't view screen
			CG_Printf( S_COLOR_YELLOW "No map information was loaded.\n" );
			trap_Cvar_Set("cg_awMenu", "0" );
			return;
		}
		break;
	}

	trap_Cvar_Set( "cg_awMenu", va( "%i", num ) );
	trap_Cvar_Set( "cg_frameWidth", "0" );
	trap_Cvar_Set( "cg_frameHeight", "0" );
}


/*
==========================
CG_DrawMenuArea
==========================
*/

void CG_DrawMenuArea( int x, int y, int w, int h, float a )
{
	vec4_t		hcolor;

	hcolor[0] = cg.color0[0];
	hcolor[1] = cg.color0[1];
	hcolor[2] = cg.color0[2];
	// alpha value
	hcolor[3] = a;

	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.hudFill );

	trap_R_SetColor( NULL );
}


/*
==========================
CG_AwMenuDrawFlexible
==========================
*/

void CG_AwMenuDrawFlexible( void ) {
	int		framenum;
	int		linenum;
	int		y;
	
	for ( framenum = 1; framenum < MI_FRAMES; framenum++ ) {
		// reset y for line counting
		y = 0;
		for ( linenum = 0; linenum < miFrame[framenum].num_lines; linenum++ ) {
			// draw lines
			if ( miFrame[framenum].line_type[linenum] == L_TEXT ) {
				// text line
				CG_DrawBigNarrowString( 40 + miFrame[framenum].x, 76 + miFrame[framenum].y + y, 
					miFrame[framenum].line[linenum], 1.0F);
				y += 16;
			} else if ( miFrame[framenum].line_type[linenum] == L_IMG || miFrame[framenum].line_type[linenum] == L_BKIMG) {
				// image line
				CG_DrawPic( 40 + miFrame[framenum].x + miFrame[framenum].line_tx[linenum], 
					76 + 4 + miFrame[framenum].line_ty[linenum] + miFrame[framenum].y + y, 
					miFrame[framenum].line_tw[linenum], miFrame[framenum].line_th[linenum], 
					miShader[ miFrame[framenum].line_tag[linenum] ] );
				if ( miFrame[framenum].line_type[linenum] == L_IMG ) y += miFrame[framenum].line_th[linenum] + 8;
			}

		}
	}
}


/*
==========================
CG_DrawAwMenu
==========================
*/

void CG_DrawAwMenu( void ) {
	int		i;
	int		x, y, w;
	int		itemNum;
	int		rank;

	if ( cgs.gametype != GT_TACTICAL ) {
		cg_awMenu.integer = 0;
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg_awMenu.integer <= 0 || cg_awMenu.integer >= SM_MAX_MENUS ) return;

	// if somebody just changed the "cg_awMenu" CVar, initialize new submenu
	if ( cg_awMenu.integer != InitializedSubMenu ) CG_AwInitMenu( cg_awMenu.integer );

	if ( cg_awMenu.integer < SM_FLEXIBLES ) {
		x = 110;
		w = 420;
	} else {
		x = 10;
		w = 620;
	}

	// store rank for further use
	rank = BG_GetTacticalRank(cg.predictedPlayerState.persistant[PERS_SCORE]);

	// draw cute frame
	CG_DrawFrame( x, 15, w, 412 );
	
	y = 32;

	for (i = 0; i < 10; i++) {
		itemNum = awMenu[i].item_num;

		if ( awMenu[i].design & DF_LINE_TOP ) CG_DrawMenuArea( x+7, y-12, w-14, 2, 0.4f );
		if ( awMenu[i].design & DF_LINE_BOTTOM ) CG_DrawMenuArea( x+7, y+28, w-14, 2, 0.4f );
		if ( awMenu[i].design & DF_BRIGHT )	CG_DrawMenuArea( x+7, y-10, w-14, 38, 0.2f );

		switch ( awMenu[i].item_type ) {

		case AM_NONE:
			// do nothing
			break;

		case AM_SPACER:
			if ( awMenu[i].item_num == SPACER_LASTLINE ) y = 392;
			else y += awMenu[i].item_num;
			break;

		case AM_TEXT:
			CG_DrawBigNarrowString( x+20 , y , awMenu[i].caption , 1.0F);
			y += 40;
			break;

		case AM_SUBMENU:
		case AM_SPECTATOR:
		case AM_AUTOJOIN:
		case AM_EXIT:
			CG_DrawBigNarrowString( x+20 , y , va("%i", awMenu[i].select_num) , 1.0F);
			CG_DrawBigNarrowString( x+60 , y , awMenu[i].caption , 1.0F);
			y += 40;
			break;

		case AM_WEAPON:
			// weapon is available			
			CG_RegisterWeapon( itemNum ); // make sure weapon is loaded
			CG_DrawBigNarrowString( x+20 , y , va("%i", awMenu[i].select_num) , ( rank >= weLi[itemNum].min_rank ? 1.0f : 0.5f ) );
			CG_DrawPic( x+60, y - 8, 64, 32, cg_weapons[itemNum].weaponIcon );
			trap_R_SetColor( cg.color1 );
			CG_DrawPic( x+140, y - 4, 24, 24, cgs.media.rankIcon[weLi[itemNum].min_rank] );
			if ( rank < weLi[itemNum].min_rank ) {
				vec4_t	color;
				color[0] = color[1] = color[2] = 1.0f;
				color[3] = 0.6f;
				trap_R_SetColor( color );
				CG_DrawPic( x+76, y - 8, 32, 32, cgs.media.noammoShader );
			}
			trap_R_SetColor( NULL );
			CG_DrawBigNarrowString( x+180 , y , awMenu[i].caption , ( rank >= weLi[itemNum].min_rank ? 1.0f : 0.5f ) );	
			y += 40;
			break;

		case AM_WP_DEACTIVATED:
			// weapon is not available			
			CG_DrawBigNarrowString( x+20 , y , va("%i", awMenu[i].select_num) , 0.5F);
			CG_DrawPic( x+60, y - 8, 64, 32, cgs.media.notAvailableShader );
			CG_DrawPic( x+140, y - 4, 24, 24, cgs.media.rankIcon[weLi[itemNum].min_rank] );
			CG_DrawBigNarrowString( x+180 , y , awMenu[i].caption , 0.5F);	
			y += 40;
			break;

		case AM_MODEL:
			CG_DrawBigNarrowString( x+20 , y , va("%i", awMenu[i].select_num) , 1.0F);
			// draw weapon and rank icons and write text			
			CG_DrawPic( x+60, y - 8, 32, 32, awModels[itemNum].modelIcon );
			CG_DrawBigNarrowString( x+110 , y , awMenu[i].caption , 1.0F);		
			y += 40;
			break;

		default:
			// isn't really possible
			break;
		}
	}

	if ( cg_awMenu.integer > SM_FLEXIBLES ) CG_AwMenuDrawFlexible();
}


/*
==========================
CG_AwMenuSelect
==========================
*/

void CG_AwMenuSelect( int num ) {
	int		selectNum;
	int		itemNum;
	int		i;

	if ( cg_awMenu.integer <= 0 || cg_awMenu.integer >= SM_MAX_MENUS ) return;

	if ( num == 10 ) num = 0;

	selectNum = -1;
	for ( i = 0; i < 10; i++ ) 
		if ( awMenu[i].select_num == num ) selectNum = i;

	// selectNum is -1 if no query for this key exists
	if ( selectNum >= 0 ) {
		// CG_Printf("%i -> %i -> %i\n", num, selectNum, awMenu[selectNum].item_num );
		itemNum = awMenu[selectNum].item_num;

		switch ( awMenu[selectNum].item_type ) {
		case AM_EXIT:
			trap_Cvar_Set("cg_awMenu", "0");
			break;
		case AM_SUBMENU:
			CG_AwInitMenu( itemNum );
			break;
		case AM_WEAPON:
		case AM_WP_DEACTIVATED:
			trap_SendConsoleCommand( va( "buy %s", weLi[itemNum].name ) );
			trap_Cvar_Set("cg_awMenu", "0");
			break;
		case AM_SPECTATOR:
			trap_SendConsoleCommand( "cmd team spectator\n" );
			trap_Cvar_Set("cg_awMenu", "0");
			break;
		case AM_AUTOJOIN:
			trap_SendConsoleCommand( "autojoin" );
			trap_Cvar_Set("cg_awMenu", "0");
			break;
		case AM_MODEL:
			if ( bg_modelslist[itemNum].team == TEAM_RED ) trap_SendConsoleCommand( "cmd team red\n" );
			else trap_SendConsoleCommand( "cmd team blue\n" );
			trap_Cvar_Set("team_model",     va( "%s/%s", bg_modelslist[itemNum].model, bg_modelslist[itemNum].modelSkin ) );
			trap_Cvar_Set("team_headModel", va( "%s/%s", bg_modelslist[itemNum].head, bg_modelslist[itemNum].headSkin ) );
			trap_Cvar_Set("cg_awMenu", "0");
			break;
		default:
			// may happen if a non specified field is selected
			break;
		}
	}
}


