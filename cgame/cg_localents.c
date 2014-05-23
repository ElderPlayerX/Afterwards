// Copyright (C) 1999-2000 Id Software, Inc.
//

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

#define	MAX_LOCAL_ENTITIES	1024
localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t	cg_activeLocalEntities;		// double linked list
localEntity_t	*cg_freeLocalEntities;		// single linked list

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
void	CG_InitLocalEntities( void ) {
	int		i;

	memset( cg_localEntities, 0, sizeof( cg_localEntities ) );
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i+1];
	}
}


/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity( localEntity_t *le ) {
	if ( !le->prev ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t	*CG_AllocLocalEntity( void ) {
	localEntity_t	*le;

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity( cg_activeLocalEntities.prev );
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}


/*
===================
CG_CheckDistance

Will return qtrue if there's an LE of the type leType 
with the specified shader within minDistance that 
lives for more than minLifeTime
===================
*/
qboolean CG_CheckDistance( vec3_t origin, int leType, int trType, int shader, int minLifeTime, int minDistance ) {
	localEntity_t	*le;
	vec3_t			delta;

	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = le->prev ) {
		VectorSubtract( le->refEntity.origin, origin, delta );
		if ( le->refEntity.customShader == shader && le->leType == leType && le->pos.trType == trType
			&& le->endTime - cg.time > minLifeTime && VectorLength( delta ) < minDistance ) {
			// found such a LE
			return qtrue;
		}
	}
	return qfalse;
}


/*
===================
CG_NearbyDrawnLeCount

Will return number of LEs of a specified leType
within distance that are already processed
===================
*/
int CG_NearbyDrawnLeCount( localEntity_t *curLe, vec3_t origin, int leType, int distance ) {
	localEntity_t	*le;
	vec3_t			delta;
	int				n;

	n = 0;
	le = cg_activeLocalEntities.prev;
	for ( ; le != curLe ; le = le->prev ) {
		VectorSubtract( le->refEntity.origin, origin, delta );
		if ( le->leType == leType && VectorLength( delta ) < distance ) {
			// found such a LE
			n++;
		}
	}
	return n;
}


/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

/*
================
CG_BloodTrail

Leave expanding blood puffs behind gibs
================
*/
void CG_BloodTrail( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t	*blood;

	step = 150;
	t = step * ( (cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	for ( ; t <= t2; t += step ) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

		blood = CG_SmokePuff( newOrigin, vec3_origin, 
					  20,		// radius
					  1, 1, 1, 1,	// color
					  2000,		// trailTime
					  t,		// startTime
					  0,		// fadeInTime
					  0,		// flags
					  cgs.media.bloodTrailShader );
		// use the optimized version
		blood->leType = LE_FALL_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		blood->pos.trDelta[2] = 40;
	}
}


/*
================
CG_FireTrail

Leave expanding fire and smoke puffs behind gibs
================
*/
void CG_FireTrail( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t	*fire;

	if ( cg_lowDetailEffects.integer ) step = 240;
	else step = 120;
	t = step * ( (cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	for ( ; t <= t2; t += step ) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

//		CG_LaunchExplode( newOrigin, vec3_origin, cgs.media.rocketExplosionShader, qtrue, 900, 250, 128 );
		fire = CG_SmokePuff( newOrigin, vec3_origin, 
					  24,		// radius
					  1, 1, 1, 1,	// color
					  1150,		// trailTime
					  t,		// startTime
					  0,		// fadeInTime
					  0,		// flags
					  cgs.media.simpleFireShader );
		// use the optimized version
		fire->leType = LE_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		fire->pos.trDelta[2] = 40;
	}
}


/*
================
CG_FragmentBounceMark
================
*/
void CG_FragmentBounceMark( localEntity_t *le, trace_t *trace ) {
	int			radius;

	if ( le->leMarkType == LEMT_BLOOD ) {

		radius = 16 + (rand()&31);
		CG_ImpactMark( cgs.media.bloodMarkShader, trace->endpos, trace->plane.normal, random()*360,
			1,1,1,1, qtrue, radius, qfalse );
	} else if ( le->leMarkType == LEMT_BURN ) {

		radius = 8 + (rand()&15);
		CG_ImpactMark( cgs.media.burnMarkShader, trace->endpos, trace->plane.normal, random()*360,
			1,1,1,1, qtrue, radius, qfalse );
	}


	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->leMarkType = LEMT_NONE;
}


/*
================
CG_ReflectVelocity
================
*/
void CG_ReflectVelocity( localEntity_t *le, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, le->pos.trDelta );

	VectorScale( le->pos.trDelta, le->bounceFactor, le->pos.trDelta );

	VectorCopy( trace->endpos, le->pos.trBase );
	le->pos.trTime = cg.time;


	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if ( trace->allsolid || ( trace->plane.normal[2] > 0 && ( le->pos.trDelta[2] < 40 || 
		le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2] ) ) ) {
		le->pos.trType = TR_STATIONARY;
	} else {

	}
}

/*
================
CG_AddFragment
================
*/
void CG_AddFragment( localEntity_t *le ) {
	vec3_t	newOrigin;
	trace_t	trace;

	if ( le->pos.trType == TR_STATIONARY ) {
		// sink into the ground if near the removal time
		int		t;
		float	oldZ;
		
		t = le->endTime - cg.time;
		if ( t < SINK_TIME ) {
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			VectorCopy( le->refEntity.origin, le->refEntity.lightingOrigin );
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * ( 1.0 - (float)t / SINK_TIME );
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.origin[2] = oldZ;
		} else {
			trap_R_AddRefEntityToScene( &le->refEntity );
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

	// trace a line from previous position to new position
	CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );
	if ( trace.fraction == 1.0 ) {
		qboolean	inWater;
		vec3_t		newDelta;

		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		// check for water
		inWater = CG_PointContents( newOrigin, -1 ) & CONTENTS_WATER;

		if ( inWater && le->pos.trType == TR_GRAVITY ) {
			BG_EvaluateTrajectoryDelta( &le->pos, cg.time, newDelta );
			le->pos.trType = TR_WATER_GRAVITY;
			VectorCopy( newOrigin, le->pos.trBase );
			VectorCopy( newDelta, le->pos.trDelta );
			le->pos.trTime = cg.time;
		}
		if ( !inWater && le->pos.trType == TR_WATER_GRAVITY ) {
			BG_EvaluateTrajectoryDelta( &le->pos, cg.time, newDelta );
			le->pos.trType = TR_GRAVITY;
			VectorCopy( newOrigin, le->pos.trBase );
			VectorCopy( newDelta, le->pos.trDelta );
			le->pos.trTime = cg.time;
		}

		if ( le->leFlags & LEF_TUMBLE ) {
			vec3_t angles;

			BG_EvaluateTrajectory( &le->angles, cg.time, angles );
			AnglesToAxis( angles, le->refEntity.axis );
		}

		trap_R_AddRefEntityToScene( &le->refEntity );

		// add a fire trail
		// CG_FireTrail( le );

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	// stop rotation
	le->angles.trType = TR_STATIONARY;

	// leave a mark
	CG_FragmentBounceMark( le, &trace );

	// reflect the velocity on the trace plane
	CG_ReflectVelocity( le, &trace );

	trap_R_AddRefEntityToScene( &le->refEntity );
}


/*
================
CG_AddFireFragment
================
*/
void CG_AddFireFragment( localEntity_t *le ) {
	vec3_t	newOrigin;
	trace_t	trace;

	if ( le->pos.trType == TR_STATIONARY ) {
		// sink into the ground if near the removal time
		int		t;
		float	oldZ;
		
		t = le->endTime - cg.time;
		if ( t < SINK_TIME ) {
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			VectorCopy( le->refEntity.origin, le->refEntity.lightingOrigin );
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * ( 1.0 - (float)t / SINK_TIME );
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.origin[2] = oldZ;
		} else {
			trap_R_AddRefEntityToScene( &le->refEntity );
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

	// trace a line from previous position to new position
	CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );
	if ( trace.fraction == 1.0 ) {
		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE ) {
			vec3_t angles;

			BG_EvaluateTrajectory( &le->angles, cg.time, angles );
			AnglesToAxis( angles, le->refEntity.axis );
		}

		trap_R_AddRefEntityToScene( &le->refEntity );

		// add a fire trail
		CG_FireTrail( le );

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	// stop rotation
	le->angles.trType = TR_STATIONARY;

	// leave a mark
	CG_FragmentBounceMark( le, &trace );

	// reflect the velocity on the trace plane
	CG_ReflectVelocity( le, &trace );

	trap_R_AddRefEntityToScene( &le->refEntity );
}


/*
================
CG_AddFireEffect
================
*/
static void CG_AddFireEffect( localEntity_t *le ) {
	refEntity_t			*re;
	trace_t				trace;

	re = &le->refEntity;

	if ( le->pos.trType == TR_STATIONARY ) {
		if ( le->leFlags & LEF_LESSOVERDRAW ) {
			// avoid too much overdraw
			if ( CG_NearbyDrawnLeCount( le, re->origin, le->leType, 52 + !!cg_lowDetailEffects.integer * 24 ) > 0 ) return;
		}
		// add to refresh list
		trap_R_AddRefEntityToScene( re );
		return;
	}

	// calculate position
	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// check for water
	if ( trap_CM_PointContents( re->origin, 0 ) & CONTENTS_WATER ) {
		// do a trace to get water surface normals
		CG_Trace( &trace, re->oldorigin, NULL, NULL, re->origin, le->owner, MASK_WATER );
		CG_FreeLocalEntity( le );

		CG_MakeExplosion( trace.endpos, trace.plane.normal, 
			cgs.media.ringFlashModel, cgs.media.vaporShader,
			500, qfalse, qtrue );
		return;
	}

	// do a trace sometimes
	if ( le->ti.trailTime++ > 5 ) {
		le->ti.trailTime = 0;

		CG_Trace( &trace, re->oldorigin, NULL, NULL, re->origin, le->owner, MASK_SHOT );
		VectorCopy( trace.endpos, re->origin );
		VectorCopy( trace.endpos, re->oldorigin );

		// hit something
		if ( trace.fraction < 1.0 ) {
			// free le if another one is nearby, otherwise make it stationary
			if ( CG_CheckDistance( re->origin, le->leType, TR_STATIONARY, re->customShader, 200, 32 ) ) { 
				CG_FreeLocalEntity( le );
				return;
			} else {
				le->pos.trType = TR_STATIONARY;
			}
		}
	}

	if ( le->leFlags & LEF_LESSOVERDRAW ) {
		// avoid too much overdraw
		if ( CG_NearbyDrawnLeCount( le, re->origin, le->leType, 52 + !!cg_lowDetailEffects.integer * 24 ) > 0 ) return;
	}

	// add to refresh list
	trap_R_AddRefEntityToScene( re );
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
CG_AddFadeRGB
====================
*/
void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	trap_R_AddRefEntityToScene( re );
}


/*
==================
CG_AddMoveScale
==================
*/
static void CG_AddMoveScale( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	if ( !( le->leFlags & (LEF_DONT_SCALE | LEF_ALT_SCALE) ) ) {
		re->radius = le->radius * ( cg.time - le->startTime ) * le->lifeRate + 8;
	}
	if ( le->leFlags & LEF_ALT_SCALE ) {
		if ( re->hModel ) {
			if ( (le->leFlags & LEF_AXIS_ALIGNED) && le->radius ) {
				// it has a variing scale
				AxisClear( re->axis );
				AxisScale( re->axis, re->radius + le->radius * (cg.time - le->startTime) / (le->endTime - le->startTime), re->axis );
			}
			if ( !(le->leFlags & LEF_AXIS_ALIGNED) ) {
				// model looks in flight direction
				vec3_t	delta, angles;
				
				BG_EvaluateTrajectoryDelta( &le->pos, cg.time, delta );
				vectoangles( delta, angles );
				AnglesToAxis( angles, re->axis );
				if ( re->radius || le->radius ) AxisScale( re->axis, re->radius + le->radius * (cg.time - le->startTime) / (le->endTime - le->startTime), re->axis );
			}
		} else {
			// sprites scale like this
			re->radius += le->radius * cg.frametime / (le->endTime - le->startTime);
		}
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// check for collisions
	if ( le->leFlags & LEF_COLLISIONS ) {
		// do a trace sometimes
		if ( le->ti.trailTime++ > 5 ) {
			trace_t		trace;

			le->ti.trailTime = 0;

			CG_Trace( &trace, re->oldorigin, NULL, NULL, re->origin, le->owner, MASK_SHOT );
			VectorCopy( re->origin, re->oldorigin );

			// hit something
			if ( trace.fraction < 1.0 ) {
				entityState_t	*s1;
				int		life;

				// do impact effect
				s1 = &cg_entities[le->owner].currentState;
				life = s1->groundEntityNum;

				if ( !( trace.surfaceFlags & SURF_NOIMPACT ) ) {
					if ( s1->generic1 & PF_IMPACT_MODEL ) {
						CG_MakeExplosion( trace.endpos, trace.plane.normal, 
								   cgs.gameModels[s1->modelindex2],	cgs.gameShaders[s1->otherEntityNum2],
								   life, qfalse, qfalse );
					} else if ( s1->modelindex2 ) {
						CG_MakeExplosion( trace.endpos, trace.plane.normal, 
								   cgs.media.dishFlashModel, cgs.gameShaders[s1->modelindex2],
								   life, qtrue, qtrue );
					}
				}

				CG_FreeLocalEntity( le );
				return;
			}
		}
	}

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}


/*
==================
CG_AddMoveScaleFade
==================
*/
static void CG_AddMoveScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	if ( le->fadeInTime > le->startTime && cg.time < le->fadeInTime ) {
		// fade / grow time
		c = 1.0 - (float) ( le->fadeInTime - cg.time ) / ( le->fadeInTime - le->startTime );
	}
	else {
		// fade / grow time
		c = ( le->endTime - cg.time ) * le->lifeRate;
	}

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	if ( !( le->leFlags & LEF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}


/*
===================
CG_AddScaleFade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void CG_AddScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];
	re->radius = le->radius * ( 1.0 - 0.5*c ) + 8;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}


/*
=================
CG_AddFallScaleFade

This is just an optimized CG_AddMoveScaleFade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
=================
*/
static void CG_AddFallScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	re->origin[2] = le->pos.trBase[2] - ( 1.0 - c ) * le->pos.trDelta[2];

	re->radius = le->radius * ( 1.0 - c ) + 16;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}



/*
================
CG_AddExplosion
================
*/
static void CG_AddExplosion( localEntity_t *ex ) {
	refEntity_t	*ent;
	float	life;

	ent = &ex->refEntity;

	life = (float)( cg.time - ex->startTime ) / ( ex->endTime - ex->startTime );

	// get colors
	ent->shaderRGBA[0] = 255 * ex->color[0] * ( 1.0 - life );
	ent->shaderRGBA[1] = 255 * ex->color[1] * ( 1.0 - life );
	ent->shaderRGBA[2] = 255 * ex->color[2] * ( 1.0 - life );
	ent->shaderRGBA[3] = 255 * ex->color[3] * ( 1.0 - life );

	// add the entity
	trap_R_AddRefEntityToScene(ent);

	// add the dlight
	if ( ex->light ) {
		float		light;

		light = life;
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = ex->light * light;
		trap_R_AddLightToScene(ent->origin, light, ex->lightColor[0], ex->lightColor[1], ex->lightColor[2] );
	}
}

/*
================
CG_AddSpriteExplosion
================
*/
static void CG_AddSpriteExplosion( localEntity_t *le ) {
	refEntity_t	re;
	float c;

	re = le->refEntity;

	c = ( le->endTime - cg.time ) / ( float ) ( le->endTime - le->startTime );
	if ( c > 1 ) {
		c = 1.0;	// can happen during connection problems
	}

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff * c * 0.33;

	re.reType = RT_SPRITE;
	re.radius = 96 * ( 1.0 - c ) + 30;

	trap_R_AddRefEntityToScene( &re );

	// add the dlight
	if ( le->light ) {
		float		light;

		light = (float)( cg.time - le->startTime ) / ( le->endTime - le->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = le->light * light;
		trap_R_AddLightToScene(re.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2] );
	}
}


/*
===================
CG_AddScorePlum
===================
*/
#define NUMBER_SIZE		8

void CG_AddScorePlum( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		origin, delta, dir, vec, up = {0, 0, 1};
	float		c, len;
	int			i, score, digits[10], numdigits, negative;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;

	score = le->radius;
	if (score < 0) {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0x11;
		re->shaderRGBA[2] = 0x11;
	}
	else {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		if (score >= 50) {
			re->shaderRGBA[1] = 0;
		} else if (score >= 20) {
			re->shaderRGBA[0] = re->shaderRGBA[1] = 0;
		} else if (score >= 10) {
			re->shaderRGBA[2] = 0;
		} else if (score >= 2) {
			re->shaderRGBA[0] = re->shaderRGBA[2] = 0;
		}

	}
	if (c < 0.25)
		re->shaderRGBA[3] = 0xff * 4 * c;
	else
		re->shaderRGBA[3] = 0xff;

	re->radius = NUMBER_SIZE / 2;

	VectorCopy(le->pos.trBase, origin);
	origin[2] += 110 - c * 100;

	VectorSubtract(cg.refdef.vieworg, origin, dir);
	CrossProduct(dir, up, vec);
	VectorNormalize(vec);

	VectorMA(origin, -10 + 20 * sin(c * 2 * M_PI), vec, origin);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < 20 ) {
		CG_FreeLocalEntity( le );
		return;
	}

	negative = qfalse;
	if (score < 0) {
		negative = qtrue;
		score = -score;
	}

	for (numdigits = 0; !(numdigits && !score); numdigits++) {
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	if (negative) {
		digits[numdigits] = 10;
		numdigits++;
	}

	for (i = 0; i < numdigits; i++) {
		VectorMA(origin, (float) (((float) numdigits / 2) - i) * NUMBER_SIZE, vec, re->origin);
		re->customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		trap_R_AddRefEntityToScene( re );
	}
}


/*
================
CG_AddMissile
================
*/
static void CG_AddMissile( localEntity_t *le ) {
	refEntity_t			*re;
	const weaponInfo_t	*weapon;
	trace_t				trace;
	centity_t			*other;
	qboolean			inWater;

	// just existing for server entity deletion
	if ( le->leFlags & LEF_FINISHED ) {
		return;
	}

	// get weapon info
	if ( le->ti.weapon > WP_NUM_WEAPONS ) {
		le->ti.weapon = 0;
	}
	weapon = &cg_weapons[le->ti.weapon];

	re = &le->refEntity;

	// calculate position
	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// special case for flames
	if ( re->reType == RT_SPRITE ) {
		int deltaTime;

		// check for water
		if ( trap_CM_PointContents( re->origin, 0 ) & CONTENTS_WATER ) {
			// do a trace to get water surface normals
			CG_Trace( &trace, re->oldorigin, NULL, NULL, re->origin, le->owner, MASK_WATER );
			CG_FreeLocalEntity( le );

			CG_MakeExplosion( trace.endpos, trace.plane.normal, 
				cgs.media.ringFlashModel, cgs.media.vaporShader,
				500, qfalse, qtrue );
			return;
		}

		// change radius over time
		deltaTime = cg.time - le->startTime;
		re->radius = deltaTime * deltaTime * ( random()*0.4f + 0.8f ) / 2000.0f + 9;

		// do a trace sometimes
		if ( le->ti.trailTime++ > 5 ) {
			le->ti.trailTime = 0;

			CG_Trace( &trace, re->oldorigin, NULL, NULL, re->origin, le->owner, MASK_SHOT );
			VectorCopy( re->origin, re->oldorigin );

			// hit something
			if ( trace.fraction < 1.0 ) {
				CG_MissileHitWall( le->ti.weapon, 0, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
				CG_FreeLocalEntity( le );
				return;
			}
		}

		// add to refresh list
		trap_R_AddRefEntityToScene( re );
		return;
	}

	// add trails
	if ( weapon->missileTrailFunc ) weapon->missileTrailFunc( &le->ti, cg.time );

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene( re->origin, weapon->missileDlight, 
			weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2] );
	}

	// flicker between two skins
	re->skinNum = cg.clientFrame & 1;

	// convert direction of travel into axis
	if ( VectorNormalize2( le->pos.trDelta, re->axis[0] ) == 0 ) {
		re->axis[0][2] = 1;
	}

	// spin as it moves
	if ( le->pos.trType != TR_STATIONARY ) {
		if ( le->pos.trType == TR_GRAVITY ) {
			RotateAroundDirection( re->axis, cg.time / 4 );
		} else if ( le->pos.trType == TR_WATER_GRAVITY ) {
			RotateAroundDirection( re->axis, cg.time / 8 );
		} else {
			RotateAroundDirection( re->axis, cg.time );
		}
	} else {
		RotateAroundDirection( re->axis, 0 );
	}

	// trace a line from previous position to new position
	CG_Trace( &trace, re->oldorigin, NULL, NULL, re->origin, le->owner, MASK_SHOT );
	VectorCopy( re->origin, re->oldorigin );

	// draw BIG grenades
	if ( weLi[le->ti.weapon].category == CT_EXPLOSIVE ) {
		AxisScale( re->axis, GRENADE_SCALE, re->axis );
	}

	if ( trace.fraction != 1.0 ) {
		// hit the sky or something like that
		if ( trace.surfaceFlags & SURF_NOIMPACT ) {
			le->leFlags |= LEF_FINISHED;
			le->endTime = cg.time + 500;
			return;
		}

		// impact
		other = &cg_entities[trace.entityNum];

		if ( le->bounceFactor > 0 && ( le->bounceFactor == BOUNCE_FACTOR_HALF || other->currentState.eType != ET_PLAYER ) ) {
			// reflect the velocity on the trace plane
			CG_ReflectVelocity( le, &trace );
			if ( cg.predictedImpacts < MAX_PREDICTED_IMPACTS ) {
				cg.predictedImpacts++;
				cg.predictedImpactsDecTime = cg.time;
			}

			// do bounce sound
			if ( rand() & 1 ) {
				trap_S_StartSound( le->pos.trBase, 0, CHAN_AUTO, cgs.media.hgrenb1aSound );
			} else {
				trap_S_StartSound( le->pos.trBase, 0, CHAN_AUTO, cgs.media.hgrenb2aSound );
			}
		} else {
			// explode missile
			if ( cg.predictedImpacts < MAX_PREDICTED_IMPACTS ) {
				cg.predictedImpacts++;
				cg.predictedImpactsDecTime = cg.time;
			}

			if ( other->currentState.eType == ET_PLAYER ) {
				CG_MissileHitPlayer( le->ti.weapon, 0, trace.endpos, trace.plane.normal, trace.entityNum );
			} else if ( !(trace.surfaceFlags & SURF_NOIMPACT) ) {
				CG_MissileHitWall( le->ti.weapon, 0, trace.endpos, trace.plane.normal, 
					(trace.surfaceFlags & SURF_METALSTEPS) ? IMPACTSOUND_METAL : IMPACTSOUND_DEFAULT );
			}
			// store the entity for deleting the server entity
			le->leFlags |= LEF_FINISHED;
			le->endTime = cg.time + 500;
			return;
		}
	}

	// check for medium change
	if ( trap_CM_PointContents( re->origin, 0 ) & CONTENTS_WATER ) inWater = qtrue;
	else inWater = qfalse;

	if (  ( !inWater && le->pos.trType == TR_WATER_GRAVITY )
		|| ( inWater && le->pos.trType == TR_GRAVITY ) ) {
		// setup new tr
		vec3_t	newDelta;

		BG_EvaluateTrajectoryDelta( &le->pos, cg.time, newDelta );
		VectorCopy( re->origin, le->pos.trBase );
		VectorCopy( newDelta, le->pos.trDelta );
		le->pos.trTime = cg.time;
		if ( inWater ) le->pos.trType = TR_WATER_GRAVITY;
		else le->pos.trType = TR_GRAVITY;
	}	

	// add to refresh list
	trap_R_AddRefEntityToScene( re );
}


/*
================
CG_ResetMissileEntity
================
*/
void CG_ResetMissileEntity( centity_t *cent ) {	
	// link trajectory
	cent->ti.trPos = &cent->currentState.pos;
	cent->ti.weapon = cent->currentState.weapon;

	// check for predicted missiles if necessary
	if ( cg.predictWeapons && cent->currentState.clientNum == cg.snap->ps.clientNum ) {
		localEntity_t	*le;

		le = cg_activeLocalEntities.prev;
		for ( ; le != &cg_activeLocalEntities ; le = le->prev ) {
			// the first (and oldest) missile le is the one represented by the new one
			if ( le->leType == LE_MISSILE && weLi[le->ti.weapon].prediction != PR_FLAME ) {
				// set the correct trailTime
				cent->ti.trailTime = le->ti.trailTime;
				VectorCopy( le->ti.lastPos, cent->ti.lastPos );

				// free the local entity
				CG_FreeLocalEntity( le );
				return;
			}
		}	
	}
	// there isn't a predicted missile, use normal trailTime reset
	cent->ti.trailTime = cent->currentState.pos.trTime + TRAIL_DELAY;
	BG_EvaluateTrajectory( cent->ti.trPos, cent->ti.trailTime, cent->ti.lastPos );
}


//==============================================================================

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities( void ) {
	localEntity_t	*le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if ( cg.time >= le->endTime ) {
			CG_FreeLocalEntity( le );
			continue;
		}
		switch ( le->leType ) {
		default:
			CG_Error( "Bad leType: %i", le->leType );
			break;

		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion( le );
			break;

		case LE_EXPLOSION:
			CG_AddExplosion( le );
			break;

		case LE_FRAGMENT:			// gibs and brass
			CG_AddFragment( le );
			break;

		case LE_FRAGMENT_FIRE:			// explosion particles
			CG_AddFireFragment( le );
			break;

		case LE_FIRE_EFFECT:
			CG_AddFireEffect( le );
			break;

		case LE_MOVE_SCALE:				// various particles
			CG_AddMoveScale( le );
			break;

		case LE_MOVE_SCALE_FADE:		// water bubbles
			CG_AddMoveScaleFade( le );
			break;

		case LE_FADE_RGB:				// teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_FALL_SCALE_FADE: // gib blood trails
			CG_AddFallScaleFade( le );
			break;

		case LE_SCALE_FADE:		// rocket trails
			CG_AddScaleFade( le );
			break;

		case LE_SCOREPLUM:
			CG_AddScorePlum( le );
			break;

		case LE_MISSILE:
			CG_AddMissile( le );
			break;
		}
	}
}




