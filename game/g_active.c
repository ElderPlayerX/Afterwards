// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"


/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->health );
		client->ps.damageEvent++;
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	int			waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("*drown.wav"));
				} else if (rand()&1) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav"));
				} else {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp2.wav"));
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && 
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= level.time	) {

			if (ent->watertype & CONTENTS_LAVA) {
				G_Damage (ent, NULL, NULL, NULL, NULL, 
					30*waterlevel, 0, MOD_LAVA);
			}

			if (ent->watertype & CONTENTS_SLIME) {
				G_Damage (ent, NULL, NULL, NULL, NULL, 
					10*waterlevel, 0, MOD_SLIME);
			}
		}
	}

	// burn
	if ( ent->client->burnTime > 0 ) {
		gclient_t	*client;
		int			burnDamage;

		client = ent->client;

		burnDamage = client->burnTime / weLi[WP_FLAMETHROWER].recoil_shot - (client->burnTime - level.time + level.previousTime) / weLi[WP_FLAMETHROWER].recoil_shot;
		client->burnTime -= level.time - level.previousTime;
		client->ps.generic1 = client->burnTime / 100;

		ent->health -= burnDamage;
		if ( ent->health <= 0 ) {
			ent->enemy = client->burnAttacker;
			ent->die (ent, client->burnAttacker, client->burnAttacker, burnDamage, MOD_BURNED);
		}
	}
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
#ifdef MISSIONPACK
	if( ent->s.eFlags & EF_TICKING ) {
		ent->client->ps.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav");
	}
	else
#endif
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam >= TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_SpectatorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = g_speed.integer * 2.0;	// faster than normal

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	} else {
		// store other clients position and view angle
		VectorCopy( g_entities[ client->sess.spectatorClient ].r.currentOrigin, ent->r.currentOrigin );
		VectorCopy( g_entities[ client->sess.spectatorClient ].client->ps.viewangles, ent->r.currentAngles );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	}

	// use button stops following
	if ( ( ent->client->buttons & BUTTON_ZOOM ) && ! ( client->oldbuttons & BUTTON_ZOOM ) &&
		 ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) ) {
		ClientBegin( ent->client - level.clients );
		ent->client->sess.spectatorState = SPECTATOR_FREE;
	}
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove || 
		client->pers.cmd.rightmove || 
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & BUTTON_ATTACK) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;

		// count down health when over max
		if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health--;
		}
	}
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( client->buttons & ( BUTTON_ATTACK ) & ( client->oldbuttons ^ client->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i, j;
	int		event;
	gclient_t *client;
	int		damage;
	vec3_t	dir;
	vec3_t	origin, angles;
//	qboolean	fired;
	gitem_t *item;
	gentity_t *drop;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];
		ent->s.eventParm = client->ps.eventParms[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
		case EV_FALL_VERY_FAR:
			if ( ent->s.eType != ET_PLAYER ) {
				break;		// not in the player model
			}
			if ( g_dmflags.integer & DF_NO_FALLING ) {
				break;
			}
			if ( event == EV_FALL_VERY_FAR ) {
				damage = 60;
			} else if ( event == EV_FALL_FAR ) {
				damage = 20;
			} else {
				damage = 5;
			}
			VectorSet (dir, 0, 0, 1);
			ent->pain_debounce_time = level.time + 400;	// no normal pain sound
			G_Damage (ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			break;

		case EV_FIRE_WEAPON:
		case EV_FIRE_WEAPON_AUTO:
			FireWeapon( ent, ent->s.eventParm );
			break;

		case EV_RELOAD_WEAPON:
			// server does nothing here
			break;

		case EV_USE_ITEM1:		// teleporter
			// drop flags in CTF
			item = NULL;
			j = 0;

/*			if ( ent->client->ps.powerups[ PW_REDFLAG ] ) {
				item = BG_FindItemForPowerup( PW_REDFLAG );
				j = PW_REDFLAG;
			} else if ( ent->client->ps.powerups[ PW_BLUEFLAG ] ) {
				item = BG_FindItemForPowerup( PW_BLUEFLAG );
				j = PW_BLUEFLAG;
			} else if ( ent->client->ps.powerups[ PW_NEUTRALFLAG ] ) {
				item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
				j = PW_NEUTRALFLAG;
			}*/

			if ( item ) {
				drop = Drop_Item( ent, item, qtrue );
				// decide how many seconds it has left
				drop->count = ( ent->client->ps.powerups[ j ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}

				ent->client->ps.powerups[ j ] = 0;
			}

			SelectSpawnPoint( ent->client->ps.origin, origin, angles );
			TeleportPlayer( ent, origin, angles );
			break;

		case EV_USE_ITEM2:		// medkit
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] + 25;

			break;

#ifdef MISSIONPACK
		case EV_USE_ITEM3:		// kamikaze
			// make sure the invulnerability is off
			ent->client->invulnerabilityTime = 0;
			// start the kamikze
			G_StartKamikaze( ent );
			break;

		case EV_USE_ITEM4:		// portal
			if( ent->client->portalID ) {
				DropPortalSource( ent );
			}
			else {
				DropPortalDestination( ent );
			}
			break;
		case EV_USE_ITEM5:		// invulnerability
			ent->client->invulnerabilityTime = level.time + 10000;
			break;
#endif

		default:
			break;
		}
	}

}


