// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_combat.c

#include "g_local.h"


/*
============
ScorePlum
============
*/
void ScorePlum( gentity_t *ent, vec3_t origin, int score ) {
	gentity_t *plum;

	plum = G_TempEntity( origin, EV_SCOREPLUM );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->s.number;
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, int score ) {
	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}
	// show score plum
	// ScorePlum(ent, origin, score);
	//
	if ( g_gametype.integer != GT_TACTICAL ) {
		ent->client->ps.persistant[PERS_SCORE] += score;
	} else {
		if ( ( TeamCount( -1, TEAM_BLUE ) < 1 && TeamCount( -1, TEAM_BLUE_SPECTATOR ) < 1 ) ||
			 ( TeamCount( -1, TEAM_RED ) < 1 && TeamCount( -1, TEAM_RED_SPECTATOR ) < 1 ) )
		{
			trap_SendServerCommand( -1, "print \"Can't score until both teams have players.\n\"");
		} else {
			ent->client->sess.score += score;
			if ( ent->client->sess.score > bg_ranklist[NUM_RANKS-1].next_rank ) { // can't exceed the score maximum
			ent->client->sess.score = bg_ranklist[NUM_RANKS-1].next_rank;
			}
		}
	}

	CalculateRanks();
}

/*
============
TacticalTeamWins

Adds score to the winning team and to the loosing one
============
*/
void TacticalTeamWins( int team ) {
	int	i;

	if ( level.teamscorecounted ) 
		return;
	
	// check if both teams have players
	if ( ( TeamCount( -1, TEAM_BLUE ) < 1 && TeamCount( -1, TEAM_BLUE_SPECTATOR ) < 1 ) ||
		 ( TeamCount( -1, TEAM_RED ) < 1 && TeamCount( -1, TEAM_RED_SPECTATOR ) < 1 ) ) {
		trap_SendServerCommand( -1, "print \"Can't score until both teams have players.\n\"");
		level.newround = 1;
		level.teamscorecounted = qtrue;
		LogExit( "New round." );
		return;
	}

	if ( team > 0 ) {
		// add team score
		level.teamScores[ team ] += 1;

		// do modify client score with balance
		if ( team == TEAM_BLUE ) {
			level.lossBalance += 2;
			if ( level.lossBalance < 0 ) level.lossBalance = 0;
			if ( level.lossBalance > TS_BALANCE + TS_MIN_BALANCE ) level.lossBalance = TS_BALANCE + TS_MIN_BALANCE;
		}
		if ( team == TEAM_RED ) {
			level.lossBalance -= 2;
			if ( level.lossBalance > 0) level.lossBalance = 0;
			if ( level.lossBalance < - TS_BALANCE - TS_MIN_BALANCE ) level.lossBalance = - TS_BALANCE - TS_MIN_BALANCE;
		}
	}

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		// add rank score modifiers
		level.clients[i].sess.score += bg_ranklist[ BG_GetTacticalRank(level.clients[i].sess.oldScore) ].score_mod;

		// add team scores (if it isn't a draw)
		if ( team > 0 ) {
			if ( level.clients[i].sess.sessionTeam == TEAM_RED || level.clients[i].sess.sessionTeam == TEAM_RED_SPECTATOR ) {
				if ( team == TEAM_RED ) level.clients[i].sess.score += TS_WIN;
				if ( team == TEAM_BLUE) level.clients[i].sess.score += TS_LOSS + (level.lossBalance > TS_MIN_BALANCE ? level.lossBalance - TS_MIN_BALANCE : 0);
			}
			if ( level.clients[i].sess.sessionTeam == TEAM_BLUE || level.clients[i].sess.sessionTeam == TEAM_BLUE_SPECTATOR ) {
				if ( team == TEAM_BLUE ) level.clients[i].sess.score += TS_WIN;
				if ( team == TEAM_RED ) level.clients[i].sess.score += TS_LOSS + (level.lossBalance < TS_MIN_BALANCE ? -level.lossBalance - TS_MIN_BALANCE : 0);
			}
		}

		// range tests
		if ( level.clients[i].sess.score > bg_ranklist[NUM_RANKS-1].next_rank ) { // can't exceed the score maximum
			level.clients[i].sess.score = bg_ranklist[NUM_RANKS-1].next_rank;
		}
		if ( level.clients[i].sess.score < 0 ) {
			level.clients[i].sess.score = 0;
		}

		// calculate delta rank (for scoreboard)
		level.clients[i].sess.deltaRank = BG_GetTacticalRank(level.clients[i].sess.score) - BG_GetTacticalRank(level.clients[i].sess.oldScore) + 3;
		if ( level.clients[i].sess.deltaRank < 1 ) level.clients[i].sess.deltaRank = 1;
		if ( level.clients[i].sess.deltaRank > 5 ) level.clients[i].sess.deltaRank = 5;

		// update scores
		level.clients[i].sess.oldScore = level.clients[i].sess.score;
		level.clients[i].ps.persistant[PERS_SCORE] = level.clients[i].sess.score;
	}

	// start new round if winlimit isn't hit yet
	if ( !g_winlimit.integer || ( level.teamScores[TEAM_RED] < g_winlimit.integer && level.teamScores[TEAM_BLUE] < g_winlimit.integer ) ) {
		level.newround = 1;
		LogExit( "New round." );
	}

	level.teamscorecounted = qtrue;
	CalculateRanks();

