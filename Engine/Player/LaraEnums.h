#pragma once

namespace TombForge
{
	// These are written as classic C-style enums as they are used 
	// as indices frequently and so are easier and quicker to type

	// Weapons that Lara can equip
	enum LaraWeapon
	{
		LARA_WEAPON_NONE,

		LARA_WEAPON_DUALS,
		LARA_WEAPON_HANDGUN,
		LARA_WEAPON_SHOTGUN,
		
		LARA_WEAPON_COUNT
	};

	// Overall player state to define which functions to use
	enum LaraState
	{
		LARA_STATE_LOCOMOTION,
		LARA_STATE_AIR,
		LARA_STATE_CLIMB,

		LARA_STATE_COUNT
	};

	// Specific animations (1-1 mapping)
	enum LaraAnim
	{
		// Locomotion
		LARA_ANIM_IDLE,
		LARA_ANIM_RUN_START,
		LARA_ANIM_RUN,
		LARA_ANIM_RUN_STOP_L,
		LARA_ANIM_RUN_STOP_R,
		LARA_ANIM_WALK_START,
		LARA_ANIM_WALK,
		LARA_ANIM_RUN_TO_JUMP_L,
		LARA_ANIM_RUN_TO_JUMP_R,
		LARA_ANIM_RUN_TURN_L,
		LARA_ANIM_RUN_TURN_R,
		LARA_ANIM_FALL_TO_RUN,

		// Air and jumping
		LARA_ANIM_RUN_JUMP_L,
		LARA_ANIM_RUN_JUMP_R,
		LARA_ANIM_JUMP_TO_FALL,
		LARA_ANIM_FALL,
		LARA_ANIM_JUMP_TO_REACH,
		LARA_ANIM_REACH,

		// Climbing
		LARA_ANIM_GRAB_LEDGE, // Air at legs
		LARA_ANIM_GRAB_WALL, // Solid at legs
		LARA_ANIM_HANG_LOOP,
		LARA_ANIM_SHIMMY_LEFT,
		LARA_ANIM_SHIMMY_RIGHT,
		LARA_ANIM_CLIMB_UP,

		// Count
		LARA_ANIM_COUNT
	};
}