#include "ResearchHooks.h"

#include "game/Player/IJudge.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "Library/LiveActor/ActorPoseKeeper.h"
#include "Library/Layout/LayoutActor.h"
#include "Library/Memory/HeapUtil.h"
#include "math/seadQuat.h"
#include "heap/seadHeap.h"
#include "heap/seadHeapMgr.h"
#include "math/seadVector.h"
#include "logger/Logger.hpp"
#include "game/Player/PlayerCollider.h"
#include <prim/seadBitFlag.h>
#include <prim/seadDelegate.h>
#include <Library/Collision/CollisionDirector.h>
#include "../../../../lib/custom/al/collision/Triangle.h"
#include "tas/input.h"
#include "tas/server.h"
#include "tas/tas.h"
#include <sys/select.h>
#include <lib.hpp>
#include "ClonableExpHeap.h"
#include "ClonableFrameHeap.h"
#include "heap/seadFrameHeap.h"

class PlayerJudgeWallCatch : IJudge {
public:
    const al::LiveActor *mPlayer;
    const PlayerConst *mConst;
    const IUsePlayerCollision *mCollision;
    const void *mModelChanger;
    const PlayerCarryKeeper *mCarryKeeper;
    const PlayerExternalVelocity *mExternalVelocity;
    const PlayerInput *mInput;
    const PlayerTrigger *mTrigger;
    const PlayerCounterForceRun *mCounterForceRun;
    bool mIsJudge;
    al::CollisionParts *mCollidedWallPart;
    sead::Vector3f mPosition;
    sead::Vector3f mCollidedWallNormal;
    sead::Vector3f mNormalAtPos;
};

class PlayerAnimator {
public:
    void startAnim(const sead::SafeString&);
};

namespace al {

s32 getRandom(s32, s32);

class SpherePoseInterpolator;
class LiveActor;
class KCollisionServer;
class KCPrismHeader;

struct KCPrismData {
    f32 mLength;
    u16 mPosIndex;
    u16 mFaceNormalIndex;
    u16 mEdgeNormalIndex[3];
    u16 mCollisionType;
    u32 mTriIndex;
};

class HitSensor {
public:
    const char* mName;
};

class HitInfo {
public:
    HitInfo();

    bool isCollisionAtFace() const;
    
public:
    al::Triangle mTriangle;
    f32 unk = 0.0f;
    sead::Vector3f mCollisionHitPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f unk3 = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mCollisionMovingReaction = {0.0f, 0.0f, 0.0f};
    u8 mCollisionLocation = 0;
};
static_assert(sizeof(HitInfo) == 0xA0);

class ArrowHitInfo : public HitInfo {};

class DiskHitInfo : public HitInfo {};

class SphereHitInfo : public HitInfo {
public:
    void calcFixVector(sead::Vector3f*, sead::Vector3f*) const;
    void calcFixVectorNormal(sead::Vector3f*, sead::Vector3f*) const;
};

}
class PlayerCollider2D3D;
class PlayerConst;
class PlayerCollider;
class PlayerColliderDisk;
struct CollisionShapeInfoArrow;

class CollidedShapeResult {
public:
    bool isArrow() const;
    bool isSphere() const;
    bool isDisk() const;
};

static void logVector(const sead::Vector3f* vec, const char* name) {
    Logger::log("%s: %f, %f, %f\n", name, vec->x, vec->y, vec->z);
}
static void logQuat(const sead::Quatf* quat, const char* name) {
    Logger::log("%s: %f, %f, %f, %f\n", name, quat->x, quat->y, quat->z, quat->w);
}

static int currentFrame = 0;
static PlayerActorHakoniwa* player = nullptr;

size_t getFreeSizeRecursive(sead::Heap* heap) {
    size_t size = heap->getFreeSize();
    for (sead::Heap& childRef : heap->mChildren) {
        sead::Heap* child = &childRef;
        if (child)
            size += getFreeSizeRecursive(child);
    }
    return size;
}

static bool doFpsTest = false;
static int numExpHeap = 0;
static int numFrameHeap = 0;
static int numSeparateHeap = 0;
static int numUnitHeap = 0;

HOOK_DEFINE_TRAMPOLINE(UpdateFrameHook) {
    static void Callback(PlayerActorHakoniwa* p) {
        currentFrame++;
        player = p;
        Orig(p);
        Logger::log("--------------------------- %d ---------------------------------\n", fl::TasHolder::instance().curFrame);
        sead::Vector3f pos = al::getTrans(p);
        sead::Vector3f vel = p->mPoseKeeper->getVelocity();
        sead::Vector3f front;
        al::calcFrontDir(&front, p);
        Logger::log("Position: (%.20f, %.20f, %.20f)\n", pos.x, pos.y, pos.z);
        Logger::log("Velocity: (%.20f, %.20f, %.20f)\n", vel.x, vel.y, vel.z);
        Logger::log("Front: (%.20f, %.20f, %.20f)\n", front.x, front.y, front.z);

        if(!smo::Server::instance().started) smo::Server::instance().start();
        if(!smo::Server::instance().connected) smo::Server::instance().connect("192.168.3.27");
        fl::TasHolder::instance().update();
        /*if(currentFrame % 60 == 0 && currentFrame > 3000) {
            Logger::log("Frame: %d\n", currentFrame);
        }*/

       /*Logger::log("Heap statistics:\n");
       Logger::log("  Exp: %d\n", numExpHeap);
       Logger::log("  Frame: %d\n", numFrameHeap);
       Logger::log("  Separate: %d\n", numSeparateHeap);
       Logger::log("  Unit: %d\n", numUnitHeap);

       sead::Heap* rootHeap = sead::HeapMgr::instance()->getRootHeap(0);
       size_t totalSize = rootHeap->getSize();
       size_t totalFree = getFreeSizeRecursive(rootHeap);
       /*sead::Heap* sceneHeap = al::getSceneHeap();
       //Logger::log("Sum of all heaps: %lu / %lu\n", (totalSize-totalFree), totalSize);
       //Logger::log("SceneHeap: %lu / %lu\n", sceneHeap->getSize()-sceneHeap->getFreeSize(), sceneHeap->getSize());

       if(isTriggerLeft() && isTriggerUp()) {
            doFpsTest = !doFpsTest;
            Logger::log("FPS test: %s\n", doFpsTest?"enabled":"disabled");
       }*/
    }
};

