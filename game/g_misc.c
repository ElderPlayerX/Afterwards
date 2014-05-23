// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_misc.c

#include "g_local.h"


/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/


/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_camp( gentity_t *self ) {
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null( gentity_t *self ) {
	G_FreeEntity( self );
}


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) linear
Non-displayed light.
"light" overrides the default 300 intensity.
Linear checbox gives linear falloff instead of inverse square
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
*/
void SP_light( gentity_t *self ) {
	G_FreeEntity( self );
}


/*QUAKED info_item (1 0 1) (-8 -8 -8) (8 8 8) RED_TEAM BLUE_TEAM FORCE_RETURN
Spawn point for an Afterwards objective item. It holds some general information about the item. The target specifies the zone the item needs to be brought to. 
Additionally it acts like a "target_switch" entity. The state is ON if the item is just lying around somewhere.
If it's triggered and the item is lying around (or the FORCE_RETURN flag is set) it will be returned.
RED_TEAM and BLUE_TEAM specify the members of which team are able to take the item with them.
"model" the model the item has lying in the world
"model2" the model the item has on the player's body (with pockets perhaps...)
"message" the message displayed when the item has been taken.
"message2" the message displayed when the item has been dropped.
*/
void Item_Use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	int	i;

	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		if ( g_entities[i].parent == self ) {
			G_FreeEntity( &g_entities[i] );
			LaunchObjectiveItem( self, self->s.origin, vec3_origin );
			self->count = 0;
			return;
		}
	}
	// force return, take items away 
	if ( self->spawnflags & 4 ) {
		gentity_t	*ent;

		for ( i = 0; i < level.maxclients; i++ ) {
			ent = &g_entities[i];
			if ( ent->client && ent->client->ps.powerups[PW_ITEM] && ent->client->itemParent == self ) {
				ent->client->ps.powerups[PW_ITEM] = 0;
				LaunchObjectiveItem( self, self->s.origin, vec3_origin );
				self->count = 0;
				return;
			}
		}
	}
}

void SP_info_item( gentity_t *self ) {
	if ( g_gametype.integer == GT_TACTICAL ) {
		self->s.eType = ET_GENERAL;
		self->s.modelindex = G_ModelIndex( self->model );
		if ( self->model2 ) self->s.modelindex2 = G_ModelIndex( self->model2 );
		else self->s.modelindex2 = G_ModelIndex( self->model );

		self->count = 0;
		self->use = Item_Use;
		G_SetOrigin( self, self->s.origin );

		LaunchObjectiveItem( self, self->s.origin, vec3_origin );
	}
}



/*
=================================================================================

TELEPORTERS

=================================================================================
*/

