// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = ent->previousTime + ( ent->currentTime - ent->previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & ( EF_BOUNCE_LESS | EF_BOUNCE_HALF ) ) {
		ent->count = 1;
		VectorScale( ent->s.pos.trDelta, ( ent->s.eFlags & EF_BOUNCE_LESS ) ? BOUNCE_FACTOR_LESS : BOUNCE_FACTOR_HALF, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = ent->currentTime;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
//	vec3_t		origin;

//	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
//	SnapVector( origin );
//	VectorCopy( origin, ent->s.origin2 );

	ent->s.time2 = level.time;

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eFlags |= EF_TRAILONLY;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	if ( ent->s.weapon != WP_FIRE_GRENADE ) ent->freeAfterEvent = qtrue;
	ent->currentTime = level.time;

	// splash damage
	if ( ent->splashDamage ) {
		G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath );
	}

	trap_LinkEntity( ent );
}


/*
================
G_MissileImpact
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	other = &g_entities[trace->entityNum];

	// check for bounce (not on sky)
	if ( !(trace->surfaceFlags & SURF_NOIMPACT) && ( ( ent->s.eFlags & EF_BOUNCE_HALF ) ||
		( other->s.eType != ET_PLAYER && ( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_LESS ) ) ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}

	// makes the cgame able to finish the trail
	ent->s.eFlags |= EF_TRAILONLY;
	ent->s.time2 = ent->previousTime + ( ent->currentTime - ent->previousTime ) * trace->fraction;
	ent->freeAfterEvent = qtrue;

	// never explode on sky
	if ( trace->surfaceFlags & SURF_NOIMPACT ) {
		trap_LinkEntity( ent );
		return;
	}

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, other, ent->splashMethodOfDeath );
	}

	// impact damage
	if (other->takedamage) {
		gentity_t	*owner;
		owner = &g_entities[ent->r.ownerNum];
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;

			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}
			G_Damage (other, ent, owner, velocity,
				ent->r.currentOrigin, ent->damage, 
				0, ent->methodOfDeath);
		}
		if ( other->client && ent->currentTime != level.time && level.delagWeapons && owner->client && !(owner->r.svFlags & SVF_BOT) ) {
			vec3_t	delta;
			// compensate for lag effects
			VectorSubtract( other->client->saved.currentOrigin, other->r.currentOrigin, delta );
			VectorAdd( trace->endpos, delta, trace->endpos );
			VectorSubtract( trace->endpos, ent->s.pos.trBase, delta );
			VectorNormalize( delta );
			VectorScale( delta, VectorLength( ent->s.pos.trDelta ), ent->s.pos.trDelta );
			SnapVector( ent->s.pos.trDelta );
		}
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->s.eType == ET_PLAYER ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	trap_LinkEntity( ent );
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;
	qboolean	inWater;

	// special routine for fire grenade damage
	if ( ent->s.eFlags & EF_TRAILONLY ) {
		if ( level.time > ent->s.time2 + weLi[ent->s.weapon].spread_move - 500 ) {
			G_FreeEntity( ent );
		} else if ( level.time > ent->currentTime + 300 ) {
			float	sec;

			ent->currentTime += 300;
			sec = ( level.time - ent->s.time2 ) / 1000.0f;

			G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage * 0.25, 
				50 + ( 0.5*weLi[ent->s.weapon].spread_recoil*sec + weLi[ent->s.weapon].spread_crouch ) * sec, 
				ent, ent->splashMethodOfDeath );
		}
		return;
	}

	// some unlag code
	if ( level.delagWeapons && (ent->flags & FL_NEW_MISSILE) ) {
		// new missiles possibly need time shifts
		if ( level.time - ( ent->s.pos.trTime + MISSILE_PRESTEP_TIME ) >= TIME_SHIFT_DELTA ) {
			ent->currentTime = ent->s.pos.trTime + MISSILE_PRESTEP_TIME;
		} else {
			ent->currentTime = level.time;
		}
		ent->previousTime = ent->s.pos.trTime;
	} else {
		// set normal level.time
		ent->currentTime = level.time;
		ent->previousTime = level.previousTime;
	}

	// it isn't a new missile anymore for sure
	ent->flags &= ~FL_NEW_MISSILE;

	while ( 1 ) {
		// shift all clients back to compensate for lag
		if ( level.delagWeapons && ent->currentTime != level.time ) {
			G_TimeShiftAllClients( ent->currentTime, ent->parent );
		}
	
		// get current position
		BG_EvaluateTrajectory( &ent->s.pos, ent->currentTime, origin );

		// missiles that left the owner bbox will attack anything, even the owner
		if ( ent->count ) {
			passent = ENTITYNUM_NONE;
		} else {
			passent = ent->r.ownerNum;
		}

		// trace a line from the previous position to the current position
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );

		if ( tr.startsolid || tr.allsolid ) {
			// make sure the tr.entityNum is set to the entity we're stuck in
			trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
			tr.fraction = 0;
		}
		else {
			VectorCopy( tr.endpos, ent->r.currentOrigin );
		}

		trap_LinkEntity( ent );

		if ( tr.fraction != 1 ) {
			G_MissileImpact( ent, &tr );
			if ( ent->s.eFlags & EF_TRAILONLY ) {
				// shift all clients back to normal time
				if ( level.delagWeapons && ent->currentTime != level.time ) {
					G_UnTimeShiftAllClients( ent->parent );
				}
				return;		// exploded or disappeared in sky
			}
		}

		// fuse the grenade against oneself if it flies for some time
		if ( !ent->count && ( ent->s.weapon == WP_GRENADE_LAUNCHER || weLi[ent->s.weapon].category == CT_EXPLOSIVE ) && ent->currentTime > ent->s.pos.trTime + 200 ) {
			trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
			if ( !tr.startsolid || tr.entityNum != ent->r.ownerNum ) {
				ent->count = 1;
			}
		}

		// check for medium change
		if ( trap_PointContents( origin, -1 ) & CONTENTS_WATER ) inWater = qtrue;
		else inWater = qfalse;

		if (  ( !inWater && ent->s.pos.trType == TR_WATER_GRAVITY )
			|| ( inWater && ent->s.pos.trType == TR_GRAVITY ) ) {
			// setup new tr
			vec3_t	newDelta;

			BG_EvaluateTrajectoryDelta( &ent->s.pos, ent->currentTime, newDelta );
			VectorCopy( origin, ent->s.pos.trBase );
			VectorCopy( newDelta, ent->s.pos.trDelta );
			ent->s.pos.trTime = ent->currentTime;
			if ( inWater ) ent->s.pos.trType = TR_WATER_GRAVITY;
			else ent->s.pos.trType = TR_GRAVITY;
		}

		// check think function after bouncing
		G_RunThink( ent );

		// shift all clients back to normal time
		if ( level.delagWeapons && ent->currentTime != level.time ) {
			G_UnTimeShiftAllClients( ent->parent );
		}

		// prepare for next time shift or exit function
		ent->previousTime = ent->currentTime;
		ent->currentTime += TIME_SHIFT_DELTA;
		if ( ent->currentTime > level.time ) {
			// don't think into future
			return;
		} else if ( level.time - ent->currentTime < TIME_SHIFT_DELTA ) {
			// don't shift for senslessly small amounts of time
			ent->currentTime = level.time;
		}
	}
}


/*
================
G_ExplodeBlast
================
*/
void G_ExplodeBlast( gentity_t *ent ) {
	// ent->enemy = bombzone
	if ( ent->enemy && ent->enemy->s.eType == ET_BOMBZONE )
		ent->enemy->use( ent->enemy, ent, ent->activator );
	
	ent->freeAfterEvent = qtrue;

	trap_LinkEntity( ent );
}


