// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_weapon.c 
// perform the server side effects of a weapon firing


// weapons definitions are in bg_misc.c and bg_public.h


#include "g_local.h"

static	vec3_t	forward, right, up;
static	vec3_t	muzzle;


/*
======================================================================

GENERAL WEAPONS FUNCTIONS

======================================================================
*/

/*
================
G_CalcLagTimeAndShiftAllClients
================
*/
void G_CalcLagTimeAndShiftAllClients( gentity_t	*ent ) {
	// set lag compensation time
	if ( level.delagWeapons && ent->client && !(ent->r.svFlags & SVF_BOT) ) {
		if ( ent->client->pers.cmd.serverTime < level.time - MAX_LAG_COMP ) {
			ent->client->lagTime = level.time - MAX_LAG_COMP;
		} else {
			ent->client->lagTime = ent->client->pers.cmd.serverTime;
		}
	} else {
		ent->client->lagTime = level.time;
	}

    // shift other clients back to the client's idea of the server
    // time to compensate for lag
    if ( level.delagWeapons && ent->client && !(ent->r.svFlags & SVF_BOT) ) {
		G_TimeShiftAllClients( ent->client->lagTime, ent );
	}	
}


/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}

void Bullet_Fire (gentity_t *ent, float spread, int damage, meansOfDeath_t MOD ) {
	trace_t		tr;
	vec3_t		end;
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			passent;
	int			randomSeed;

	randomSeed = ent->client->ps.randomSeed;
	r = Q_crandom( &randomSeed ) * M_PI * 2.0f;
	u = sin(r) * Q_crandom( &randomSeed ) * spread * 16;
	r = cos(r) * Q_crandom( &randomSeed ) * spread * 16;
	VectorMA (muzzle, 8192*16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);

	passent = ent->s.number;

	trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		// compensate for lag effects
		if ( level.delagWeapons && ent->client && !(ent->r.svFlags & SVF_BOT) ) {
			VectorSubtract( traceEnt->client->saved.currentOrigin, traceEnt->r.currentOrigin, end );
			VectorAdd( tr.endpos, end, tr.endpos );
		}
		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		// create impact entity
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
	} else {
		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
		tent->s.eventParm = DirToByte( tr.plane.normal );
	}
	tent->s.otherEntityNum = ent->s.number;

	if ( traceEnt->takedamage) {
			G_Damage( traceEnt, ent, ent, forward, tr.endpos,
				damage, 0, MOD);
	}
}


// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads

gentity_t	*shotEnt;

