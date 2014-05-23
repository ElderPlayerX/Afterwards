//
// cg_shoot.c -- handling almost everything related to shooting
//
// Those functions are called on several events.
// The most important one is the EV_FIRE_WEAPON event, caused by the "bg_pmove" module. 
// After that event all the client side prediction of the weapons is done.
//
#include "cg_local.h"


/*
===================================================================================================

GENERIC FUNCTIONS

===================================================================================================
*/


/*
======================
CG_IncPredictedImpacts
======================
*/
static void	CG_IncPredictedImpacts( void ) {
	if ( cg.predictedImpacts < MAX_PREDICTED_IMPACTS ) {
		cg.predictedImpacts++;
		cg.predictedImpactsDecTime = cg.time;
	}
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}


/*
===================================================================================================

MELEE WEAPONS RELATED FUNCTIONS

===================================================================================================
*/


/*
================
CG_MeleeFireCheck
================
*/
int	lastHitTime = 0;
qboolean CG_MeleeFireCheck( playerState_t *ps ) {
	trace_t		tr;
	vec3_t		forward, dummy, end, muzzle;

	// check if we do predict
	if ( !cg.predictWeapons && cg_predictWeapons.integer > 1 ) {
		return qfalse;
	}
	// don't predict too often, if it was a misprediction last time
	if ( cg.time < lastHitTime + weLi[WP_SHOCKER].nextshot ) {
		return qfalse;
	}

	// calculate forward direction
	AngleVectors( ps->viewangles, forward, dummy, dummy );

	// calculate muzzle point
	VectorCopy( ps->origin, muzzle );
	muzzle[2] += ps->viewheight;
	VectorMA( muzzle, 14, forward, muzzle );

	// and trace end
	VectorMA (muzzle, 32, forward, end);

	CG_Trace (&tr, muzzle, NULL, NULL, end, ps->clientNum, MASK_SHOT);
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	// allow hit if it's a client 
	if ( tr.entityNum < MAX_CLIENTS ) {
		// spawn blood and play sound
		lastHitTime = cg.time;
		CG_IncPredictedImpacts();
		CG_Bleed( tr.endpos, tr.entityNum );
		trap_S_StartSound( NULL, ps->clientNum, CHAN_WEAPON, cg_weapons[WP_SHOCKER].flashSound[0] );
		return qtrue;
	}

	// hit nothing
	return qfalse;
}


/*
===================================================================================================

SHOTGUN RELATED FUNCTIONS

===================================================================================================
*/


/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet( vec3_t start, vec3_t end, int skipNum, int mode ) {
	trace_t		tr;
	int sourceContentType, destContentType;

	CG_Trace( &tr, start, NULL, NULL, end, skipNum, MASK_SHOT );

	sourceContentType = trap_CM_PointContents( start, 0 );
	destContentType = trap_CM_PointContents( tr.endpos, 0 );

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if ( sourceContentType == destContentType ) {
		if ( sourceContentType & CONTENTS_WATER ) {
			CG_BubbleTrail( start, tr.endpos, 32 );
		}
	} else if ( sourceContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( start, trace.endpos, 32 );
	} else if ( destContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( tr.endpos, trace.endpos, 32 );
	}

	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( cg_entities[tr.entityNum].currentState.eType == ET_PLAYER ) {
		if ( mode & SSM_BLOOD ) {
			CG_MissileHitPlayer( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, tr.entityNum );
		}
	} else {
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if ( mode & SSM_WALL ) {
			if ( tr.surfaceFlags & SURF_METALSTEPS ) {
				CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL );
			} else {
				CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT );
			}
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int otherEntNum, int mode ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	int			randomSeed;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	randomSeed = cg.predictedPlayerState.randomSeed;
	// generate the "random" spread pattern
	for ( i = 0 ; i < weLi[WP_SHOTGUN].recoil_shot ; i++ ) {
		r = Q_crandom( &randomSeed ) * weLi[WP_SHOTGUN].spread_recoil * 16;
		u = Q_crandom( &randomSeed ) * weLi[WP_SHOTGUN].spread_recoil * 16;
		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		CG_ShotgunPellet( origin, end, otherEntNum, mode );
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( entityState_t *es, int mode ) {
	vec3_t	v;
	int		contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t			up;

		contents = trap_CM_PointContents( es->pos.trBase, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			VectorSet( up, 0, 0, 8 );
			CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
		}
	}
	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->otherEntityNum, mode );
}


