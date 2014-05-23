// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"


// *************************************************************

// in correspondance to the wpEv struct in cg_local.h
char *wpEvNames[] = {
	"NONE",
	"IDLE",
	"IDLE_EMPTY",
	"FIRE",
	"FIRE_EMPTY",
	"RELOAD",
	"RELOAD_EMPTY",
	"DROP",
	"RAISE"
};

// ------------------------------------------------
typedef enum {
	WA_NONE,
	WA_ANIM,
	WA_SND,
	WA_FUNC,
	WA_EVENT,
	WA_CONTINUE,
	WA_GOTO,
	WA_HOLD,
	WA_NUM_ACTIONS
} wpAction_t;

// in correspondance to the wpAction struct
char *wpActionNames[] = {
	"none",
	"anim",
	"snd",
	"func",
	"event",
	"continue",
	"goto",
	"hold"
};

// ------------------------------------------------
typedef enum {
	WF_NONE,
	WF_EJECT,
	WF_EJECT_NEXT,
	WF_SHADER_CHANGE,
	WF_NUM_FUNCS
} wpFunc_t;

// in correspondance to the wpEvAction struct
char *wpFuncNames[] = {
	"none",
	"eject",
	"eject_next",
	"shader_change"
};

// ------------------------------------------------
typedef enum {
	WS_NONE,
	WS_CLIPIN,
	WS_CLIPOUT,
	WS_VERSCHLUSS,
	WS_NUM_SOUNDS
} wpSounds_t;

// in correspondance to the wpSounds struct
char *wpSoundNames[] = {
	"none",
	"clipIn",
	"clipOut",
	"verschluss"
};


static void CG_EjectBrass( refEntity_t *parent, centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity;

	if ( cg_brassTime.integer <= 0 || !cg_weapons[ cent->currentState.weapon ].brass ) {
		return;
	}
	
	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	// position the brass at the tag_eject
	CG_PositionEntityOnTag( re, parent, parent->hModel, "tag_eject" );

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	// check for water
	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		le->pos.trType = TR_WATER_GRAVITY;
	}

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	// get standard origin (can be altered later)
	VectorCopy( re->origin, le->pos.trBase );

	// set standard velocity direction (can be altered later)
	VectorNormalize2( re->axis[1], velocity );

	// do triple eject special shifts
	if ( cg_weapons[ cent->currentState.weapon ].brass == BT_TRIPLE ) {
		vec3_t	axis[3];

		VectorNormalize2( re->axis[0], axis[0] );
		VectorNormalize2( re->axis[1], axis[1] );
		VectorNormalize2( re->axis[2], axis[2] );

		if ( cent->ejectBrass == 1 ) {
			// shift first brass upwards
			VectorMA( re->origin, 2, axis[1], le->pos.trBase );
			VectorCopy( le->pos.trBase, re->origin );
		} else if ( cent->ejectBrass == 2 ) {
			// shift second brass to the left
			VectorMA( re->origin, -2, axis[2], le->pos.trBase );
			VectorMA( re->origin, -2, axis[0], le->pos.trBase );
			VectorCopy( le->pos.trBase, re->origin );
			VectorScale( axis[2], -1, velocity );
			velocity[2] += 0.3f;
		} else if ( cent->ejectBrass == 3 ) {
			// shift second brass to the right
			VectorMA( re->origin, 2, axis[2], le->pos.trBase );
			VectorMA( re->origin, 2, axis[0], le->pos.trBase );
			VectorCopy( le->pos.trBase, re->origin );
			VectorCopy( axis[2], velocity );
			velocity[2] += 0.3f;
		}
		// don't shift if not a known number
	}

	// stretch velocity
	VectorScale( velocity, 120 + 40 * crandom(), velocity );

	// give some additional upwards speed
	velocity[2] += 30;

	// add some random effects
	velocity[0] += 20 * crandom();
	velocity[1] += 20 * crandom();
	velocity[2] += 20 * crandom();

	// add playerspeed to velocity
	VectorAdd( velocity, cent->currentState.pos.trDelta, le->pos.trDelta );

//	AxisCopy( axisDefault, re->axis );
	le->bounceFactor = 0.4f;

	// the rotation
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	VectorCopy( cent->lerpAngles, le->angles.trBase );
	le->angles.trDelta[0] = 160 * crandom();
	le->angles.trDelta[1] = 160 * crandom();
	le->angles.trDelta[2] = 160 * crandom();

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;

	// select brass model
	if ( cg_weapons[ cent->currentState.weapon ].brass == BT_STANDARD ) {
		if ( cg_lowDetailEffects.integer ) re->hModel = cgs.media.stdBrassModelLow;
		else re->hModel = cgs.media.stdBrassModel;
	} else re->hModel = cgs.media.shotBrassModel;
}


