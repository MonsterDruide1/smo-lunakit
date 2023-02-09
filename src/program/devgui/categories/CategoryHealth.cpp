#include "program/devgui/categories/CategoryHealth.h"

#include "game/player/PlayerFunction.h"
#include "helpers/PlayerHelper.h"

CategoryHealth::CategoryHealth(const char* catName, const char* catDesc)
    : CategoryBase(catName, catDesc)
{
}

void CategoryHealth::updateCat()
{
    // Get the player's hit point data pointer if it doesn't exist already
    if (!mHitData) {
        HakoniwaSequence* gameSeq = tryGetHakoniwaSequence();
        if (gameSeq) {
            mHitData = gameSeq->mGameDataHolder.mData->mGameDataFile->mPlayerHitPointData;
        }
    }

    // Get the player actor and check if they are dead
    PlayerActorBase* player = tryGetPlayerActor();
    if (!player)
        return;

    bool isPlayerDead = PlayerFunction::isPlayerDeadStatus(player);

    // Override the player's health if they exist and are alive
    if (mIsOverride && !isPlayerDead) {
        mHitData->mCurrentHit = mTargetHealth;
        mHitData->mIsKidsMode = mIsKidsMode;
    }

    // Try killing the player if the kill button is pressed
    StageScene* curScene = tryGetStageScene();
    if (isInStageScene(curScene) && mIsKillPlayer && !isPlayerDead) {
        PlayerHelper::killPlayer(player);
        mIsKillPlayer = false;
    }
}

void CategoryHealth::updateCatDisplay()
{
    CategoryBase::updateCatDisplay();

    if (ImGui::Checkbox("Edit Health", &mIsOverride) && mHitData) {
        mTargetHealth = mHitData->mCurrentHit;
        mIsKidsMode = mHitData->mIsKidsMode;
    }

    if (mIsOverride) {
        ImGui::SameLine();
        ImGui::Checkbox("Is Kids Mode", &mIsKidsMode);

        ImGui::SliderInt("Health", &mTargetHealth, 1, 9);
    }

    if (isInStageScene() && ImGui::Button("Kill Player"))
        mIsKillPlayer = true;
}