/*
================
CG_FireShotgunPredicted
================
*/
void CG_FireShotgunPredicted( void ) {
	playerState_t	*ps;
	vec3_t			muzzle, v;
	vec3_t			forward, right, up;

	ps = &cg.predictedPlayerState;

	if ( weLi[ps->weapons[ps->wpcat]].prediction != PR_PELLETS ) {
		 return;
	}

	// calculate forward direction
	AngleVectors( ps->viewangles, forward, right, up );

	// calculate muzzle point
	VectorCopy( ps->origin, muzzle );
	muzzle[2] += ps->viewheight;
	VectorMA( muzzle, 14, forward, muzzle );
	VectorMA( muzzle, 32, forward, v );

	// do smoke
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t		up;
		int			contents;

		contents = trap_CM_PointContents( muzzle, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			VectorSet( up, 0, 0, 8 );
			CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
		}
	}

	// fire it
	if ( cg_predictWeapons.integer == 1 ) {
		CG_ShotgunPattern( muzzle, forward, ps->clientNum, SSM_WALL );
	} else {
		CG_ShotgunPattern( muzzle, forward, ps->clientNum, SSM_BLOOD | SSM_WALL );
	}
}


/*
===================================================================================================

BULLET RELATED FUNCTIONS

===================================================================================================
*/


/*
================
CG_FireBulletPredicted
================
*/
void CG_FireBulletPredicted( void ) {
	playerState_t	*ps;
	trace_t			tr;
	vec3_t			muzzle, end;
	vec3_t			forward, right, up;
	float			r;
	float			u;
	int				passent;
	int				spread;
	int				randomSeed;
	int				weapon;

	ps = &cg.predictedPlayerState;
	weapon = ps->weapons[ps->wpcat];

	if ( weLi[weapon].prediction != PR_BULLET ) {
		 return;
	}

	// calculate spread
	spread = weLi[weapon].spread_base;
	if ( !VectorCompare(cg.snap->ps.origin, ps->origin) ) spread *= weLi[weapon].spread_move;		
	if ( ps->pm_flags & PMF_DUCKED ) spread *= weLi[weapon].spread_crouch;
	if ( (weapon == WP_PHOENIX || weapon == WP_SNIPER) && !(ps->pm_flags & PMF_ZOOM) ) spread += weLi[weapon].tr_type;

	spread += weLi[weapon].spread_recoil * ps->recoilY / weLi[weapon].recoil_time;

	// calculate axis for spread and forward direction
	AngleVectors( ps->viewangles, forward, right, up );

	// calculate muzzle point
	VectorCopy( ps->origin, muzzle );
	muzzle[2] += ps->viewheight;
	VectorMA( muzzle, 14, forward, muzzle );

	// calculate synchronized spread
	randomSeed = ps->randomSeed;	
	r = Q_crandom( &randomSeed ) * M_PI * 2.0f;
	u = sin(r) * Q_crandom( &randomSeed ) * spread * 16;
	r = cos(r) * Q_crandom( &randomSeed ) * spread * 16;
	VectorMA (muzzle, 8192*16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);

	passent = ps->clientNum;

	CG_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	// do bullet impact
	if ( tr.entityNum < MAX_CLIENTS ) {
		if ( cg_predictWeapons.integer > 1 ) {
			CG_Bullet( tr.endpos, ps->clientNum, tr.plane.normal, qtrue, tr.entityNum );
		}
	} else {
		CG_Bullet( tr.endpos, ps->clientNum, tr.plane.normal, qfalse, tr.entityNum );
	}

}


/*
===================================================================================================

OMEGA RELATED FUNCTIONS

===================================================================================================
*/