/*
==========================
CG_RocketTrail
==========================
*/
static void CG_RocketTrail( trailInfo_t *ti, int endTime ) {
	int		step;
	vec3_t	origin, altPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	vec3_t	up;
	localEntity_t	*smoke;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	startTime = ti->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( ti->trPos, endTime, origin );
	contents = CG_PointContents( origin, -1 );

	ti->trailTime = endTime;

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( ti->trPos->trType == TR_STATIONARY ) {
		return;
	}

	lastContents = CG_PointContents( ti->lastPos, -1 );

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( ti->lastPos, origin, 8 );
		}
		VectorCopy( origin, ti->lastPos );
		return;
	}
	VectorCopy( origin, ti->lastPos );

	for ( ; t <= endTime ; t += step ) {
		BG_EvaluateTrajectory( ti->trPos, t, altPos );

		smoke = CG_SmokePuff( altPos, up, 
					  cg_weapons[ti->weapon].trailRadius, 
					  1, 1, 1, 0.55f,
					  cg_weapons[ti->weapon].wiTrailTime, 
					  cg.time,
					  0,
					  0, 
					  cgs.media.smokePuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}

/*
==========================
CG_DBTTrail
==========================
*/
static void CG_DBTTrail( trailInfo_t *ti, int endTime ) {
	int		step;
	vec3_t	origin, altPos;
	int		t;
	int		startTime;
	int		contents, lastContents;
	localEntity_t	*ring;

	step = 16;

	startTime = ti->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( ti->trPos, endTime, origin );
	contents = CG_PointContents( origin, -1 );
	lastContents = CG_PointContents( ti->lastPos, -1 );

	ti->trailTime = endTime;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( ti->lastPos, origin, 8 );
		}
		VectorCopy( origin, ti->lastPos );
		return;
	}
	VectorCopy( origin, ti->lastPos );

	for ( ; t <= endTime ; t += step ) {
		BG_EvaluateTrajectory( ti->trPos, t, altPos );

		ring = CG_AllocLocalEntity();

		ring->leType = LE_SCALE_FADE;
		ring->refEntity.hModel = cgs.media.plasmaRingModel;
		ring->refEntity.shaderTime = cg.time / 1000.0f;
		ring->startTime = cg.time;
		ring->endTime = cg.time + cg_weapons[ti->weapon].wiTrailTime;
		ring->lifeRate = 1.0 / ( ring->endTime - ring->startTime );

		VectorCopy( altPos, ring->refEntity.origin );

		// convert direction of travel into axis
		if ( VectorNormalize2( ti->trPos->trDelta, ring->refEntity.axis[0] ) == 0 ) {
			ring->refEntity.axis[0][2] = 1;
		}
		RotateAroundDirection( ring->refEntity.axis, cg.time );
	}

}


/*
==========================
CG_TripactTrail
==========================
*/
static void CG_TripactTrail( trailInfo_t *ti, int endTime ) {
	localEntity_t	*le;
	refEntity_t		beam;
	vec3_t	origin;
	int		contents, lastContents;
	int		step, t;

	step = 30;

	t = ti->trailTime + step;

	BG_EvaluateTrajectory( ti->trPos, endTime, origin );
	contents = CG_PointContents( origin, -1 );

	lastContents = CG_PointContents( ti->lastPos, -1 );

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( ti->lastPos, origin, 8 );
		}
		VectorCopy( origin, ti->lastPos );
		ti->trailTime = endTime;
		return;
	}

	for ( ; t <= endTime ; t += step ) {
		BG_EvaluateTrajectory( ti->trPos, t, origin );

		// add trail
		le = CG_AllocLocalEntity();
		le->leType = LE_FADE_RGB;
		le->refEntity.reType = RT_LIGHTNING;
		le->refEntity.customShader = cgs.media.tripactTrailShader;
		le->startTime = cg.time;
		le->endTime = cg.time + cg_weapons[ti->weapon].wiTrailTime;
		le->lifeRate = 1.0 / cg_weapons[ti->weapon].wiTrailTime;
		le->color[0] = le->color[1] = le->color[2] = le->color[3] = 0.5f;
		VectorCopy( origin, le->refEntity.oldorigin );
		VectorCopy( ti->lastPos, le->refEntity.origin );

		// add knot
		le = CG_AllocLocalEntity();
		le->leType = LE_FADE_RGB;
		le->refEntity.reType = RT_SPRITE;
		le->refEntity.radius = 10;
		le->refEntity.customShader = cgs.media.tripactTrailKnotShader;
		le->startTime = cg.time;
		le->endTime = cg.time + cg_weapons[ti->weapon].wiTrailTime;
		le->lifeRate = 1.0 / cg_weapons[ti->weapon].wiTrailTime;
		le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0f;
		VectorCopy( origin, le->refEntity.origin );

		VectorCopy( origin, ti->lastPos );
		ti->trailTime = t;
	}


	memset( &beam, 0, sizeof( beam ) );

	// define start and endpoint
	BG_EvaluateTrajectory( ti->trPos, endTime, beam.oldorigin );
	VectorCopy( ti->lastPos, beam.origin );

	beam.shaderRGBA[0] = 0x7f * ((t - step + cg_weapons[ti->weapon].wiTrailTime) - endTime) / cg_weapons[ti->weapon].wiTrailTime;
	beam.shaderRGBA[1] = beam.shaderRGBA[0];
	beam.shaderRGBA[2] = beam.shaderRGBA[0];
	beam.shaderRGBA[3] = beam.shaderRGBA[0];

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.tripactTrailShader;
	trap_R_AddRefEntityToScene( &beam );
}


/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail( trailInfo_t *ti, int endTime ) {
	CG_RocketTrail( ti, endTime );
}



/*
==========================
WPS Parsing functions
==========================
*/

#define MAX_WPS_LENGTH		2000
char wps[MAX_WPS_LENGTH];

