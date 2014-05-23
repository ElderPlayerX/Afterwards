// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

// for game/cgame version compare
#define	VERSION_SHORT		"aw-b2.0"

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	"Afterwards Beta 2.0"

#define MIN_HUNKMEGS		72
#define MIN_ZONEMEGS		16
#define MIN_SOUNDMEGS		8

#define	DEFAULT_GRAVITY		800

#define	ARMOR_PROTECTION	0.66

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection
#define ITEM_TAKETIME		500		// time until a thrown item can be taken again

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define TACTICAL_FREEZE_TIME 4000	// the players are freezed for 4 seconds in GT_TACTICAL

#define	MISSILE_PRESTEP_TIME	10	// shared because of missile prediction

#define	MINS_Z				-24
#define	DEFAULT_VIEWHEIGHT	26
#define CROUCH_VIEWHEIGHT	12
#define	DEAD_VIEWHEIGHT		-16

#define BOUNCE_FACTOR_LESS	0.8f
#define BOUNCE_FACTOR_HALF	0.6f

#define	MAX_OMEGA_HITS		4

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11

#define CS_TEAMVOTE_TIME		12
#define CS_TEAMVOTE_STRING		14
#define	CS_TEAMVOTE_YES			16
#define	CS_TEAMVOTE_NO			18

#define	CS_GAME_VERSION			20
#define	CS_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CS_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23		// string indicating flag status in CTF
#define CS_SHADERSTATE			24
#define CS_BOTINFO				25

#define	CS_ITEMS				27		// string of 0's and 1's that tell which items are present

#define CS_ATMOSEFFECT			28		// Atmospheric effect, if any.

#define	CS_MODELS				32
#define	CS_SOUNDS				(CS_MODELS+MAX_MODELS)
#define CS_SHADERS				(CS_SOUNDS+MAX_SOUNDS)
#define	CS_PLAYERS				(CS_SHADERS+MAX_SHADERS)
#define CS_LOCATIONS			(CS_PLAYERS+MAX_CLIENTS)

#define CS_MAX					(CS_LOCATIONS+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum {
	GT_TACTICAL,			// afterwards gametype

	// all gametypes before DM are team games
	GT_DM,				// deathmatch

	GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOMOVE,		// no control
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY, 
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_RELOADING,
	WEAPON_FIRING,
	WEAPON_START_FIRING,
	WEAPON_CONTINUE_FIRING
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			2
#define	PMF_JUMP_HELD		4
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	128		// pm_time is waterjump
#define	PMF_RESPAWNED		256		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	512
#define PMF_GRAPPLE_PULL	1024	// pull towards grapple location
#define PMF_FOLLOW			2048	// spectate following another player
#define PMF_SCOREBOARD		4096	// spectate as a scoreboard
#define PMF_INVULEXPAND		8192	// invulnerability sphere set to full size
#define PMF_ZOOM			16384	// zoom button held

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	canFire;			// for shocker and blast

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	vec3_t		mins, maxs;			// bounding box size
	
	int			watertype;
	int			waterlevel;

	float		xyspeed;

	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	STAT_WEAPON_TIME,
	STAT_DEAD_YAW,					// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH					// health / armor limit, changable by handicap
} statIndex_t;


// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,						// player rank or team rank
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
	PERS_ATTACKER,					// clientnum of last damage inflicter
	PERS_ATTACKEE_ARMOR,			// health/armor of last person we attacked
	PERS_KILLED,					// count of the number of times you died
} persEnum_t;


// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define EF_TRAILONLY		0x00000002		// for missiles, only draw the trail
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_LESS		0x00000020		// for missiles
#define	EF_BOUNCE_HALF		0x00000040		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000080		// draw a gauntlet sprite
#define	EF_NODRAW			0x00000100		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000200		// for lightning gun
#define	EF_KAMIKAZE			0x00000400
#define	EF_MOVER_STOP		0x00000800		// will push otherwise
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define EF_TEAMVOTED		0x00008000		// already cast a team vote
#define EF_OBJECTIVE		0x00010000		// is an objective