/*
================
CG_FireOmegaPredicted
================
*/
void CG_FireOmegaPredicted( centity_t *cent, int weaponTime ) {
	playerState_t	*ps;
	entityState_t	*es;
	int		weapon;
	vec3_t	forward, right, start;
	vec3_t	muzzle, end;
	trace_t	trace;
	int		skipNum;
	int		i;

	ps = &cg.predictedPlayerState;
	weapon = ps->weapons[ps->wpcat];

	if ( weLi[weapon].prediction != PR_OMEGA ) {
		 return;
	}

	// calculate forward direction
	AngleVectors( ps->viewangles, forward, right, start );

	// calculate muzzle point
	VectorCopy( ps->origin, muzzle );
	muzzle[2] += ps->viewheight;
	VectorMA( muzzle, 14, forward, muzzle );

	// prepare trace
	VectorMA (muzzle, 8192, forward, end);

	// do the necessary traces
	skipNum = ps->clientNum;
	VectorCopy( muzzle, start );
	for ( i = 0; i < MAX_OMEGA_HITS; i++ ) {
		CG_Trace( &trace, start, NULL, NULL, end, skipNum, MASK_SHOT );
		
		es = &cg_entities[ trace.entityNum ].currentState;
		if ( es->eType != ET_PLAYER && !( es->eType == ET_BREAKABLE && es->generic1 ) ) {
			break;
		}

		// do impact
		CG_IncPredictedImpacts();

		if ( es->eType == ET_PLAYER ) {
			CG_MissileHitPlayer( weapon, weaponTime, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
		} else {
			CG_MissileHitWall( weapon, weaponTime, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
		}

		// prepare next trace
		skipNum = trace.entityNum;
		VectorCopy( trace.endpos, start );
	}

	// draw trail and impact
	CG_OmegaTrail( weaponTime, cent->muzzlePoint, trace.endpos );

	if ( !( trace.surfaceFlags & SURF_NOIMPACT ) ) {
		CG_MissileHitWall( cent->currentState.weapon, weaponTime, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
	}
}



/*
===================================================================================================

MISSILE RELATED FUNCTIONS

===================================================================================================
*/


/*
================
CG_FireMissilePredicted
================
*/
void CG_FireMissilePredicted( int weaponTime ) {
	playerState_t	*ps;
	int				weapon;
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			forward, right, start;
	vec3_t			muzzle;
	int				speed;
	usercmd_t		cmd;
	int				time;

	ps = &cg.predictedPlayerState;
	weapon = ps->weapons[ps->wpcat];

	if ( weLi[weapon].prediction != PR_MISSILE ) {
		 return;
	}

	// calculate forward direction
	AngleVectors( ps->viewangles, forward, right, start );

	// calculate muzzle point
	VectorCopy( ps->origin, muzzle );
	muzzle[2] += ps->viewheight;
	VectorMA( muzzle, 14, forward, muzzle );

	// extra vertical velocity for grenades
	if ( weLi[weapon].tr_type == TR_GRAVITY ) {
		forward[2] += 0.3f;
		VectorNormalize( forward );
	}

	// get the correct time
	CG_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd );
	time = cmd.serverTime;
	
	// create local entity
	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	// initialize type and times
	le->leType = LE_MISSILE;
	le->owner = ps->clientNum;

	le->startTime = time - MISSILE_PRESTEP_TIME;
	le->endTime = time + weLi[weapon].spread_move;

	// set position
	le->pos.trType = weLi[weapon].tr_type;
	le->pos.trTime = le->startTime;
	VectorCopy( muzzle, le->pos.trBase );
	VectorCopy( muzzle, re->oldorigin );

	// set speed
	speed = weLi[weapon].tr_delta;
	if ( weLi[weapon].category == CT_EXPLOSIVE ) {
		speed += weLi[weapon].recoil_amount * ( weaponTime << 4 ) / weLi[weapon].recoil_time;
	}
	VectorScale( forward, speed, le->pos.trDelta );

	// set bounce and acceleration
	if ( weLi[weapon].tr_type == TR_ACCELERATE ) le->pos.trDuration = weLi[weapon].spread_crouch;
	if ( weLi[weapon].tr_type == TR_GRAVITY ) le->bounceFactor = BOUNCE_FACTOR_LESS;
	if ( weLi[weapon].category == CT_EXPLOSIVE ) le->bounceFactor = BOUNCE_FACTOR_HALF;

	// set correct render options
	re->hModel = cg_weapons[weapon].missileModel;
	re->renderfx = cg_weapons[weapon].missileRenderfx | RF_NOSHADOW;

	// set trail vars
	le->ti.weapon = weapon;
	le->ti.trailTime = le->startTime + TRAIL_DELAY;
	le->ti.trPos = &le->pos;
	BG_EvaluateTrajectory( &le->pos, le->ti.trailTime, le->ti.lastPos );
}


/*
===================================================================================================

FLAMETHROWER RELATED FUNCTIONS

===================================================================================================
*/


/*
================
CG_FireFlame
================
*/
#define FLAME_HIGH_STEP		8
#define FLAME_LOW_STEP		17
#define BUBBLE_STEP			40

void CG_FireFlame( centity_t *cent ) {
	int				weapon;
	int				time;
	int				step;
	qboolean		inWater;
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			forward, dummy;
	vec3_t			muzzle;

	weapon = cent->currentState.weapon;

	if ( weLi[weapon].prediction != PR_FLAME ) {
		 return;
	}

	// calculate forward direction
	AngleVectors( cent->lerpAngles, forward, dummy, dummy );
	VectorCopy( cent->muzzlePoint, muzzle );

	// spawn bubbles underwater	
	if ( trap_CM_PointContents( muzzle, 0 ) & CONTENTS_WATER ) {
		// spawn bubbles later
		inWater = qtrue;
	} else {
		// play fire sound above water
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cg_weapons[weapon].missileSound );
		inWater = qfalse;
	}

	// set step time
	if ( !inWater ) {
		if ( cg_lowDetailEffects.integer ) step = FLAME_LOW_STEP;
		else step = FLAME_HIGH_STEP;
	} else {
		step = BUBBLE_STEP;	
	}
	time = ( cg.oldTime / step + 1 ) * step;

	while ( time <= cg.time ) {
		if ( !inWater ) {
			// create local entity
			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			// initialize type and times
			le->leType = LE_MISSILE;
			le->ti.weapon = weapon;
			le->owner = cent->currentState.number;
			le->startTime = time - MISSILE_PRESTEP_TIME;
			le->ti.trailTime = le->startTime + TRAIL_DELAY;
			le->endTime = time + weLi[weapon].recoil_time * ( random() * 0.8f + 0.2f );

			// set position
			le->pos.trType = weLi[weapon].tr_type;
			le->pos.trTime = le->startTime;
			VectorCopy( muzzle, le->pos.trBase );
			VectorCopy( muzzle, re->oldorigin );
			VectorScale( forward, weLi[weapon].tr_delta, le->pos.trDelta );

			// set correct render options
			re->reType = RT_SPRITE;
			re->customShader = cgs.media.fireBallShader;
			re->shaderTime = time / 1000.0f;
		} else {
			// spawn bubbles underwater
			CG_Bubble( muzzle );
		}
		// new step time
		time += step;
	}
}


/*
===================================================================================================

EVENT CAUSED FUNCTIONS

===================================================================================================
*/


/*
================
CG_PredictWeapons
================
*/
void CG_PredictWeapons( centity_t *cent, int weaponTime ) {
	// check if prediction makes sense
	if ( !cg.predictWeapons || cg.snap->ps.clientNum != cent->currentState.number ) {
		return;
	}

	// check prediction functions
	CG_FireBulletPredicted( );
	CG_FireShotgunPredicted( );
	CG_FireOmegaPredicted( cent, weaponTime );
	CG_FireMissilePredicted( weaponTime );
}


/*
================
CG_EffectWeapon

Caused by an EV_EFFECT_WEAPON event
================
*/
void CG_EffectWeapon( centity_t *cent ) {
	weaponInfo_t	*weap;
	int		c;

	// set the times
	cent->startEffectTime = cg.time;
	cent->lastEffectTime = 0;

	weap = &cg_weapons[ cent->currentState.weapon ];

	// don't play sound?
	if ( cent->currentState.weapon == WP_OMEGA ) {
		return;
	}

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weap->flashSound[c] )
		{
			trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}

}