/*
=================
CG_ParseWPSActions

Parse the actions after an event in a WPS file
=================
*/
void CG_ParseWPSActions( char *filename, wpActions_t *wA, int eventNum, char **wps_p ) {
	qboolean	found;
	char		*token;
	int			i,j;
	int			curLine;
	int			actionNum;
	int			tag1;

	actionNum = 0;
	token = COM_Parse( wps_p );

	while ( 1 ) {
		if ( !*token ) {
			CG_Printf( S_COLOR_YELLOW "Action or '}' expected in '%s' on line %i.\n", filename, COM_GetCurrentParseLine(), token );
			return;
		}

		// check for end of event
		if ( *token == '}' ) {
//			CG_Printf( "Parsed %i actions in event %s.\n", actionNum, wpEvNames[eventNum] );
			return;
		}

		found = qfalse;
		// token should be an action name, look which one it is
		for ( i = 0; i < WA_NUM_ACTIONS; i++ ) {
			if ( !Q_stricmp( token, wpActionNames[i] ) ) {
				// store the type
				(*wA)[eventNum][actionNum].type = i;

				curLine = COM_GetCurrentParseLine();
				token = COM_Parse( wps_p );

				switch ( i ) {
				case WA_NONE:
					// do nothing
					break;

				case WA_ANIM:
					// get the time
					sscanf( token, "%i", &tag1 );
					(*wA)[eventNum][actionNum].tag1 = tag1;
					(*wA)[eventNum][actionNum].tag2 = -1;

					token = COM_Parse( wps_p );	
					// additional parameter if no new line
					if ( curLine == COM_GetCurrentParseLine() ) {
						sscanf( token, "%i", &((*wA)[eventNum][actionNum].tag2) );

						token = COM_Parse( wps_p );	
						// all following parameters are additional anim frames
						while ( curLine == COM_GetCurrentParseLine() ) {
							actionNum++;
							(*wA)[eventNum][actionNum].type = i;
							(*wA)[eventNum][actionNum].tag1 = tag1;
							sscanf( token, "%i", &((*wA)[eventNum][actionNum].tag2) );
							token = COM_Parse( wps_p );	
						}
					}
					break;

				case WA_SND:
					tag1 = 0;
					for ( j = 1; j < WS_NUM_SOUNDS; j++ ) {
						if ( !Q_stricmp( token, wpSoundNames[j] ) ) tag1 = j;
					}
					if ( !tag1 ) CG_Printf( S_COLOR_YELLOW "Unknown sound '%s' in '%s' on line %i.\n", token, filename, COM_GetCurrentParseLine() );
					else {
						(*wA)[eventNum][actionNum].tag1 = tag1;
						token = COM_Parse( wps_p );
					}
					break;

				case WA_FUNC:
					tag1 = 0;
					for ( j = 1; j < WF_NUM_FUNCS; j++ ) {
						if ( !Q_stricmp( token, wpFuncNames[j] ) ) tag1 = j;
					}
					if ( !tag1 ) CG_Printf( S_COLOR_YELLOW "Unknown function '%s' in '%s' on line %i.\n", token, filename, COM_GetCurrentParseLine() );
					else {
						(*wA)[eventNum][actionNum].tag1 = tag1;
						token = COM_Parse( wps_p );
					}
					break;

				case WA_EVENT:
					tag1 = 0;
					for ( j = 1; j < WE_NUM_EVENTS; j++ ) {
						if ( !Q_stricmp( token, wpEvNames[j] ) ) tag1 = j;
					}
					if ( !tag1 ) CG_Printf( S_COLOR_YELLOW "Unknown event '%s' in '%s' on line %i.\n", token, filename, COM_GetCurrentParseLine() );
					else {
						(*wA)[eventNum][actionNum].tag1 = tag1;
						token = COM_Parse( wps_p );
					}
					break;

				case WA_CONTINUE:
					// nothing more to do
					break;

				case WA_GOTO:
					// get the action number
					sscanf( token, "%i", &tag1 );
					(*wA)[eventNum][actionNum].tag1 = tag1 - 1;

					token = COM_Parse( wps_p );
					break;

				case WA_HOLD:
					// nothing more to do
					break;

				default:
					// shouldn't happen
					break;
				}

				// ignore all following parameters on the line
				while ( curLine == COM_GetCurrentParseLine() ) {
					CG_Printf( S_COLOR_YELLOW "Too many parameters in '%s' on line %i.\n", filename, COM_GetCurrentParseLine(), token );
					token = COM_Parse( wps_p );	
				}
				
				//				CG_Printf( "event: %s, action: %i, type: %s, tag1: %i, tag2: %i\n", wpEvNames[eventNum], actionNum, wpActionNames[(*wA)[eventNum][actionNum].type], (*wA)[eventNum][actionNum].tag1, (*wA)[eventNum][actionNum].tag2 );
				actionNum++;
				found = qtrue;
				break;
			}
		}

		// didn't find that action type
		if ( !found ) {
			CG_Printf( S_COLOR_YELLOW "Action type expected in '%s' on line %i, but found '%s'.\n", filename, COM_GetCurrentParseLine(), token );
			return;
		}
	}
}