/*
================
G_RunBlast
================
*/
void G_RunBlast( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	passent = ent->r.ownerNum;
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );

	if ( tr.fraction != 1 && tr.plane.normal[2] > 0.2 ) {
		tr.endpos[2] += 3;
		G_SetOrigin( ent, tr.endpos );
	}

	trap_LinkEntity( ent );

	G_RunThink ( ent );
}


//=============================================================================



/*
=================
fire_flame
=================
*/
gentity_t *fire_flame (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "flame";
	bolt->nextthink = level.time + weLi[WP_FLAMETHROWER].recoil_time; // recoil_time is the lifetime
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_NOCLIENT;
	bolt->s.weapon = WP_FLAMETHROWER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = weLi[WP_FLAMETHROWER].damage;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_FLAMETHROWER;
	bolt->splashMethodOfDeath = MOD_FLAMETHROWER;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	bolt->flags = FL_NEW_MISSILE;

	bolt->s.pos.trType = weLi[self->s.weapon].tr_type;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	bolt->s.time = self->client->pers.cmd.serverTime;			// used by the client to determine the size of a fireball

//	start[0] += dir[1]*10; // start a little bit more on the right (better client view)
//	start[1] -= dir[0]*10;
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, weLi[WP_FLAMETHROWER].tr_delta, bolt->s.pos.trDelta );	// recoil_amount is speed
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}	