/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, int weaponTime, qboolean auto_fire ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if (  c > 0 && ent->weapon != WP_RPG && 
		( ent->weapon != WP_SHOCKER || ent->number != cg.snap->ps.clientNum || cg.time > lastHitTime + weLi[WP_SHOCKER].nextshot ) ) {
		c = rand() % c;
		if ( weap->flashSound[c] )
		{
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}

	// set effect time for WP_OMEGA
	if ( ent->weapon == WP_OMEGA ) {
		cent->startEffectTime = cg.time - weap->effectStayTime;
	}
	// set effect time for grenade perishing
	if ( weLi[ent->weapon].category == CT_EXPLOSIVE ) {
		cent->lastEffectTime = cg.time + weLi[ent->weapon].nextshot + weLi[ent->weapon].reload_time / 2;
	}

	// do brass ejection
	if ( cent->currentState.clientNum != cg.snap->ps.clientNum && cg_brassTime.integer > 0 ) {
		if ( auto_fire ) cent->ejectBrass = -cent->ejectBrass + 1;
		else cent->ejectBrass = 1;
	}

	// set weaponTime to nonPredictedCent
	cg_entities[cent->currentState.clientNum].weaponTime = weaponTime;

	// do weapons prediction
	CG_PredictWeapons( cent, weaponTime );
}