/*
=================
CG_ParseWPS

Parse a WPS file and save the appropriate info
=================
*/
void CG_ParseWPS( char *filename, int weaponNum ) {
	wpActions_t		*wA;
	fileHandle_t	f;
	qboolean		found;
	char	*wps_p;
	char	*token;
	int		len;
	int		i;
	int		numEvents;

	// reset wpAction structure
	wA = &cg_weapons[weaponNum].action;
	memset( wA, 0, sizeof( *wA ) );

	// read the WPS file
	len = trap_FS_FOpenFile( filename , &f, FS_READ );
	if ( len <= 0 ) {
		CG_Printf( S_COLOR_YELLOW "Weapon Script File '%s' doesn't exist.\n", filename );
		return;
	}
	if ( len >= sizeof( wps ) - 1 ) {
		CG_Printf( S_COLOR_YELLOW "Weapon Script File '%s' too long.\n", filename );
		return;
	}
	trap_FS_Read( wps, len, f );
	wps[len] = 0;
	trap_FS_FCloseFile( f );

	// prepare parsing
	wps_p = wps;
	numEvents = 0;
	COM_ResetParseLines();

	// parse the WPS file, events first
	while ( 1 ) {
		token = COM_Parse( &wps_p );

		// check for additional events
		if ( !*token ) {
//			CG_Printf( "Parsed %i events in '%s'.\n", numEvents, filename );
			return;
		}
		
		found = qfalse;
		// token should be an event name, look which one it is
		for ( i = 0; i < WE_NUM_EVENTS; i++ ) {
			if ( !Q_stricmp( token, wpEvNames[i] ) ) {
				// found the corresponding event name
				numEvents++;

				token = COM_Parse( &wps_p );

				if ( *token != '{' ) {
					CG_Printf( S_COLOR_YELLOW "'{' expected in '%s' on line %i, but found '%s'.\n", filename, COM_GetCurrentParseLine(), token );
					return;
				}

				// parse actions
				CG_ParseWPSActions( filename, wA, i, &wps_p );

				found = qtrue;
				break;
			}
		}

		// didn't find that event type
		if ( !found ) {
			CG_Printf( S_COLOR_YELLOW "Event type expected in '%s' on line %i, but found '%s'.\n", filename, COM_GetCurrentParseLine(), token );
			return;
		}
	}		
}


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path );
	strcat( path, "_flash.md3" );
	weaponInfo->flashModel = trap_R_RegisterModel( path );

	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path );
	strcat( path, "_barrel.md3" );
	weaponInfo->barrelModel = trap_R_RegisterModel( path );

	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path );
	strcat( path, "_barrel_hand.md3" );
	weaponInfo->barrelHandModel = trap_R_RegisterModel( path );

	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path );
	strcat( path, "_hand.md3" );
	weaponInfo->handsModel = trap_R_RegisterModel( path );

	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path );
	strcat( path, "_blue.skin" );
	weaponInfo->blueSkin = trap_R_RegisterSkin( path );	

	weaponInfo->loopFireSound = qfalse;

	// parse the WPS
	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path );
	strcat( path, ".wps" );
	CG_ParseWPS( path, weaponNum );
	
	switch ( weaponNum ) {

	case WP_SHOCKER:
		MAKERGB( weaponInfo->flashDlightColor, 0.3f, 0.3f, 0.6f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/shocker_active.wav", qfalse );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/shocker_fire.wav", qfalse );
		break;

	case WP_STANDARD:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/standard_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_HANDGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/handgun_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_MICRO:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/micro_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_SHOTGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/shotgun_fire.wav", qfalse );
		weaponInfo->brass = BT_SHOTGUN;
		break;

	case WP_SUB:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/sub_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_A_RIFLE:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/a_rifle_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_SNIPER:
		MAKERGB( weaponInfo->flashDlightColor, 0.5f, 0.5f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/soff_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_MINIGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/minigun_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_GRENADE_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/misc/grenade.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 16;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/grenade_fire.wav", qfalse );
		break;

	case WP_DBT:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/misc/plasmabolt.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponInfo->missileDlight = 200;
		weaponInfo->missileTrailFunc = CG_DBTTrail;
		weaponInfo->wiTrailTime = 240;
		weaponInfo->trailRadius = 4;
		
		MAKERGB( weaponInfo->missileDlightColor, 0.1f, 0.2f, 1 );
		MAKERGB( weaponInfo->flashDlightColor, 0.1f, 0.2f, 1 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/dbt_fire.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		break;

	case WP_FLAMETHROWER:
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.4f, 0 );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		break;

	case WP_TRIPACT:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/misc/tripactbolt.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponInfo->missileTrailFunc = CG_TripactTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 400;
		weaponInfo->trailRadius = 12;
		
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/tripact_fire.wav", qfalse );
		cgs.media.tripactExplosionShader = trap_R_RegisterShader( "tripactExplosion" );
		cgs.media.tripactTrailShader = trap_R_RegisterShader( "tripactTrail" );
		cgs.media.tripactTrailKnotShader = trap_R_RegisterShader( "tripactTrailKnot" );
		weaponInfo->brass = BT_TRIPLE;
		break;

	case WP_RPG:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/misc/rocket.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 500;
		weaponInfo->trailRadius = 64;
		
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket_fire.wav", qfalse );
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		break;

	case WP_OMEGA:
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 240;
		weaponInfo->trailRadius = 4;
		
		weaponInfo->effectChangeTime = weLi[WP_OMEGA].spread_crouch;
		weaponInfo->effectStayTime = weLi[WP_OMEGA].spread_crouch + weLi[WP_OMEGA].spread_move;
		weaponInfo->effectRecoverTime = weLi[WP_OMEGA].spread_crouch + weLi[WP_OMEGA].spread_move + weLi[WP_OMEGA].nextshot;

		MAKERGB( weaponInfo->missileDlightColor, 0.1f, 0.2f, 1 );
		MAKERGB( weaponInfo->flashDlightColor, 0.1f, 0.2f, 1 );

		// weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/omega_load.wav", qfalse );
		// weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/omega_load2.wav", qfalse );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/dbt_fire.wav", qfalse );
		cgs.media.omegaExplosionShader = trap_R_RegisterShader( "omegaExplosion" );
		cgs.media.omegaRingsShader = trap_R_RegisterShader( "railDisc" );
		cgs.media.omegaCoreShader = trap_R_RegisterShader( "railCore" );

		// load additional skins for shader effect
		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path );
		strcat( path, "_blue_change.skin" );
		weaponInfo->changeBlueSkin = trap_R_RegisterSkin( path );	

		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path );
		strcat( path, "_blue_red.skin" );
		weaponInfo->altBlueSkin = trap_R_RegisterSkin( path );	

		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path );
		strcat( path, "_blue_recover.skin" );
		weaponInfo->recoverBlueSkin = trap_R_RegisterSkin( path );	

		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path );
		strcat( path, "_red_change.skin" );
		weaponInfo->changeRedSkin = trap_R_RegisterSkin( path );	

		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path );
		strcat( path, "_red_red.skin" );
		weaponInfo->altRedSkin = trap_R_RegisterSkin( path );	

		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path );
		strcat( path, "_red_recover.skin" );
		weaponInfo->recoverRedSkin = trap_R_RegisterSkin( path );	
		break;

	case WP_THERMO:
		MAKERGB( weaponInfo->flashDlightColor, 0, 1, 0 );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/thermo_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );

		cgs.media.thermoBeamShader = trap_R_RegisterShader( "thermoBeam");
		cgs.media.thermoKnotShader = trap_R_RegisterShader( "thermoKnot");
		cgs.media.thermoExplosionShader = trap_R_RegisterShader( "thermoExplosion" );
		break;

	case WP_PHOENIX:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/phoenix_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		weaponInfo->brass = BT_STANDARD;
		break;

	case WP_FRAG_GRENADE:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weapons2/fraggren/fraggren.md3" );
		break;

	case WP_FIRE_GRENADE:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weapons2/firegren/firegren.md3" );
		break;

	case WP_BLAST:
		// nothing here
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		break;

	default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/a_rifle_fire.wav", qfalse );
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( &itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );

	itemInfo->icon = trap_R_RegisterShader( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED			0.7f
#define		THERMO_SPIN_SPEED	0.02f
#define		START_TIME			600
#define		STOP_TIME			2000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	if ( cent->currentState.weapon == WP_FLAMETHROWER ) 
		return 0;
	
	delta = cg.time - cent->pe.barrelTime;

	if ( cent->currentState.weapon == WP_THERMO ) speed = THERMO_SPIN_SPEED;
	else speed = SPIN_SPEED;

	/*	if ( cent->currentState.weapon == WP_THERMO ) {
		angle = cent->pe.barrelAngle + delta * THERMO_SPIN_SPEED;
		return angle;
	}*/

	if ( cent->pe.barrelSpinning ) {
		if ( delta > START_TIME ) {
			angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
		}
		else {
			speed = 0.5 * SPIN_SPEED * (float)( delta ) / START_TIME;
			angle = cent->pe.barrelAngle + delta * speed;
		}
		//	speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
	} else {
		if ( delta > STOP_TIME ) {
			delta = STOP_TIME;
		}

		delta = STOP_TIME - delta;

		speed = 0.5 * SPIN_SPEED * (float)( delta ) / STOP_TIME ;
		if ( speed < 0) {
			speed = 0;
		}
		angle = cent->pe.barrelAngle + STOP_TIME * SPIN_SPEED - delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
		// play the sound
		if ( cent->currentState.weapon == WP_MINIGUN ) {
			if ( cent->pe.barrelSpinning ) trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.minigunStartSound );
			else trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.minigunStopSound );
		}
	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	trap_R_AddRefEntityToScene( gun );

