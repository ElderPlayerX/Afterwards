// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"


/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ((refEntity_t *)parent)->axis, entity->axis );
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	} else {
		trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent ) {

	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound ) {
		if (cent->currentState.eType != ET_SPEAKER) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		} else {
			trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		}
	}


	// constant light glow
	if ( cent->currentState.constantLight ) {
		int		cl;
		int		i, r, g, b;

		cl = cent->currentState.constantLight;
		r = cl & 255;
		g = ( cl >> 8 ) & 255;
		b = ( cl >> 16 ) & 255;
		i = ( ( cl >> 24 ) & 255 ) * 4;
		trap_R_AddLightToScene( cent->lerpOrigin, i, r, g, b );
	}

}


/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	// add to refresh list
	trap_R_AddRefEntityToScene (&ent);
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

/*
==================
CG_Item
==================
*/
static void CG_Item( centity_t *cent ) {
	refEntity_t		ent;
	entityState_t	*es;
	gitem_t			*item;
//	int				msec;
	float			frac;
//	float			scale;
	weaponInfo_t	*wi;
	vec3_t spinAngles;

	es = &cent->currentState;
	if ( es->modelindex >= bg_numItems ) {
		CG_Error( "Bad item index %i on entity", es->modelindex );
	}

	// if set to invisible, skip
	if ( !es->modelindex || ( es->eFlags & EF_NODRAW ) ) {
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// its a mapper defined holdable item
	if ( es->modelindex2 > 0 ) {
		cent->lerpOrigin[2] -= 6;						// move item down to make it lie on the ground
		VectorCopy( cent->lerpOrigin, ent.origin);
		VectorCopy( cent->lerpOrigin, ent.oldorigin);

		ent.hModel = cgs.gameModels[es->modelindex];

		// convert angles to axis
		AnglesToAxis( cent->lerpAngles, ent.axis );

		// add to refresh list
		trap_R_AddRefEntityToScene (&ent);
		return;
	}

	item = &bg_itemlist[ es->modelindex ];

	AnglesToAxis( cent->currentState.angles, ent.axis );
	
//	ent.axis[0][0] = 1.0;
//	RotateAroundDirection( ent.axis, 90 );

	wi = NULL;
	if ( item->giType == IT_WEAPON ) {
		wi = &cg_weapons[item->giTag];
		// rotate weapon and move onto the ground
		cent->lerpOrigin[2] -= 12.0;
	}

	ent.hModel = cg_items[es->modelindex].models[0];

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	frac = 1.0;

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	if ( item->giType == IT_WEAPON ) {
		ent.renderfx |= RF_MINLIGHT;

		// draw BIG grenades
		if ( weLi[item->giTag].category == CT_EXPLOSIVE ) {
			AxisScale( ent.axis, GRENADE_SCALE, ent.axis );
		}
	}
	
	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	if ( item->giType == IT_WEAPON && wi->barrelModel && item->giTag != WP_FLAMETHROWER ) {
		refEntity_t	barrel;

		memset( &barrel, 0, sizeof( barrel ) );

		barrel.hModel = wi->barrelModel;

		VectorCopy( ent.lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		CG_PositionRotatedEntityOnTag( &barrel, &ent, wi->weaponModel, "tag_barrel" );

		AxisCopy( ent.axis, barrel.axis );
		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap_R_AddRefEntityToScene( &barrel );
	}

	// accompanying rings / spheres for powerups
	VectorClear( spinAngles );

	if ( item->giType == IT_HEALTH || item->giType == IT_POWERUP )
	{
		if ( ( ent.hModel = cg_items[es->modelindex].models[1] ) != 0 )
		{
			if ( item->giType == IT_POWERUP )
			{
				ent.origin[2] += 12;
				spinAngles[1] = ( cg.time & 1023 ) * 360 / -1024.0f;
			}
			AnglesToAxis( spinAngles, ent.axis );
			
			// scale up if respawning
			if ( frac != 1.0 ) {
				VectorScale( ent.axis[0], frac, ent.axis[0] );
				VectorScale( ent.axis[1], frac, ent.axis[1] );
				VectorScale( ent.axis[2], frac, ent.axis[2] );
				ent.nonNormalizedAxes = qtrue;
			}
			trap_R_AddRefEntityToScene( &ent );
		}
	}
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;

	s1 = &cent->currentState;

	// test for drawing
	if ( !cent->ti.trailTime ) {
		return;
	}

	// get weapon info
	if ( s1->weapon > WP_NUM_WEAPONS ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

	// add trails
	if ( weapon->missileTrailFunc ) {
		weapon->missileTrailFunc( &cent->ti, ( s1->eFlags & EF_TRAILONLY ) ? s1->time2 : cg.time );
	}

	// stop if only the trails should be drawn
	if ( s1->eFlags & EF_TRAILONLY ) 
		return;

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, 
			weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2] );
	}

	// add missile sound
	if ( weapon->missileSound && cg.snap->ps.clientNum != s1->clientNum ) {
		vec3_t	velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );

		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound );
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}

	// spin as it moves
	if ( s1->pos.trType != TR_STATIONARY ) {
		if ( s1->pos.trType == TR_GRAVITY ) {
			RotateAroundDirection( ent.axis, cg.time / 4 );
		} else if ( s1->pos.trType == TR_WATER_GRAVITY ) {
			RotateAroundDirection( ent.axis, cg.time / 8 );
		} else {
			RotateAroundDirection( ent.axis, cg.time * 2 );
		}
	} else {
		RotateAroundDirection( ent.axis, s1->time );
	}

	// draw BIG grenades
	if ( weLi[s1->weapon].category == CT_EXPLOSIVE ) {
		AxisScale( ent.axis, GRENADE_SCALE, ent.axis );
	}

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