/*
============================================================================

WEAPON IMPACTS

============================================================================
*/


/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( int weapon, int weaponTime, vec3_t origin, vec3_t dir, impactSound_t soundType ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		alphaFade;
	qboolean		isSprite;
	qboolean		checkDistance;
	int				duration;
	vec3_t			speed;
	vec3_t			smokeOrigin;
	int				num;

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	checkDistance = qfalse;
	duration = 600;


	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		CG_BigExplode( origin );
		break;

	case WP_DBT:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.burnMarkShader;
		radius = 12;
		light = 300;
		duration = 200;
		lightColor[0] = 0.1f;
		lightColor[1] = 0.2f;
		lightColor[2] = 1.0f;
		break;

	case WP_FLAMETHROWER:
		mark = cgs.media.burnMarkShader;
		radius = 32;
		checkDistance = qtrue;
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.fireBallShader;
		duration = 600;
		break;

	case WP_TRIPACT:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.tripactExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.burnMarkShader;
		radius = 8;
		light = 300;
		duration = 200;
		lightColor[0] = 1.0f;
		lightColor[1] = 0.7f;
		lightColor[2] = 0.0f;
		break;

	case WP_RPG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		CG_BigExplode( origin );
		break;

	case WP_OMEGA:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.omegaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.burnMarkShader;
		radius = 12;
		light = 300;
		duration = 500;
		lightColor[0] = ( (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.75;
		lightColor[1] = ( 1 - (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.25;
		lightColor[2] = ( 1 - (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.75;
		break;

	case WP_THERMO:
		mark = cgs.media.burnMarkShader;
		radius = 16;
		checkDistance = qtrue;
		if ( weaponTime ) {
			VectorScale( dir, 200.0f, lightColor );
			CG_GenerateParticles( 0, cgs.media.thermoExplosionShader, origin, 10.0f, lightColor, 30.0f, 
				70.0f, (cg_lowDetailEffects.integer) ? 2 : 7, 0, cg.time, 500, 150, 10, 3, 3, 1, PF_GRAVITY, 0 );
		}
		lightColor[0] = 0.1f;
		lightColor[1] = 1.0f;
		lightColor[2] = 0.1f;
		light = 300;
		break;

	case WP_FRAG_GRENADE:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		CG_BigExplode( origin );
		break;

	case WP_FIRE_GRENADE:
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 128;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		CG_FireExplode( origin );
		break;

	default:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
	
		// generate sparks
		if ( weLi[weapon].prediction == PR_PELLETS ) num = cg_lowDetailEffects.integer ? 1 : 2;
		else num = cg_lowDetailEffects.integer ? 2 : 6;

		VectorScale( dir, 400, speed );
		CG_GenerateParticles( cgs.media.bulletImpactModel, 0, origin, 3, speed, 30, 200, num, 
			ENTITYNUM_WORLD, cg.time, 280, 50, 0, 0, 0, 0, PF_GRAVITY, 0 );

		// generate smoke
		if ( weLi[weapon].prediction == PR_PELLETS ) num = cg_lowDetailEffects.integer ? !(int)(random()*4) : !(int)(random()*2);
		else num = 1;

		VectorMA( origin, 6, dir, smokeOrigin );
		VectorScale( dir, 20, speed );
		speed[2] += 30;
		CG_GenerateParticles( 0, cgs.media.vaporShader, smokeOrigin, 2, speed, 30, 15, num, 
			ENTITYNUM_WORLD, cg.time, 450, 50, 8, 2, 20, 6, 0, 0 );

		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}
		radius = 3;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite, checkDistance );
		if ( le ) {
			le->light = light;
			VectorCopy( lightColor, le->lightColor );
			// colorize with beam color
			if ( weapon == WP_OMEGA ) {
				VectorCopy( lightColor, le->color );
			}
		}
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color

	CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, int weaponTime, vec3_t origin, vec3_t dir, int entityNum ) {
	if ( weapon != WP_FLAMETHROWER ) {
		CG_Bleed( origin, entityNum );
	}

	// some weapons will make an explosion with the blood, while
	// others will just make the blood

	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_DBT:
	case WP_TRIPACT:
	case WP_RPG:
	case WP_OMEGA:
		CG_MissileHitWall( weapon, weaponTime, origin, dir, IMPACTSOUND_FLESH );
		break;
	default:
		break;
	}
}


/*
============================================================================

BULLETS

============================================================================
*/


/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest ) {
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 ) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );

	midpoint[0] = ( start[0] + finish[0] ) * 0.5;
	midpoint[1] = ( start[1] + finish[1] ) * 0.5;
	midpoint[2] = ( start[2] + finish[2] ) * 0.5;

	// add the tracer sound
	trap_S_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );

}


