// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_misc.c -- both games misc functions, all completely stateless

#include "q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/


// definition of gweapon_t is in bg_public.h

gweapon_t	weLi[WP_NUM_WEAPONS] = 
{
	{
		NULL,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0
	},	// leave index 0 alone

	{
		"shocker",
		CT_MELEE,
		WT_SHOCKER,
		MOD_SHOCKER,
		55,			// damage
		0,			// knockback
		370,		// nextshot
		-1,			// ammo_max
		-1,			// clip_max
		0,			// spread_base
		0,			// spread_crouch
		0,			// spread_move
		0,			// spread_recoil
		0,			// recoil_shot
		0,			// recoil_time
		0,			// recoil_amount
		0,			// recoil_side
		0,			// recoil_crouch
		0,			// recoil_move
		0,			// reload_time
		0,			// tr_type
		0,			// tr_delta
		0,			// min_rank
		PR_MELEE	// prediction
	},

	{
		"standard",
		CT_SIDEARM,
		WT_PISTOL,
		MOD_STANDARD,
		17,			// damage
		10,			// knockback
		300,		// nextshot
		45,			// ammo_max
		15,			// clip_max
		450,		// spread_base
		0.7f,		// spread_crouch
		1.7f,		// spread_move
		1000,		// spread_recoil
		550,		// recoil_shot
		1100,		// recoil_time
		1500,		// recoil_amount
		750,		// recoil_side
		0.8f,		// recoil_crouch
		1.2f,		// recoil_move
		2100,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		0,			// min_rank
		PR_BULLET	// prediction
	},

	{
		"20mm",
		CT_SIDEARM,
		WT_PISTOL,
		MOD_HANDGUN,
		40,			// damage
		30,			// knockback
		450,		// nextshot
		27,			// ammo_max
		9,			// clip_max
		500,		// spread_base
		0.8f,		// spread_crouch
		2.1f,		// spread_move
		1000,		// spread_recoil
		700,		// recoil_shot
		1400,		// recoil_time
		2000,		// recoil_amount
		1300,		// recoil_side
		0.8f,		// recoil_crouch
		1.2f,		// recoil_move
		1800,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		3,			// min_rank
		PR_BULLET	// prediction
	},

	{
		"micro",
		CT_SIDEARM,
		WT_PISTOL,
		MOD_MICRO,
		16,			// damage
		15,			// knockback
		80,			// nextshot
		80,			// ammo_max
		40,			// clip_max
		1100,		// spread_base
		0.9f,		// spread_crouch
		1.1f,		// spread_move
		300,		// spread_recoil
		125,		// recoil_shot
		500,		// recoil_time
		2000,		// recoil_amount
		500,		// recoil_side
		0.8f,		// recoil_crouch
		1.2f,		// recoil_move
		2600,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		5,			// min_rank
		PR_BULLET	// prediction
	},
		
	{
		"rotshotgun",
		CT_PRIMARY,
		WT_STANDARD,
		MOD_SHOTGUN,
		6,			// damage
		12,			// knockback
		800,		// nextshot
		28,			// ammo_max
		7,			// clip_max
		0,			// spread_base
		0,			// spread_crouch
		0,			// spread_move
		1200,		// spread_recoil
		14,			// recoil_shot
		700,		// recoil_time
		1500,		// recoil_amount
		1700,		// recoil_side
		0.9f,		// recoil_crouch
		1.1f,		// recoil_move
		2600,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		1,			// min_rank
		PR_PELLETS	// prediction
	},
		
	{
		"smg",
		CT_PRIMARY,
		WT_STANDARD,
		MOD_SUB,
		18,			// damage
		18,			// knockback
		130,		// nextshot
		80,			// ammo_max
		20,			// clip_max
		300,		// spread_base
		0.6f,		// spread_crouch
		2.1f,		// spread_move
		700,		// spread_recoil
		280,		// recoil_shot
		800,		// recoil_time
		1600,		// recoil_amount
		500,		// recoil_side
		0.7f,		// recoil_crouch
		1.2f,		// recoil_move
		2200,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		1,			// min_rank
		PR_BULLET	// prediction
	},
		
	{
		"assault",
		CT_PRIMARY,
		WT_UP,
		MOD_A_RIFLE,
		17,			// damage
		30,			// knockback
		110,		// nextshot
		90,			// ammo_max
		30,			// clip_max
		240,		// spread_base
		0.7f,		// spread_crouch
		3.2f,		// spread_move
		1000,		// spread_recoil
		300,		// recoil_shot
		800,		// recoil_time
		2000,		// recoil_amount
		400,		// recoil_side
		0.7f,		// recoil_crouch
		1.2f,		// recoil_move
		2400,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		2,			// min_rank
		PR_BULLET	// prediction
	},
		
	{
		"sniper",
		CT_PRIMARY,
		WT_UP,
		MOD_SNIPER,
		105,		// damage
		10,			// knockback
		2900,		// nextshot
		15,			// ammo_max
		5,			// clip_max
		60,			// spread_base
		0.5f,		// spread_crouch
		17.0f,		// spread_move
		0,			// spread_recoil
		1000,		// recoil_shot
		3000,		// recoil_time
		2200,		// recoil_amount
		300,		// recoil_side
		0.6f,		// recoil_crouch
		3.0f,		// recoil_move
		3600,		// reload_time
		600,		// tr_type		(spread not zoomed)
		0,			// tr_delta
		2,			// min_rank
		PR_BULLET	// prediction
	},
		
	{
		"phoenix",
		CT_PRIMARY,
		WT_UP,
		MOD_PHOENIX,
		35,			// damage
		30,			// knockback
		450,		// nextshot
		33,			// ammo_max
		11,			// clip_max
		100,		// spread_base
		0.6f,		// spread_crouch
		4.2f,		// spread_move
		600,		// spread_recoil
		700,		// recoil_shot
		1300,		// recoil_time
		1400,		// recoil_amount
		500,		// recoil_side
		0.7f,		// recoil_crouch
		1.2f,		// recoil_move
		2900,		// reload_time
		350,		// tr_type		(spread not zoomed)
		0,			// tr_delta
		3,			// min_rank
		PR_BULLET	// prediction
	},
		
	{
		"tripact",
		CT_PRIMARY,
		WT_STANDARD,
		MOD_TRIPACT,
		38,			// damage
		36,			// knockback
		180,		// nextshot (time until next shot)
		60,			// ammo_max
		15,			// clip_max
		0,			// spread_base
		400,		// spread_crouch (time to next burst from last shot)
		7000,		// spread_move (flight time)
		400,		// spread_recoil
		600,		// recoil_shot
		1200,		// recoil_time
		1400,		// recoil_amount
		400,		// recoil_side
		0.8f,		// recoil_crouch
		1.4f,		// recoil_move
		2000,		// reload_time
		TR_LINEAR,	// tr_type
		2200,		// tr_delta
		3,			// min_rank
		PR_MISSILE	// prediction
	},
		
	{
		"grenadel",
		CT_PRIMARY,
		WT_LONG,
		MOD_GRENADE,
		140,		// damage
		80,			// knockback
		1200,		// nextshot
		24,			// ammo_max
		8,			// clip_max
		240,		// spread_base (splash radius)
		0,			// spread_crouch
		2000,		// spread_move (flight time)
		0,			// spread_recoil
		0,			// recoil_shot
		600,		// recoil_time
		1500,		// recoil_amount
		2400,		// recoil_side
		0.8f,		// recoil_crouch
		1.2f,		// recoil_move
		2500,		// reload_time
		TR_GRAVITY,	// tr_type
		600,		// tr_delta
		4,			// min_rank
		PR_MISSILE	// prediction
	},

	{
		"minigun",
		CT_PRIMARY,
		WT_STANDARD,
		MOD_MINIGUN,
		15,			// damage
		20,			// knockback
		60,			// nextshot
		200,		// ammo_max
		100,		// clip_max
		1500,		// spread_base
		0.9f,		// spread_crouch
		1.1f,		// spread_move
		500,		// spread_recoil
		80,			// recoil_shot
		700,		// recoil_time
		1200,		// recoil_amount
		700,		// recoil_side
		0.8f,		// recoil_crouch
		1.4f,		// recoil_move
		3100,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		4,			// min_rank
		PR_BULLET	// prediction
	},
		
	{
		"flame",
		CT_PRIMARY,
		WT_STANDARD,
		MOD_FLAMETHROWER,
		14,			// damage
		0,			// knockback
		50,			// nextshot
		140,		// ammo_max
		70,			// clip_max
		0,			// spread_base
		0,			// spread_crouch
		0.2f,		// spread_move   (recoil amount)
		4,			// spread_recoil (number of shots for recoil change)
		100,		// recoil_shot   (1hp_burndown_time)
		500,		// recoil_time   (flight_time)
		0,			// recoil_amount
		0,			// recoil_side
		0.7f,		// recoil_crouch
		2.0f,		// recoil_move
		3000,		// reload_time
		TR_LINEAR,	// tr_type
		800,		// tr_delta
		4,			// min_rank
		PR_FLAME	// prediction
	},
		
	{
		"dbt",
		CT_PRIMARY,
		WT_UP,
		MOD_DBT,
		65,			// damage
		36,			// knockback
		300,		// nextshot
		40,			// ammo_max
		10,			// clip_max
		30,			// spread_base (splash radius)
		0,			// spread_crouch
		7000,		// spread_move (flight time)
		400,		// spread_recoil
		800,		// recoil_shot
		1200,		// recoil_time
		1700,		// recoil_amount
		700,		// recoil_side
		0.7f,		// recoil_crouch
		1.4f,		// recoil_move
		2100,		// reload_time
		TR_LINEAR,	// tr_type
		3000,		// tr_delta
		5,			// min_rank
		PR_MISSILE	// prediction
	},
		
	{
		"rpg",
		CT_PRIMARY,
		WT_ROCK,
		MOD_ROCKET,
		150,		// damage
		80,			// knockback
		400,		// nextshot (time until rocket fire)
		7,			// ammo_max
		1,			// clip_max
		300,		// spread_base (splash radius)
		1200,		// spread_crouch (acceleration)
		7000,		// spread_move (flight time)
		0,			// spread_recoil
		1800,		// recoil_shot
		1800,		// recoil_time
		1800,		// recoil_amount
		1200,		// recoil_side
		0.7f,		// recoil_crouch
		1.4f,		// recoil_move
		2200,		// reload_time
		TR_ACCELERATE,	// tr_type
		300,		// tr_delta
		5,			// min_rank
		PR_MISSILE	// prediction
	},
		
	{
		"omega",
		CT_PRIMARY,
		WT_UP,
		MOD_OMEGA,
		180,		// damage
		36,			// knockback
		500,		// nextshot (time to next recharge after shot)
		-1,			// ammo_max
		-1,			// clip_max
		500,		// spread_base   (min time)
		2000,		// spread_crouch (charge time)
		1000,		// spread_move   (hold time)
		400,		// spread_recoil
		900,		// recoil_shot
		1200,		// recoil_time
		1200,		// recoil_amount
		500,		// recoil_side
		0.7f,		// recoil_crouch
		1.4f,		// recoil_move
		2100,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		5,			// min_rank
		PR_OMEGA	// prediction
	},
		
	{
		"thermo",
		CT_PRIMARY,
		WT_STANDARD,
		MOD_THERMO,
		11,			// damage
		0,			// knockback
		50,			// nextshot
		160,		// ammo_max
		80,			// clip_max
		4096,		// spread_base   (max beam range)
		0,			// spread_crouch
		0.3f,		// spread_move   (recoil amount)
		4,			// spread_recoil (number of shots for recoil change)
		0,			// recoil_shot
		2000,		// recoil_time   (time until self destruction)
		0,			// recoil_amount
		0,			// recoil_side
		0.7f,		// recoil_crouch
		2.0f,		// recoil_move
		5100,		// reload_time
		0,			// tr_type
		0,			// tr_delta
		6,			// min_rank
		PR_BEAM		// prediction
	},
				
	{
		"fraggren",
		CT_EXPLOSIVE,
		WT_GREN,
		MOD_FRAG_GRENADE,
		120,		// damage
		80,			// knockback
		300,		// nextshot      (throw time)
		3,			// ammo_max
		1,			// clip_max
		300,		// spread_base   (splash radius)
		0,			// spread_crouch
		2000,		// spread_move   (flight time)
		0,			// spread_recoil
		500,		// recoil_shot   (min time)
		1500,		// recoil_time   (charge time)
		700,		// recoil_amount (additional charged speed)
		0,			// recoil_side
		0,			// recoil_crouch
		0,			// recoil_move
		1500,		// reload_time
		TR_GRAVITY,	// tr_type
		200,		// tr_delta
		0,			// min_rank
		PR_MISSILE	// prediction
	},

	{
		"firegren",
		CT_EXPLOSIVE,
		WT_GREN,
		MOD_FIRE_GRENADE,
		110,		// damage
		60,			// knockback
		300,		// nextshot      (throw time)
		3,			// ammo_max
		1,			// clip_max
		250,		// spread_base   (splash radius)
		500,		// spread_crouch (fire base velocity)
		2000,		// spread_move   (flight time)
		-250,		// spread_recoil (fire acceleration)
		500,		// recoil_shot   (min time)
		1500,		// recoil_time   (charge time)
		700,		// recoil_amount (additional charged speed)
		0,			// recoil_side
		0,			// recoil_crouch
		0,			// recoil_move
		1500,		// reload_time
		TR_GRAVITY,	// tr_type
		200,		// tr_delta
		4,			// min_rank
		PR_MISSILE	// prediction
	},

	{
		"blast",
		CT_SPECIAL,
		WT_PISTOL,
		-1,
		0,			// damage
		0,			// knockback
		3500,		// nextshot
		-1,			// ammo_max
		-1,			// clip_max
		0,			// spread_base
		0,			// spread_crouch
		0,			// spread_move
		0,			// spread_recoil
		0,			// recoil_shot
		0,			// recoil_time
		0,			// recoil_amount
		0,			// recoil_side
		0,			// recoil_crouch
		0,			// recoil_move
		0,			// reload_time
		0,			// tr_type
		0,			// tr_delta
		0,			// min_rank
		PR_NONE		// prediction
	}
		
		
	// end of list marker
	/*{NULL}*/
};