//============================================================================

/*
===============
CG_Blast
===============
*/
static void CG_Blast( centity_t *cent ) {
	refEntity_t			ent;

	// add dynamic light
//	trap_R_AddLightToScene(cent->lerpOrigin, 50, 1.0f, 0, 0 );

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.skinNum = 1;
	ent.hModel = cg_weapons[WP_BLAST].weaponModel;
	ent.renderfx = RF_NOSHADOW;

	ent.axis[0][0] = 1;
	RotateAroundDirection( ent.axis, 0 );
	// add to refresh list, possibly with quad glow
	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_Mover
===============
*/
static void CG_Mover( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}

}


/*
==================
CG_ScriptedModel
==================
*/
static void CG_ScriptedModel( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	int		oldFrame;
	int		curFrame;
	int		nextFrame;

	s1 = &cent->currentState;

	// if no model set, skip
	if ( !s1->modelindex ) {
		return;
	}

	// stop if it's invisible
	if ( s1->generic1 & SMF_INVISIBLE ) {
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// read frame values
	oldFrame = ( s1->otherEntityNum2 ) & 0xFF;
	curFrame = ( s1->frame >> 8 ) & 0xFF;
	nextFrame = s1->frame & 0xFF;

	// set correct frames and backlerp
	if ( cg.time - s1->time < s1->time2 ) {
		ent.frame = curFrame;
		ent.oldframe = oldFrame;
		ent.backlerp = 1.0 - (float)( cg.time - s1->time ) / s1->time2;
	} else if ( cg.time - s1->time < 2 * s1->time2 ) {
		ent.frame = nextFrame;
		ent.oldframe = curFrame;
		ent.backlerp = 1.0 - (float)( cg.time - s1->time - s1->time2 ) / s1->time2;
	} else {
		ent.frame = nextFrame;
		ent.oldframe = nextFrame;
		ent.backlerp = 0.0;
	}

	// set origin
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	// set model
	ent.hModel = cgs.gameModels[s1->modelindex];

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	// add to refresh list
	trap_R_AddRefEntityToScene (&ent);
}


/*
===============
CG_Breakable
===============
*/
static void CG_Breakable( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->currentState.origin, ent.origin);
	VectorCopy( cent->currentState.origin, ent.oldorigin);
	AnglesToAxis( cent->currentState.angles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}

}