class GameSystem;
HOOK_DEFINE_TRAMPOLINE(GameSystemMovement) {
    static void Callback(GameSystem* p) {
        if(doFpsTest) {
            const int testFrames = 1000;
            u64 startTime = nn::os::GetSystemTick();
            for(int i = 0; i < testFrames; i++) {
                Orig(p);
            }
            u64 endTime = nn::os::GetSystemTick();
            Logger::log("%d frames took %f seconds\n", testFrames, (endTime - startTime) / (double)nn::os::GetSystemTickFrequency());
            Logger::log("=> %f FPS\n", testFrames / ((endTime - startTime) / (double)nn::os::GetSystemTickFrequency()));
            doFpsTest = false;
        }
        Orig(p);
    }
};

HOOK_DEFINE_TRAMPOLINE(SpherePoseInterpolatorStartHook) {
    static void Callback(al::SpherePoseInterpolator *result,
            const sead::Vector3f &a2,
            const sead::Vector3f &a3,
            float a6,
            float a7,
            const sead::Quatf &a4,
            const sead::Quatf &a5,
            float a8
    ) {
        Logger::log("SpherePoseInterpolator::startInterp(start=(%f, %f, %f), end=(%f, %f, %f), ..., steps=%f)\n", a2.x, a2.y, a2.z, a3.x, a3.y, a3.z, a8);
        Orig(result, a2, a3, a6, a7, a4, a5, a8);
    }
};

HOOK_DEFINE_TRAMPOLINE(SpherePoseInterpolatorCalcInterpHook) {
    static void Callback(al::SpherePoseInterpolator *self, sead::Vector3f* pos, f32* size, sead::Quatf* quat, sead::Vector3f* remainMoveVec) {
        Orig(self, pos, size, quat, remainMoveVec);
        Logger::log("SpherePoseInterpolator::calcInterp(pos=(%f, %f, %f), size=%f, quat=(%f, %f, %f, %f), remainMoveVec=(%f, %f, %f))\n", pos->x, pos->y, pos->z, *size, quat->x, quat->y, quat->z, quat->w, remainMoveVec?remainMoveVec->x:-0, remainMoveVec?remainMoveVec->y:-0, remainMoveVec?remainMoveVec->z:-0);
    }
};