/*	if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
		gun->customShader = cgs.media.battleWeaponShader;
	trap_R_AddRefEntityToScene( gun );
	}
	if ( powerups & ( 1 << PW_QUAD ) ) {
		gun->customShader = cgs.media.quadWeaponShader;
		trap_R_AddRefEntityToScene( gun );
	}*/
}


/*
===============
CG_ThermoBeam
===============
*/
#define KNOT_DIST		60
#define KNOT_SPEED		300
#define PARTICLE_TIME	50

static void CG_ThermoBeam( centity_t *cent, vec3_t origin ) {
	trace_t			trace;
	refEntity_t		beam;
	vec3_t			forward;
	vec3_t			muzzlePoint, endPoint;
	float			frac;

	if ( weLi[cent->currentState.weapon].prediction != PR_BEAM ) {
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	// find muzzle point for this frame
	VectorCopy( cent->lerpOrigin, muzzlePoint );
	AngleVectors( cent->lerpAngles, forward, NULL, NULL );

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( origin, weLi[cent->currentState.weapon].spread_base, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, origin, vec3_origin, vec3_origin, endPoint, 
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.thermoBeamShader;
	trap_R_AddRefEntityToScene( &beam );

	// add additional sprites
	frac = KNOT_SPEED * ( cg.time % (int)(1000 * KNOT_DIST / KNOT_SPEED) ) / 1000.0f;

	memset( &beam, 0, sizeof( beam ) );
	beam.reType = RT_SPRITE;
	beam.radius = 8;
	beam.shaderTime = cg.time / 1000.0f;
	beam.customShader = cgs.media.thermoKnotShader;

	while ( frac / weLi[cent->currentState.weapon].spread_base < trace.fraction ) {
		// set the sprite position and add it
		VectorMA( origin, frac, forward, beam.origin );
		trap_R_AddRefEntityToScene( &beam );

		frac += KNOT_DIST;
	}
	
	// add the impact flare and mark if it hit something
	if ( trace.fraction < 1.0 && !(trace.surfaceFlags & SURF_NOIMPACT) ) {
		if ( cg.time > cent->weaponTime + PARTICLE_TIME ) {
			// it's time to emit impact particles
			cent->weaponTime += PARTICLE_TIME;
			if ( cent->weaponTime < cg.time ) cent->weaponTime = cg.time;
			CG_MissileHitWall( cent->currentState.weapon, qtrue, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
		} else {
			CG_MissileHitWall( cent->currentState.weapon, qfalse, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
		}
	}
}


/*
===============
CG_EffectSkins
===============
*/
void CG_EffectSkins( centity_t *cent, refEntity_t *re, int weaponNum, qboolean blueSkin ) {
	weaponInfo_t	*weapon;

	weapon = &cg_weapons[ weaponNum ];

	if ( blueSkin ) re->customSkin = weapon->blueSkin;

	if ( weaponNum == WP_OMEGA ) {
		re->shaderTime = ( cent->startEffectTime + weapon->effectRecoverTime ) / 1000.0f;

		if ( blueSkin && weapon->blueSkin ) {
			if ( !cent->startEffectTime || cg.time - cent->startEffectTime > weapon->effectRecoverTime ) {
				re->customSkin = weapon->blueSkin;
			} else if ( weapon->changeBlueSkin && cg.time - cent->startEffectTime < weapon->effectChangeTime ) {
				re->customSkin = weapon->changeBlueSkin;
				re->shaderTime = cent->startEffectTime / 1000.0f;
			} else if ( weapon->altBlueSkin && cg.time - cent->startEffectTime < weapon->effectStayTime ) {
				re->customSkin = weapon->altBlueSkin;
				re->shaderTime = cent->startEffectTime / 1000.0f;
			} else {
				re->customSkin = weapon->recoverBlueSkin;
			}
		} else {
			if ( !cent->startEffectTime || cg.time - cent->startEffectTime > weapon->effectRecoverTime ) {
				re->customSkin = 0;
			} else if ( weapon->changeBlueSkin && cg.time - cent->startEffectTime < weapon->effectChangeTime ) {
				re->customSkin = weapon->changeRedSkin;
				re->shaderTime = cent->startEffectTime / 1000.0f;
			} else if ( weapon->altBlueSkin && cg.time - cent->startEffectTime < weapon->effectStayTime ) {
				re->customSkin = weapon->altRedSkin;
				re->shaderTime = cent->startEffectTime / 1000.0f;
			} else {
				re->customSkin = weapon->recoverRedSkin;
			}
		}
	} else {
		float	fak = 1.0f;

		if ( cent->startEffectTime > 0 ) fak = 1.0f - (cg.time - cent->startEffectTime) / 700.0f; 
		else if ( cent->startEffectTime < 0 ) fak = (cg.time + cent->startEffectTime) / 700.0f; 

		if ( fak > 1.0f ) fak = 1.0f;
		if ( fak < 0.0f ) fak = 0.0f;

		re->shaderRGBA[0] = 255 * fak;
		re->shaderRGBA[1] = 255 * fak;
		re->shaderRGBA[2] = 255 * fak;
		re->shaderRGBA[3] = 255;
	}
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	centity_t	*selectedCent;
//	int	col;

	weaponNum = cent->currentState.weapon;

	// check for grenade drawing
	if ( weLi[weaponNum].category == CT_EXPLOSIVE && cg.time < cent->lastEffectTime ) {
		return;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel) {
		return;
	}

	CG_EffectSkins( cent, &gun, weaponNum, qfalse );	

	CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");

	if ( !ps ) {
		cent->pe.lightningFiring = qfalse;
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			cent->pe.lightningFiring = qtrue;
		} 
		else if ( weapon->readySound ) {
			// trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}

		CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );

		// add the spinning barrel
		if ( weapon->barrelModel ) {
			memset( &barrel, 0, sizeof( barrel ) );
			VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx = parent->renderfx;

			CG_EffectSkins( cent, &barrel, weaponNum, qfalse );	

			barrel.hModel = weapon->barrelModel;
			angles[YAW] = 0;
			angles[PITCH] = 0;
			angles[ROLL] = CG_MachinegunSpinAngle( cent );
			AnglesToAxis( angles, barrel.axis );

			CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );

			if ( weaponNum != WP_FLAMETHROWER || !( CG_PointContents( barrel.origin, -1 ) & CONTENTS_WATER ) ) {
				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
			}
		}
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}
	if ( cg.predictWeapons ) selectedCent = cent;
	else selectedCent = nonPredictedCent;

	// eject brass
	if ( !ps && cent->ejectBrass > 0 ) {
		CG_EjectBrass( &gun, cent );
		cent->ejectBrass = -cent->ejectBrass;
	}

	// add smoke (not with ragepro)
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO && weaponNum == WP_RPG && 
		cg.time - cent->startEffectTime < weLi[weaponNum].nextshot ) {
		refEntity_t	smoke;
		float	phase;

		// initialize
		memset( &smoke, 0, sizeof( refEntity_t ) );

		// get positions
		if ( !ps ) CG_PositionEntityOnTag( &smoke, &gun, weapon->weaponModel, "tag_eject");
		else CG_PositionEntityOnTag( &smoke, parent, weapon->handsModel, "tag_eject");

		// don't create smoke underwater
		if ( CG_PointContents( smoke.origin, -1 ) & CONTENTS_WATER ) {
			return;
		}

		// add time
		cent->lastEffectTime += cg.frametime;
		phase = 1.0 - (float)( cg.time - cent->startEffectTime ) / weLi[weaponNum].nextshot * 0.8;

		while ( cent->lastEffectTime > 0 ) {
			vec3_t	origin;
			vec3_t	speed;

			// back smoke
			VectorScale( smoke.axis[0], 150 * phase, speed );
			VectorMA( smoke.origin, 150 * phase * cent->lastEffectTime / 1000.0f, smoke.axis[0], origin );

			CG_SmokePuff( origin, speed, 
						64, 1, 1, 1, 0.55f,
						1000, cg.time, 0, 0, 
						cgs.media.smokePuffShader );

			// subtract time
			cent->lastEffectTime -= 40;
		}
	}

	// position the flash
	memset( &flash, 0, sizeof( flash ) );

	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );
	if ( !ps ) CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash");
	else CG_PositionRotatedEntityOnTag( &flash, parent, weapon->handsModel, "tag_flash");

	VectorCopy( flash.origin, nonPredictedCent->muzzlePoint );
	VectorCopy( flash.origin, cent->muzzlePoint );

	// add the flash
	if ( ( weLi[weaponNum].prediction == PR_MELEE || weLi[weaponNum].prediction == PR_FLAME || weLi[weaponNum].prediction == PR_BEAM ) ) {
		// continuous flash
		if ( !(selectedCent->currentState.eFlags & EF_FIRING) ) {
			return;
		}
	} else {
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME ) { 
			return;
		}
	}

	// draw Thermo Lance beam and flamethrower fire
	if ( ps || cg.renderingThirdPerson ||
		cent->currentState.number != cg.predictedPlayerState.clientNum ) {

		if ( cg.predictWeapons ) {
			CG_ThermoBeam( cent, flash.origin );
			CG_FireFlame( cent );
		} else {
			CG_ThermoBeam( nonPredictedCent, flash.origin );
			CG_FireFlame( nonPredictedCent );
		}
	}

	if ( CG_PointContents( flash.origin, -1 ) & CONTENTS_WATER ) {
		// no flashes underwater (yet?)
		return;
	}

	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	// colorize flash
	if ( weaponNum == WP_OMEGA ) {
		int	weaponTime;

		// look up weaponTime
		weaponTime = nonPredictedCent->weaponTime << 4;

		flash.shaderRGBA[0] = 255 * ( (float)weaponTime / weLi[WP_OMEGA].spread_crouch ) * 0.75;
		flash.shaderRGBA[1] = 255 * ( 1 - (float)weaponTime / weLi[WP_OMEGA].spread_crouch ) * 0.25;
		flash.shaderRGBA[2] = 255 * ( 1 - (float)weaponTime / weLi[WP_OMEGA].spread_crouch ) * 0.75;
		flash.shaderRGBA[3] = 255;
	}

	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
		return;
	}

	trap_R_AddRefEntityToScene( &flash );

	if ( ps || cg.renderingThirdPerson ||
		cent->currentState.number != cg.predictedPlayerState.clientNum ) {

		if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flash.origin, 300 + (rand()&31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
		}
	} 

}