/*
===============
CG_Effect
===============
*/
static void CG_Effect( centity_t *cent ) {
	entityState_t		*s1;

	s1 = &cent->currentState;

	// add the secondary model
	if ( s1->modelindex && ( !(s1->generic1 & (1<<6)) || s1->generic1 & (1<<7) ) ) {
		refEntity_t			ent;
		// create the render entity
		memset (&ent, 0, sizeof(ent));
		VectorCopy( cent->currentState.origin, ent.origin);
		VectorCopy( cent->currentState.origin, ent.oldorigin);
		AnglesToAxis( cent->currentState.angles, ent.axis );

		ent.renderfx = RF_NOSHADOW;
	
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex];
		trap_R_AddRefEntityToScene(&ent);
	}

	// add effect if active
	if ( s1->generic1 & (1<<7) ) {
		switch ( s1->generic1 & ~(1<<7 | 1<<6) ) {
		case FX_FIRE:
			CG_FXFire( cent );
			break;
		case FX_LIGHTNING:
			CG_FXLightning( cent );
			break;
		default:
			// shouldn't happen
			break;
		}
	}
}


/*
===============
CG_EmitParticles
===============
*/
#define MAX_PARTICLES_PER_FRAME		20

void CG_EmitParticles( centity_t *cent, qboolean instantly ) {
	entityState_t	*s1;
	int		numParticles;
	int		addTime;
	int		time;

	// get important vars
	s1 = &cent->currentState;

	// determine number of particles to render
	if ( instantly ) {
		float	randomPps, pps;

		pps = s1->time / 100.0f;
		randomPps = s1->time2 / 100.0f;
		numParticles = pps + crandom()*randomPps;
		time = cg.time;
		addTime = 0;
	} else {
		if ( cent->miscTime < cent->startEffectTime ) cent->miscTime = cent->startEffectTime;
		numParticles = ( cg.time - cent->miscTime ) * cent->pps / 1000.0f;
		addTime = numParticles * 1000.0f / cent->pps;
		time = cent->miscTime;
		if ( numParticles > MAX_PARTICLES_PER_FRAME ) numParticles = MAX_PARTICLES_PER_FRAME;
	}

	// render particles if there are any
	if ( numParticles ) {
		qhandle_t	model, shader;
		float	randomSpeed, size, randomSize, addSize, randomAddSize;
		int		randomDir, randomPos, life, randomLife;
		int		i;

		// get some vars
		randomDir = (((int)s1->angles2[0] >> 8) & 0xFF);
		randomSpeed = (((int)s1->angles2[0] >> 16) & 0xFFFF) / 10.0f;
		randomPos = ((int)s1->angles2[0] & 0xFF) << 2;
		size = ((int)s1->origin2[0] & 0xFFFF) / 100.0f;
		randomSize = (((int)s1->origin2[0] >> 16) & 0xFFFF) / 100.0f;
		addSize = ((int)s1->angles2[2] & 0xFFFF) / 100.0f;
		randomAddSize = (((int)s1->angles2[2] >> 16) & 0xFFFF) / 100.0f;
		life = (int)s1->angles2[1] & 0xFFFF;
		randomLife = ((int)s1->angles2[1] >> 16) & 0xFFFF;
		
		if ( s1->generic1 & PF_MODEL ) {
			model = cgs.gameModels[s1->modelindex];
			shader = 0;
		} else {
			model = 0;
			shader = cgs.gameShaders[s1->modelindex];
		}

		for ( i = 0; i < numParticles; i++ ) {
			// go on in time
			time += addTime / numParticles;

			// generate particle
			CG_GenerateParticles( model, shader, s1->origin, randomPos, s1->angles, randomDir, 
						  randomSpeed, 1, s1->number, time, life, randomLife, size, 
						  randomSize, addSize, randomAddSize, s1->generic1, 0 );
		}
	}

	// set new time
	cent->miscTime += addTime;
}


