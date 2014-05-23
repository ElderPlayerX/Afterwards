// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t	*t;
	trace_t		trace;

	if ( !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), ent->target)) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity( t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->use = Use_Target_Give;
}


//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void Use_target_remove_powerups( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if( !activator->client ) {
		return;
	}

/*	if( activator->client->ps.powerups[PW_REDFLAG] ) {
		Team_ReturnFlag( TEAM_RED );
	} else if( activator->client->ps.powerups[PW_BLUEFLAG] ) {
		Team_ReturnFlag( TEAM_BLUE );
	} else if( activator->client->ps.powerups[PW_NEUTRALFLAG] ) {
		Team_ReturnFlag( TEAM_FREE );
	}*/

	memset( &activator->client->ps.powerups, 0, sizeof( activator->client->ps.powerups ) );
}

void SP_target_remove_powerups( gentity_t *ent ) {
	ent->use = Use_target_remove_powerups;
}


//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay( gentity_t *ent ) {
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent ) {
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) ) {
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	AddScore( activator, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}


//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if ( activator->client && ( ent->spawnflags & 4 ) ) {
		trap_SendServerCommand( activator-g_entities, va("cp \"%s\"", ent->message ));
		return;
	}

	if ( ent->spawnflags & 3 ) {
		if ( ent->spawnflags & 1 ) {
			G_TeamCommand( TEAM_RED, va("cp \"%s\"", ent->message) );
		}
		if ( ent->spawnflags & 2 ) {
			G_TeamCommand( TEAM_BLUE, va("cp \"%s\"", ent->message) );
		}
		return;
	}

	trap_SendServerCommand( -1, va("cp \"%s\"", ent->message ));
}