// ps->powerups[PW_ZONES]
#define ZF_BUYZONE			0x00000001		// set if player is in a buyzone
#define ZF_BOMBZONE			0x00000002		// set if player is in a bombzone
#define ZF_ITEMZONE			0x00000004		// set if player is in an itemzone

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,

	PW_ITEM,
	PW_ZONES,

	PW_FLIGHT,
	PW_REDFLAG,
	PW_BLUEFLAG,

	PW_NUM_POWERUPS

} powerup_t;

typedef enum {
	HI_NONE,

	HI_TELEPORTER,
	HI_MEDKIT,
	HI_KAMIKAZE,
	HI_PORTAL,
	HI_INVULNERABILITY,

	HI_NUM_HOLDABLE
} holdable_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002
#define PLAYEREVENT_HOLYSHIT			0x0004

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,
	EV_FALL_VERY_FAR,

	EV_JUMP_PAD,			// boing sound at origin, jump sound on player

	EV_JUMP,
	EV_WATER_TOUCH,	// foot touches
	EV_WATER_LEAVE,	// foot leaves
	EV_WATER_UNDER,	// head touches
	EV_WATER_CLEAR,	// head leaves

	EV_ITEM_OBJECTIVE,
	EV_ITEM_PICKUP,			// normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP,	// powerup / team sounds are broadcast to everyone

	EV_NOAMMO,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,
	EV_FIRE_WEAPON_AUTO,
	EV_EFFECT_WEAPON,
	EV_RELOAD_WEAPON,
	EV_SERVER_CHANGE_WEAPON,

	EV_BOMB_PLANTED,
	EV_PARTICLES,

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_GLOBAL_TEAM_SOUND,

	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_OMEGATRAIL,
	EV_SHOTGUN,
	EV_BULLET,				// otherEntity is the shooter

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,

	EV_BREAKABLE,			// breakable is breaking :)
	EV_SCOREPLUM,			// score plum

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,
	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL

} entity_event_t;


typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE
} global_team_sound_t;

// animations
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	// animations that are afterwards specific, they are not required (right now...)
	TORSO_ATTACK_LW,
	TORSO_DROP_LW,
	TORSO_RAISE_LW,
	TORSO_STAND_LW,

	TORSO_ATTACK_PIST,
	TORSO_DROP_PIST,
	TORSO_RAISE_PIST,
	TORSO_STAND_PIST,

	TORSO_ATTACK_UP,
	TORSO_DROP_UP,
	TORSO_RAISE_UP,
	TORSO_STAND_UP,

	TORSO_ATTACK_ROCK,
	TORSO_DROP_ROCK,
	TORSO_RAISE_ROCK,
	TORSO_STAND_ROCK,

	TORSO_ATTACK_GREN,
	TORSO_DROP_GREN,
	TORSO_RAISE_GREN,
	TORSO_STAND_GREN,

	TORSO_RELOAD,
	TORSO_RELOAD_LW,
	TORSO_RELOAD_PIST,
	TORSO_RELOAD_UP,
	TORSO_RELOAD_ROCK,
	TORSO_RELOAD_GREN,

	MAX_ANIMATIONS,

	LEGS_BACKCR,
	LEGS_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,

	MAX_TOTALANIMATIONS
} animNumber_t;


typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
	int		reversed;			// true if animation is reversed
	int		flipflop;			// true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		128


typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,
	TEAM_RED_SPECTATOR,
	TEAM_BLUE_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

// ********* Trefferzonen (aus Tutorial 14) ****************************** 

#define LOCATION_NONE		0x00000000// Height layers
#define LOCATION_HEAD		0x00000001 // [F,B,L,R] Top of head
#define LOCATION_FACE		0x00000002 // [F] Face [B,L,R] Head
#define LOCATION_SHOULDER	0x00000004 // [L,R] Shoulder [F] Throat, [B] Neck
#define LOCATION_CHEST		0x00000008 // [F] Chest [B] Back [L,R] Arm
#define LOCATION_STOMACH	0x00000010 // [L,R] Sides [F] Stomach [B] Lower Back
#define LOCATION_GROIN		0x00000020 // [F] Groin [B] Butt [L,R] Hip
#define LOCATION_LEG		0x00000040 // [F,B,L,R] Legs
#define LOCATION_FOOT		0x00000080 // [F,B,L,R] Bottom of Feet
// Relative direction strike came from
#define LOCATION_LEFT		0x00000100
#define LOCATION_RIGHT		0x00000200
#define LOCATION_FRONT		0x00000400
#define LOCATION_BACK		0x00000800