HOOK_DEFINE_TRAMPOLINE(PlayerCollider2D3DctorHook) {
    static void Callback(PlayerCollider2D3D* a1, al::LiveActor * a2,PlayerConst const* a3,PlayerCollider * a4,PlayerColliderDisk * a5) {
        Orig(a1, a2, a3, a4, a5);
        Logger::log("PlayerCollider2D3D ctor\n");
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerColliderCollideHook) {
    static sead::Vector3f Callback(PlayerCollider* a1, const sead::Vector3f& a2) {
        sead::Vector3f result = Orig(a1, a2);
        Logger::log("Collide: (%.02f, %.02f, %.02f)+(%.02f, %.02f, %.02f) => (%.02f, %.02f, %.02f) ; isRecovery=%s\n", a1->mTrans.x, a1->mTrans.y, a1->mTrans.z, a2.x, a2.y, a2.z, result.x, result.y, result.z, (a1->mIsDuringRecovery) ? "true" : "false");
        return result;
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerColliderMoveCollideHook) {
    static void Callback(PlayerCollider* a0, sead::Vector3<float> * a1,float * a2,sead::Quat<float> * a3,sead::Vector3<float> const& a4,float a42,sead::Quat<float> const& a5,sead::Vector3<float> const& a6,float a7,bool a8) {
        //Logger::log("PreMoveCollide: (%.02f, %.02f, %.02f) ; %.02f ; (%.02f, %.02f, %.02f, %.02f) ; (%.02f, %.02f, %.02f) ; %.02f ; (%.02f, %.02f, %.02f, %.02f) ; (%.02f, %.02f, %.02f) ; %.02f ; %s\n", a1->x, a1->y, a1->z, *a2, a3->x, a3->y, a3->z, a3->w, a4.x, a4.y, a4.z, a42, a5.x, a5.y, a5.z, a5.w, a6.x, a6.y, a6.z, a7, (a8) ? "true" : "false");
        Orig(a0, a1, a2, a3, a4, a42, a5, a6, a7, a8);
        Logger::log("PostMoveCollide: (%.020f, %.020f, %.020f) ; %.020f ; (%.020f, %.020f, %.020f, %.020f) ; (%.020f, %.020f, %.020f) ; %.020f ; (%.020f, %.020f, %.020f, %.020f) ; (%.020f, %.020f, %.020f) ; %.020f ; %s\n", a1->x, a1->y, a1->z, *a2, a3->x, a3->y, a3->z, a3->w, a4.x, a4.y, a4.z, a42, a5.x, a5.y, a5.z, a5.w, a6.x, a6.y, a6.z, a7, (a8) ? "true" : "false");
        //Logger::log("flag1: %s, flag2: %s, mCollisionShapeScale: %.02f\n", (a0->flag1) ? "true" : "false", (a0->flag2) ? "true" : "false", a0->mCollisionShapeScale);
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionShapeKeeperUpdateShapeHook) {
    static void Callback(CollisionShapeKeeper* a1) {
        Logger::log("PRE: UpdateShape\n");
        Orig(a1);
        Logger::log("POST: UpdateShape\n");
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionShapeInfoArrowCtorHook) {
    static void Callback(CollisionShapeInfoArrow* a1, const char* a2, const sead::Vector3f& a3, const sead::Vector3f& a4, float a5, int a6) {
        Logger::log("CollisionShapeInfoArrow::CollisionShapeInfoArrow(%s, (%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f), %.02f, %d)\n", a2, a3.x, a3.y, a3.z, a4.x, a4.y, a4.z, a5, a6);
        Orig(a1, a2, a3, a4, a5, a6);
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionShapeInfoArrowGetBoundingCenterHook) {
    static sead::Vector3f& Callback(CollisionShapeInfoArrow* a1) {
        auto& result = Orig(a1);
        Logger::log("CollisionShapeInfoArrow::getBoundingCenter(%p) => (%.02f, %.02f, %.02f)\n", a1, result.x, result.y, result.z);
        return result;
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionMultiShapeCheckHook) {
    static bool Callback(CollisionMultiShape *self, CollisionShapeKeeper * a1,sead::Matrix34<float> const* a2,float a3,sead::Vector3<float> const& a4,al::CollisionPartsFilterBase const* a5) {
        bool result = Orig(self, a1, a2, a3, a4, a5);
        Logger::log("CollisionMultiShape::check(%p, (%.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f) %.02f, (%.02f, %.02f, %.02f), %p) => %s\n", a1, a2->m[0][0], a2->m[0][1], a2->m[0][2], a2->m[0][3], a2->m[1][0], a2->m[1][1], a2->m[1][2], a2->m[1][3], a2->m[2][0], a2->m[2][1], a2->m[2][2], a2->m[2][3], a3, a4.x, a4.y, a4.z, a5, (result) ? "true" : "false");
        return result;
    }
};
HOOK_DEFINE_TRAMPOLINE(KCHitSphereForPlayerHook) {
    static bool Callback(al::KCollisionServer* self, const al::KCPrismData* data, const al::KCPrismHeader* header, const sead::Vector3f* position, f32 a5,
                            f32 a6, f32* a7, u8* a8) {
        Logger::log("KCHitSphereForPlayer(%p(%d), %p, (%.02f, %.02f, %0.2f), %f, %f)\n", data, data->mTriIndex, header, position->x, position->y, position->z, a5, a6);
        bool result = Orig(self, data, header, position, a5, a6, a7, a8);
        Logger::log("hitsphere=%s\n", result?"true":"false");
        return result;
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerColliderCalcResultVec) {
    static void Callback(PlayerCollider* self, sead::Vector3f *a2,sead::Vector3f *a3,sead::Vector3f const&a4){
        Orig(self, a2, a3, a4);
        Logger::log("calcResultVec result: (%f, %f, %f), (%f, %f, %f)\n", a2->x, a2->y, a2->z, a3->x, a3->y, a3->z);
    }
};
HOOK_DEFINE_TRAMPOLINE(KCHitArrowHook) {
    static bool Callback(al::KCollisionServer* self, al::KCPrismData const* a1,al::KCPrismHeader const* a2,sead::Vector3<float> const& a3,sead::Vector3<float> const& a4,float * a5,uchar * a6) {
        Logger::log("KCHitArrow(%p, %p, (%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f), %f)\n", self, a1, a2, a3.x, a3.y, a3.z, a4.x, a4.y, a4.z, *a5);
        bool result = Orig(self, a1, a2, a3, a4, a5, a6);
        Logger::log("KCHitArrow result: %s ; %d=%f\n", result?"true":"false", *a6, *a5);
        if(result) Logger::log("!!!!!!!!!!!!!!!!!!%f!!!!!!!!!!!!!!!!!\n", *a5);
        return result;
    }
};
HOOK_DEFINE_TRAMPOLINE(CheckHitSegmentSphereHook) {
    static bool Callback(sead::Vector3f const& a1,sead::Vector3f const& a2,sead::Vector3f const& a3,float a4,sead::Vector3f* a5,sead::Vector3f* a6) {
        bool result = Orig(a1, a2, a3, a4, a5, a6);
        Logger::log("CheckHitSegmentSphere((%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f), %.02f) => %s", a1.x, a1.y, a1.z, a2.x, a2.y, a2.z, a3.x, a3.y, a3.z, a4, result?"true":"false");
        if(a5 != nullptr) Logger::log(" ; (%.02f, %.02f, %.02f)", a5->x, a5->y, a5->z);
        else Logger::log(" ; null");
        if(a6 != nullptr) Logger::log(" ; (%.02f, %.02f, %.02f)\n", a6->x, a6->y, a6->z);
        else Logger::log(" ; null\n");
        return result;
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionShapeInfoArrowCalcRelativeShapeInfoHook) {
    static void Callback(CollisionShapeInfoArrow* self, sead::Matrix34f const& mtx) {
        Orig(self, mtx);
        Logger::log("CollisionShapeInfoArrow::calcRelativeShapeInfo((%.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f, %.02f))\n", mtx.m[0][0], mtx.m[0][1], mtx.m[0][2], mtx.m[0][3], mtx.m[1][0], mtx.m[1][1], mtx.m[1][2], mtx.m[1][3], mtx.m[2][0], mtx.m[2][1], mtx.m[2][2], mtx.m[2][3]);
        sead::Vector3f vec2 = *((sead::Vector3f*)(((u8*)self)+100));
        Logger::log("vec2: (%.02f, %.02f, %.02f)\n", vec2.x, vec2.y, vec2.z);
        sead::Vector3f vec5 = *((sead::Vector3f*)(((u8*)self)+136));
        Logger::log("vec5: (%.02f, %.02f, %.02f)\n", vec5.x, vec5.y, vec5.z);
        sead::Vector3f vec4 = *((sead::Vector3f*)(((u8*)self)+124));
        Logger::log("vec4: (%.02f, %.02f, %.02f)\n", vec4.x, vec4.y, vec4.z);
        sead::Vector3f vec6 = *((sead::Vector3f*)(((u8*)self)+148));
        Logger::log("vec6: (%.02f, %.02f, %.02f)\n", vec6.x, vec6.y, vec6.z);
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerColliderCalcResultVecArrowHook) {
    static void Callback(PlayerCollider* self, sead::BitFlag<uint> *a1,sead::Vector3<float> *a2,sead::Vector3<float> *a3,sead::Vector3<float> *a4,sead::Vector3<float> *a5,CollidedShapeResult const* a6) {
        Orig(self, a1, a2, a3, a4, a5, a6);
        Logger::log("calcResultVecArrow: (%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f)\n", a2->x, a2->y, a2->z, a3->x, a3->y, a3->z, a4->x, a4->y, a4->z, a5->x, a5->y, a5->z);
    
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerColliderCalcResultVecSphereHook) {
    static void Callback(PlayerCollider* self, sead::BitFlag<uint> *a1,sead::Vector3<float> *a2,sead::Vector3<float> *a3,sead::Vector3<float> *a4,sead::Vector3<float> *a5,CollidedShapeResult const* a6) {
        Orig(self, a1, a2, a3, a4, a5, a6);
        Logger::log("calcResultVecSphere: (%.020f, %.020f, %.020f), (%.020f, %.020f, %.020f), (%.020f, %.020f, %.020f), (%.020f, %.020f, %.020f)\n", a2->x, a2->y, a2->z, a3->x, a3->y, a3->z, a4->x, a4->y, a4->z, a5->x, a5->y, a5->z);
    
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionMultiShapeCallbackFromServerHook) {
    static void Callback(CollisionMultiShape *self, al::KCPrismData *data, al::KCPrismHeader *header) {
        Logger::log("CollisionMultiShape::callbackFromServer::START\n");
        Orig(self, data, header);
        Logger::log("CollisionMultiShape::callbackFromServer::END\n");
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionShapeKeeperRegisterCollideResult) {
    static void Callback(CollisionShapeKeeper* self, const CollidedShapeResult& result) {
        Logger::log("CollisionShapeKeeper::registerCollideResult(type=%d)\n", *(((int*)&result)+4));
        Orig(self, result);
    }
};
HOOK_DEFINE_TRAMPOLINE(CollisionShapeKeeperRegisterCollideSupportResult) {
    static void Callback(CollisionShapeKeeper* self, const CollidedShapeResult& result) {
        Logger::log("CollisionShapeKeeper::registerCollideSupportResult(type=%d)\n", *(((int*)&result)+4));
        Orig(self, result);
    }
};
HOOK_DEFINE_TRAMPOLINE(KCollisionServerSearchPrismMinMaxHook) {
    static void Callback(al::KCollisionServer* self, const sead::Vector3f& a2, const sead::Vector3f& a3, sead::IDelegate2<const al::KCPrismData*, const al::KCPrismHeader*>& a4) {
        Logger::log("KCollisionServer::searchPrismMinMax((%.02f, %.02f, %.02f), (%.02f, %.02f, %.02f), ...)\n", a2.x, a2.y, a2.z, a3.x, a3.y, a3.z);
        Orig(self, a2, a3, a4);
    }
};

HOOK_DEFINE_INLINE(PlayerColliderMoveCollideProgressHook) {
    static void Callback(exl::hook::InlineCtx* ctx) {
        al::SpherePoseInterpolator* spi = (al::SpherePoseInterpolator*)ctx->X[0];
        Logger::log("before calcResultVec: progress=%f\n", *((f32*) (uintptr_t(spi)+68)));
    }
};
HOOK_DEFINE_INLINE(PlayerColliderCalcResultVecAfterSpecificHook) {
    static void Callback(exl::hook::InlineCtx* ctx) {
        int i = ctx->W[26];
        int mNumCollideResult = ctx->W[19];
        CollidedShapeResult* v42 = (CollidedShapeResult*)ctx->X[27];
        const sead::Vector3f& v136 = (const sead::Vector3f&)ctx->X[2];
        const sead::Vector3f& v137 = (const sead::Vector3f&)ctx->X[3];
        const sead::Vector3f& v138 = (const sead::Vector3f&)ctx->X[4];
        const sead::Vector3f& v139 = (const sead::Vector3f&)ctx->X[5];

        Logger::log("after %d/%d (%s): v136=(%.02f, %.02f, %.02f), v137=(%.02f, %.02f, %.02f), v138=(%.02f, %.02f, %.02f), v139=(%.02f, %.02f, %.02f)\n",
            i+1, mNumCollideResult, v42->isArrow() ? "arrow" : v42->isSphere() ? "sphere" : "disk", v136.x, v136.y, v136.z, v137.x, v137.y, v137.z, v138.x, v138.y, v138.z, v139.x, v139.y, v139.z);
  
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerColliderGroundArrowAverageHook) {
    static void Callback(PlayerCollider* self, bool * a2,sead::Vector3f * a3,bool * a4,sead::Vector3f * a5,CollisionShapeKeeper const* a6){
        Orig(self, a2, a3, a4, a5, a6);
        Logger::log("Result of GroundArrowAverage: %s=(%f, %f, %f), %s=(%f, %f, %f)\n", *a2 ? "true" : "false", a3->x, a3->y, a3->z, *a4 ? "true" : "false", a5->x, a5->y, a5->z);
    }
};

HOOK_DEFINE_TRAMPOLINE(CollidedShapeResultSetArrowHitInfoHook) {
    static void Callback(CollidedShapeResult* self, const al::ArrowHitInfo& hitInfo) {
        Logger::log("CollidedShapeResult::setArrowHitInfo{tri.index=%d, unk=%f, mCollisionHitPos=(%f, %f, %f), unk3=(%f, %f, %f), mCollisionMovingReaction=(%f, %f, %f), mCollisionLocation=%d}\n",
           hitInfo.mTriangle.mKCPPrismData->mTriIndex, hitInfo.unk, hitInfo.mCollisionHitPos.x, hitInfo.mCollisionHitPos.y, hitInfo.mCollisionHitPos.z,
           hitInfo.unk3.x, hitInfo.unk3.y, hitInfo.unk3.z, hitInfo.mCollisionMovingReaction.x, hitInfo.mCollisionMovingReaction.y, hitInfo.mCollisionMovingReaction.z, hitInfo.mCollisionLocation);
        Orig(self, hitInfo);
    }
};
HOOK_DEFINE_TRAMPOLINE(CollidedShapeResultSetSphereHitInfoHook) {
    static void Callback(CollidedShapeResult* self, const al::SphereHitInfo& hitInfo) {
        Logger::log("CollidedShapeResult::setSphereHitInfo{tri.index=%d, unk=%f, mCollisionHitPos=(%f, %f, %f), unk3=(%f, %f, %f), mCollisionMovingReaction=(%f, %f, %f), mCollisionLocation=%d}\n",
           hitInfo.mTriangle.mKCPPrismData->mTriIndex, hitInfo.unk, hitInfo.mCollisionHitPos.x, hitInfo.mCollisionHitPos.y, hitInfo.mCollisionHitPos.z,
           hitInfo.unk3.x, hitInfo.unk3.y, hitInfo.unk3.z, hitInfo.mCollisionMovingReaction.x, hitInfo.mCollisionMovingReaction.y, hitInfo.mCollisionMovingReaction.z, hitInfo.mCollisionLocation);
        Orig(self, hitInfo);
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerJudgeSpeedCheckFallJudgeHook) {
    static bool Callback(PlayerJudgeSpeedCheckFall* self) {
        return false;
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerCounterForceRunCtorHook) {
    static void Callback(PlayerCounterForceRun* self) {
        self->_0 = 3;  // non-zero value
        self->_4 = 38.0f;  // value of rocket flower
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerCounterForceRunUpdateHook) {
    static void Callback(PlayerCounterForceRun* self) {
        // do nothing, especially not decrease _0
    }
};
HOOK_DEFINE_TRAMPOLINE(PlayerContinuousLongJumpUpdateHook) {
    static void Callback(PlayerContinuousLongJump* self) {
        (*(int*)(uintptr_t(self)+8)) = ((*(int*)(uintptr_t(self)+8)) + 1) % 9;
        // do nothing, especially not decrease _0
    }
};

HOOK_DEFINE_TRAMPOLINE(PlayerCounterAfterUpperPunchUpdateHook) {
    static void Callback(PlayerCounterAfterUpperPunch* self) {
        Orig(self);
        Logger::log("PlayerCounterAfterUpperPunch::update: %d\n", *(int*)self);
    }
};

HOOK_DEFINE_TRAMPOLINE(PlayerTriggerIsOnUpperPunchHit) {
    static bool Callback(PlayerTrigger* self) {
        bool result = Orig(self);
        Logger::log("PlayerTrigger::isOnUpperPunchHit: %s ; collision=%d at %p\n", result ? "true" : "false", *(int*)self, self);
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(StartHitReactionHipDropLandHook) {
    static void Callback(const al::LiveActor *self, bool isLandingWeak) {
        static bool replacement = false;
        replacement = !replacement;
        Logger::log("StartHitReactionHipDropLand(%s => )\n", isLandingWeak ? "true" : "false", replacement ? "true" : "false");
        Orig(self, replacement);
    }
};

HOOK_DEFINE_TRAMPOLINE(PlayerJudgeWallCatchUpdate) {
    static void Callback(PlayerJudgeWallCatch* self) {
        Orig(self);
        Logger::log("PlayerJudgeWallCatch::update resulted in %s\n", self->mIsJudge);
    }
};

HOOK_DEFINE_TRAMPOLINE(PlayerAnimatorStartAnimDead) {
    static void Callback(PlayerAnimator* self) {
        static int i=0;
        const char* arr[] = {"Dead01", "Dead02", "Dead03", "Dead04"};
        self->startAnim(arr[i % 4]);
        i++;
    }
};

HOOK_DEFINE_TRAMPOLINE(PlayerActorHakoniwaAttackSensorHook) {
    static void Callback(PlayerActorHakoniwa* self, al::HitSensor* sensor1, al::HitSensor* sensor2) {
        Logger::log("PlayerActorHakoniwa::attackSensor(%p=%s, %p=%s)\n", sensor1, sensor1->mName, sensor2, sensor2->mName);
        Orig(self, sensor1, sensor2);
    }
};
HOOK_DEFINE_TRAMPOLINE(sendMsgPlayerCapHipDropHook) {
    static bool Callback(al::HitSensor* sensor1, al::HitSensor* sensor2) {
        bool result = Orig(sensor1, sensor2);
        Logger::log("rs::sendMsgPlayerCapHipDrop(%p=%s, %p=%s), %s\n", sensor1, sensor1->mName, sensor2, sensor2->mName, result ? "true" : "false");
        return result;
    }
};
HOOK_DEFINE_TRAMPOLINE(sendMsgPlayerCapTrampleHook) {
    static bool Callback(al::HitSensor* sensor1, al::HitSensor* sensor2) {
        bool result = Orig(sensor1, sensor2);
        Logger::log("rs::sendMsgPlayerCapTrampleHook(%p=%s, %p=%s), %s\n", sensor1, sensor1->mName, sensor2, sensor2->mName, result ? "true" : "false");
        return result;
    }
};
class FilterFly : public al::LayoutActor {
public:
    sead::Vector2f mTargetPos = sead::Vector2f::zero;
    sead::Vector2f mVelocity = sead::Vector2f::zero;
};
HOOK_DEFINE_TRAMPOLINE(FilterFlyExeMoveHook) {
    static void Callback(FilterFly* self) {
        Orig(self);
        Logger::log("FilterFly::exeMove: mTargetPos=(%f, %f), mVelocity=(%f, %f)\n", self->mTargetPos.x, self->mTargetPos.y, self->mVelocity.x, self->mVelocity.y);
    }
};
HOOK_DEFINE_TRAMPOLINE(FilterFlyExeWaitHook) {
    static void Callback(FilterFly* self) {
        Orig(self);
        Logger::log("FilterFly::exeWait: mTargetPos=(%f, %f), mVelocity=(%f, %f)\n", self->mTargetPos.x, self->mTargetPos.y, self->mVelocity.x, self->mVelocity.y);
    }
};
HOOK_DEFINE_INLINE(FilterFlyEnableHook) {
    static void Callback(exl::hook::InlineCtx* ctx) {
        ctx->X[0] = 0;
    }
};
namespace al {
    const sead::Vector3f& getSensorPos(const al::HitSensor* sensor);
    const sead::Vector3f& getGravity(const al::LiveActor* actor);
    bool tryNormalizeOrZero(sead::Vector3f* vec);
}
HOOK_DEFINE_TRAMPOLINE(isEnableSendTrampleMsgHook) {
    static bool Callback(al::LiveActor* self, al::HitSensor* sensor1, al::HitSensor* sensor2) {
        bool result = Orig(self, sensor1, sensor2);
        Logger::log("rs::isEnableSendTrampleMsgHook(%p=%s, %p=%s), %s\n", sensor1, sensor1->mName, sensor2, sensor2->mName, result ? "true" : "false");
        sead::Vector3f sensorPos1 = al::getSensorPos(sensor1);
        sead::Vector3f sensorPos2 = al::getSensorPos(sensor2);
        Logger::log("sensorPos1: (%f, %f, %f), sensorPos2: (%f, %f, %f)\n", sensorPos1.x, sensorPos1.y, sensorPos1.z, sensorPos2.x, sensorPos2.y, sensorPos2.z);
        sead::Vector3f difference = sensorPos1 - sensorPos2;
        al::tryNormalizeOrZero(&difference);
        Logger::log("normDifference: (%f, %f, %f)\n", difference.x, difference.y, difference.z);
        sead::Vector3f gravity = al::getGravity(self);
        Logger::log("gravity: (%f, %f, %f)\n", gravity.x, gravity.y, gravity.z);
        f32 dotP = gravity.dot(difference);
        Logger::log("dotP: %f\n", dotP);
        Logger::log("-dotP = %f > 0.34202 ? %s\n", -dotP, (-dotP > 0.34202) ? "true" : "false");
        return result;
    }
};

class PlayerJointControlGroundPose;
HOOK_DEFINE_TRAMPOLINE(PlayerJointControlGroundPoseUpdateHook) {
    static void Callback(PlayerJointControlGroundPose* self, f32 s0, f32 s1, f32 s2, f32 s3, bool a6) {
        Orig(self, s0, s1, s2, s3, a6);
        Logger::log("PlayerJointControlGroundPose::update: s0=%f, s1=%f, s2=%f, s3=%f, a6=%s\n", s0, s1, s2, s3, a6 ? "true" : "false");
    }
};

HOOK_DEFINE_REPLACE(EmptyForWaitGPUDone) {
    static void Callback() {
        //Logger::log("EmptyForWaitGPUDone\n");
    }
};
HOOK_DEFINE_REPLACE(HideModelIfShowDisable) {
    static void Callback() {
    }
};

HOOK_DEFINE_TRAMPOLINE(ExpHeapCtor) {
    static void Callback(sead::SafeStringBase<char> const& a1,sead::Heap * a2,void *a3,ulong a4,sead::Heap::HeapDirection a5,bool a6) {
        numExpHeap++;
        Orig(a1, a2, a3, a4, a5, a6);
    }
};

HOOK_DEFINE_TRAMPOLINE(FrameHeapCtor) {
    static void Callback(sead::SafeStringBase<char> const& a1,sead::Heap * a2,void *a3,ulong a4,sead::Heap::HeapDirection a5,bool a6) {
        numFrameHeap++;
        Orig(a1, a2, a3, a4, a5, a6);
    }
};

HOOK_DEFINE_TRAMPOLINE(SeparateHeapCtor) {
    static void Callback(sead::SafeStringBase<char> const& a1,sead::Heap * a2,void *a3,ulong a4,void *a5,ulong a6,bool a7) {
        numSeparateHeap++;
        Orig(a1, a2, a3, a4, a5, a6, a7);
    }
};

HOOK_DEFINE_TRAMPOLINE(UnitHeapCtor) {
    static void Callback(sead::SafeStringBase<char> const& a1,sead::Heap * a2,void *a3,ulong a4,uint a5,bool a6) {
        numUnitHeap++;
        Orig(a1, a2, a3, a4, a5, a6);
    }
};

HOOK_DEFINE_REPLACE(EnableSeeOddSpace) {
    static bool Callback(const al::LiveActor*) {
        return true;
    }
};

#define LogRegister(name, reg, format) HOOK_DEFINE_INLINE(Log##name) { \
    static void Callback(exl::hook::InlineCtx* ctx) { \
        Logger::log(#name "=" #format "\n", ctx->reg); \
    } \
}

LogRegister(S0,  S[0],  %f);
LogRegister(S1,  S[1],  %f);
LogRegister(S2,  S[2],  %f);
LogRegister(S3,  S[3],  %f);
LogRegister(S4,  S[4],  %f);
LogRegister(S5,  S[5],  %f);
LogRegister(S6,  S[6],  %f);
LogRegister(S7,  S[7],  %f);
LogRegister(S8,  S[8],  %f);
LogRegister(S9,  S[9],  %f);
LogRegister(S10, S[10], %f);
LogRegister(S12, S[12], %f);
LogRegister(S13, S[13], %f);
LogRegister(S14, S[14], %f);

void exlSetupResearchHooks() {
    //exlSetupPracticeTASHooks();

    //UpdateFrameHook::InstallAtSymbol("_ZN19PlayerActorHakoniwa8movementEv");
    //SpherePoseInterpolatorStartHook::InstallAtSymbol("_ZN2al22SpherePoseInterpolator11startInterpERKN4sead7Vector3IfEES5_ffRKNS1_4QuatIfEES9_f");
    //SpherePoseInterpolatorCalcInterpHook::InstallAtSymbol("_ZNK2al22SpherePoseInterpolator10calcInterpEPN4sead7Vector3IfEEPfPNS1_4QuatIfEES4_");
    //PlayerColliderCollideHook::InstallAtSymbol("_ZN14PlayerCollider7collideERKN4sead7Vector3IfEE");
    //PlayerColliderMoveCollideHook::InstallAtSymbol("_ZN14PlayerCollider11moveCollideEPN4sead7Vector3IfEEPfPNS0_4QuatIfEERKS2_fRKS6_S9_fb");
    //CollisionShapeKeeperUpdateShapeHook::InstallAtSymbol("_ZN20CollisionShapeKeeper11updateShapeEv");
    //CollisionShapeInfoArrowCtorHook::InstallAtSymbol("_ZN23CollisionShapeInfoArrowC2EPKcRKN4sead7Vector3IfEES6_fi");
    //CollisionShapeInfoArrowGetBoundingCenterHook::InstallAtSymbol("_ZNK23CollisionShapeInfoArrow17getBoundingCenterEv");
    //CollisionMultiShapeCheckHook::InstallAtSymbol("_ZN19CollisionMultiShape5checkEP20CollisionShapeKeeperPKN4sead8Matrix34IfEEfRKNS2_7Vector3IfEEPKN2al24CollisionPartsFilterBaseE");
    //KCHitSphereForPlayerHook::InstallAtSymbol("_ZN2al16KCollisionServer20KCHitSphereForPlayerEPKNS_11KCPrismDataEPKNS_13KCPrismHeaderEPKN4sead7Vector3IfEEffPfPh");
    //PlayerColliderCalcResultVec::InstallAtSymbol("_ZN14PlayerCollider13calcResultVecEPN4sead7Vector3IfEES3_RKS2_");
    //LogS1::InstallAtOffset(0x421314);
    //LogS4::InstallAtOffset(0x421360);
    //LogS0::InstallAtOffset(0x3F7080);
    //LogS0::InstallAtOffset(0x3F70C8);
    //LogS8::InstallAtOffset(0x3F70D0);
    //LogS0::InstallAtOffset(0x431F8C);
    //LogS1::InstallAtOffset(0x431FA0);
    //KCHitArrowHook::InstallAtSymbol("_ZNK2al16KCollisionServer10KCHitArrowEPKNS_11KCPrismDataEPKNS_13KCPrismHeaderERKN4sead7Vector3IfEESB_PfPh");
    //CheckHitSegmentSphereHook::InstallAtSymbol("_ZN2al21checkHitSegmentSphereERKN4sead7Vector3IfEES4_S4_fPS2_S5_");
    //CollisionShapeInfoArrowCalcRelativeShapeInfoHook::InstallAtSymbol("_ZN23CollisionShapeInfoArrow21calcRelativeShapeInfoERKN4sead8Matrix34IfEE");
    //PlayerColliderCalcResultVecArrowHook::InstallAtSymbol("_ZN14PlayerCollider18calcResultVecArrowEPN4sead7BitFlagIjEEPNS0_7Vector3IfEES6_S6_S6_PK19CollidedShapeResult");
    //PlayerColliderCalcResultVecSphereHook::InstallAtSymbol("_ZN14PlayerCollider19calcResultVecSphereEPN4sead7BitFlagIjEEPNS0_7Vector3IfEES6_S6_S6_PK19CollidedShapeResult");
    //CollisionMultiShapeCallbackFromServerHook::InstallAtSymbol("_ZN19CollisionMultiShape18callbackFromServerEPKN2al11KCPrismDataEPKNS0_13KCPrismHeaderE");
    //CollisionShapeKeeperRegisterCollideResult::InstallAtSymbol("_ZN20CollisionShapeKeeper21registerCollideResultERK19CollidedShapeResult");
    //CollisionShapeKeeperRegisterCollideSupportResult::InstallAtSymbol("_ZN20CollisionShapeKeeper28registerCollideSupportResultERK19CollidedShapeResult");
    //KCollisionServerSearchPrismMinMaxHook::InstallAtSymbol("_ZN2al16KCollisionServer17searchPrismMinMaxERKN4sead7Vector3IfEES5_RNS1_10IDelegate2IPKNS_11KCPrismDataEPKNS_13KCPrismHeaderEEE");

    //LogS1::InstallAtOffset(0x3F72D4); // sphere
    //LogS2::InstallAtOffset(0x3F7604); // disk
    //LogS3::InstallAtOffset(0x3F702C); // arrow

    // wonder run badge mod!
    //PlayerJudgeSpeedCheckFallJudgeHook::InstallAtSymbol("_ZNK25PlayerJudgeSpeedCheckFall5judgeEv");
    //PlayerCounterForceRunCtorHook::InstallAtSymbol("_ZN21PlayerCounterForceRunC2Ev");
    //PlayerCounterForceRunUpdateHook::InstallAtSymbol("_ZN21PlayerCounterForceRun6updateEv");
    //PlayerContinuousLongJumpUpdateHook::InstallAtSymbol("_ZN24PlayerContinuousLongJump6updateEv");


    //PlayerColliderMoveCollideProgressHook::InstallAtOffset(0x430B38);
    //PlayerColliderCalcResultVecAfterSpecificHook::InstallAtOffset(0x4312E4);
    //PlayerColliderGroundArrowAverageHook::InstallAtSymbol("_ZN14PlayerCollider22calcGroundArrowAverageEPbPN4sead7Vector3IfEES0_S4_PK20CollisionShapeKeeper");
    //CollidedShapeResultSetArrowHitInfoHook::InstallAtSymbol("_ZN19CollidedShapeResult15setArrowHitInfoERKN2al12ArrowHitInfoE");
    //CollidedShapeResultSetSphereHitInfoHook::InstallAtSymbol("_ZN19CollidedShapeResult16setSphereHitInfoERKN2al13SphereHitInfoE");

    //PlayerJointControlGroundPoseUpdateHook::InstallAtSymbol("_ZN28PlayerJointControlGroundPose6updateEffffb");

    //PlayerCounterAfterUpperPunchUpdateHook::InstallAtSymbol("_ZN28PlayerCounterAfterUpperPunch6updateEPK13PlayerTrigger");
    //PlayerTriggerIsOnUpperPunchHit::InstallAtSymbol("_ZNK13PlayerTrigger17isOnUpperPunchHitEv");

    //StartHitReactionHipDropLandHook::InstallAtSymbol("_ZN2rs27startHitReactionHipDropLandEPN2al9LiveActorEb");

    //PlayerJudgeWallCatchUpdate::InstallAtSymbol("_ZN20PlayerJudgeWallCatch6updateEv");

    //PlayerAnimatorStartAnimDead::InstallAtSymbol("_ZN14PlayerAnimator13startAnimDeadEv");

    //PlayerActorHakoniwaAttackSensorHook::InstallAtSymbol("_ZN19PlayerActorHakoniwa12attackSensorEPN2al9HitSensorES2_");
    //sendMsgPlayerCapHipDropHook::InstallAtSymbol("_ZN2rs23sendMsgPlayerCapHipDropEPN2al9HitSensorES2_");
    //sendMsgPlayerCapTrampleHook::InstallAtSymbol("_ZN2rs23sendMsgPlayerCapTrampleEPN2al9HitSensorES2_");
    //isEnableSendTrampleMsgHook::InstallAtSymbol("_ZN2rs22isEnableSendTrampleMsgEPKN2al9LiveActorEPNS0_9HitSensorES5_");
    //FilterFlyExeMoveHook::InstallAtSymbol("_ZN9FilterFly7exeMoveEv");
    //FilterFlyExeWaitHook::InstallAtSymbol("_ZN9FilterFly7exeWaitEv");
    //FilterFlyEnableHook::InstallAtOffset(0x287D98);

    /*exl::patch::CodePatcher p("_ZN2al15GameFrameworkNx10procFrame_Ev", 0x1bc);
    namespace inst = exl::armv8::inst;
    p.WriteInst(inst::Nop());
    p.Seek(0x008A6EA4);
    for(int i=0; i<10; i++) {
        p.WriteInst(inst::Nop());
    }

    EmptyForWaitGPUDone::InstallAtSymbol("_ZN4sead15GameFrameworkNx15waitForGpuDone_Ev");
    */

    //GameSystemMovement::InstallAtSymbol("_ZN10GameSystem8movementEv");

    //HideModelIfShowDisable::InstallAtSymbol("_ZN2al15hideModelIfShowEPNS_9LiveActorE");

    //_ZN4sead7ExpHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmNS5_13HeapDirectionEb	.text	000000710073F724	00000094	00000020		R	.	.	.	.	B	T	.
    //_ZN4sead9FrameHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmNS5_13HeapDirectionEb	.text	0000007100742C1C	00000040	00000020		R	.	.	.	.	B	T	.
    //_ZN4sead12SeparateHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmS7_mb	.text	0000007100B5B6D8	00000118	00000030		R	.	.	.	.	B	T	.
    //_ZN4sead8UnitHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmjb	.text	0000007100B5C96C	00000054	00000020		R	.	.	.	.	B	T	.
    //ExpHeapCtor::InstallAtSymbol("_ZN4sead7ExpHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmNS5_13HeapDirectionEb");
    //FrameHeapCtor::InstallAtSymbol("_ZN4sead9FrameHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmNS5_13HeapDirectionEb");
    //SeparateHeapCtor::InstallAtSymbol("_ZN4sead12SeparateHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmS7_mb");
    //UnitHeapCtor::InstallAtSymbol("_ZN4sead8UnitHeapC2ERKNS_14SafeStringBaseIcEEPNS_4HeapEPvmjb");

    EnableSeeOddSpace::InstallAtSymbol("_ZN2rs27isPlayerEnableToSeeOddSpaceEPKN2al9LiveActorE");

    //exl::patch::CodePatcher p(0x00A6B304);
    //namespace inst = exl::armv8::inst;
    //p.BranchLinkInst(reinterpret_cast<void*>(&sead::ClonableFrameHeap::refCreate));

    // fix signals in Metro turning off when receiving explosion damage
    //p.Seek(0x0024C644);
    //p.BranchInst(0x0090D808);
}