//=============================================================================

/*
=================
fire_blast
=================
*/
gentity_t *fire_blast (gentity_t *self, vec3_t start) {
	gentity_t	*bolt;

	bolt = G_Spawn();
	bolt->classname = "blastpack";
	bolt->nextthink = level.time + BOMB_TIME;
	bolt->think = G_ExplodeBlast;
	bolt->s.eType = ET_BLAST;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BLAST;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->clipmask = MASK_SHOT | MASK_WATER;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time;
	
	VectorCopy( start, bolt->s.pos.trBase );
//	VectorScale( dir, 10, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}	

//=============================================================================


/*
=================
fire_missile
=================
*/
gentity_t *fire_missile (gentity_t *self, int weaponTime, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;
	int			speed;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "missile";
	bolt->nextthink = level.time + weLi[self->s.weapon].spread_move;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = self->s.weapon;
	bolt->r.ownerNum = self->s.number;
	bolt->s.clientNum = self->s.number;
	bolt->parent = self;
	bolt->damage = weLi[self->s.weapon].damage;
	bolt->splashDamage = weLi[self->s.weapon].damage;
	bolt->splashRadius = weLi[self->s.weapon].spread_base;
	bolt->methodOfDeath = weLi[self->s.weapon].mod;
	bolt->splashMethodOfDeath = weLi[self->s.weapon].mod + 1; // take note of this!
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	bolt->flags = FL_NEW_MISSILE;

	if ( weLi[self->s.weapon].tr_type == TR_ACCELERATE ) bolt->s.pos.trDuration = weLi[self->s.weapon].spread_crouch;
	if ( weLi[self->s.weapon].tr_type == TR_GRAVITY ) bolt->s.eFlags = EF_BOUNCE_LESS;
	if ( weLi[self->s.weapon].category == CT_EXPLOSIVE ) bolt->s.eFlags = EF_BOUNCE_HALF;

	// count is used to check if the missile left the player bbox
	// if count == 1 then the missile left the player bbox and can attack him
	bolt->count = 0;

	bolt->s.pos.trType = weLi[self->s.weapon].tr_type;
	bolt->s.pos.trTime = self->client->lagTime - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorCopy( start, bolt->r.currentOrigin );
	
	speed = weLi[self->s.weapon].tr_delta;
	if ( weLi[self->s.weapon].category == CT_EXPLOSIVE && weaponTime ) {
		speed += weLi[self->s.weapon].recoil_amount * ( weaponTime << 4 ) / weLi[self->s.weapon].recoil_time;
	}

	VectorScale( dir, speed, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth


	return bolt;
}