/*
===============
CG_Particles
===============
*/
static void CG_Particles( centity_t *cent ) {
	entityState_t	*s1;
	int wait;
	float	pps;

	// get important vars
	s1 = &cent->currentState;
	wait = ((int)s1->origin2[2] & 0xFFFF) * 10;
	pps = s1->time / 100.0f;

	// do nothing if off or no duration is specified
	if ( (s1->generic1 & PF_OFF) ) {
		return;
	}

	// never again (until triggered)
	if ( cg.time >= cent->lastEffectTime ) {
		cent->startEffectTime = 0;
	}
	// end of phase 
	if ( wait && !(s1->generic1 & PF_FOREVER) && cg.time >= cent->lastEffectTime ) {
		int		duration, randomDuration, randomWait;
		float	randomPps;

		duration = ((int)s1->origin2[1] & 0xFFFF) * 10;
		randomDuration = (((int)s1->origin2[1] >> 16) & 0xFFFF) * 10;
		randomWait = (((int)s1->origin2[2] >> 16) & 0xFFFF) * 10;
		randomPps = s1->time2 / 100.0f;

		cent->startEffectTime = cg.time + wait + crandom()*randomWait; 
		cent->lastEffectTime = cent->startEffectTime + duration + crandom()*duration;
		cent->pps = pps + crandom()*randomPps;
	}

	if ( (s1->generic1 & PF_FOREVER) || ( cent->startEffectTime && cg.time >= cent->startEffectTime ) ) {
		// tiny hack...
		if ( s1->generic1 & PF_FOREVER ) cent->pps = pps;
		// play looped sound
		if ( s1->otherEntityNum ) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ s1->otherEntityNum ] );
		}
		// emit particles
		CG_EmitParticles( cent, qfalse );
	}
}


/*
===============
CG_GetLightColor
===============
*/
void CG_GetLightColor( int source, vec3_t *color ) {
	(*color)[0] = BYTE2FLOAT(source & 0xFF);
	(*color)[1] = BYTE2FLOAT((source >> 8) & 0xFF);
	(*color)[2] = BYTE2FLOAT((source >> 16) & 0xFF);
}


/*
===============
CG_Light
===============
*/
static void CG_Light( centity_t *cent ) {
	entityState_t	*s1;
	vec3_t			color;
	int		light_off, light_on;

	s1 = &cent->currentState;

	// get intensity values
	light_off = (int)s1->origin2[2] & 0xFFFF;
	light_on = (int)s1->origin2[2] >> 16;

	if ( s1->generic1 & LP_ACTIVE && light_on ) {
		// add light on
		CG_GetLightColor( (int)s1->origin2[1], &color );
		trap_R_AddLightToScene( s1->origin, light_on, color[0], color[1], color[2] );
	}
	if ( !(s1->generic1 & LP_ACTIVE) && light_off ) {
		// add light off 
		CG_GetLightColor( (int)s1->origin2[0], &color );
		trap_R_AddLightToScene( s1->origin, light_off, color[0], color[1], color[2] );
	}
}


/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[0] );
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;		// rotation speed
	ent.skinNum = s1->clientNum/256.0 * 360;	// roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out ) {
	centity_t	*cent;
	vec3_t	oldOrigin, origin, deltaOrigin;
	vec3_t	oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );

	// FIXME: origin change when on a rotating object
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t		current, next;
	float		f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent ) {

	// if this player does not want to see extrapolated players
	if ( !cg_smoothClients.integer ) {
		// make sure the clients use TR_INTERPOLATE
		if ( cent->currentState.number < MAX_CLIENTS ) {
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
											cent->currentState.number < MAX_CLIENTS) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum, 
		cg.snap->serverTime, cg.time, cent->lerpOrigin );
	}
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	switch ( cent->currentState.eType ) {
	default:
		CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_SCRIPTED_MODEL:
		CG_ScriptedModel( cent );
		break;
	case ET_BREAKABLE:
		CG_Breakable( cent );
		break;
	case ET_EFFECT:
		CG_Effect( cent );
		break;
	case ET_PARTICLES:
		CG_Particles( cent );
		break;
	case ET_LIGHT:
		CG_Light( cent );
		break;
	case ET_BLAST:
		CG_Blast( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
	}

	// the entity is completely calculated
	cent->notDone = qfalse;
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
	} else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because 
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = 0;
//	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = 0;
//	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

	// mustn't be added in order, so give a possibility to see which ones are already added
	for ( num = 0 ; num < cg.snap->numEntities ; num++ )
		cg_entities[ cg.snap->entities[ num ].number ].notDone = qtrue;

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		if ( cent->notDone ) CG_AddCEntity( cent );
	}
}