// --------------------------------------------------------------


gweAnim_t	weAnims[WT_NUM_WEAPONTYPES] = 
{
	{
		TORSO_STAND2,
		TORSO_ATTACK2,
		TORSO_RAISE,
		TORSO_DROP,
		TORSO_RELOAD
	},
	{
		TORSO_STAND,
		TORSO_ATTACK,
		TORSO_RAISE,
		TORSO_DROP,
		TORSO_RELOAD
	},
	{
		TORSO_STAND_LW,
		TORSO_ATTACK_LW,
		TORSO_RAISE_LW,
		TORSO_DROP_LW,
		TORSO_RELOAD_LW
	},
	{
		TORSO_STAND_UP,
		TORSO_ATTACK_UP,
		TORSO_RAISE_UP,
		TORSO_DROP_UP,
		TORSO_RELOAD_UP
	},
	{
		TORSO_STAND_PIST,
		TORSO_ATTACK_PIST,
		TORSO_RAISE_PIST,
		TORSO_DROP_PIST,
		TORSO_RELOAD_PIST
	},
	{
		TORSO_STAND_ROCK,
		TORSO_ATTACK_ROCK,
		TORSO_RAISE_ROCK,
		TORSO_DROP_ROCK,
		TORSO_RELOAD_ROCK
	},
	{
		TORSO_STAND_GREN,
		TORSO_ATTACK_GREN,
		TORSO_RAISE_GREN,
		TORSO_DROP_GREN,
		TORSO_RELOAD_GREN
	}
};