//	SendScoreboardMessageToAllClients();
}

/*
=================
DropFusedGrenade
=================
*/
void DropFusedGrenade( gentity_t *self ) {
	playerState_t	*ps;
	ps = &self->client->ps;

	if ( ps->wpcat == CT_EXPLOSIVE && ps->weaponstate == WEAPON_START_FIRING ) {
		vec3_t	start, dir;

		// calculate starting point and direction
		BG_EvaluateTrajectory( &self->s.pos, level.time, start );
		start[2] += ps->viewheight;
		VectorSet( dir, crandom(), crandom(), random() );

		// drop the grenade
		fire_missile( self, 0, start, dir);
	}
}


/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	if ( self->client->sess.sessionTeam >= TEAM_SPECTATOR )
		return;

	// drop fused grenades
	DropFusedGrenade( self );

	// drop the weapon if possible (checked in DropWeapon function)
	DropWeapon( self, self->client->ps.wpcat, qfalse );

	// drop the objective item
	if ( self->client->ps.powerups[PW_ITEM] ) {
		self->client->itemThrowTime = 0;
		Drop_Objective_Item( self, qfalse );
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t		dir;
	vec3_t		angles;

	if ( attacker && attacker != self ) {
		VectorSubtract (attacker->s.pos.trBase, self->s.pos.trBase, dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );

	angles[YAW] = vectoyaw ( dir );
	angles[PITCH] = 0; 
	angles[ROLL] = 0;
}

// these are just for logging, the client prints its own messages
char	*modNames[] = {
	"MOD_UNKNOWN",
	"MOD_STANDARD",
	"MOD_HANDGUN",
	"MOD_MICRO",
	"MOD_SHOTGUN",
	"MOD_SUB",
	"MOD_A_RIFLE",
	"MOD_SNIPER",
	"MOD_MINIGUN",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_DBT",
	"MOD_DBT_SPLASH",
	"MOD_SHOCKER",
	"MOD_FLAMETHROWER",
	"MOD_TRIPACT",
	"MOD_OMEGA",
	"MOD_THERMO",
	"MOD_PHOENIX",
	"MOD_FRAG_GRENADE",
	"MOD_FRAG_GRENADE_SPLASH",
	"MOD_FIRE_GRENADE",
	"MOD_FIRE_GRENADE_SPLASH",
	"MOD_BURNED",
	"MOD_EXPLODED",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TEAMCHANGE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT"
};

#ifdef MISSIONPACK
/*
==================
Kamikaze_DeathActivate
==================
*/
void Kamikaze_DeathActivate( gentity_t *ent ) {
	G_StartKamikaze(ent);
	G_FreeEntity(ent);
}

/*
==================
Kamikaze_DeathTimer
==================
*/
void Kamikaze_DeathTimer( gentity_t *self ) {
	gentity_t *ent;

	ent = G_Spawn();
	ent->classname = "kamikaze timer";
	VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->think = Kamikaze_DeathActivate;
	ent->nextthink = level.time + 5 * 1000;

	ent->activator = self;
}

#endif

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t	*ent;
	int			anim;
	int			contents;
	int			killer;
	int			i;
	char		*killerName, *obit;

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

	self->client->ps.pm_type = PM_DEAD;

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[0] ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[ meansOfDeath ];
	}

	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n", 
		killer, self->s.number, meansOfDeath, killerName, 
		self->client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST;	// send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	if ( attacker && attacker->client ) {
		attacker->client->lastkilled_client = self->s.number;

		if ( attacker == self ) {
			if ( g_gametype.integer == GT_TACTICAL ) {
				if ( meansOfDeath != MOD_TEAMCHANGE ) AddScore( attacker, TS_SELFKILL );
			} else {
				AddScore( attacker, -1 );
			}
		} else if ( OnSameTeam (self, attacker ) ) {
			AddScore( attacker, TS_TEAMKILL );
		} else {
			if ( g_gametype.integer == GT_TACTICAL ) AddScore( attacker, TS_KILL );
			else AddScore( attacker, 1 );

			attacker->client->lastKillTime = level.time;
		}
	} else {
		if ( g_gametype.integer == GT_TACTICAL ) AddScore( self, TS_SELFKILL );
		else AddScore( self, -1 );
	}

	// decrease score in GT_TACTICAL
	if ( g_gametype.integer == GT_TACTICAL && meansOfDeath != MOD_TEAMCHANGE ) {
		AddScore( self, TS_DIE );
	}

	// if client is in a nodrop area, don't drop anything (but return CTF flags!)
	contents = trap_PointContents( self->r.currentOrigin, -1 );
	if ( !( contents & CONTENTS_NODROP )) {
		TossClientItems( self );
	}

	// remove weapons
	memset( &self->client->ps.weapons, 0, sizeof( self->client->ps.weapons ) );

	Cmd_Score_f( self );		// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam < TEAM_SPECTATOR ) {
			continue;
		}
		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE | CONTENTS_BODY;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller (self, inflictor, attacker);

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	// but note: in GT_TACTICAL you'll NEVER respawn (until a new round starts)
	self->client->respawnTime = level.time + 1700;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof(self->client->ps.powerups) );

	// never gib in a nodrop
	/*if ( (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP) && g_blood.integer) || meansOfDeath == MOD_SUICIDE) {
		// gib death
		GibEntity( self, killer );*/

	// normal death
	{
		static int i;

		switch ( i ) {
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

		self->client->ps.legsAnim = 
			( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		self->client->ps.torsoAnim = 
			( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		// add event with "burninfo"
		G_AddEvent( self, EV_DEATH1 + i, killer );

		// the body can still be gibbed
		self->die = NULL;

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	trap_LinkEntity (self);
}


/*
================
RaySphereIntersections
================
*/
int RaySphereIntersections( vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2] ) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
		(point[1] - origin[1]) * (point[1] - origin[1]) +
		(point[2] - origin[2]) * (point[2] - origin[2]) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (- b + sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[0]);
		t = (- b - sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (- b ) / 2;
		VectorMA(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

#ifdef MISSIONPACK
/*
================
G_InvulnerabilityEffect
================
*/
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir ) {
	gentity_t	*impact;
	vec3_t		intersections[2], vec;
	int			n;

	if ( !targ->client ) {
		return qfalse;
	}
	VectorCopy(dir, vec);
	VectorInverse(vec);
	// sphere model radius = 42 units
	n = RaySphereIntersections( targ->client->ps.origin, 42, point, vec, intersections);
	if (n > 0) {
		impact = G_TempEntity( targ->client->ps.origin, EV_INVUL_IMPACT );
		VectorSubtract(intersections[0], targ->client->ps.origin, vec);
		vectoangles(vec, impact->s.angles);
		impact->s.angles[0] += 90;
		if (impact->s.angles[0] > 360)
			impact->s.angles[0] -= 360;
		if ( impactpoint ) {
			VectorCopy( intersections[0], impactpoint );
		}
		if ( bouncedir ) {
			VectorCopy( vec, bouncedir );
			VectorNormalize( bouncedir );
		}
		return qtrue;
	}
	else {
		return qfalse;
	}
}
#endif


/*
================
G_Burn
================
*/
#define MAX_BURN_TIME	2500
#define BURN_TIME		700

void G_Burn( gentity_t *targ, gentity_t *attacker ) {
	if ( !targ->client )
		return;
		
	if ( targ->client->burnTime < 0 ) targ->client->burnTime = 0;
	targ->client->burnTime += BURN_TIME;
	if ( targ->client->burnTime > MAX_BURN_TIME ) targ->client->burnTime = MAX_BURN_TIME;
	targ->client->burnAttacker = attacker;
	targ->client->ps.generic1 = targ->client->burnTime / 100;
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

/* ============G_LocationDamage============*/
int G_LocationDamage(vec3_t point, gentity_t* targ, gentity_t* attacker, int take) {
	vec3_t bulletPath;	
	vec3_t bulletAngle;	
	int clientHeight;	
	int clientFeetZ;
	int clientRotation;	
	int bulletHeight;
	int bulletRotation;	
	// Degrees rotation around client.
	// used to check Back of head vs. Face	
	int impactRotation;
	// First things first.  If we're not damaging them, why are we here? 
	if (!take) 		return 0;
	// Point[2] is the REAL world Z. We want Z relative to the clients feet	
	// Where the feet are at [real Z]
	clientFeetZ  = targ->r.currentOrigin[2] + targ->r.mins[2];	
	// How tall the client is [Relative Z]
	clientHeight = targ->r.maxs[2] - targ->r.mins[2];
	// Where the bullet struck [Relative Z]	
	bulletHeight = point[2] - clientFeetZ;
	// Get a vector aiming from the client to the bullet hit 
	VectorSubtract(targ->r.currentOrigin, point, bulletPath); 
	// Convert it into PITCH, ROLL, YAW	
	vectoangles(bulletPath, bulletAngle);
	clientRotation = targ->client->ps.viewangles[YAW];
	bulletRotation = bulletAngle[YAW];
	impactRotation = abs(clientRotation-bulletRotation);	
	impactRotation += 45; // just to make it easier to work with
	impactRotation = impactRotation % 360; // Keep it in the 0-359 range
	if (impactRotation < 90)		
		targ->client->lasthurt_location = LOCATION_BACK;
	else if (impactRotation < 180)
		targ->client->lasthurt_location = LOCATION_RIGHT;
	else if (impactRotation < 270)
		targ->client->lasthurt_location = LOCATION_FRONT;
	else if (impactRotation < 360)
		targ->client->lasthurt_location = LOCATION_LEFT;	
	else
		targ->client->lasthurt_location = LOCATION_NONE;
	// The upper body never changes height, just distance from the feet
		if (bulletHeight > clientHeight - 2)
			targ->client->lasthurt_location |= LOCATION_HEAD;
		else if (bulletHeight > clientHeight - 6)
			targ->client->lasthurt_location |= LOCATION_FACE;
		else if (bulletHeight > clientHeight - 10)
			targ->client->lasthurt_location |= LOCATION_SHOULDER;
		else if (bulletHeight > clientHeight - 16)
			targ->client->lasthurt_location |= LOCATION_CHEST;
		else if (bulletHeight > clientHeight - 26)
			targ->client->lasthurt_location |= LOCATION_STOMACH;
		else if (bulletHeight > clientHeight - 29)
			targ->client->lasthurt_location |= LOCATION_GROIN;
		else if (bulletHeight < 4)			
			targ->client->lasthurt_location |= LOCATION_FOOT;
		else			// The leg is the only thing that changes size when you duck,
						// so we check for every other parts RELATIVE location, and
						// whats left over must be the leg. 
			targ->client->lasthurt_location |= LOCATION_LEG; 		
		
		// Check the location ignoring the rotation info
		switch ( targ->client->lasthurt_location & 
				~(LOCATION_BACK | LOCATION_LEFT | LOCATION_RIGHT | LOCATION_FRONT) )		{
		case LOCATION_HEAD:			
			take *= 2.0;
			//trap_SendServerCommand( attacker-g_entities,va("print \"Headshot\n\""));
			break;		
		case LOCATION_FACE:
			take *= 2.0;			
			//trap_SendServerCommand( attacker-g_entities,va("print \"Faceshot\n\""));
			break;
		case LOCATION_SHOULDER:
			if (targ->client->lasthurt_location & (LOCATION_FRONT | LOCATION_BACK))
				take *= 1.3; // Throat or nape of neck			
			else				
				take *= 1.3; // Shoulders
			//trap_SendServerCommand( attacker-g_entities,va("print \"Shouldershot!\n\""));			
			break;		
		case LOCATION_CHEST:
			if (targ->client->lasthurt_location & (LOCATION_FRONT | LOCATION_BACK)) {
				take *= 1.3; // Belly or back
				//trap_SendServerCommand( attacker-g_entities,va("print \"Chestshot!\n\""));			
			}
			else {			
				take *= 0.8; // Arms
				//trap_SendServerCommand( attacker-g_entities,va("print \"Armshot!\n\""));			
			}
			break;
		case LOCATION_STOMACH:			
			take *= 1.0;			
			//trap_SendServerCommand( attacker-g_entities,va("print \"Stomachshot!\n\""));			
			break;		
		case LOCATION_GROIN:
			if (targ->client->lasthurt_location & LOCATION_FRONT)
				take *= 1.0; // Groin shot			
			//trap_SendServerCommand( attacker-g_entities,va("print \"Groinshot!\n\""));			
			break;		
		case LOCATION_LEG:			
			take *= 0.6;
			//trap_SendServerCommand( attacker-g_entities,va("print \"Legshot!\n\""));			
			break;		
		case LOCATION_FOOT:			
			take *= 0.5;			
			//trap_SendServerCommand( attacker-g_entities,va("print \"Footshot!\n\""));			
			break;		
		}	
		return take;
}


void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t	*client;
	int			take;
	int			save;
	int			knockback;
	int			max;
	int			weapon;
	vec3_t		kvel;

	if (!targ->takedamage) {
		return;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring in DeathMatch
	if ( level.intermissionQueued && g_gametype.integer == GT_DM ) {
		return;
	}
	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health -> deactivated
/*	if ( targ->s.eType == ET_MOVER ) {
		if ( targ->use && ( targ->moverState == MOVER_POS1 || targ->moverState == ROTATOR_POS1) ) {
			targ->use( targ, inflictor, attacker );
		}
		return;
	}*/

	// if we shot a breakable item subtract the damage from its health and try to break it
	if ( targ->s.eType == ET_BREAKABLE ) {
		targ->health -= damage;
 		G_Breakable( targ, NULL, NULL );
 		return;
 	}

	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping
	if ( attacker->client && attacker != targ ) {
		max = attacker->client->ps.stats[STAT_MAX_HEALTH];
		damage = damage * max / 100;
	}

	client = targ->client;

	if ( client ) {
		if ( client->noclip ) {
			return;
		}
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize(dir);
	}

	weapon = BG_FindWeaponForMod( mod );	
	if ( weapon > 0 ) {
		knockback = weLi[weapon].knockback;
	} else {
		knockback = damage / 2;
		if ( knockback > 60 ) {
			knockback = 60;
		// extra knockback for explosions
		if ( mod == MOD_EXPLODED ) knockback *= 2;
	}

	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( targ != attacker && OnSameTeam( targ, attacker ) && !g_friendlyFire.integer ) {
		knockback = 0;
	}
	if ( targ != attacker && OnSameTeam( targ, attacker ) && g_friendlyFire.integer == 1 ) {
		knockback /= 3.0;
	}

	VectorSet( kvel, 0, 0, 0 );
	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
		float		mass = 200;

		VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
		kvel[2] = kvel[2]/3 + knockback;
		VectorAdd (targ->client->ps.velocity, kvel, targ->client->ps.velocity);

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			int		t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( targ != attacker && OnSameTeam (targ, attacker)  ) {
			if ( !g_friendlyFire.integer ) {
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if ( attacker->client && targ != attacker && targ->health > 0
			&& targ->s.eType != ET_MISSILE
			&& targ->s.eType != ET_GENERAL) {
		if ( OnSameTeam( targ, attacker ) ) {
			attacker->client->ps.persistant[PERS_HITS]--;
		} else {
			attacker->client->ps.persistant[PERS_HITS]++;
		}
	}

	// hurt teammates less than enemies
	if ( targ != attacker && OnSameTeam( attacker, targ ) && g_friendlyFire.integer == 1 ) {
		damage /= 3.0;
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;
	save = 0;

	if ( g_debugDamage.integer ) {
		G_Printf( "%i: client:%i health:%i damage:%i\n", level.time, targ->s.number,
			targ->health, take );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_blood += take;
		client->damage_knockback += knockback;
		if ( dir ) {
			VectorCopy ( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy ( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	if (targ->client) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;

		// Modify the damage for location damage
		if ( point && targ && targ->health > 0 && attacker && take && 
			( mod == MOD_STANDARD || mod == MOD_HANDGUN || mod == MOD_MICRO || mod == MOD_SHOTGUN || mod == MOD_SUB || 
			  mod == MOD_A_RIFLE || mod == MOD_SNIPER || mod == MOD_MINIGUN || mod == MOD_DBT || mod == MOD_OMEGA ||
			  mod == MOD_THERMO ) ) {
				// do locational damage
				take = G_LocationDamage(point, targ, attacker, take);
				// special case
//				if ( mod == MOD_GRENADE && take < damage ) take = damage;
			}
		else {
//			trap_SendServerCommand( attacker-g_entities,va("print \"No locational damage done!\n\""));
			targ->client->lasthurt_location = LOCATION_NONE; 
		}

		// make 'em burn (burnTime / 100 is transferred to ps.generic1)
		if ( mod == MOD_FLAMETHROWER || mod == MOD_FIRE_GRENADE_SPLASH ) {
			G_Burn( targ, attacker );
		}
	}

	// do the damage
	if (take) {
		targ->health = targ->health - take;
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}
			
		if ( targ->health <= 0 ) {
			if ( client ) {
				targ->flags |= FL_NO_KNOCKBACK;
				
				// give some extra knockback (with some fixed vertical speed)
				VectorNormalize( kvel );
				if ( weapon > 0 ) {
					VectorScale( kvel, weLi[weapon].knockback * 2, kvel );
					kvel[2] = 100 + weLi[weapon].knockback;
				} else {
					VectorScale( kvel, 50, kvel );
					kvel[2] = 50;
				}
				VectorAdd (targ->client->ps.velocity, kvel, targ->client->ps.velocity);
			}

			if (targ->health < -999)
				targ->health = -999;

			targ->enemy = attacker;
			targ->die (targ, inflictor, attacker, take, mod);
			return;
		} else if ( targ->pain ) {
			targ->pain (targ, attacker, take);
		}
	}

}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (gentity_t *targ, int passent, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale (midpoint, 0.5, midpoint);

	VectorCopy (midpoint, dest);
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, passent, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection, 
	// rather than in world coordinate, and also include Z
	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, passent, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, passent, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, passent, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, passent, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;


	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
void G_RadiusDamage ( vec3_t origin, gentity_t *attacker, float damage, float radius,
					 gentity_t *ignore, int mod) {
	float		points, dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if( CanDamage (ent, ignore->s.number, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage (ent, NULL, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
		}
	}
}
