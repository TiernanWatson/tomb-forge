#include "Lara.h"

#include "States/LocomotionState.h"
#include "States/AirState.h"
#include "States/ClimbState.h"

#include "../Assets/AnimationLoader.h"

namespace TombForge
{
    void Lara::LoadAnimations(AnimationLoader& loader)
    {
        // Locomotion
        animations[LARA_ANIM_IDLE] = loader.Load("Lara\\Anims\\LARA_STANCE.tfanim");
        animations[LARA_ANIM_RUN] = loader.Load("Lara\\Anims\\LARA_RUN.tfanim");
        animations[LARA_ANIM_RUN_START] = loader.Load("Lara\\Anims\\LARA_STANCE_TO_RUN.tfanim");
        animations[LARA_ANIM_RUN_TO_JUMP_L] = loader.Load("Lara\\Anims\\LARA_RUN_TO_RUNJUMP.tfanim");
        animations[LARA_ANIM_WALK_START] = loader.Load("Lara\\Anims\\LARA_STANCE_TO_WALK.tfanim");
        animations[LARA_ANIM_WALK] = loader.Load("Lara\\Anims\\LARA_WALK.tfanim");

        // Jump and air
        animations[LARA_ANIM_RUN_JUMP_L] = loader.Load("Lara\\Anims\\LARA_RUNJUMP.tfanim");
        animations[LARA_ANIM_JUMP_TO_FALL] = loader.Load("Lara\\Anims\\LARA_JUMPF_TO_FALLF.tfanim");
        animations[LARA_ANIM_FALL] = loader.Load("Lara\\Anims\\LARA_FALL.tfanim");
        animations[LARA_ANIM_FALL_TO_RUN] = loader.Load("Lara\\Anims\\LARA_RUNJUMP_TO_RUN.tfanim");
        animations[LARA_ANIM_JUMP_TO_REACH] = loader.Load("Lara\\Anims\\LARA_JUMPF_TO_REACH.tfanim");
        animations[LARA_ANIM_REACH] = loader.Load("Lara\\Anims\\LARA_REACH.tfanim");
        animations[LARA_ANIM_GRAB_LEDGE] = loader.Load("Lara\\Anims\\LARA_REACH_TO_GRAB.tfanim");
        animations[LARA_ANIM_GRAB_WALL] = loader.Load("Lara\\Anims\\LARA_REACH_TO_GRAB.tfanim");

        // Climbing
        animations[LARA_ANIM_HANG_LOOP] = loader.Load("Lara\\Anims\\LARA_HANGLOOP.tfanim");
        animations[LARA_ANIM_SHIMMY_LEFT] = loader.Load("Lara\\Anims\\LARA_SHIMMY.tfanim");
        animations[LARA_ANIM_SHIMMY_RIGHT] = loader.Load("Lara\\Anims\\LARA_SHIMMY_MIRROR.tfanim");
        animations[LARA_ANIM_CLIMB_UP] = loader.Load("Lara\\Anims\\LARA_VAULT_A.tfanim");
    }

    void Lara::SetAnimation(LaraAnim anim, float fadeTime, bool loop)
    {
        if (anim > animations.size() || animations[anim] == nullptr)
        {
            LOG_ERROR("Unable to transition to animation %i", anim);
            return;
        }

        if (fadeTime == 0.0f)
        {
            animPlayer.Play(animations[anim], loop);
        }
        else
        {
            animPlayer.BlendTo(animations[anim], fadeTime, loop);
        }

        animIndex = anim;
    }

    bool Lara::IsGrounded() const
    {
        return physics->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    }
}
