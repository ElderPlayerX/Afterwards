// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;

	s = va("%i %i %i %i %i %i %i %i %i", 
		client->sess.sessionTeam,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.score,
		client->sess.enterTime,
		client->sess.weapons[CT_SIDEARM],
		client->sess.weapons[CT_PRIMARY],
		client->sess.weapons[CT_EXPLOSIVE]
		);

	var = va( "session%i", client - level.clients );

	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	var = va( "session%i", client - level.clients );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i",
		&client->sess.sessionTeam,
		&client->sess.spectatorTime,
		&client->sess.spectatorState,
		&client->sess.spectatorClient,
		&client->sess.score,
		&client->sess.enterTime,
		&client->sess.weapons[CT_SIDEARM],
		&client->sess.weapons[CT_PRIMARY],
		&client->sess.weapons[CT_EXPLOSIVE]
		);

	client->sess.oldScore = client->sess.score;
//	G_Printf( "Score: %i\n", client->sess.wins );

	if (client->sess.sessionTeam == TEAM_RED_SPECTATOR) client->sess.sessionTeam = TEAM_RED;
	if (client->sess.sessionTeam == TEAM_BLUE_SPECTATOR) client->sess.sessionTeam = TEAM_BLUE;
	client->sess.spectatorState = SPECTATOR_FREE;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	// initial team determination
	if ( g_gametype.integer < GT_DM ) {
		if ( g_teamAutoJoin.integer ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			sess->sessionTeam = TEAM_SPECTATOR;	
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			if ( g_gametype.integer == GT_DM ) {
				if ( g_maxGameClients.integer > 0 && 
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;

	sess->enterTime = level.time;

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;
	int			newround;

/*	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );*/

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	sscanf( s, "%i %i %i %i %i %i",
		&gt,
		&level.teamScores[TEAM_RED], 
		&level.teamScores[TEAM_BLUE],
		&level.lossBalance,
		&level.gameStartTime,
		&newround
		);

	// reset session data if no new round started (mapchange, restart)
	// or if the gametype changed since the last session
	if ( newround != 2 || g_gametype.integer != gt ) {
		level.newSession = qtrue;
	}

	// reset some things if a new Session has been started
	if ( level.newSession) {
		level.gameStartTime = level.time;
		level.teamScores[TEAM_RED] = 0;
		level.teamScores[TEAM_BLUE] = 0;
		level.lossBalance = 0;
	}

	level.newround = 0;
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i %i %i %i %i %i", 
		g_gametype.integer,
		level.teamScores[TEAM_RED], 
		level.teamScores[TEAM_BLUE],
		level.lossBalance,
		level.gameStartTime,
		level.newround
		));

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			if ( ( level.clients[i].sess.sessionTeam == TEAM_RED || level.clients[i].sess.sessionTeam == TEAM_BLUE ) &&
				 ( level.clients[i].ps.pm_type != PM_DEAD && g_gametype.integer == GT_TACTICAL) ) {
				memcpy( &level.clients[i].sess.weapons, &level.clients[i].ps.weapons, sizeof( level.clients[i].ps.weapons ) );
			} else {
				memset( &level.clients[i].sess.weapons, 0, sizeof(level.clients[i].sess.weapons) );
			}
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