/*
================
ShotgunPellet
================
*/
void ShotgunPellet( vec3_t start, vec3_t end, gentity_t *ent ) {
	trace_t		tr;
	int			passent;
	gentity_t	*traceEnt;
	vec3_t		tr_start, tr_end;

	passent = ent->s.number;
	VectorCopy( start, tr_start );
	VectorCopy( end, tr_end );

	trap_Trace (&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
	traceEnt = &g_entities[ tr.entityNum ];

	// send bullet impact
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( traceEnt->takedamage ) {
		shotEnt = traceEnt;
		G_Damage( traceEnt, ent, ent, forward, tr.endpos, weLi[ent->s.weapon].damage, 0, weLi[ent->s.weapon].mod);
	}
}

// this should match CG_ShotgunPattern
/*
================
Shotgun_Pattern
================
*/
void ShotgunPattern( vec3_t origin, vec3_t origin2, gentity_t *ent ) {
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

	randomSeed = ent->client->ps.randomSeed;
	// generate the "random" spread pattern
	for ( i = 0 ; i < weLi[ent->s.weapon].recoil_shot ; i++ ) {
		r = Q_crandom( &randomSeed ) * weLi[ent->s.weapon].spread_recoil * 16;
		u = Q_crandom( &randomSeed ) * weLi[ent->s.weapon].spread_recoil * 16;
		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		ShotgunPellet( origin, end, ent );
	}
}


/*
======================================================================

MELEE WEAPONS FIRE CHECK

======================================================================
*/

/*
===============
G_MeleeFireCheck
===============
*/
qboolean G_MeleeFireCheck( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;
	gentity_t	*traceEnt;

	// only living beeings can attack
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	// set aiming directions
	AngleVectors ( ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint ( ent, forward, right, up, muzzle );

	VectorMA (muzzle, 32, forward, end);

	// compensate for lag
	G_CalcLagTimeAndShiftAllClients( ent );

	trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

    // move the clients back to their proper positions
	if ( level.delagWeapons && ent->client && !(ent->r.svFlags & SVF_BOT) ) {
		G_UnTimeShiftAllClients( ent );
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send blood impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		// compensate for lag effects
		if ( level.delagWeapons && ent->client && !(ent->r.svFlags & SVF_BOT) ) {
			VectorSubtract( traceEnt->client->saved.currentOrigin, traceEnt->r.currentOrigin, end );
			VectorAdd( tr.endpos, end, tr.endpos );
		}
		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		// create impact entity
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
	}

	if ( !traceEnt->takedamage) {
		return qfalse;
	}

	G_Damage( traceEnt, ent, ent, forward, tr.endpos,
		weLi[WP_SHOCKER].damage, 0, MOD_SHOCKER );

	return qtrue;
}


/*
======================================================================

WEAPON GENERIC MISSILE FIRE

======================================================================
*/

void weapon_missile_fire ( gentity_t *ent, int weaponTime ) {
	gentity_t	*m;

	// extra vertical velocity
	if ( weLi[ent->s.weapon].tr_type == TR_GRAVITY ) {
		forward[2] += 0.3f;
		VectorNormalize( forward );
	}

	// spawn grenade
	m = fire_missile (ent, weaponTime, muzzle, forward);

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

WEAPON FLAMETHROWER FIRE

======================================================================
*/

void weapon_flamethrower_fire (gentity_t *ent) {
	gentity_t	*m;

	// fire
	m = fire_flame (ent, muzzle, forward);

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

WEAPON BLAST FIRE

======================================================================
*/

void weapon_blast_fire (gentity_t *ent) {
	if ( ent->client->bombzone_time <= 0 ) {
		trap_SendServerCommand( ent-g_entities, "cp \"Plant the blastpack near a bomb site!\n\"");
		return;
	}

	ent->client->bomb_plant_time = level.time + weLi[WP_BLAST].nextshot - 100;
}


/*
======================================================================

WEAPON OMEGA FIRE

======================================================================
*/


void weapon_omega_fire ( gentity_t *ent, int weaponTime ) {
	vec3_t		end, delta;
	trace_t		trace;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;
	int			i;
	int			unlinked;
	int			passent;
	gentity_t	*unlinkedEntities[MAX_OMEGA_HITS];

	weaponTime <<= 4;
	damage = weLi[WP_OMEGA].damage * weaponTime / weLi[WP_OMEGA].spread_crouch;

	VectorMA( muzzle, 8192, forward, end);
	VectorSet( delta, 0, 0, 0 );

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	passent = ent->s.number;
	do {
		trap_Trace (&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT );
		if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL ) {
			break;
		}
		traceEnt = &g_entities[ trace.entityNum ];
		if ( traceEnt->s.eType != ET_PLAYER && !( traceEnt->s.eType == ET_BREAKABLE && traceEnt->s.generic1 ) ) {
			// do the damage
			if ( traceEnt->takedamage ) G_Damage (traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_OMEGA);
			// we hit something solid enough to stop the beam			
			break;
		}
		// do the damage
		if ( traceEnt->takedamage ) {
			G_Damage (traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_OMEGA);

			// compensate for lag effects
			if ( !unlinked && traceEnt->client ) {
				VectorSubtract( traceEnt->client->saved.currentOrigin, traceEnt->r.currentOrigin, delta );
				VectorScale( delta, 1.0 / ( trace.fraction + 0.001 ), delta );
			}
		}
		// create effects on the compensated trail
		VectorMA( trace.endpos, trace.fraction, delta, trace.endpos );

		// create an impact effect
		SnapVectorTowards( trace.endpos, muzzle );
		tent = G_TempEntity( trace.endpos, ( traceEnt->s.eType == ET_PLAYER ) ? EV_MISSILE_HIT : EV_MISSILE_MISS );
		// set generic1 to charge time for correct trail color
		tent->s.generic1 = weaponTime >> 4;
		VectorCopy( trace.endpos, tent->s.origin2 );
		tent->s.eventParm = DirToByte( trace.plane.normal );
		tent->s.weapon = WP_OMEGA;

		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity( traceEnt );
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while ( unlinked < MAX_OMEGA_HITS );

	// link back in any entities we unlinked
	for ( i = 0 ; i < unlinked ; i++ ) {
		trap_LinkEntity( unlinkedEntities[i] );
	}

	// the final trace endpos will be the terminal point of the rail trail

	// create effects on the compensated trail
	VectorMA( trace.endpos, trace.fraction, delta, trace.endpos );

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	SnapVectorTowards( trace.endpos, muzzle );

	// send railgun beam effect
	tent = G_TempEntity( trace.endpos, EV_OMEGATRAIL );

	// set generic1 to charge time for correct trail color
	tent->s.generic1 = weaponTime >> 4;

	VectorCopy( muzzle, tent->s.origin2 );
	// move origin a bit to come closer to the drawn gun muzzle
	VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
	VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if ( trace.surfaceFlags & SURF_NOIMPACT ) {
		tent->s.eventParm = 255;	// don't make the explosion at the end
	} else {
		tent->s.eventParm = DirToByte( trace.plane.normal );
	}
	tent->s.clientNum = ent->s.clientNum;

}


/*
======================================================================

WEAPON BEAM FIRE

======================================================================
*/

void weapon_beam_fire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*traceEnt;
	int			passent;

	passent = ent->s.number;
	VectorMA( muzzle, weLi[ent->s.weapon].spread_base, forward, end );

	trap_Trace( &tr, muzzle, NULL, NULL, end, passent, MASK_SHOT );

	if ( tr.entityNum == ENTITYNUM_NONE ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt->takedamage) {
			G_Damage( traceEnt, ent, ent, forward, tr.endpos,
				weLi[ent->s.weapon].damage, 0, MOD_THERMO );
	}

/*	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	} else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
		tent->s.eventParm = DirToByte( tr.plane.normal );
	}*/
}


/*
======================================================================

WEAPON BULLET FIRE

======================================================================
*/

void weapon_bullet_fire (gentity_t *ent) {
	double spread;

	// calculate spread
	spread = weLi[ent->s.weapon].spread_base;
	if ( !VectorCompare(ent->client->ps.origin, ent->client->oldOrigin) ) spread *= weLi[ent->s.weapon].spread_move;
	if (ent->client->ps.pm_flags & PMF_DUCKED) spread *= weLi[ent->s.weapon].spread_crouch;
	if ( (ent->s.weapon == WP_PHOENIX || ent->s.weapon == WP_SNIPER) && !(ent->client->ps.pm_flags & PMF_ZOOM) ) spread += weLi[ent->s.weapon].tr_type;

	spread += weLi[ent->s.weapon].spread_recoil * ent->client->ps.recoilY / weLi[ent->s.weapon].recoil_time;

	// fire
	Bullet_Fire( ent, spread, weLi[ent->s.weapon].damage, weLi[ent->s.weapon].mod );
}



/*
======================================================================

WEAPON SHOTGUN FIRE

======================================================================
*/


void weapon_shotgun_fire (gentity_t *ent) {
	gentity_t		*tent;

	// send shotgun blast
	tent = G_TempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
	tent->s.otherEntityNum = ent->s.number;

	shotEnt = NULL;
	ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, ent );

	if ( shotEnt && shotEnt->takedamage && shotEnt->client && level.delagWeapons && shotEnt->client && !(ent->r.svFlags & SVF_BOT) ) { 
		vec3_t	delta;
		float	distance;
		
		// compensate for lag effects
		VectorSubtract( shotEnt->r.currentOrigin, ent->r.currentOrigin, delta );
		distance = VectorLength( delta );
		VectorSubtract( shotEnt->client->saved.currentOrigin, shotEnt->r.currentOrigin, delta );
		VectorMA( tent->s.origin2, 4096 / distance, delta, tent->s.origin2 );

		SnapVector( tent->s.origin2 );
	}
}


/*
======================================================================

NON WEAPON SPECIFIC FUNCTIONS

======================================================================
*/


/*
===============
LogAccuracyHit
===============
*/
/*qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if( !attacker->client ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}*/


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePointOrigin ( gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}


/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent, int weaponTime ) {
	// track shots taken for accuracy tracking.
	if( qtrue ) {
		ent->client->accuracy_shots++;
	}

	// set aiming directions
	AngleVectors( ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin( ent, ent->client->oldOrigin, forward, right, up, muzzle );

	// compensate for lag
	G_CalcLagTimeAndShiftAllClients( ent );

	// fire the specific weapon
	switch( weLi[ent->s.weapon].prediction ) {
	case PR_MELEE:
		// no weapon function needed here (look at g_active.c)
		break;
	case PR_BULLET:
		weapon_bullet_fire( ent );
		break;
	case PR_PELLETS:
		weapon_shotgun_fire( ent );
		break;
	case PR_MISSILE:
		weapon_missile_fire( ent, weaponTime );
		break;
	case PR_FLAME:
		weapon_flamethrower_fire( ent );
		break;
	case PR_OMEGA:
		weapon_omega_fire( ent, weaponTime );
		break;
	case PR_BEAM:
		weapon_beam_fire( ent );
		break;
	case PR_NONE:
		if ( ent->s.weapon == WP_BLAST ) weapon_blast_fire( ent );
		break;
	default:
		// shouldn't happen
		break;
	}

    // move the clients back to their proper positions
	if ( level.delagWeapons && ent->client && !(ent->r.svFlags & SVF_BOT) ) {
		G_UnTimeShiftAllClients( ent );
	}
}

