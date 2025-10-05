#include "Engine/Physics/Physics.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#include "Core/Config.h"
#include "Core/Debug.h"

#include <varargs.h>

namespace TombForge
{
    namespace
    {
        constexpr uint32_t MaxTmpAllocSize = 10 * 1024 * 1024; // 10 MB

        void TraceImpl(const char* inFMT, ...)
        {
            va_list list;
            va_start(list, inFMT);
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), inFMT, list);
            va_end(list);

            LOG_ERROR("Jolt Trace: %s", buffer);
        }

#ifdef JPH_ENABLE_ASSERTS
        bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
        {
            LOG_ERROR("Jolt Assert Failed: %s, %s, %s, %i", inExpression, inMessage, inFile, inLine);
            return false; // Return false to stop breaking into the debugger
        };
#endif
    }

    bool InitPhysics(PhysicsContext& ctx)
    {
        const Config& config = Config::Get();

        JPH::RegisterDefaultAllocator();

        ctx.system = new JPH::PhysicsSystem();

        JPH::Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        ctx.tmpAllocator = new JPH::TempAllocatorImpl(MaxTmpAllocSize);

        // Note: replace this with own implementation if ever implementing a job system
        ctx.jobSystem = new JPH::JobSystemThreadPool(
            JPH::cMaxPhysicsJobs, 
            JPH::cMaxPhysicsBarriers, 
            std::thread::hardware_concurrency() - 1);

        ctx.system->Init(config.maxPhysicsBodies,
            config.numPhysicsBodyMutexes,
            config.maxPhysicsBodyPairs,
            config.maxPhysicsContactConstraints,
            ctx.bpLayerInterface,
            ctx.objVsBpLayerFilter,
            ctx.objVsObjLayerFilter);

        return true;
    }

    void DestroyPhysics(PhysicsContext& ctx)
    {
        delete ctx.system;
        ctx.system = nullptr;

        delete ctx.jobSystem;
        ctx.jobSystem = nullptr;

        delete ctx.tmpAllocator;
        ctx.tmpAllocator = nullptr;

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        JPH::UnregisterTypes();
    }
}