// --------------------------------------------------------------

// definition of grank_t is in bg_public.h

grank_t	bg_ranklist[NUM_RANKS] =
{
	{
		"Cannon Fodder",
		2,	// next_rank
		2	// score_mod
	},
	{
		"Rookie",
		8,
		1
	},
	{
		"Rapier",
		12,
		0
	},
	{
		"Enforcer",
		16,
		-1
	},
	{
		"Assassin",
		20,
		-2
	},
	{
		"Elite Warrior",
		26,
		-3
	},
	{
		"Dominator",
		32,
		-5
	}
};

// --------------------------------------------------------------

modelslist_t	bg_modelslist[MAX_TACTICAL_MODELS] =
{
	{
		"Tribal",				// name
		"tm1_male",				// model
		"default",				// modelSkin
		"&head",				// head
		"default",				// headSkin
		TEAM_RED				// team
	},
	{
		"Tribeswoman",
		"tm1_fem",	
		"default",
		"&head",
		"default",
		TEAM_RED
	},
	{
		"Raider",
		"tm1_raider",	
		"default",
		"&head",
		"default",
		TEAM_RED
	},
	{
		"Female Raider",
		"tm1_fraider",	
		"default",
		"&head",
		"default",
		TEAM_RED
	},
	{
		"Soldier",
		"tm2_male",
		"default",
		"&head",
		"default",
		TEAM_BLUE
	},
	{
		"Female Soldier",
		"tm2_fem",
		"default",
		"&head",	
		"default",
		TEAM_BLUE
	},
	{
		"Commando",
		"tm2_commando",
		"default",
		"&head",
		"default",
		TEAM_BLUE
	},
	{
		"Female Commando",
		"tm2_fcommando",
		"default",
		"&head",
		"default",
		TEAM_BLUE
	}
};