// =====================
// View Weapon Functions
// =====================


void CG_NewWeaponEvent( centity_t *cent, wpActions_t *wA, int newEvent, qboolean force ) {
	if ( force || cent->pe.curEvent != newEvent || (*wA)[newEvent][0].type != WA_CONTINUE ) {
		// set new event handling routine
		cent->pe.curEvent = newEvent;
		cent->pe.curAction = -1;
		cent->pe.hand.animationTime = 0;
		if ( newEvent == WE_RAISE || newEvent == WE_DROP ) cent->startEffectTime = 0;
//		CG_Printf( "New event: %s\n", wpEvNames[newEvent] );
	}
}


/*
==============
CG_DoWeaponActions

Perform the actions parsed during weapon loading
==============
*/
void CG_DoWeaponActions( centity_t *cent, wpActions_t *wA, qboolean stop ) {
	int		cE;
	int		type, tag1, tag2;

	cE = cent->pe.curEvent;

	// interpolate lerpFrame
	if ( cent->pe.hand.animationTime > 0 ) {
		// decrease animationTime
		cent->pe.hand.animationTime -= cg.frametime;
		// modify backlerp (do animation)
		cent->pe.hand.backlerp = (float)cent->pe.hand.animationTime / cent->pe.hand.frameTime;
	}

	// check if single-frame animation is finished and perform actions until we need time to animate
	while ( cent->pe.hand.animationTime <= 0 ) {
		if ( (*wA)[cE][cent->pe.curAction].type == WA_HOLD ) break;
		if ( cent->pe.curAction >= 0 && (*wA)[cE][cent->pe.curAction].type == WA_NONE ) {
			// end of event handling routine
			if ( cE != WE_IDLE && cE != WE_IDLE_EMPTY ) CG_NewWeaponEvent( cent, wA, WE_IDLE, qfalse );
			break;
		}

		// perform next action
		cent->pe.curAction++;
		type = (*wA)[cE][cent->pe.curAction].type;
		tag1 = (*wA)[cE][cent->pe.curAction].tag1;
		tag2 = (*wA)[cE][cent->pe.curAction].tag2;

//		CG_Printf( "New action: %i in event %s, type: %s, tag1: %i, tag2: %i\n", cent->pe.curAction, wpEvNames[cE], wpActionNames[type], tag1, tag2 );

		switch ( type ) {
		case WA_ANIM:
			cent->pe.hand.frameTime = tag1;
			cent->pe.hand.animationTime += tag1;
			cent->pe.hand.oldFrame = cent->pe.hand.frame;
			cent->pe.hand.frame = tag2;
			cent->pe.hand.backlerp = (float)cent->pe.hand.animationTime / tag1;
			break;

		case WA_SND:
			if ( !cg.renderingThirdPerson ) {
				switch ( tag1 ) {
				case WS_CLIPIN:
					trap_S_StartSound (NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.clipInSound );
					break;
				case WS_CLIPOUT:
					trap_S_StartSound (NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.clipOutSound );
					break;
				case WS_VERSCHLUSS:
					trap_S_StartSound (NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.verschlussSound );
					break;
				default:
					// shouldn't happen
					break;
				}
			}
			break;

		case WA_FUNC:
			switch ( tag1 ) {
			case WF_EJECT:
				cent->ejectBrass = 1;
				break;
			case WF_EJECT_NEXT:
				cent->ejectBrass = -cent->ejectBrass + 1;
				break;
			case WF_SHADER_CHANGE:
				if ( cent->startEffectTime <= 0 ) cent->startEffectTime = cg.time;
				else cent->startEffectTime = -cg.time;
				break;
			default:
				// shouldn't happen
				break;
			}
			break;

		case WA_EVENT:
			if ( tag1 > 0 ) CG_NewWeaponEvent( cent, wA, tag1, qtrue );
			cE = cent->pe.curEvent;
			break;

		case WA_CONTINUE:
			// just do nothing here
			break;

		case WA_GOTO:
			if ( !stop ) cent->pe.curAction = tag1;
			break;

		case WA_HOLD:
			
			break;

		default:
			// shouldn't happen
			break;
		}
	}

	// never extrapolate
	if ( cent->pe.hand.backlerp < 0 ) cent->pe.hand.backlerp = 0;
}