/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum ) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t		start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 ) {
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
			sourceContentType = trap_CM_PointContents( start, 0 );
			destContentType = trap_CM_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, 32 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, 32 );
			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( trace.endpos, end, 32 );
			}

			// draw a tracer
			if ( random() < cg_tracerChance.value ) {
				CG_Tracer( start, end );
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		CG_Bleed( end, fleshEntityNum );
	} else {
		CG_MissileHitWall( WP_STANDARD, 0, end, normal, IMPACTSOUND_DEFAULT );
	}
}


/*
============================================================================

OMEGA TRAIL

============================================================================
*/


/*
==========================
CG_OmegaTrail
==========================
*/
#define OMEGA_TRAIL_TIME	400

void CG_OmegaTrail( int weaponTime, vec3_t start, vec3_t end ) {
	localEntity_t	*le;
	refEntity_t		*re;

	//
	// rings
	//
	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + OMEGA_TRAIL_TIME;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_RINGS;
	re->customShader = cgs.media.omegaRingsShader;
	
	VectorCopy( start, re->origin );
	VectorCopy( end, re->oldorigin );

	// calculate color out of charge time
	le->color[0] = ( (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.75;
	le->color[1] = ( 1 - (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.25;
	le->color[2] = ( 1 - (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.75;
	le->color[3] = 1.0f;

	AxisClear( re->axis );

	//
	// core
	//
	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + OMEGA_TRAIL_TIME;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.omegaCoreShader;

	VectorCopy( start, re->origin );
	VectorCopy( end, re->oldorigin );

	// calculate color out of charge time
	le->color[0] = ( (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.75;
	le->color[1] = ( 1 - (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.25;
	le->color[2] = ( 1 - (float)(weaponTime << 4) / weLi[WP_OMEGA].spread_crouch ) * 0.75;
	le->color[3] = 1.0f;

	AxisClear( re->axis );
}

