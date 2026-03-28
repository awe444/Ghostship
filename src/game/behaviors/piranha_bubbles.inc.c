
#include <math.h>

/**
 * Behavior for bhvPiranhaPlantBubble and bhvPiranhaPlantWakingBubbles.
 *
 * This controls the bubble that grows and shrinks from a Piranha Plant's nose
 * as it sleeps, and the smaller bubbles that explode outwards when the Piranha
 * Plant is woken up.
 */

/**
 * Main loop for bhvPiranhaPlantWakingBubbles. Initialize a random 3D velocity
 * vector, then follow its path according to gravity.
 */
void bhv_piranha_plant_waking_bubbles_loop(void) {
    if (o->oTimer == 0) {
        o->oVelY = random_float() * 10.0f + 5.0f;
        o->oForwardVel = random_float() * 10.0f + 5.0f;
        o->oMoveAngleYaw = random_u16();
    }
    cur_obj_move_using_fvel_and_gravity();
}

/**
 * Main loop for bhvPiranhaPlantBubble. After it is initialized, the bubble
 * will grow and shrink continuously while the Piranha Plant sleeps. If the
 * Piranha Plant ever stops sleeping, the bubble will disappear and spawn many
 * bubbles to appear to burst. After bursting, the bubble is reinitialized
 * and the cycle can repeat.
 */
void bhv_piranha_plant_bubble_loop(void) {
    struct Object *parent = o->parentObj; // the Piranha Plant
    f32 scale = 0;
    s32 i;
    UNUSED u8 filler[4];

    if (parent == NULL || parent == o || parent->activeFlags == ACTIVE_FLAG_DEACTIVATED) {
        obj_mark_for_deletion(o);
        return;
    }

    switch (o->oAction) {
        case PIRANHA_PLANT_BUBBLE_ACT_IDLE:
            cur_obj_disable_rendering();

            if (parent->oAction == PIRANHA_PLANT_ACT_SLEEPING) {
                o->oAction++; // move to PIRANHA_PLANT_BUBBLE_ACT_GROW_SHRINK_LOOP
            }
            break;

        case PIRANHA_PLANT_BUBBLE_ACT_GROW_SHRINK_LOOP:
        {
            // TODO: rename lastFrame if it is inaccurate
            s32 animFrame;
            s32 lastFrame;

            if (parent->header.gfx.animInfo.curAnim == NULL) {
                break;
            }

            animFrame = parent->header.gfx.animInfo.animFrame;
            lastFrame = parent->header.gfx.animInfo.curAnim->loopEnd - 2;

            // Inline cur_obj_set_pos_relative(parent, 0, 72.0f, 180.0f) using
            // cosf/sinf to avoid gSineTable page boundary access issue on ARM64.
            {
                f32 angle_rad = (f32)((u16)parent->oMoveAngleYaw) * (f32)(2.0 * M_PI / 65536.0);
                f32 facingZ = cosf(angle_rad);
                f32 facingX = sinf(angle_rad);

                o->oMoveAngleYaw = parent->oMoveAngleYaw;
                o->oPosX = parent->oPosX + 180.0f * facingX;
                o->oPosY = parent->oPosY + 72.0f;
                o->oPosZ = parent->oPosZ + 180.0f * facingZ;
            }

            if (parent->oDistanceToMario < parent->oDrawingDistance) {
                cur_obj_enable_rendering();

                if (parent->oAction == PIRANHA_PLANT_ACT_SLEEPING) {
                    /**
                     * Set the frame after shrinking is done to be slightly before
                     * halfway through the animation, and the frame before growing
                     * slightly after halfway. This leaves about 8 frames during
                     * which the bubble is at its smallest, where its scale is 1.0f.
                     */
                    // the first frame after shrinking is done
                    f32 doneShrinkingFrame = lastFrame / 2.0f - 4.0f;
                    // the frame just before growing begins
                    f32 beginGrowingFrame = lastFrame / 2.0f + 4.0f;

                    // Note that the bubble always starts this loop at its largest.
                    if (animFrame < doneShrinkingFrame) {
                        // Shrink from 5.0f to 1.0f.
                        // Use cosf instead of coss table lookup to avoid
                        // gSineTable page boundary access issue on ARM64.
                        // coss(ratio * 0x4000) = cos(ratio * pi/2)
                        scale = cosf(animFrame / doneShrinkingFrame * (f32)(M_PI / 2)) * 4.0f + 1.0f;
                    } else if (animFrame > beginGrowingFrame) {
                        // Grow from 1.0f to 5.0f.
                        // Use sinf instead of sins table lookup (same reason).
                        // sins(ratio * 0x4000) = sin(ratio * pi/2)
                        scale = sinf((
                                    // they should have used beginGrowingFrame here:
                                    (animFrame - (lastFrame / 2.0f + 4.0f)) / beginGrowingFrame
                                    ) * (f32)(M_PI / 2)) * 4.0f + 1.0f;
                    } else {
                        // Stay at 1.0f for a few frames.
                        scale = 1.0f;
                    }
                } else {
                    // Piranha Plant is no longer sleeping.
                    o->oAction++; // move to PIRANHA_PLANT_BUBBLE_ACT_BURST
                }
            } else {
                cur_obj_disable_rendering();
            }
            break;
        }

        case PIRANHA_PLANT_BUBBLE_ACT_BURST:
            cur_obj_disable_rendering();
            scale = 0.0f;

            // Spawn 15 small bubbles to make it look like this bubble burst.
            for (i = 0; i < 15; i++) {
                try_to_spawn_object(0, 1.0f, o, MODEL_BUBBLE, bhvPiranhaPlantWakingBubbles);
            }

            o->oAction = PIRANHA_PLANT_BUBBLE_ACT_IDLE;
            scale = 1.0f; // this has no effect; it is set to 0 in the idle state
            break;
    }

    cur_obj_scale(scale);
}