void SP_target_print( gentity_t *ent ) {
	ent->use = Use_Target_Print;
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
void Use_Target_Speaker (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if (ent->spawnflags & 3) {	// looping sound toggles
		if (ent->s.loopSound)
			ent->s.loopSound = 0;	// turn it off
		else
			ent->s.loopSound = ent->noise_index;	// start it
	}else {	// normal sound
		if ( ent->spawnflags & 8 ) {
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		} else if (ent->spawnflags & 4) {
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}
}

void SP_target_speaker( gentity_t *ent ) {
	char	buffer[MAX_QPATH];
	char	*s;

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( !G_SpawnString( "noise", "NOSOUND", &s ) ) {
		G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
	}

	// force all client reletive sounds to be "activator" speakers that
	// play on the entity that activates it
	if ( s[0] == '*' ) {
		ent->spawnflags |= 8;
	}

	if (!strstr( s, ".wav" )) {
		Com_sprintf (buffer, sizeof(buffer), "%s.wav", s );
	} else {
		Q_strncpyz( buffer, s, sizeof(buffer) );
	}
	ent->noise_index = G_SoundIndex(buffer);

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;


	// check for prestarted looping sound
	if ( ent->spawnflags & 1 ) {
		ent->s.loopSound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	if (ent->spawnflags & 4) {
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
}



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think (gentity_t *self) {
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;

	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA (self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		VectorMA (point, 0.5, self->enemy->r.maxs, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	// fire forward and see what we hit
	VectorMA (self->s.origin, 2048, self->movedir, end);

	trap_Trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE);

	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage ( &g_entities[tr.entityNum], self, self->activator, self->movedir, 
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
	}

	VectorCopy (tr.endpos, self->s.origin2);

	trap_LinkEntity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on (gentity_t *self)
{
	if (!self->activator)
		self->activator = self;
	target_laser_think (self);
}

void target_laser_off (gentity_t *self)
{
	trap_UnlinkEntity( self );
	self->nextthink = 0;
}

void target_laser_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->activator = activator;
	if ( self->nextthink > 0 )
		target_laser_off (self);
	else
		target_laser_on (self);
}

void target_laser_start (gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target) {
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent) {
			G_Printf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		self->enemy = ent;
	} else {
		G_SetMovedir (self->s.angles, self->movedir);
	}

	self->use = target_laser_use;
	self->think = target_laser_think;

	if ( !self->damage ) {
		self->damage = 1;
	}

	if (self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void SP_target_laser (gentity_t *self)
{
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


//==========================================================

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t	*dest;

	if (!activator->client)
		return;
	dest = 	G_PickTarget( self->target );
	if (!dest) {
		G_Printf ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self ) {
	if (!self->targetname)
		G_Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	self->use = target_teleporter_use;
}

//==========================================================

// condition check functions
enum {
	CCERR_NONE,
	CCERR_KLAMMER,
	CCERR_NOEND,
	CCERR_NOTFOUND
} CCerror;

#define MAX_SWITCHNAME_LENGTH	32
char switchName[MAX_SWITCHNAME_LENGTH];

qboolean CheckOR( char **s );
qboolean CheckAND( char **s );

static void SkipWhiteSpaces( char **s ) {
	while ( **s == ' ' ) { 
		(*s)++;
	}
}

static void NextChar( char **s ) {
	(*s)++;
	// skip white-spaces too
	SkipWhiteSpaces( s );
}

static char *GetSwitchName( char **s ) {
	int	pos = 0;

	while ( **s > 32 && **s != '^' && **s != '|' && **s != '&' && **s != '!' && **s != '(' && **s != ')' ) {
		switchName[ pos++ ] = **s;
		(*s)++;
	}
	switchName[ pos ] = 0;

	return switchName;
}

qboolean CheckOR( char **s ) {
	qboolean result = qfalse;
	int		goon = 0;

	SkipWhiteSpaces( s );
	do {
		if ( goon == 2 ) result ^= CheckAND( s );
		else result |= CheckAND( s );

		if ( **s == '|' ) {
			goon = 1;
			NextChar( s );
		} else if ( **s == '^' ) {
			goon = 2;
			NextChar( s );
		} else {
			goon = 0;
		}
	} while ( goon && !CCerror );

	return result;
}

qboolean CheckAND( char **s ) {
	gentity_t	*other;
	qboolean result = qtrue;
	qboolean negate = qfalse;
	qboolean goon = qfalse;

	do {
		while ( **s == '!' ) {
			negate ^= qtrue;
			NextChar( s );
		}
		if ( **s == '(' ) {
			NextChar( s );
			result &= CheckOR( s ) ^ negate;
			if ( **s != ')' ) CCerror = CCERR_KLAMMER;
			NextChar( s );
		} else {
			// state of the switch is saved in count
			other = G_Find( NULL, FOFS(name), GetSwitchName( s ) );
			if ( !other ) CCerror = CCERR_NOTFOUND;
			else result &= (qboolean)other->count ^ negate;
			SkipWhiteSpaces( s );
		}

		if ( **s == '&' ) {
			goon = qtrue;
			NextChar( s );
		} else {
			goon = qfalse;
		}
		negate = qfalse;
	} while ( goon && !CCerror );

	return result;
}

qboolean CheckCondition( gentity_t *self ) {
	qboolean result;
	char	*s;

	// true if empty
	if ( !(*self->condition) ) {
		return qtrue;
	}

	CCerror = CCERR_NONE;
	s = self->condition;
	result = CheckOR( &s );
	// check for no end error
	if ( *s && !CCerror ) CCerror = CCERR_NOEND;

	switch ( CCerror ) {
	case CCERR_NONE:
		// do nothing
		return result;
	case CCERR_KLAMMER:
		G_Printf( S_COLOR_YELLOW "')' expected in condition of 'target_relay' at %s.\n", vtos( self->s.origin ) );
		return qtrue;
	case CCERR_NOEND:
		G_Printf( S_COLOR_YELLOW "Unexpected end of condition in 'target_relay' at %s.\n", vtos( self->s.origin ) );
		return result;
	case CCERR_NOTFOUND:
		G_Printf( S_COLOR_YELLOW "Switch '%s' not found in 'target_relay' at %s.\n", switchName, vtos( self->s.origin ) );
		return qfalse;
	default:
		// shouldn't happen
		G_Printf( S_COLOR_YELLOW "Unknown CheckCondition error in 'target_relay' at %s!\n", vtos( self->s.origin ) );
		return qtrue;
	}
}


/*QUAKED target_relay (1 0 0) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY ONCE_ONLY RANDOM
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
"count"	number of times the relay must be triggered until it triggers it's targets (default is 1)
"condition" is a boolean logic expression using "target_switch" entities. The relay is only triggered if the condition is met. 
Example: "a|!b&(c|d)"
a,b,c and d are all "name" values of "target_switch" (or similar) entities
'|' => OR, '^' => exlusive-OR, '&' => AND, '!' => NOT
*/
void target_relay_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	// check condition
	if ( !CheckCondition( self ) ) {
		return;
	}

	// check count
	self->count2++;
	if ( self->count2 < self->count ) {
		return;
	}

	// check team rules
	if ( ( self->spawnflags & 1 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	// random activation
	if ( self->spawnflags & 8 ) {
		gentity_t	*ent;

		ent = G_PickTarget( self->target );
		if ( ent && ent->use ) {
			ent->use( ent, self, activator );
		}
	} else {
		G_UseTargets( self, activator );
	}

	// check ONLY_ONCE
	if ( self->spawnflags & 4 ) {
		G_FreeEntity( self );
		return;
	} else self->count2 = 0;

}

void SP_target_relay (gentity_t *self) {
	char	*s;
	G_SpawnString( "condition", "", &s );
	self->condition = G_NewString( s );

	self->use = target_relay_use;
	self->count2 = 0;
}


//==========================================================


/*QUAKED target_switch (1 0 0) (-8 -8 -8) (8 8 8) START_ON ONCE_ONLY
A switch that can save two states: OFF and ON
Those states can be used for evaluations by a target_relay.
If the switch is triggered the state changes and its targets are fired.
"name" is the name used for the conditions in a target_relay
"message" is displayed when the state changes from OFF to ON
"message2" is displayed when the state changes from ON to OFF
*/
void target_switch_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	// change the state
	self->count = !self->count;

	// display message
	if ( self->count ) {
		if ( self->message ) trap_SendServerCommand( -1, va( "cp \"%s\n\"", self->message ) );
	} else {
		if ( self->message2 ) trap_SendServerCommand( -1, va( "cp \"%s\n\"", self->message2 ) );
	}

	// use the targets
	G_UseTargets( self, activator );

	// only once?
	if ( self->spawnflags & 2 ) {
		self->use = NULL;
	}
}

void SP_target_switch (gentity_t *self) {
	self->use = target_switch_use;
	self->count = self->spawnflags & 1;
}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
}

void SP_target_kill( gentity_t *self ) {
	self->use = target_kill_use;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_position( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
}

static void target_location_linkup(gentity_t *ent)
{
	int i;
	int n;

	if (level.locationLinked) 
		return;

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	trap_SetConfigstring( CS_LOCATIONS, "unknown" );

	for (i = 0, ent = g_entities, n = 1;
			i < level.num_entities;
			i++, ent++) {
		if (ent->classname && !Q_stricmp(ent->classname, "target_location")) {
			// lets overload some variables!
			ent->health = n; // use for location marking
			trap_SetConfigstring( CS_LOCATIONS + n, ent->message );
			n++;
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}
	}

	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location( gentity_t *self ){
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;  // Let them all spawn first

	G_SetOrigin( self, self->s.origin );
}


//==========================================================


/*QUAKED target_goal (1 0 1) (-8 -8 -8) (8 8 8) RED_TEAM BLUE_TEAM
This entity is used only by some gametypes.
If it's triggered the team specified by the flags wins. 
"reward" score added to the activating player (max 5)
"message" display this win message (instead of the standard message)
*/
void target_goal_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	qboolean	notEnough = qfalse;
	team_t	winnerTeam;
	
	if ( g_gametype.integer != GT_TACTICAL ) {
		return;
	}
	if ( level.teamscorecounted ) { // trigger only once
		return;
	}
	if ( !(self->spawnflags & 3) ) {
		G_Printf( "target_goal has doesn't have any team specified" );
		return;
	}

	if ( ( TeamCount( -1, TEAM_BLUE ) < 1 && TeamCount( -1, TEAM_BLUE_SPECTATOR ) < 1 ) ||
		 ( TeamCount( -1, TEAM_RED ) < 1 && TeamCount( -1, TEAM_RED_SPECTATOR ) < 1 ) ) {
		notEnough = qtrue;
	}

	winnerTeam = 0;

	if ( self->spawnflags & 1 ) {
		G_LogPrintf("Tribes win!\n" );
		if ( self->message ) {
			trap_SendServerCommand( -1, va( "cp \"%s\n\"", self->message ) );
		} else {
			trap_SendServerCommand( -1, "cp \"The Wasteland Tribes win!\n\"");
		}
		winnerTeam = TEAM_RED;
	}
	if ( self->spawnflags & 2 ) { // && activator->client->sess.sessionTeam == TEAM_BLUE
		G_LogPrintf("Australians win!\n" );
		if ( self->message ) {
			trap_SendServerCommand( -1, va( "cp \"%s\n\"", self->message ) );
		} else {
			trap_SendServerCommand( -1, "cp \"The Australian Army wins!\n\"");
		}
		winnerTeam = TEAM_BLUE;
	}

	if ( notEnough ) trap_SendServerCommand( -1, "print \"Can't score until both teams have players.\n\"");
	else { 
		// add reward for activator
		if ( activator ) {
			if ( activator->client->sess.sessionTeam == winnerTeam ) AddScore( activator, self->count2 );
			else AddScore( activator, self->count2 ); // punish
		}

		// add scores for teammates
		TacticalTeamWins( winnerTeam );
		return;
	}

	level.newround = 1;
	level.teamscorecounted = qtrue;
	LogExit( "New round." );
}

void SP_target_goal (gentity_t *self) {
	if (self->count2 > 5) { // stop crazy mappers :)
		self->count2 = 5;
	}
	self->use = target_goal_use;
}