/*
==============
CG_AnimViewWeapon

Animate the weapon in the player's view
==============
*/
void CG_AnimViewWeapon( centity_t *cent, playerState_t *ps ) {
	wpActions_t		*wA;	

	wA = &cg_weapons[ps->weapons[ps->wpcat]].action;

	// check for changing follow mode
	if ( cg.followChange ) {
		cg.followChange = qfalse;
		// reset animations
		cent->pe.curEvent = 0;
		cent->pe.curAction = -1;
		cent->pe.hand.animationTime = 0;
		cent->startEffectTime = 0;
	}

	// check for potential animation changes
	if ( ps->weaponTime > cent->pe.oldWeaponTime || ps->weaponstate != cent->pe.oldWeaponState ) { 
		switch (ps->weaponstate) {
		case WEAPON_FIRING:
			if ( ps->wpcat == CT_EXPLOSIVE ) break;
			if ( ps->clipammo[ps->wpcat] > 0 ) CG_NewWeaponEvent( cent, wA, WE_FIRE, qfalse );
			else CG_NewWeaponEvent( cent, wA, WE_FIRE_EMPTY, qfalse );
			break;
		case WEAPON_DROPPING:
			if ( cent->pe.oldWeaponState != WEAPON_RAISING ) CG_NewWeaponEvent( cent, wA, WE_DROP, qfalse );
			else CG_NewWeaponEvent( cent, wA, WE_RAISE, qtrue );
			break;
		case WEAPON_RAISING:
			CG_NewWeaponEvent( cent, wA, WE_RAISE, qtrue );
			break;
		case WEAPON_RELOADING:
			if ( ps->clipammo[ps->wpcat] > 0 ) CG_NewWeaponEvent( cent, wA, WE_RELOAD, qfalse );
			else CG_NewWeaponEvent( cent, wA, WE_RELOAD_EMPTY, qfalse );
			break;
		case WEAPON_START_FIRING:
			if ( ps->wpcat == CT_EXPLOSIVE && ps->weaponstate != cent->pe.oldWeaponState ) CG_NewWeaponEvent( cent, wA, WE_FIRE_EMPTY, qfalse );
			break;
		case WEAPON_CONTINUE_FIRING:
			if ( ps->wpcat == CT_EXPLOSIVE ) CG_NewWeaponEvent( cent, wA, WE_FIRE, qfalse );
			break;
		default:
			break;
		}
	}

	// perform actions
	CG_DoWeaponActions( cent, wA, ps->weaponTime <= 0 );

	// correct curEvent WE_IDLE if necessary
	if ( ps->clipammo[ps->wpcat] == 0 && ps->weaponstate != WEAPON_RELOADING && cent->pe.curEvent == WE_IDLE ) 
		CG_NewWeaponEvent( cent, wA, WE_IDLE_EMPTY, qtrue );

	// store old weaponTime and State to check for differences
	cent->pe.oldWeaponTime = ps->weaponTime;
	cent->pe.oldWeaponState = ps->weaponstate;
}