// ********* Trefferzonen (aus Tutorial 14) ****************************** 

//team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE, 
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
	MOD_UNKNOWN,
	MOD_STANDARD,
	MOD_HANDGUN,
	MOD_MICRO,
	MOD_SHOTGUN,
	MOD_SUB,
	MOD_A_RIFLE,
	MOD_SNIPER,
	MOD_MINIGUN,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_GRENADE,
	MOD_GRENADE_SPLASH,
	MOD_DBT,
	MOD_DBT_SPLASH,
	MOD_SHOCKER,
	MOD_FLAMETHROWER,
	MOD_TRIPACT,
	MOD_OMEGA,
	MOD_THERMO,
	MOD_PHOENIX,
	MOD_FRAG_GRENADE,
	MOD_FRAG_GRENADE_SPLASH,
	MOD_FIRE_GRENADE,
	MOD_FIRE_GRENADE_SPLASH,
	MOD_BURNED,
	MOD_EXPLODED,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TEAMCHANGE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_HOLDABLE,			// single use, holdable item
							// EFX: rotate + bob
	IT_DROPPED,
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s {
	char		*classname;	// spawning name
	char		*pickup_sound;
	char		*world_model[MAX_ITEM_MODELS];

	char		*icon;
	char		*pickup_name;	// for printing on pickup

	int			quantity;		// for ammo how much, or duration of powerup
	int			quantity2;		// for clip ammo

	itemType_t  giType;			// IT_* flags

	int			giTag;

	char		*precaches;		// string of all models and images this item will use
	char		*sounds;		// string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern	gitem_t	bg_itemlist[];
extern	int		bg_numItems;

//---------------------------------------------------------

// Herby weapons definitions
typedef enum { 
	WP_NONE,

	WP_SHOCKER,

	WP_STANDARD,
	WP_HANDGUN,
	WP_MICRO,

	WP_SHOTGUN,
	WP_SUB,
	WP_A_RIFLE,
	WP_SNIPER,
	WP_PHOENIX,
	WP_TRIPACT,
	WP_GRENADE_LAUNCHER,
	WP_MINIGUN,
	WP_FLAMETHROWER,
	WP_DBT,
	WP_RPG,
	WP_OMEGA,
	WP_THERMO,

	WP_FRAG_GRENADE,
	WP_FIRE_GRENADE,

	WP_BLAST,

	WP_NUM_WEAPONS
} weapon_t;

typedef enum { 
	CT_MELEE,
	CT_SIDEARM,
	CT_PRIMARY,
	CT_EXPLOSIVE,
	CT_SPECIAL,

	CT_NUM_CATEGORIES
} weapon_categories_t;

typedef enum { 
	WT_SHOCKER,
	WT_STANDARD,
	WT_LONG,
	WT_UP,
	WT_PISTOL,
	WT_ROCK,
	WT_GREN,
	WT_NUM_WEAPONTYPES
} weapon_type_t;

typedef enum { 
	PR_NONE,
	PR_BULLET,
	PR_PELLETS,
	PR_MISSILE,
	PR_FLAME,
	PR_OMEGA,
	PR_BEAM,
	PR_MELEE
} weapon_prediction_t;


// look at bg_misc.c for the filled in values !!
typedef struct gweapon_s {
	char		*name;			// name

	int			category;
	int			weapon_type;	// weapon type (->weapon_type_t)
	int			mod;			// the corresponding Mode of Death
	int			damage;			// standard damage per bullet
	int			knockback;		// knockback per hit
	int			nextshot;		// time in msec to next shot
	int			ammo_max;		// max ammo in bag
	int			clip_max;		// max ammo in clip
	int			spread_base;	// base spread standing
	float		spread_crouch;	// factor for spread ducked 
	float		spread_move;	// factor for spread moving
	int			spread_recoil;	// spread increase due to recoil
	int			recoil_shot;	// amount of recoil per shot (in time)
	int			recoil_time;	// time to recover from recoil
	int			recoil_amount;	// amount of recoil (upwards)
	int			recoil_side;	// amount of recoil (sidewards)
	float		recoil_crouch;	// factor for recoil ducked
	float		recoil_move;	// factor for recoil moving
	int			reload_time;	// time it takes to reload the weapon
	int			tr_type;		// type of missile movement
	int			tr_delta;		// speed of tr
	int			min_rank;		// rank required to buy this weapon
	int			prediction;		// the way the weapon is client side predicted
} gweapon_t;


typedef struct gweAnim_s {
	int		stand;
	int		attack;
	int		drop;
	int		raise;
	int		reload;
} gweAnim_t;


// included in both the game dll and the client
extern	gweapon_t	weLi[WP_NUM_WEAPONS];

extern	gweAnim_t weAnims[WT_NUM_WEAPONTYPES];

//============================================================================

#define TS_KILL				2
#define TS_DIE				-1
#define TS_WIN				2
#define TS_LOSS				-1
#define TS_BALANCE			5
#define TS_MIN_BALANCE		2
#define TS_TEAMKILL			-4
#define TS_SELFKILL			-1

#define NUM_RANKS			7

typedef struct grank_s {
	char		*rankname;		// name
	int			next_rank;		// score for next higher rank
	int			score_mod;		// modifier for score
} grank_t;

// filled in values in bg_misc.c
extern	grank_t	bg_ranklist[NUM_RANKS]; 

//============================================================================

#define MAX_TACTICAL_MODELS	8

typedef struct modelslist_s {
	char			*name;
	char			*model;
	char			*modelSkin;
	char			*head;
	char			*headSkin;
	team_t	team;
} modelslist_t;

// filled in values in bg_misc.c
extern	modelslist_t	bg_modelslist[MAX_TACTICAL_MODELS]; 


//---------------------------------------------------------

int	BG_GetTacticalRank( int score );
gitem_t	*BG_FindItem( const char *name );
gitem_t	*BG_FindItemForWeapon( weapon_t weapon );
gitem_t	*BG_FindItemForPowerup( powerup_t pw );
gitem_t	*BG_FindItemForHoldable( holdable_t pw );
int	BG_FindWeaponForMod( int mod );
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps, int atTime );