// --------------------------------------------------------------

gitem_t	bg_itemlist[] = 
{
	{
		NULL,
		NULL,
		{ NULL,
		NULL,
		0, 0} ,
/* icon */		NULL,
/* pickup */	NULL,
		0,
		0,
		0,
		0,
/* precache */ "",
/* sounds */ ""
	},	// leave index 0 alone


	//
	// WEAPONS 
	//

/*QUAKED weapon_shocker (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_shocker", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/stromer/stromer.md3",
		0, 0, 0},
/* icon */		"icons/iconw_stromer",
/* pickup */	"P-UI Shocker",
		-1,
		-1,
		IT_WEAPON,
		WP_SHOCKER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_standard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_standard",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/standardgun/standardgun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_standard",
/* pickup */	"TS-3K Handgun",
		45,
		15,
		IT_WEAPON,
		WP_STANDARD,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_20mm (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_20mm",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/handgun/handgun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_handgun",
/* pickup */	"EL-KI 20mm Handgun",
		27,
		9,
		IT_WEAPON,
		WP_HANDGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_micro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_micro",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/micro/micro.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_micro",
/* pickup */	"T2050 Micro Erazor",
		80,
		40,
		IT_WEAPON,
		WP_MICRO,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_rotshotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_rotshotgun",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/shotgun/shotgun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_shotgun",
/* pickup */	"Q2ROT Shotgun",
		28,
		7,
		IT_WEAPON,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_smg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_smg",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/smg/smg.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_smg",
/* pickup */	"JAG-20 SMG",
		80,
		20,
		IT_WEAPON,
		WP_SUB,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_assault (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_assault",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/assault/assault.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_assault",
/* pickup */	"ALS1G Assault Rifle",
		90,
		30,
		IT_WEAPON,
		WP_A_RIFLE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_sniper (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_sniper",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/soff/soff.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_soff",
/* pickup */	"SSR Sniper Rifle",
		15,
		5,
		IT_WEAPON,
		WP_SNIPER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_phoenix (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_phoenix",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/phoenix/phoenix.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_phoenix",
/* pickup */	"Phoenix FAS",
		42,
		14,
		IT_WEAPON,
		WP_PHOENIX,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_tripact (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_tripact",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/tripact/tripact.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_tripact",
/* pickup */	"HIR-3 Tripact",
		60,
		15,
		IT_WEAPON,
		WP_TRIPACT,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grenadel (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grenadel",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/grenl/grenl.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_grenade",
/* pickup */	"S-1/2AT2 G-Launcher",
		24,
		8,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_minigun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_minigun",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/minigun/minigun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_minigun",
/* pickup */	"M6B Minigun",
		200,
		100,
		IT_WEAPON,
		WP_MINIGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_flame (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_flame",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/flame/flame.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_flame",
/* pickup */	"NO-2 Flamethrower",
		140,
		70,
		IT_WEAPON,
		WP_FLAMETHROWER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_dbt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_dbt",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/dbt/dbt.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_dbt",
/* pickup */	"DBT Prototype",
		40,
		10,
		IT_WEAPON,
		WP_DBT,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_rpg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_rpg",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/rpg/rpg.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_rpg",
/* pickup */	"XS-103 RPG",
		11,
		1,
		IT_WEAPON,
		WP_RPG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_omega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_omega",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/omega/omega.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_omega",
/* pickup */	"OMEGA Beam Gun",
		-1,
		-1,
		IT_WEAPON,
		WP_OMEGA,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_thermo (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_thermo",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/thermo/thermo.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_thermo",
/* pickup */	"TL-M72 Thermo Lance",
		160,
		80,
		IT_WEAPON,
		WP_THERMO,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_fraggren (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_fraggren",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/fraggren/fraggren.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_fraggren",
/* pickup */	"HG-1 Frag Grenade",
		3,
		1,
		IT_WEAPON,
		WP_FRAG_GRENADE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_firegren (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_firegren",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/firegren/firegren.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_firegren",
/* pickup */	"Pyr-0 Fire Grenade",
		2,
		1,
		IT_WEAPON,
		WP_FIRE_GRENADE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_blast (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_blast",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/blast/blast.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_blast",
/* pickup */	"SIBB Blastpack",
		-1,
		-1,
		IT_WEAPON,
		WP_BLAST,
/* precache */ "",
/* sounds */ ""
	},

	// end of list marker
	{NULL}
};

int		bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) - 1;


/*
==============
BG_GetTacticalRank
==============
*/
int	BG_GetTacticalRank( int score ) {
	int		rank;
	int		i;

	rank = 0;
	for ( i = 0; i < NUM_RANKS-1; i++ ) {
		if ( score >= bg_ranklist[i].next_rank ) rank = i+1;
	}

	return rank;
}


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t	*BG_FindItemForPowerup( powerup_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( (bg_itemlist[i].giType == IT_POWERUP || 
					bg_itemlist[i].giType == IT_TEAM ||
					bg_itemlist[i].giType == IT_PERSISTANT_POWERUP) && 
			bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable( holdable_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++) {
		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}


/*
===============
BG_FindItem

===============
*/
gitem_t	*BG_FindItem( const char *name ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->classname, name ) || !Q_stricmp( it->pickup_name, name ) )
			return it;
	}

	return NULL;
}

/*
===============
BG_FindWeaponForMod

===============
*/
int	BG_FindWeaponForMod( int mod ) {
	int		i;

	if ( mod == MOD_GRENADE_SPLASH ) return WP_GRENADE_LAUNCHER;
	if ( mod == MOD_DBT_SPLASH ) return WP_DBT;
	
	for ( i = 1; i < WP_NUM_WEAPONS; i++) {
		if ( mod == weLi[i].mod ) {
			return i;
		}
	}

	return 0;
}


/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t		origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps, int atTime ) {
	gitem_t	*item;
#ifdef MISSIONPACK
	int		upperBound;
#endif

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	if ( atTime < ent->time + ITEM_TAKETIME )
		return qfalse;

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {
	case IT_WEAPON:
		// check if player already has a gun of that category
		if ( ps->weapons[weLi[item->giTag].category] ) return qfalse;
		if ( gametype == GT_TACTICAL && item->giTag == WP_THERMO ) return qfalse;
		return qtrue;

/*	case IT_AMMO:
		if ( ps->ammo[ item->giTag ] >= 200 ) {
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
		if ( item->quantity == 5 || item->quantity == 100 ) {
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;*/

	case IT_POWERUP:
		return qtrue;	// powerups are always picked up

	case IT_TEAM: // team items, such as flags
		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if ( ps->stats[STAT_HOLDABLE_ITEM] ) {
			return qfalse;
		}
		return qtrue;

        case IT_BAD:
            Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
	}

	return qfalse;
}

//======================================================================

#define WATER_RESISTANCE	0.02
#define WATER_GRAVSPEED		30.0

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;
	vec3_t		normDelta;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;
		break;
	case TR_ACCELERATE:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		phase = 0.5 * tr->trDuration * deltaTime * deltaTime;
		VectorNormalize2( tr->trDelta, normDelta );
		VectorMA( tr->trBase, phase, normDelta, result );
		VectorMA( result, deltaTime, tr->trDelta, result );
		break;
	case TR_LOW_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;
		break;
	case TR_WATER_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorNormalize2( tr->trDelta, normDelta );
		VectorMA( tr->trBase, Q_ln( WATER_RESISTANCE * VectorLength( tr->trDelta ) * deltaTime + 1 ) / WATER_RESISTANCE, normDelta, result );
		result[2] -= WATER_GRAVSPEED * deltaTime;
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;
	vec3_t		normDelta;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	case TR_ACCELERATE:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		phase = tr->trDuration * deltaTime;
		VectorNormalize2( tr->trDelta, normDelta );
		VectorMA( tr->trDelta, phase, normDelta, result );
		break;
	case TR_LOW_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	case TR_WATER_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorScale( tr->trDelta, 1 / ( WATER_RESISTANCE * deltaTime * VectorLength( tr->trDelta ) + 1 ), result );
		result[2] -= WATER_GRAVSPEED;
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_RELOAD_WEAPON",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",
	"EV_BURN",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",

	"EV_SCOREPLUM",			// score plum

#ifdef MISSIONPACK
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
	"EV_KAMIKAZE",			// kamikaze explodes
	"EV_OBELISKEXPLODE",		// obelisk explodes
	"EV_INVUL_IMPACT",		// invulnerability sphere impact
	"EV_JUICED",				// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",		// lightning bolt bounced of invulnerability sphere
#endif

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT"

};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if ( atof(buf) != 0 ) {
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// flying characters don't hit bounce pads
	if ( ps->powerups[PW_FLIGHT] ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( jumppad->origin2, angles);
		p = fabs( AngleNormalize180( angles[PITCH] ) );
		if( p < 45 ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
//	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
//		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction
	VectorCopy( ps->velocity, s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = (ps->entityEventSequence-1) & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		if ( ps->entityEventSequence < ps->eventSequence ) {
			ps->entityEventSequence++;
		}
	}

	s->weapon = ps->weapons[ps->wpcat];
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	// for mapper defined items
	s->modelindex2 = ps->powerups[PW_ITEM];
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
//	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
//		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = (ps->entityEventSequence-1) & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		if ( ps->entityEventSequence < ps->eventSequence ) {
			ps->entityEventSequence++;
		}
	}

	s->weapon = ps->weapons[ps->wpcat];
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	// for mapper defined items
	s->modelindex2 = ps->powerups[PW_ITEM];
}