/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	refEntity_t	barrel;
	centity_t	*cent;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;
	entityState_t *ent;

	if ( ps->persistant[PERS_TEAM] >= TEAM_SPECTATOR || ps->pm_type >= PM_SPECTATOR || ps->weapons[ps->wpcat] == WP_NONE ) {
		return;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];

	// animate the weapon even if not displayed
	CG_AnimViewWeapon( cent, ps );

	// no gun if in third person view and zoom
	if ( cg.renderingThirdPerson || cg.zoomed ) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fov.integer > 90 ) {
		fovOffset = -0.2 * ( cg_fov.integer - 90 );
	} else {
		fovOffset = 0;
	}

	CG_RegisterWeapon( ps->weapons[ps->wpcat] );
	weapon = &cg_weapons[ ps->weapons[ps->wpcat] ];
	ent = &cent->currentState;

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// copy the animation data into the refEntity
	hand.frame = cent->pe.hand.frame;
	hand.oldframe = cent->pe.hand.oldFrame;
	hand.backlerp = cent->pe.hand.backlerp;

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	CG_EffectSkins( cent, &hand, ps->weapons[ps->wpcat], cgs.clientinfo[ps->clientNum].handType == HT_BLUE );

	// add the weapon (with hands) to the scene
	trap_R_AddRefEntityToScene( &hand );

	// add the spinning barrel
	if ( weapon->barrelHandModel ) {
		memset( &barrel, 0, sizeof( barrel ) );
		VectorCopy( hand.lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = hand.shadowPlane;
		barrel.renderfx = hand.renderfx;

		CG_EffectSkins( cent, &barrel, ps->weapons[ps->wpcat], qfalse );

		barrel.hModel = weapon->barrelHandModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle( cent );
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &hand, weapon->handsModel, "tag_barrel" );

		if ( ps->weapons[ps->wpcat] != WP_FLAMETHROWER || !( CG_PointContents( barrel.origin, -1 ) & CONTENTS_WATER ) ) {
			trap_R_AddRefEntityToScene( &barrel );
		}
	}

	// eject brass
	if ( cent->ejectBrass > 0 ) {
		CG_EjectBrass( &hand, cent );
		cent->ejectBrass = -cent->ejectBrass;
	}

	// add the effects you have on worldother characters too
	CG_AddPlayerWeapon( &hand, ps, cent, 1 );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_CatSelectable( int i ) {
	if ( !cg.predictedPlayerState.weapons[i] ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.catSelectTime = cg.time;
	original = cg.catSelect;

	for ( i = 0 ; i < MAX_WP_CATEGORIES ; i++ ) {
		cg.catSelect++;
		if ( cg.catSelect == MAX_WP_CATEGORIES ) {
			cg.catSelect = 0;
		}
		if ( CG_CatSelectable( cg.catSelect ) ) {
			break;
		}
	}
	if ( i == MAX_WP_CATEGORIES ) {
		cg.catSelect = original;
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.catSelectTime = cg.time;
	original = cg.catSelect;

	for ( i = 0 ; i < MAX_WP_CATEGORIES ; i++ ) {
		cg.catSelect--;
		if ( cg.catSelect == -1 ) {
			cg.catSelect = MAX_WP_CATEGORIES - 1;
		}
		if ( CG_CatSelectable( cg.catSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.catSelect = original;
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_WeaponChange( int num ) {
	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num--;
	if ( num < 0 || num >= MAX_WP_CATEGORIES || !CG_CatSelectable( num ) ) {
		return;
	}

	cg.catSelectTime = cg.time;
	cg.catSelect = num;
}