void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles ) {
	gentity_t	*tent;

	// use temp events at source and destination to prevent the effect
	// from getting item by a second player event
	if ( player->client->sess.sessionTeam < TEAM_SPECTATOR ) {
		tent = G_TempEntity( player->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity (player);

	VectorCopy ( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;

	// spit the player out
	AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	VectorScale( player->client->ps.velocity, 400, player->client->ps.velocity );
	player->client->ps.pm_time = 160;		// hold time
	player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// kill anything at the destination
	if ( player->client->sess.sessionTeam < TEAM_SPECTATOR ) {
		G_KillBox (player);
	}

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( player->client->sess.sessionTeam < TEAM_SPECTATOR ) {
		trap_LinkEntity (player);
	}

    // reset the origin trail for this player to avoid lerping in a time shift
    G_ResetTrail( player );
}


/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest( gentity_t *ent ) {
}


//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 file to display
*/
void SP_misc_model( gentity_t *ent ) {

#if 0
	ent->s.modelindex = G_ModelIndex( ent->model );
	VectorSet (ent->mins, -16, -16, -16);
	VectorSet (ent->maxs, 16, 16, 16);
	trap_LinkEntity (ent);

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
#else
	G_FreeEntity( ent );
#endif
}



//===========================================================

/*QUAKED misc_scripted_model (1 0 0) (-16 -16 -16) (16 16 16) FINISH_ANIM
"model"		model to display
"speed"		the animation playback speed (time from frame to frame in sec)
"script0"	first script line that controls the model
"scriptX"	every time this entity is triggered, the next script line is executed

FINISH_ANIM	after a trigger the animation currently running is finished before proceeding to next script line

SCRIPT COMMANDS
"next"		proceed to next script line
"invisible"	make it invisible
"visible"	make it visible
"destroy"	destroy it (it won't be seen again)
"trigger"	trigger it's targets
"speed xxx"	set new playback speed
"pause"		pause for one frame
"wait"		wait until triggered
or just a number representing the next frame of the animation.

NOTE: Never create a script line without a pause, destroy, next or a frame, as this will result in an infinite loop.
*/

// some typedefs
typedef enum {
	SS_END,
	SS_NEXT_LINE,
	SS_NEXT_LINE_CONTINUE,
	SS_INVISIBLE,
	SS_VISIBLE,
	SS_DESTROY,
	SS_TRIGGER,
	SS_SPEED,
	SS_PAUSE,
	SS_WAIT,
	SS_FRAME
} ssType_t;

void G_GetNextScriptLine( gentity_t *ent ) {
	while ( 1 ) {
		if ( level.ssType[ent->ssNum][ent->count] == SS_NEXT_LINE ) {
			ent->count++;
			return;
		}
		if ( level.ssType[ent->ssNum][ent->count] == SS_END ) {
			ent->count = 0;
			return;
		}
		ent->count++;
	}
}

void misc_scripted_model_think( gentity_t *ent ) {
	int		tag;

	// execute commands until there's a frame display request
	while ( 1 ) {
		tag = level.ssTag[ent->ssNum][ent->count];
		switch ( level.ssType[ent->ssNum][ent->count] ) {
		case SS_END:
			// go back to the beginning of that line if not triggered
			if ( !ent->ssContinue ) ent->count = tag;
			else ent->ssContinue--;
			break;
		case SS_NEXT_LINE:
			// go back to the beginning of that line if not triggered
			if ( !ent->ssContinue ) ent->count = tag;
			else ent->ssContinue--;
			break;
		case SS_NEXT_LINE_CONTINUE:
			// continue
			break;
		case SS_INVISIBLE:
			// set invisible flag
			ent->s.generic1 |= SMF_INVISIBLE;
			break;
		case SS_VISIBLE:
			// clear invisible flag
			ent->s.generic1 &= ~SMF_INVISIBLE;
			break;
		case SS_DESTROY:
			// destroy the entity
			G_FreeEntity( ent );
			return;
		case SS_TRIGGER:
			// trigger targets
			G_UseTargets( ent, ent->activator );
			break;
		case SS_SPEED:
			// set new speed
			ent->s.time2 = tag;
			break;
		case SS_PAUSE:
			// set new time for pause
			ent->s.time = level.time;

			// prepare next think
			ent->nextthink = level.time + ent->s.time2;
			ent->count++;
			return;
		case SS_WAIT:
			// don't do a next think, just wait
			ent->nextthink = 0;
			return;
		case SS_FRAME:
			// set new time and shift framelist and insert new frame
			ent->s.time = level.time;
			ent->s.otherEntityNum2 = ( ent->s.frame >> 8 );
			ent->s.frame = ( ent->s.frame << 8 ) | ( tag & 0xFF );

			// prepare next think
			ent->nextthink = level.time + ent->s.time2;
			ent->count++;
			return;
		default:
			// impossible
			G_Printf( S_COLOR_YELLOW "Error in script executing. Stopping!\n" );
			ent->nextthink = 0;
			return;
		}
		ent->count++;
	}
}

void misc_scripted_model_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	// store activator
	self->activator = activator;

	if ( (self->spawnflags & 1) && self->nextthink ) 
		// finish anim and continue later
		self->ssContinue++;
	else {
		// get next script command
		G_GetNextScriptLine( self );
		misc_scripted_model_think( self );
	}
	// no more actions here, entity could be destroyed
}

void SP_misc_scripted_model( gentity_t *ent ) {
	char	*parse;
	char	*token;
	int		ssPos;
	int		lastNewLine;

	// register model
	ent->s.modelindex = G_ModelIndex( ent->model );

	// set some important info
	ent->s.eType = ET_SCRIPTED_MODEL;
	VectorSet (ent->r.mins, -16, -16, -16);
	VectorSet (ent->r.maxs, 16, 16, 16);

	// set origin
	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	// setup animation
	ent->s.frame = 0;
	ent->s.time = level.time;
	if ( ent->speed <= 0 ) ent->speed = 1.0f;
	ent->s.time2 = (int)(ent->speed * 1000);

	// link the entity
	trap_LinkEntity (ent);

	// get first script line
	level.lastSS++;
	ent->ssNum = level.lastSS;
	ent->count = 0;
	G_SpawnString( "script0", NULL, &parse );

	if ( !parse ) {
		// no script, don't do any thinking
		G_Printf( S_COLOR_YELLOW "Scripted model doesn't have a script0. Just displaying the model." );
		ent->nextthink = 0;
	}

	// setup use & think function for future frames
	ent->use = misc_scripted_model_use;
	ent->think = misc_scripted_model_think;
	ent->nextthink = level.time + FRAMETIME;

	// parse the scripts
	ssPos = 0;
	lastNewLine = -1;
	while ( ssPos < MAX_SEQUENCE_LENGTH-1 ) {
		// get the next script command
		token = COM_Parse( &parse );

		if ( !*token || level.ssType[ent->ssNum][ssPos] == SS_NEXT_LINE_CONTINUE ) {
			// no script command -> go on
			G_SpawnString( va( "script%i", ent->count+1 ), NULL, &parse );
			if ( parse ) {
				// there's another line, continue
				ent->count++;
				if ( level.ssType[ent->ssNum][ssPos] != SS_NEXT_LINE_CONTINUE ) 
					level.ssType[ent->ssNum][ssPos] = SS_NEXT_LINE;
				level.ssTag[ent->ssNum][ssPos] = lastNewLine;
				lastNewLine = ssPos;
				ssPos++;
				token = COM_Parse( &parse );
			} else {
				// was last line -> finalize
				if ( level.ssType[ent->ssNum][ssPos] == SS_NEXT_LINE_CONTINUE ) ssPos++;
				level.ssType[ent->ssNum][ssPos] = SS_END;
				level.ssTag[ent->ssNum][ssPos] = lastNewLine;
				ent->count = 0;
				return;
			}
		}

		if ( !Q_stricmp( token, "next" ) ) {
			// get next script command
			level.ssType[ent->ssNum][ssPos] = SS_NEXT_LINE_CONTINUE;
		} else if ( !Q_stricmp( token, "invisible" ) ) {
			// make entity invisible
			level.ssType[ent->ssNum][ssPos] = SS_INVISIBLE;
			ssPos++;
		} else if ( !Q_stricmp( token, "visible" ) ) {
			// make entity visible
			level.ssType[ent->ssNum][ssPos] = SS_VISIBLE;
			ssPos++;
		} else if ( !Q_stricmp( token, "destroy" ) ) {
			// destroy entity
			level.ssType[ent->ssNum][ssPos] = SS_DESTROY;
			ssPos++;
		} else if ( !Q_stricmp( token, "trigger" ) ) {
			// trigger targets
			level.ssType[ent->ssNum][ssPos] = SS_TRIGGER;
			ssPos++;
		} else if ( !Q_stricmp( token, "speed" ) ) {
			// set new animation speed
			token = COM_Parse( &parse );
			ent->speed = atof( token );
			if ( ent->speed <= 0 ) {
				// just use the old speed if nothing useful is specified
				G_Printf( S_COLOR_YELLOW "Impossible speed in script: '%s' skipped\n", token );
			} else {
				level.ssType[ent->ssNum][ssPos] = SS_SPEED;
				level.ssTag[ent->ssNum][ssPos] = (int)(ent->speed * 1000);
				ssPos++;
			}
		} else if ( !Q_stricmp( token, "pause" ) ) {
			// pause
			level.ssType[ent->ssNum][ssPos] = SS_PAUSE;
			ssPos++;
		} else if ( !Q_stricmp( token, "wait" ) ) {
			// wait until triggered
			level.ssType[ent->ssNum][ssPos] = SS_WAIT;
			ssPos++;
		} else {
			// has to be a frame for an animation
			if ( token[0] < '0' || token[0] > '9' ) {
				G_Printf( S_COLOR_YELLOW "Unknown script command: '%s' skipped\n", token );
				return;
			}
			level.ssType[ent->ssNum][ssPos] = SS_FRAME;
			level.ssTag[ent->ssNum][ssPos] = atoi( token );
			ssPos++;
		}		
	}
	// sequence too long
	G_Printf( S_COLOR_YELLOW "Script sequence too long (more than 64 commands), ignoring commands.\n" );
	level.ssType[ent->ssNum][MAX_SEQUENCE_LENGTH-1] == SS_END;
}

//===========================================================

void locateCamera( gentity_t *ent ) {
	vec3_t		dir;
	gentity_t	*target;
	gentity_t	*owner;

	owner = G_PickTarget( ent->target );
	if ( !owner ) {
		G_Printf( "Couldn't find target for misc_partal_surface\n" );
		G_FreeEntity( ent );
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if ( owner->spawnflags & 1 ) {
		ent->s.frame = 25;
	} else if ( owner->spawnflags & 2 ) {
		ent->s.frame = 75;
	}

	// set to 0 for no rotation at all
	ent->s.powerups = 1;

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy( owner->s.origin, ent->s.origin2 );

	// see if the portal_camera has a target
	target = G_PickTarget( owner->target );
	if ( target ) {
		VectorSubtract( target->s.origin, owner->s.origin, dir );
		VectorNormalize( dir );
	} else {
		G_SetMovedir( owner->s.angles, dir );
	}

	ent->s.eventParm = DirToByte( dir );
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void SP_misc_portal_surface(gentity_t *ent) {
	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity (ent);

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if ( !ent->target ) {
		VectorCopy( ent->s.origin, ent->s.origin2 );
	} else {
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera(gentity_t *ent) {
	float	roll;

	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity (ent);

	G_SpawnFloat( "roll", "0", &roll );

	ent->s.clientNum = roll/360.0 * 256;
}

/*
======================================================================

  SHOOTERS

======================================================================
*/

void Use_Shooter( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	vec3_t		dir;
	float		deg;
	vec3_t		up, right;

	// see if we have a target
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->r.currentOrigin, ent->s.origin, dir );
		VectorNormalize( dir );
	} else {
		VectorCopy( ent->movedir, dir );
	}

	// randomize a bit
	PerpendicularVector( up, dir );
	CrossProduct( up, dir, right );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, up, dir );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, right, dir );

	VectorNormalize( dir );

	// Herby modify this !
/*	fire_rocket( ent, ent->s.origin, dir );
	switch ( ent->s.weapon ) {
	case WP_GRENADE_LAUNCHER:
		fire_grenade( ent, ent->s.origin, dir );
		break;
	case WP_ROCKET_LAUNCHER:
		fire_rocket( ent, ent->s.origin, dir );
		break;
	case WP_PLASMAGUN:
		fire_plasma( ent, ent->s.origin, dir );
		break;
	}*/

	G_AddEvent( ent, EV_FIRE_WEAPON, 0 );
}


static void InitShooter_Finish( gentity_t *ent ) {
	ent->enemy = G_PickTarget( ent->target );
	ent->think = 0;
	ent->nextthink = 0;
}

void InitShooter( gentity_t *ent, int weapon ) {
	ent->use = Use_Shooter;
	ent->s.weapon = weapon;

	RegisterItem( BG_FindItemForWeapon( weapon ) );

	G_SetMovedir( ent->s.angles, ent->movedir );

	if ( !ent->random ) {
		ent->random = 1.0;
	}
	ent->random = sin( M_PI * ent->random / 180 );
	// target might be a moving object, so we can't set movedir for it
	if ( ent->target ) {
		ent->think = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	trap_LinkEntity( ent );
}

/*QUAKED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_rocket( gentity_t *ent ) {
	InitShooter( ent, WP_GRENADE_LAUNCHER ); // Herby modify this !
}

/*QUAKED shooter_plasma (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_plasma( gentity_t *ent ) {
	InitShooter( ent, WP_GRENADE_LAUNCHER ); // Herby modify this !
}

/*QUAKED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_grenade( gentity_t *ent ) {
	InitShooter( ent, WP_GRENADE_LAUNCHER );
}


#ifdef MISSIONPACK
static void PortalDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
	G_FreeEntity( self );
	//FIXME do something more interesting
}


void DropPortalDestination( gentity_t *player ) {
	gentity_t	*ent;
	vec3_t		snapped;

	// create the portal destination
	ent = G_Spawn();
	ent->s.modelindex = G_ModelIndex( "models/powerups/teleporter/tele_exit.md3" );

	VectorCopy( player->s.pos.trBase, snapped );
	SnapVector( snapped );
	G_SetOrigin( ent, snapped );
	VectorCopy( player->r.mins, ent->r.mins );
	VectorCopy( player->r.maxs, ent->r.maxs );

	ent->classname = "hi_portal destination";
	ent->s.pos.trType = TR_STATIONARY;

	ent->r.contents = CONTENTS_CORPSE;
	ent->takedamage = qtrue;
	ent->health = 200;
	ent->die = PortalDie;

	VectorCopy( player->s.apos.trBase, ent->s.angles );

	ent->think = G_FreeEntity;
	ent->nextthink = level.time + 2 * 60 * 1000;

	trap_LinkEntity( ent );

	player->client->portalID = ++level.portalSequence;
	ent->count = player->client->portalID;

	// give the item back so they can drop the source now
	player->client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItem( "Portal" ) - bg_itemlist;
}


static void PortalTouch( gentity_t *self, gentity_t *other, trace_t *trace) {
	gentity_t	*destination;

	// see if we will even let other try to use it
	if( other->health <= 0 ) {
		return;
	}
	if( !other->client ) {
		return;
	}
//	if( other->client->ps.persistant[PERS_TEAM] != self->spawnflags ) {
//		return;
//	}

	if ( other->client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
		Drop_Item( other, BG_FindItemForPowerup( PW_NEUTRALFLAG ), 0 );
		other->client->ps.powerups[PW_NEUTRALFLAG] = 0;
	}
	else if ( other->client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
		Drop_Item( other, BG_FindItemForPowerup( PW_REDFLAG ), 0 );
		other->client->ps.powerups[PW_REDFLAG] = 0;
	}
	else if ( other->client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
		Drop_Item( other, BG_FindItemForPowerup( PW_BLUEFLAG ), 0 );
		other->client->ps.powerups[PW_BLUEFLAG] = 0;
	}

	// find the destination
	destination = NULL;
	while( (destination = G_Find(destination, FOFS(classname), "hi_portal destination")) != NULL ) {
		if( destination->count == self->count ) {
			break;
		}
	}

	// if there is not one, die!
	if( !destination ) {
		if( self->pos1[0] || self->pos1[1] || self->pos1[2] ) {
			TeleportPlayer( other, self->pos1, self->s.angles );
		}
		G_Damage( other, other, other, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
		return;
	}

	TeleportPlayer( other, destination->s.pos.trBase, destination->s.angles );
}


static void PortalEnable( gentity_t *self ) {
	self->touch = PortalTouch;
	self->think = G_FreeEntity;
	self->nextthink = level.time + 2 * 60 * 1000;
}


void DropPortalSource( gentity_t *player ) {
	gentity_t	*ent;
	gentity_t	*destination;
	vec3_t		snapped;

	// create the portal source
	ent = G_Spawn();
	ent->s.modelindex = G_ModelIndex( "models/powerups/teleporter/tele_enter.md3" );

	VectorCopy( player->s.pos.trBase, snapped );
	SnapVector( snapped );
	G_SetOrigin( ent, snapped );
	VectorCopy( player->r.mins, ent->r.mins );
	VectorCopy( player->r.maxs, ent->r.maxs );

	ent->classname = "hi_portal source";
	ent->s.pos.trType = TR_STATIONARY;

	ent->r.contents = CONTENTS_CORPSE | CONTENTS_TRIGGER;
	ent->takedamage = qtrue;
	ent->health = 200;
	ent->die = PortalDie;

	trap_LinkEntity( ent );

	ent->count = player->client->portalID;
	player->client->portalID = 0;

//	ent->spawnflags = player->client->ps.persistant[PERS_TEAM];

	ent->nextthink = level.time + 1000;
	ent->think = PortalEnable;

	// find the destination
	destination = NULL;
	while( (destination = G_Find(destination, FOFS(classname), "hi_portal destination")) != NULL ) {
		if( destination->count == ent->count ) {
			VectorCopy( destination->s.pos.trBase, ent->pos1 );
			break;
		}
	}

}
#endif


/*
======================
Think_SpawnNewBreakableTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewBreakableTrigger( gentity_t *ent ) {
	gentity_t		*other;
	vec3_t		mins, maxs;
	int			i, best;

	// find the bounds of everything on the team
	VectorCopy (ent->r.absmin, mins);
	VectorCopy (ent->r.absmax, maxs);

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs[i] - mins[i] < maxs[best] - mins[best] ) {
			best = i;
		}
	}
	maxs[best] += 2;
	mins[best] -= 2;

	// create a spectator trigger for teleporting with this size
	other = G_Spawn ();
	other->classname = "breakable_trigger";
	VectorCopy (mins, other->r.mins);
	VectorCopy (maxs, other->r.maxs);
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_SpectatorTrigger;
	// remember the thinnest axis
	other->count = best;
	trap_LinkEntity (other);

	ent->teamchain = other;
}


/*QUAKED func_breakable (0.2 0.2 0.2) ? TRIGGER_ONLY SPECTATOR_PASSABLE 
Creates different types of breakable objects.
"health" is the number of health points it has.
"spldmg" is the splash damage it does.
"splrad" is the splash radius it has (in game units).
"model2" .md3 model to also draw
"effect" is the type of the breaking effect.
	"none" no effect, just deletion.
	"glass" breaks a glass.
	"wood" breaks wood.
	"metal" breaks metal.
	"explosive" explodes (use "spldmg" and "splrad"!).	
*/
void SP_func_breakable( gentity_t *ent ) {
	char	*s;
	
	// Make it appear as the brush
	trap_SetBrushModel( ent, ent->model );
	
	ent->use = G_Breakable;
	ent->s.eType = ET_BREAKABLE;

	if ( ent->spawnflags & 1 )	ent->takedamage = qfalse;
	else ent->takedamage = qtrue;

	// determine the effect the breakable has
	G_SpawnString( "effect", "none", &s );
	if ( !strcmp( s, "glass" ) ) { ent->effect = BE_GLASS; ent->s.generic1 = 1; }
	else if ( !strcmp( s, "wood" ) ) { ent->effect = BE_WOOD; ent->s.generic1 = 1; }
	else if ( !strcmp( s, "metal" ) ) { ent->effect = BE_METAL; ent->s.generic1 = 0; }
	else if ( !strcmp( s, "explosive" ) ) { ent->effect = BE_EXPLOSIVE; ent->s.generic1 = 0; }
	else { ent->effect = BE_NONE; ent->s.generic1 = 0; }

	// Lets give it 5 health if the mapper did not set its health
	if( ent->health <= 0 ) ent->health = 5;

	// If the mapper gave it a model, use it
	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}
	
	// make it passable for spectators if needed
	if ( ent->spawnflags & 2 ) {
		ent->nextthink = level.time + FRAMETIME;
		ent->think = Think_SpawnNewBreakableTrigger;
	}

	// Link all ^this^ info into the ent
	trap_LinkEntity (ent);
}


/*
=================
G_Breakable

other is NULL if the entity has been shot
if not, it is set
=================
*/
void G_Breakable( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t   *tent;
    vec3_t      mins, maxs;
    vec3_t      center, size;
	int		effect;
 
 	// If the breakable has no more life or it is triggered, BREAK IT
 	if( self->health <= 0 || other) {
		G_UseTargets( self, activator );

		// Get the center of the breakable
		VectorCopy( self->r.maxs, mins );
		VectorCopy( self->r.mins, maxs );

		VectorSubtract(maxs, mins, size);
		VectorScale(size, 0.5, size);
		VectorAdd(mins, size, center);
 
		// do the splash damage
		if ( self->splashDamage && self->splashRadius )
			G_RadiusDamage( center, NULL, self->splashDamage, self->splashRadius, self, MOD_EXPLODED );
		
		effect = self->effect;

		// delete entity
		if ( self->teamchain ) G_FreeEntity( self->teamchain );
		G_FreeEntity( self );

		// create entity for client effects
		tent = G_TempEntity( center, EV_BREAKABLE );
 		tent->s.eventParm = effect;
		VectorCopy( maxs, tent->s.origin );
		VectorCopy( mins, tent->s.origin2 );
		SnapVector( tent->s.origin );
		SnapVector( tent->s.origin2 );
 	}
}


/*
=================
G_EffectThink
=================
*/
void G_EffectThink( gentity_t *ent ) {
	if ( ent->s.generic1 & FX_LIGHTNING ) {
		// initialize the lightning now
		gentity_t	*target;

		// search target for lightning
		target = G_Find (NULL, FOFS(target), ent->targetname);
		if ( target ) {
			// save the position of the target entity
			VectorCopy( target->s.origin, ent->s.origin2 );
		} else {
			// no target found, free the entity
			G_Printf( S_COLOR_YELLOW "Can't find target for lightning!\n" );
			G_FreeEntity( ent );
			return;
		}
	} else {
		// that's for the auto-return option
		// swap on/off flag
		ent->s.generic1 ^= (1 << 7);
	}
	ent->nextthink = 0;
}


/*
=================
G_EffectUse
=================
*/
void G_EffectUse( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( self->s.generic1 & FX_LIGHTNING ) {
		// setting the time let's the lightning appear
		if ( other->s.eType == ET_EFFECT && other->s.generic1 & FX_LIGHTNING ) self->s.time = other->s.time + 20;
		else self->s.time = level.time;
	} else {
		// swap on/off flag
		self->s.generic1 ^= (1 << 7);
		// autoreturn is based on nextthink
		if ( self->spawnflags & 4 ) {
			self->nextthink = level.time + self->wait * 1000;
		}
	}
	G_UseTargets( self, activator );
}


/*QUAKED misc_effect (0.2 0.2 0.2) (-8 -8 -8) (8 8 8) START_OFF MODELSWITCH AUTO_RETURN
Creates a special effect.

START_OFF must be triggered to start the effect
MODELSWITCH models appears and disappears if triggered
AUTO_RETURN sets the state automatically back instead of toggling
"model" .md3 model to also draw
"wait" autoreturn time (in sec, def: 4) or duration of lightning (in msec, def: 400)
"effect" is the type of effect
	"none" is no effect
	"fire" is a big fire
	"lightning" a lightning from that point to the only entity with the corresponding "target" value
*/
void SP_misc_effect( gentity_t *ent ) {
	char		*s;
	
	ent->use = G_EffectUse;
	ent->think = G_EffectThink;
	ent->s.eType = ET_EFFECT;

	// start on/off
	if ( ent->spawnflags & 1 ) ent->s.generic1 = 0;
	else ent->s.generic1 = (1 << 7);
	// model appear/disappear
	if ( ent->spawnflags & 2 ) ent->s.generic1 |= (1 << 6);

	// determine the effect the breakable has
	G_SpawnString( "effect", "none", &s );
	if ( !strcmp( s, "fire" ) ) ent->s.generic1 |= FX_FIRE;
	else if ( !strcmp( s, "lightning" ) ) ent->s.generic1 |= FX_LIGHTNING;
	else {
		G_Printf( S_COLOR_YELLOW "misc_effect with no effect type specified at %s\n", vtos(ent->s.origin)  );
		G_FreeEntity( ent );
		return;
	}

	// If the mapper gave it a model, use it
	if ( ent->model ) {
		ent->s.modelindex = G_ModelIndex( ent->model );
	}

	if ( ent->s.generic1 & FX_LIGHTNING ) {
		// set the time values
		ent->s.time = 0;
		if ( !ent->wait ) ent->wait = 200;
		ent->s.time2 = ent->wait;

		// store the size in s.otherEntityNum2
		if ( !ent->count ) ent->count = 30;
		ent->s.otherEntityNum2 = ent->count;

		// search for lightning targets later
		ent->nextthink = level.time + 300;
	} else {
		// set autoreturn default time
		if ( !ent->wait ) ent->wait = 4;
	}
	
	// Link all ^this^ info into the ent
	trap_LinkEntity (ent);
}


/*
=================
G_ParticlesUse
=================
*/
void G_ParticlesUse( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	// toggle active flag and add event
	self->s.generic1 ^= PF_OFF;
	G_AddEvent( self, EV_PARTICLES, 0 );
}


/*QUAKED misc_particles (0.6 0.2 0.2) (-8 -8 -8) (8 8 8) START_OFF GRAVITY AXIS_ALIGNED COLLISIONS
Creates a particle emitting entity. Particles can be either shader sprites or models.
If a value has a variance, add a second value and the resulting value will be somewhere between (value - variance) and (value + variance).
Default values are always 0 if nothing else is specified.

START_OFF must be triggered to start the effect
GRAVITY particles affected by gravity
AXIS_ALIGNED models aren't rotated into flight direction
COLLISIONS the particles are able to collide and generate an impact effect 
"model" particle model
"shader" particle shader (for sprites)
"randomPos" starting position variance (max 1023)
"size" size of a particle with variance (not needed with models, but can be used for scaling, max 655.35)
"addSize" size of a particle with variance (not needed for models too, max 655.35)
"life" lifetime of a particle in seconds with variance (max 65.535)
"count" number of particles per second with variance (max 655.35)
"duration" time it emits particles with variance (use 0 to emit <count> particles instantly or use negative numbers to emit them forever, max 655.35) 
"wait" time until next particle emitting phase in seconds with variance(use 0 to emit the particles only if the entity is triggered, max 655.35) 
"dir" direction vector of a particle with variance in degrees from the direction vector (vector will be normalized, a variance of 180 emits in a sphere, default: 0 0 1 30)
"speed" speed of a particle with variance (max 6553.5)
"impactModel" impact model
"impactShader" impact shader (either the texture of the impact model or the shader for a sprite)
"impactLife" lifetime of an impact effect (max 65.535)
"sound" looped sound that plays when turned on or normal sound for instantly emitted particles
*/
void SP_misc_particles( gentity_t *ent ) {	
	char	*s;
	float	size, randomSize;
	float	duration, randomDuration;
	float	wait, randomWait;
	float	life, randomLife;
	float	addSize, randomAddSize;
	float	count, randomCount;
	float	impactLife;
	vec3_t	dir;
	int		randomPos, randomDir;
	float	speed, randomSpeed;
	
	// entity setup
	ent->s.eType = ET_PARTICLES;
	ent->use = G_ParticlesUse;
	// look at bg_public.h -> PF_***
	ent->s.generic1 = ent->spawnflags & 15;

	// check if the mapper gave it a shader
	G_SpawnString( "shader", "", &s );
	if ( !(*s) ) {
		G_SpawnString( "model", "", &s );
		if ( !(*s) ) {
			G_Printf( S_COLOR_YELLOW "misc_particle without shader and model at %s\n", vtos(ent->s.origin) );
			G_FreeEntity( ent );
			return;
		} else {
			ent->s.modelindex = G_ModelIndex( s );
			ent->s.generic1 |= PF_MODEL;
		}
	} else ent->s.modelindex = G_ShaderIndex( s );

	// check for impact effects
	G_SpawnFloat( "impactLife", "0", &impactLife );
	if ( ent->s.generic1 & PF_COLLISIONS ) {
		if ( impactLife > 0.0 ) {
			G_SpawnString( "impactModel", "", &s );
			if ( *s ) {
				ent->s.modelindex2 = G_ModelIndex( s );
				ent->s.generic1 |= PF_IMPACT_MODEL;
			}
			G_SpawnString( "impactShader", "", &s );
			if ( *s ) {
				if ( ent->s.generic1 & PF_IMPACT_MODEL ) {
					ent->s.otherEntityNum2 = G_ShaderIndex( s );
				} else {
					ent->s.modelindex2 = G_ShaderIndex( s );
				}
			}
		} else {
			G_SpawnString( "impactModel", "", &s );
			if ( *s ) G_Printf( S_COLOR_YELLOW "misc_particle impact without lifetime specified at %s\n", vtos(ent->s.origin) );
			else {
				G_SpawnString( "impactShader", "", &s );
				if ( *s ) G_Printf( S_COLOR_YELLOW "misc_particle impact without lifetime specified at %s\n", vtos(ent->s.origin) );
			}
		}
	}

	// check for a looped sound
	G_SpawnString( "sound", "", &s );
	if ( *s ) ent->s.otherEntityNum = G_SoundIndex( s );

	// get direction vector, speed and randomPos
	G_SpawnString( "dir", "0 0 1 30", &s );
	sscanf( s, "%f %f %f %i", &dir[0], &dir[1], &dir[2], &randomDir );
	G_SpawnFloatVariance( "speed", "0 0", &speed, &randomSpeed );
	G_SpawnInt( "randomPos", "0", &randomPos );

	// size into ent->s.origin2[0]
	G_SpawnFloatVariance( "size", "0 0", &size, &randomSize );
	if ( size <= 0 && !(ent->s.generic1 & PF_MODEL) ) {
		G_Printf( S_COLOR_YELLOW "misc_particle sprite without particle size at %s\n", vtos(ent->s.origin) );
		G_FreeEntity( ent );
		return;
	}
	ent->s.origin2[0] = ((int)(size*100) & 0xFFFF) | (((int)(randomSize*100) & 0xFFFF) << 16);

	// duration into ent->s.origin2[1]
	G_SpawnFloatVariance( "duration", "0 0", &duration, &randomDuration );
	if ( duration < 0 ) {
		// set forever flag
		ent->s.generic1 |= PF_FOREVER;
		duration = 1;
	}
	ent->s.origin2[1] = ((int)(duration*100) & 0xFFFF) | (((int)(randomDuration*100) & 0xFFFF) << 16);
	
	// wait and randomWait into ent->s.origin2[2]
	G_SpawnFloatVariance( "wait", "0 0", &wait, &randomWait );
	ent->s.origin2[2] = ((int)(wait*100) & 0xFFFF) | (((int)(randomWait*100) & 0xFFFF) << 16);
	
	// variances into ent->s.angles2[0]
	ent->s.angles2[0] = ((randomPos >> 2) & 0xFF) | ((randomDir & 0xFF) << 8) | (((int)(randomSpeed*10) & 0xFFFF) << 16);

	// lifetimes into ent->s.angles2[1]
	G_SpawnFloatVariance( "life", "0 0", &life, &randomLife );
	if ( life <= 0.0 ) {
		G_Printf( S_COLOR_YELLOW "misc_particle without lifetime specified at %s\n", vtos(ent->s.origin) );
		G_FreeEntity( ent );
		return;
	}
	ent->s.angles2[1] = ((int)(life*1000) & 0xFFFF) | (((int)(randomLife*1000) & 0xFFFF) << 16);

	// add size into ent->s.angles2[2]
	G_SpawnFloatVariance( "addSize", "0 0", &addSize, &randomAddSize );
	ent->s.angles2[2] = ((int)(addSize*100) & 0xFFFF) | (((int)(randomAddSize*100) & 0xFFFF) << 16);

	// add count into ent->s.time and ent->s.time2
	G_SpawnFloatVariance( "count", "0 0", &count, &randomCount );
	if ( count <= 0.0 ) {
		G_Printf( S_COLOR_YELLOW "misc_particle without particle count specified at %s\n", vtos(ent->s.origin) );
		G_FreeEntity( ent );
		return;
	}
	ent->s.time  = (int)(count*100) & 0xFFFF;
	ent->s.time2 = (int)(randomCount*100) & 0xFFFF;

	// add impact lifetime to ent->s.groundEntityNum
	if ( ent->s.modelindex2 ) ent->s.groundEntityNum = (int)(impactLife*1000);

	VectorNormalize( dir );
	VectorScale( dir, speed, ent->s.angles );
	SnapVector( ent->s.angles );

	// Link all ^this^ info into the ent
	trap_LinkEntity (ent);
}



/*
=================
G_LightUse
=================
*/
void G_LightUse( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	// turn on/off
	self->s.generic1 ^= LP_ACTIVE;

	// autoreturn if wanted
	if ( self->wait ) {
		self->nextthink = level.time + self->wait * 1000;
	}

	// use targets
	G_UseTargets( self, activator );
}


/*
=================
G_LightThink
=================
*/
void G_LightThink( gentity_t *ent ) {
	// turn on/off
	ent->s.generic1 ^= LP_ACTIVE;

	ent->nextthink = 0;
}


/*QUAKED misc_dynamic_light (0.2 0.2 0.2) (-8 -8 -8) (8 8 8) START_ON
Creates a dynamic light.

"color_off"	RGB value of color (def: 1.0 1.0 1.0) when off
"color_on"	RGB value of color (def: 1.0 1.0 1.0) when on
"light_off"	intensity of light (def: 0) when off
"light_on"	intensity of light (def: 300) when on
"wait"	seconds until it returns to previous state (def: 0, don't return)
*/
void SP_misc_dynamic_light( gentity_t *ent ) {
	vec3_t	color;
	int		light_off, light_on;

	ent->use = G_LightUse;
	ent->think = G_LightThink;
	ent->s.eType = ET_LIGHT;

	// store off color in origin2[0]
	G_SpawnVector( "color_off", "1 1 1", color );
	ent->s.origin2[0] = FLOAT2BYTE(color[0]) | (FLOAT2BYTE(color[1]) << 8) | (FLOAT2BYTE(color[2]) << 16);	
	// store on color in origin2[1]
	G_SpawnVector( "color_on", "1 1 1", color );
	ent->s.origin2[1] = FLOAT2BYTE(color[0]) | (FLOAT2BYTE(color[1]) << 8) | (FLOAT2BYTE(color[2]) << 16);
	// store on and off intensity in origin2[2]
	G_SpawnInt( "light_off", "0", &light_off );
	G_SpawnInt( "light_on", "300", &light_on );
	ent->s.origin2[2] = light_off | (light_on << 16);

	if ( ent->spawnflags & 1 ) ent->s.generic1 |= LP_ACTIVE;

	// Link all ^this^ info into the ent
	trap_LinkEntity (ent);
}