void BotTestSolid(vec3_t origin);


/*
=============
DropWeapon
=============
*/
void DropWeapon( gentity_t *ent, int catNum, qboolean thrown ) {

	gclient_t	*client;
	usercmd_t	*ucmd;
	gitem_t		*item;
	gentity_t	*drop;
//	byte	i;
	int		ammo;
	int		clip;
	int		weapon;

	client = ent->client;
	ucmd = &ent->client->pers.cmd;
	weapon = client->ps.weapons[catNum];

	if ( catNum < 0 || catNum > CT_NUM_CATEGORIES ) {
		return;
	}

	if ( (thrown && (ucmd->buttons & BUTTON_ATTACK) ) || (weapon == WP_NONE) || 
		(weapon == WP_SHOCKER) || (weapon == WP_BLAST) ) {
		return;
	}

	item = BG_FindItemForWeapon( weapon );

	clip = client->ps.clipammo[ catNum ]; // save amount of ammo
	ammo = client->ps.bagammo[ catNum ]; // save amount of ammo
	client->ps.weapons[ catNum ] = WP_NONE;
	client->ps.clipammo[ catNum ] = -1;
	client->ps.bagammo[ catNum ] = -1;

	drop = Drop_Item( ent, item, thrown );

	// set the dropped weapons ammo stats
	drop->r.ownerNum = ent->s.number;
	drop->count = ammo;
	drop->count2 = clip;
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	} 

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	if ( pmove_fixed.integer || client->pers.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( client->sess.sessionTeam >= TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		if ( g_gametype.integer != GT_TACTICAL || level.time - level.startTime > 4000 ) client->ps.pm_type = PM_NORMAL;
		else client->ps.pm_type = PM_NOMOVE;
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	// check for the hit-scan melee, don't let the action
	// go through as an attack unless it actually hits something
	if ( client->ps.wpcat == CT_MELEE && !( ucmd->buttons & BUTTON_TALK ) &&
		( ucmd->buttons & BUTTON_ATTACK ) && client->ps.weaponTime <= 0 ) {
		pm.canFire = G_MeleeFireCheck( ent );
	}

	// check for bomb planting
	if ( client->ps.weapons[client->ps.wpcat] == WP_BLAST && client->bombzone_time > 0 ) {
		pm.canFire = qtrue;
	}

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

	VectorCopy( client->ps.origin, client->oldOrigin );

	Pmove (&pm);

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
	}
	else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	}
	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		client->fireHeld = qfalse;		// for grapple
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

    // store the client's new position
    G_StoreTrail( ent );	

	// test for solid areas in the AAS file
	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		// wait for the attack button to be pressed
		if ( g_gametype.integer != GT_TACTICAL && level.time > client->respawnTime ) {
			// forcerespawn is to prevent users from waiting out powerups
			if ( g_forcerespawn.integer > 0 && 
				( level.time - client->respawnTime ) > g_forcerespawn.integer * 1000 ) {
				respawn( ent );
				return;
			}
		
			// pressing attack or use is the normal respawn method
			if ( ucmd->buttons & ( BUTTON_ATTACK ) ) {
				respawn( ent );
			}
		}
		if ( g_gametype.integer == GT_TACTICAL && level.time > client->respawnTime ) {
//			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) {
				if (client->sess.sessionTeam == TEAM_RED) SetTeam(ent,"rs");
				if (client->sess.sessionTeam == TEAM_BLUE) SetTeam(ent,"bs");
//			}			
		}
		return;
	}

	// item drop
	if ( ucmd->buttons & BUTTON_DROPITEM ) {
		Drop_Objective_Item( ent, qtrue );
	}

	// weapon drop
	if ( (ucmd->buttons & BUTTON_DROP) && ent->client->ps.weaponTime <= 0 && !client->droping ) {
		DropWeapon( ent, client->ps.wpcat, qtrue );
		client->droping = qtrue;
	}
	if ( !( ucmd->buttons & BUTTON_DROP ) ) client->droping = qfalse;

	// buyzone checking timer
	if ( client->buyzone_time > 0 ) {
		client->ps.powerups[PW_ZONES] |= ZF_BUYZONE;
		client->buyzone_time -= msec;
	} else {
		client->ps.powerups[PW_ZONES] &= ~ZF_BUYZONE;
	}

	// check for bomb planting
	if ( client->bomb_plant_time > 0 && level.time >= client->bomb_plant_time ) {
		client->bomb_plant_time = 0;

		// check if we're still in the bombzone
		if ( client->bombzone_time > 0  ) {
			gentity_t	*m;

			m = fire_blast (ent, client->oldOrigin);
			m->activator = ent;
			m->enemy = &g_entities[client->bombzone_target];

			if ( client->ps.weapons[CT_PRIMARY] ) client->ps.wpcat = CT_PRIMARY;
			else if ( client->ps.weapons[CT_SIDEARM] ) client->ps.wpcat = CT_SIDEARM;
			else client->ps.wpcat = CT_MELEE;

			client->ps.weaponstate = WEAPON_RAISING;
			client->ps.weaponTime = 250;

			trap_SendServerCommand( -1, "cp \"A blastpack has been planted!\n\"");
			G_AddEvent( ent, EV_BOMB_PLANTED, client->ps.wpcat );

		} else trap_SendServerCommand( ent-g_entities, "cp \"Plant the blastpack near a bomb site!\n\"");
	}
	
	// bombzone checking timer
	if ( client->bombzone_time > 0 ) {
		client->ps.powerups[PW_ZONES] |= ZF_BOMBZONE;
		client->bombzone_time -= msec;
	} else {
		client->ps.powerups[PW_ZONES] &= ~ZF_BOMBZONE;
	}

	// itemzone checking timer
	if ( client->itemzone_time > 0 ) {
		client->ps.powerups[PW_ZONES] |= ZF_ITEMZONE;
		client->itemzone_time -= msec;
	} else {
		client->ps.powerups[PW_ZONES] &= ~ZF_ITEMZONE;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t			*ent;
	usercmd_t			ucmd;
	clientPersistant_t	*cp;
	int					newtime;

	ent = g_entities + clientNum;
	cp = &ent->client->pers;
	trap_GetUsercmd( clientNum, &ucmd );

	// calculate actual servertime
	newtime = level.time + trap_Milliseconds() - level.frameStartTime;

	// range check g_simLag
	if ( g_simLag.integer > MAX_SIMLAG ) trap_Cvar_Set( "g_simLag", va( "%i", MAX_SIMLAG ) ); 
	if ( g_simLag.integer < 0 ) trap_Cvar_Set( "g_simLag", "0" ); 

	if ( !g_simLag.integer || ent->r.svFlags & SVF_BOT ) {
		// always use current usercmd if we don't simulate lag
		cp->cmd = ucmd;
	} else {
		int	curCmd;

		// alter usercmd time, because of simulated server>>client lag
		ucmd.serverTime -= g_simLag.integer / 2;

		// store usercmds to simulate lag
		cp->cmdHead++;
		if ( cp->cmdHead >= MAX_OLDCMDS ) cp->cmdHead = 0;
		cp->oldCmd[cp->cmdHead] = ucmd;

		// find most appropriate usercmd for specified amount of lag
		curCmd = cp->cmdHead - cp->cmdBack;
		if ( curCmd < 0 ) curCmd += MAX_OLDCMDS;

		// it is important that every oldCmd should be used to avoid glitches, so there's a tolerance
		// if the oldCmd is too old, go less backwards in time
		while ( cp->oldCmd[curCmd].serverTime < ucmd.serverTime - g_simLag.integer / 2 - 30 && cp->cmdBack > 0 ) {
			cp->cmdBack--;
			curCmd++;
			if ( curCmd >= MAX_OLDCMDS ) curCmd -= MAX_OLDCMDS;
		}
		// if the oldCmd is too new, go more backwards in time
		while ( cp->oldCmd[curCmd].serverTime > ucmd.serverTime - g_simLag.integer / 2 + 30 && cp->cmdBack < MAX_OLDCMDS - 1 ) {
			cp->cmdBack++;
			curCmd--;
			if ( curCmd < 0 ) curCmd += MAX_OLDCMDS;
		}

		// use that usercmd
		cp->cmd = cp->oldCmd[curCmd];
	}

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = newtime;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
}


void G_RunClient( gentity_t *ent ) {
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );

}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum, flags;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam < TEAM_SPECTATOR ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					vec3_t saveOrigin;
					vec3_t saveAngles;

					// store origin and angles to set the spectator to the momentary place
					VectorCopy( ent->r.currentOrigin, saveOrigin );
					VectorCopy( ent->r.currentAngles, saveAngles );

					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin( ent->client - level.clients );

					VectorCopy( saveOrigin, ent->r.currentOrigin );
					VectorCopy( saveAngles, ent->r.currentAngles );
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	clientPersistant_t	*pers;

	if ( ent->client->sess.sessionTeam >= TEAM_SPECTATOR ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	pers = &ent->client->pers;

	// turn off any expired powerups
	// for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
	// 	if ( ent->client->ps.powerups[ i ] < level.time ) {
	//		ent->client->ps.powerups[ i ] = 0;
	//	}
	// }

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

	// set the latest infor
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
	}
	else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	}

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}


