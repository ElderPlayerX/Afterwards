// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"


/*
==================
CG_Bubble
==================
*/
void CG_Bubble( vec3_t origin ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = LEF_DONT_SCALE;
	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = cg.time;
	le->endTime = cg.time + 1000 + random() * 250;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	re = &le->refEntity;
	re->shaderTime = cg.time / 1000.0f;

	re->reType = RT_SPRITE;
	re->rotation = 0;
	re->radius = 3;
	re->customShader = cgs.media.waterBubbleShader;
	re->shaderRGBA[0] = 0xff;
	re->shaderRGBA[1] = 0xff;
	re->shaderRGBA[2] = 0xff;
	re->shaderRGBA[3] = 0xff;

	le->color[3] = 1.0;

	VectorCopy( origin, le->pos.trBase );
	le->pos.trType = TR_LINEAR;
	le->pos.trTime = cg.time;
	le->pos.trDelta[0] = crandom()*5;
	le->pos.trDelta[1] = crandom()*5;
	le->pos.trDelta[2] = crandom()*5 + 6;
}


/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing ) {
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	VectorMA( move, i, vec, move );

	VectorScale (vec, spacing, vec);

	for ( ; i < len; i += spacing ) {
		CG_Bubble( move );
		VectorAdd (move, vec, move);
	}
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t *CG_SmokePuff( const vec3_t p, const vec3_t vel, 
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader ) {
	static int	seed = 0x92;
	localEntity_t	*le;
	refEntity_t		*re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random( &seed ) * 360;
	re->radius = radius;
	re->shaderTime = startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + duration;
	if ( fadeInTime > startTime ) {
		le->lifeRate = 1.0 / ( le->endTime - le->fadeInTime );
	}
	else {
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	}
	le->color[0] = r;
	le->color[1] = g; 
	le->color[2] = b;
	le->color[3] = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy( vel, le->pos.trDelta );
	VectorCopy( p, le->pos.trBase );

	VectorCopy( p, re->origin );
	re->customShader = hShader;

	// rage pro can't alpha fade, so use a different shader
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		re->customShader = cgs.media.smokePuffRageProShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;
	} else {
		re->shaderRGBA[0] = le->color[0] * 0xff;
		re->shaderRGBA[1] = le->color[1] * 0xff;
		re->shaderRGBA[2] = le->color[2] * 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

/*
=====================
CG_GenerateParticles

Generates multiple generic particles
=====================
*/
void CG_GenerateParticles( qhandle_t model, qhandle_t shader, vec3_t pos, float randomPos, vec3_t speed, float randomDir, 
						  float randomSpeed, int numParticles, int owner, int time, int life, int randomLife, int size, 
						  int randomSize, int addSize, int randomAddSize, int flags, int renderfx ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t	angles;
	vec3_t	axis[3];
	int		i;
	int		num;

	num = numParticles / ( !!cg_lowDetailEffects.integer + 1 ) + 1;
	for ( i = 0; i < num; i++ ) {
		// initialize new le
		le = CG_AllocLocalEntity();
		le->leType = LE_MOVE_SCALE;
		le->leFlags = LEF_ALT_SCALE;
		le->owner = owner;
		le->startTime = time;
		le->endTime = time + life + crandom()*randomLife;
		le->radius = addSize + crandom()*randomAddSize;
		re = &le->refEntity;
		re->shaderTime = time / 1000.0f;
		re->radius = size + crandom()*randomSize;
		re->renderfx = renderfx;

		if ( model ) {
			re->hModel = model;
			re->customShader = shader;
			if ( flags & PF_AXIS_ALIGNED ) {
				le->leFlags |= LEF_AXIS_ALIGNED;
				AxisClear( re->axis );
				if ( re->radius ) AxisScale( re->axis, re->radius, re->axis );
			}
		} else {
			re->reType = RT_SPRITE;
			re->customShader = shader;
		}

		// initialize movement
		if ( flags & PF_GRAVITY ) le->pos.trType = TR_GRAVITY;
		else le->pos.trType = TR_LINEAR;
		if ( flags & PF_COLLISIONS ) le->leFlags |= LEF_COLLISIONS;
		le->pos.trTime = time;
		VectorSet( le->pos.trBase, pos[0] + crandom()*randomPos, pos[1] + crandom()*randomPos, pos[2] + crandom()*randomPos );

		vectoangles( speed, angles );
		angles[0] += crandom()*randomDir;
		angles[1] += crandom()*randomDir;
		angles[2] += crandom()*randomDir;
		AnglesToAxis( angles, axis );
		VectorNormalize( axis[0] );
		VectorScale( axis[0], VectorLength(speed) + crandom()*randomSpeed, le->pos.trDelta );
	}
}


/*
=====================
CG_MoveParticle

Adds a moving particle localEntity.
=====================
*/
localEntity_t *CG_MoveParticle( const vec3_t p, const vec3_t vel, int trType,
				   float radius, float duration, int startTime,
				   int leFlags, qhandle_t hShader ) {

	static int	seed = 0x92;
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random( &seed ) * 360;
	re->radius = radius;
	re->shaderTime = startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE;
	le->startTime = startTime;
	le->endTime = startTime + duration;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->pos.trType = trType;
	le->pos.trTime = startTime;
	VectorCopy( vel, le->pos.trDelta );
	VectorCopy( p, le->pos.trBase );

	VectorCopy( p, re->origin );
	re->customShader = hShader;

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}


/*
==================
CG_SpawnEffect

Player teleporting in or out
==================
*/
void CG_SpawnEffect( vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 500;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

#ifndef MISSIONPACK
	re->customShader = cgs.media.teleportEffectShader;
#endif
	re->hModel = cgs.media.teleportEffectModel;
	AxisClear( re->axis );

	VectorCopy( org, re->origin );
#ifdef MISSIONPACK
	re->origin[2] += 16;
#else
	re->origin[2] -= 24;
#endif
}


/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum( int client, vec3_t org, int score ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;
	static vec3_t lastPos;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_scorePlum.integer == 0) {
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 4000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	
	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;
	
	VectorCopy( org, le->pos.trBase );
	if (org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20) {
		le->pos.trBase[2] -= 20;
	}

	//CG_Printf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	VectorCopy(org, lastPos);


	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	VectorClear(angles);
	AnglesToAxis( angles, re->axis );
}


/*
====================
CG_MakeExplosion
====================
*/
localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir, 
								qhandle_t hModel, qhandle_t shader,
								int msec, qboolean isSprite, qboolean checkDistance ) {
	localEntity_t	*ex;
	float			ang;
	int				offset;
	vec3_t			tmpVec, newOrigin;

	if ( msec <= 0 ) {
		CG_Error( "CG_MakeExplosion: msec = %i", msec );
	}

	// check for too much overdraw
	if ( checkDistance && CG_CheckDistance( origin, LE_EXPLOSION, TR_STATIONARY, shader, (int)(msec * 0.5), 32 ) ) {
		return NULL;
	}

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if ( isSprite ) {
		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		VectorScale( dir, 16, tmpVec );
		VectorAdd( tmpVec, origin, newOrigin );
	} else {
		ex->leType = LE_EXPLOSION;
		VectorCopy( origin, newOrigin );

		// set axis with random rotate
		if ( !dir ) {
			AxisClear( ex->refEntity.axis );
		} else {
			ang = rand() % 360;
			VectorCopy( dir, ex->refEntity.axis[0] );
			RotateAroundDirection( ex->refEntity.axis, ang );
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy( newOrigin, ex->refEntity.origin );
	VectorCopy( newOrigin, ex->refEntity.oldorigin );

	ex->color[0] = ex->color[1] = ex->color[2] = ex->color[3] = 1.0;

	return ex;
}


/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit
=================
*/
void CG_Bleed( vec3_t origin, int entityNum ) {
	vec3_t		speed;

	if ( !cg_blood.integer ) {
		return;
	}

	VectorSet( speed, 20, 0, 0 );
	CG_GenerateParticles( 0, cgs.media.bloodExplosionShader, origin, 10, speed, 180, 20, 4, 
		ENTITYNUM_WORLD, cg.time, 500, 0, 6, 3, 6, 3, 0, entityNum == cg.snap->ps.clientNum ? RF_THIRD_PERSON : 0 );
}


/*
==================
CG_LaunchExplode
==================
*/
localEntity_t *CG_LaunchExplode( vec3_t origin, vec3_t velocity, qhandle_t shader, qboolean instantly, int duration, int displace, int size ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FIRE_EFFECT;
	le->startTime = cg.time;	
	if ( instantly ) {
		le->endTime = le->startTime + duration;
		re->shaderTime = le->startTime / 1000.0f;
	}
	else {
		le->endTime = le->startTime + (2*duration + random() * duration) / 3;
		re->shaderTime = (le->endTime - duration) / 1000.0f;
	}

	re->reType = RT_SPRITE;
	re->radius = size;
	re->rotation = random() * 360;
	re->customShader = shader;
	VectorCopy( origin, re->origin );
	VectorCopy( origin, re->oldorigin );

	if ( velocity[0] || velocity[1] || velocity[2] ) {
		le->pos.trType = TR_LINEAR;
		VectorCopy( origin, le->pos.trBase );
		VectorCopy( velocity, le->pos.trDelta );
		le->pos.trTime = cg.time - displace;
	} else {
		le->pos.trType = TR_STATIONARY;
	}

	le->leBounceSoundType = LEBS_NONE;
	le->leMarkType = LEMT_NONE;

	return le;
}

/*
===================
CG_BigExplode

Generated a bunch of particles
===================
*/
#define	EXP_VELOCITY	30
#define	EXP_JUMP		20
#define EXP_DISPLACE	500

void CG_BigExplode( vec3_t origin ) {
	vec3_t	velocity;
	int		i;

	for ( i = 0; i < 2; i++ ) {
		velocity[0] = crandom()*EXP_VELOCITY;
		velocity[1] = crandom()*EXP_VELOCITY;
		velocity[2] = (EXP_JUMP + crandom()*EXP_VELOCITY) * 1.5;
		CG_LaunchExplode( origin, velocity, cgs.media.explosionSmokeShader, qfalse, 4000, 2500, 192 );
	}

	if ( cg_lowDetailEffects.integer ) {
		return;
	}

	for ( i = 0; i < 4; i++ ) {
		velocity[0] = crandom()*EXP_VELOCITY;
		velocity[1] = crandom()*EXP_VELOCITY;
		velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
		CG_LaunchExplode( origin, velocity, cgs.media.rocketExplosionShader, qtrue, 900, 2500, 128 );
	}
}


/*
===================
CG_FireExplode

Generated a bunch of particles
===================
*/
#define FIRE_DENSITY_HIGH	10
#define FIRE_DENSITY_LOW	6

void CG_FireExplode( vec3_t origin ) {
	localEntity_t	*le;
	vec3_t	velocity;
	int		particles;
	int		i,j;
	int		planeParticles;
	float	tilt, yaw;
	float	tiltSin, tiltCos;
	float	velRandom;

	if ( cg_lowDetailEffects.integer ) particles = FIRE_DENSITY_LOW;
	else particles = FIRE_DENSITY_HIGH;

	origin[2] += 10;

	for ( j = 0; j < particles; j++ ) {
		tilt = ((float)j/particles-0.5)*M_PI;
		tiltSin = sin(tilt);
		tiltCos = cos(tilt);
		planeParticles = particles * tiltCos + 1;

		for ( i = 0; i < planeParticles; i++ ) {
			yaw = ((float)i/planeParticles-0.5)*4*M_PI;

			velRandom = random()*0.5+0.5;
			velocity[0] = cos(yaw) * tiltCos * weLi[WP_FIRE_GRENADE].spread_crouch * velRandom;
			velocity[1] = sin(yaw) * tiltCos * weLi[WP_FIRE_GRENADE].spread_crouch * velRandom;
			velocity[2] = tiltSin * weLi[WP_FIRE_GRENADE].spread_crouch * velRandom;
			le = CG_LaunchExplode( origin, velocity, cgs.media.simpleFireShader, qfalse, 1900, 250, 128 );

			le->pos.trType = TR_ACCELERATE;
			le->pos.trDuration = weLi[WP_FIRE_GRENADE].spread_recoil*velRandom;
			le->leFlags |= LEF_LESSOVERDRAW;
		}
	}
}


/*
==================
CG_LaunchDebris
==================
*/
void CG_LaunchDebris( vec3_t origin, vec3_t velocity, qhandle_t hModel, qboolean fire ) {
	localEntity_t	*le;
 	refEntity_t		*re;
	vec3_t	rot;
 
 	le = CG_AllocLocalEntity();
 	re = &le->refEntity;
 
 	if ( fire ) le->leType = LE_FRAGMENT_FIRE;
	else le->leType = LE_FRAGMENT;
 	le->startTime = cg.time;
 	le->endTime = le->startTime + 10000 + random() * 3000;
 
 	VectorCopy( origin, re->origin );
 	AxisCopy( axisDefault, re->axis );
 	re->hModel = hModel;

	le->pos.trType = TR_LOW_GRAVITY;
 	VectorCopy( origin, le->pos.trBase );
 	VectorCopy( velocity, le->pos.trDelta );
 	le->pos.trTime = cg.time;
 
	rot[0] = random()*600 - 300;
	rot[1] = random()*600 - 300;
	rot[2] = random()*600 - 300;
	le->angles.trType = TR_LINEAR;
 	VectorCopy( rot, le->angles.trBase );
 	VectorCopy( rot, le->angles.trDelta );
 	le->angles.trTime = cg.time;
 
 	le->bounceFactor = 0.2f;
 
 	le->leFlags = LEF_TUMBLE;
 	le->leBounceSoundType = LEBS_BRASS;
 	le->leMarkType = LEMT_NONE;
}
 
/*
===================
CG_BreakGlass

Generated a bunch of glass shards launching out from the glass location
===================
*/
#define	GLASS_VELOCITY	100

void CG_BreakWithGibs( centity_t *cent, int effect ) {
 	vec3_t	origin, delta, velocity;
	int     count;

	// How many shards to generate
	// depends on the volume of the object
	VectorSubtract( cent->currentState.origin2, cent->currentState.origin, delta );
	count =  abs ( (delta[0] < 30 ? 30 : delta[0]) * (delta[1] < 30 ? 30 : delta[1]) *
		           (delta[2] < 30 ? 30 : delta[2]) ) / 20000 + 1;
  
 	// Countdown "count" so this will subtract 1 from the "count"
 	// X many times. X being the "count" value
 	while ( count-- ) {

		origin[0] = cent->currentState.origin[0] + delta[0]*random();
		origin[1] = cent->currentState.origin[1] + delta[1]*random();
		origin[2] = cent->currentState.origin[2] + delta[2]*random();

		velocity[0] = crandom()*GLASS_VELOCITY;
	 	velocity[1] = crandom()*GLASS_VELOCITY;
	 	velocity[2] = crandom()*GLASS_VELOCITY;

		switch (effect) {
		case BE_GLASS:	
			CG_LaunchDebris( origin, velocity, cgs.media.glass[(int)(random()*3)], qfalse );
			break;
		case BE_WOOD:	
			CG_LaunchDebris( origin, velocity, cgs.media.wood[(int)(random()*3)], qfalse );
			break;
		case BE_METAL:
			CG_LaunchDebris( origin, velocity, cgs.media.debris[(int)(random()*3)], qfalse );
			break;
		}
 	}
}


/*
===================
CG_BreakExplosive

Generate a big explosion and a bunch of debris launching out from the explosion location
===================
*/
#define	DEBRIS_VELOCITY	250
#define	DEBRIS_JUMP		200
#define MIN_SHARDS		5
#define MAX_SHARDS		30

void CG_BreakExplosive( centity_t *cent ) {
 	vec3_t	origin, delta, velocity;
	int     count;
//	localEntity_t *le;
//	vec3_t	dir;

	VectorSubtract( cent->currentState.origin2, cent->currentState.origin, delta );

	CG_BigExplode( cent->currentState.origin );

	// How many shards to generate
	// depends on the volume of the object
	count =  abs ( (delta[0] < 30 ? 30 : delta[0]) * (delta[1] < 30 ? 30 : delta[1]) *
		           (delta[2] < 30 ? 30 : delta[2]) ) / 30000 + MIN_SHARDS;
	if ( count > MAX_SHARDS ) {
		count = MAX_SHARDS;
	}
  
 	// Countdown "count" so this will subtract 1 from the "count"
 	// X many times. X being the "count" value

	while ( count-- ) {

		origin[0] = cent->currentState.origin[0] + delta[0]*random();
		origin[1] = cent->currentState.origin[1] + delta[1]*random();
		origin[2] = cent->currentState.origin[2] + delta[2]*random();

		velocity[0] = crandom()*DEBRIS_VELOCITY;
	 	velocity[1] = crandom()*DEBRIS_VELOCITY;
	 	velocity[2] = crandom()*DEBRIS_VELOCITY+DEBRIS_JUMP;

		CG_LaunchDebris( origin, velocity, cgs.media.debris[(int)(random()*4)], qtrue );
 	}
}


/*
===================
CG_FXFire
===================
*/
void CG_FXFire( centity_t *cent ) {
	int i;
	int	num;
	vec3_t			origin;

	cent->miscTime += cg.frametime;
	if ( cg_lowDetailEffects.integer ) {
		num = cent->miscTime / 800;
		cent->miscTime -= 800 * num;
	} else {
		num = cent->miscTime / 400;
		cent->miscTime -= 400 * num;
	}

	for (i=0; i < num; i++) {
		VectorSet( origin, 
			cent->lerpOrigin[0] + random()*30 - 15, 
			cent->lerpOrigin[1] + random()*30 - 15, 
			cent->lerpOrigin[2] + random()*70 - 25 
			);

		CG_MakeExplosion( origin, vec3_origin, cgs.media.dishFlashModel, cgs.media.rocketExplosionShader, 900, qtrue, qfalse );
	}	
}


/*
===================
CG_FXLightning
===================
*/
void CG_FXLightning( centity_t *cent ) {
  	vec3_t  	  	forward, right;
  	vec2_t  	  	line;
  	vec3_t  	  	start, finish;
	polyVert_t		pverts[4];
	int				i,j;
	int				size;

	// check if the lightning should appear
	if ( cg.time < cent->currentState.time || cg.time > cent->currentState.time + cent->currentState.time2 ) {
		return;
	}

	// draw the lightning
  	VectorCopy( cent->currentState.origin, start );
  	VectorCopy( cent->currentState.origin2, finish );

	VectorSubtract( finish, start, forward );
	VectorNormalize( forward );

  	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
  	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

  	VectorScale( cg.refdef.viewaxis[1], line[1], right );
  	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
  	VectorNormalize( right );

	size = cent->currentState.otherEntityNum2;

	for ( i = 0; i < 4; i++ ) 
		for ( j = 0; j < 4; j++ )
			pverts[i].modulate[j] = 255;

  	VectorMA( start, size, right, pverts[3].xyz );
	pverts[0].st[0] = 1;
  	pverts[0].st[1] = 0;

  	VectorMA( finish, size, right, pverts[0].xyz );
	pverts[1].st[0] = 0;
  	pverts[1].st[1] = 0;

  	VectorMA( finish, -size, right, pverts[1].xyz );
	pverts[2].st[0] = 0;
  	pverts[2].st[1] = 1;

  	VectorMA( start, -size, right, pverts[2].xyz );
	pverts[3].st[0] = 1;
  	pverts[3].st[1] = 1;

  	trap_R_AddPolyToScene( cgs.media.lightningShader, 4, pverts );
}


