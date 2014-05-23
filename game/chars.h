// Copyright (C) 1999-2000 Id Software, Inc.
//
//===========================================================================
//
// Name:			chars.h
// Function:		bot characteristics
// Programmer:		Mr Elusive (MrElusive@idsoftware.com)
// Last update:		1999-09-08
// Tab Size:		4 (real tabs)
//===========================================================================


//========================================================
//========================================================
//name
#define CHARACTERISTIC_NAME							0	//string
//gender of the bot
#define CHARACTERISTIC_GENDER						1	//string ("male", "female", "it")
//attack skill
// >  0.0 && <  0.2 = don't move
// >  0.3 && <  1.0 = aim at enemy during retreat
// >  0.0 && <  0.4 = only move forward/backward
// >= 0.4 && <  1.0 = circle strafing
// >  0.7 && <  1.0 = random strafe direction change
#define CHARACTERISTIC_ATTACK_SKILL					2	//float [0, 1]
//weapon weight file
#define CHARACTERISTIC_WEAPONWEIGHTS				3	//string
//view angle difference to angle change factor
#define CHARACTERISTIC_VIEW_FACTOR					4	//float [0, 1]
//maximum view angle change
#define CHARACTERISTIC_VIEW_MAXCHANGE				5	//float [1, 360]
//reaction time in seconds
#define CHARACTERISTIC_REACTIONTIME					6	//float [0, 5]
//accuracy when aiming
#define CHARACTERISTIC_AIM_ACCURACY					7	//float [0, 1]
//weapon specific aim accuracy
#define CHARACTERISTIC_AIM_ACCURACY_PISTOLS			8	//float [0, 1]
#define CHARACTERISTIC_AIM_ACCURACY_SMGS			9	//float [0, 1]
#define CHARACTERISTIC_AIM_ACCURACY_RIFLES			10	//float [0, 1]
#define CHARACTERISTIC_AIM_ACCURACY_SNIPER			11	//float [0, 1]
#define CHARACTERISTIC_AIM_ACCURACY_BIGGUNS			12	//float [0, 1]
#define CHARACTERISTIC_AIM_ACCURACY_BEAMS			13	//float [0, 1]
#define CHARACTERISTIC_AIM_ACCURACY_GRENADES		14	//float [0, 1]
//skill when aiming
// >  0.0 && <  0.9 = aim is affected by enemy movement
// >  0.4 && <= 0.8 = enemy linear leading
// >  0.8 && <= 1.0 = enemy exact movement leading
// >  0.5 && <= 1.0 = prediction shots when enemy is not visible
// >  0.6 && <= 1.0 = splash damage by shooting nearby geometry
#define CHARACTERISTIC_AIM_SKILL					15	//float [0, 1] (infinite speed)
//weapon specific aim skill
#define CHARACTERISTIC_AIM_SKILL_PROJECTILES		16	//float [0, 1] (finite speed)
#define CHARACTERISTIC_AIM_SKILL_GRENADES			17	//float [0, 1]
//========================================================
//chat
//========================================================
//file with chats
#define CHARACTERISTIC_CHAT_FILE					30	//string
//name of the chat character
#define CHARACTERISTIC_CHAT_NAME					31	//string
//characters per minute type speed
#define CHARACTERISTIC_CHAT_CPM						32	//integer [1, 4000]
//tendency to insult/praise
#define CHARACTERISTIC_CHAT_INSULT					33	//float [0, 1]
//tendency to chat misc
#define CHARACTERISTIC_CHAT_MISC					34	//float [0, 1]
//tendency to chat at start or end of level
#define CHARACTERISTIC_CHAT_STARTENDLEVEL			35	//float [0, 1]
//tendency to chat entering or exiting the game
#define CHARACTERISTIC_CHAT_ENTEREXITGAME			36	//float [0, 1]
//tendency to chat when killed someone
#define CHARACTERISTIC_CHAT_KILL					37	//float [0, 1]
//tendency to chat when died
#define CHARACTERISTIC_CHAT_DEATH					38	//float [0, 1]
//tendency to chat when enemy suicides
#define CHARACTERISTIC_CHAT_ENEMYSUICIDE			39	//float [0, 1]
//tendency to chat when hit while talking
#define CHARACTERISTIC_CHAT_HITTALKING				40	//float [0, 1]
//tendency to chat when bot was hit but didn't dye
#define CHARACTERISTIC_CHAT_HITNODEATH				41	//float [0, 1]
//tendency to chat when bot hit the enemy but enemy didn't dye
#define CHARACTERISTIC_CHAT_HITNOKILL				42	//float [0, 1]
//tendency to randomly chat
#define CHARACTERISTIC_CHAT_RANDOM					43	//float [0, 1]
//tendency to reply
#define CHARACTERISTIC_CHAT_REPLY					44	//float [0, 1]
//========================================================
//movement
//========================================================
//tendency to crouch
#define CHARACTERISTIC_CROUCHER						45	//float [0, 1]
//tendency to jump
#define CHARACTERISTIC_JUMPER						46	//float [0, 1]
//tendency to walk
#define CHARACTERISTIC_WALKER						47	//float [0, 1]
//tendency to jump using a weapon
#define CHARACTERISTIC_WEAPONJUMPING				48	//float [0, 1]
//========================================================
//goal
//========================================================
//item weight file
#define CHARACTERISTIC_ITEMWEIGHTS					49	//string
//the aggression of the bot
#define CHARACTERISTIC_AGGRESSION					50	//float [0, 1]
//the self preservation of the bot (rockets near walls etc.)
#define CHARACTERISTIC_SELFPRESERVATION				51	//float [0, 1]
//how likely the bot is to take revenge
#define CHARACTERISTIC_VENGEFULNESS					52	//float [0, 1]
//tendency to camp
#define CHARACTERISTIC_CAMPER						53	//float [0, 1]
//========================================================
//========================================================
//tendency to get easy frags
#define CHARACTERISTIC_EASY_FRAGGER					54	//float [0, 1]
//how alert the bot is (view distance)
#define CHARACTERISTIC_ALERTNESS					55	//float [0, 1]
//how much the bot fires it's weapon
#define CHARACTERISTIC_FIRETHROTTLE					56	//float [0, 1]