// g_dmflags->integer flags
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)


//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_FLAMES,
	ET_MOVER,
	ET_SCRIPTED_MODEL,
	ET_BREAKABLE,
	ET_EFFECT,
	ET_PARTICLES,
	ET_BLAST,
	ET_BOMBZONE,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_LIGHT,
	ET_INVISIBLE,

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;


//
// breakable effect
//
typedef enum {
	BE_NONE,
	BE_GLASS,
	BE_WOOD,
	BE_METAL,
	BE_EXPLOSIVE
} breakableEffect_t;


//
// effects (of misc_effect entity max. 64)
//
typedef enum {
	FX_NONE,
	FX_FIRE,
	FX_LIGHTNING
} miscEffect_t;


//
// particle flags
//
#define PF_OFF			1
#define PF_GRAVITY		2
#define PF_AXIS_ALIGNED	4
#define PF_COLLISIONS	8
#define PF_FOREVER		16
#define PF_MODEL		32
#define PF_IMPACT_MODEL	64


//
// dynamic light flags
//
#define LP_ACTIVE		1


//
// scripted model flags
//
#define SMF_INVISIBLE	1


void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void	BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void	BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
void	BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );

qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );


#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		8192


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME		0
#define KAMI_SHOCKWAVEFADE_STARTTIME	1500
#define KAMI_SHOCKWAVE_ENDTIME			2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME			250
#define KAMI_IMPLODE_STARTTIME			2000
#define KAMI_IMPLODE_ENDTIME			2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME		2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME	2500
#define KAMI_SHOCKWAVE2_ENDTIME			3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS		88
#define KAMI_BOOMSPHEREMODEL_RADIUS		72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS		1320
#define KAMI_BOOMSPHERE_MAXRADIUS		720
#define KAMI_SHOCKWAVE2_MAXRADIUS		704

