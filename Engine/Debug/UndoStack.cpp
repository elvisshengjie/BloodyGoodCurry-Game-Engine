/*********************************************************************************************
 \file      UndoStack.cpp
 \brief     Implementation of the editor undo system.
*********************************************************************************************/

#include "Debug/UndoStack.h"

#include <vector>

#include "Component/TransformComponent.h"
#include "Component/RenderComponent.h"
#include "Component/CircleRenderComponent.h"
#include "Factory/Factory.h"
#include "Debug/Selection.h"

namespace mygame
{
    namespace editor
    {
        namespace
        {
            enum class UndoKind
            {
                Transform,
                Created,
                Deleted
            };

            struct UndoAction
            {
                UndoKind          kind = UndoKind::Transform;
                Framework::GOCId  objectId = 0;

                // For Transform
                TransformSnapshot before;
                TransformSnapshot after;

                // For Created / Deleted: full object snapshot
                Framework::json   snapshot;
            };

            constexpr std::size_t kMaxUndoDepth = 50;
            std::vector<UndoAction> gUndoStack;

            void PushAction(UndoAction action)
            {
                if (gUndoStack.size() >= kMaxUndoDepth)
                    gUndoStack.erase(gUndoStack.begin());
                gUndoStack.emplace_back(std::move(action));
            }

            void ApplyTransformSnapshot(Framework::GOC& object,
                const TransformSnapshot& state)
            {
                if (state.hasTransform)
                {
                    if (auto* tr = object.GetComponentType<Framework::TransformComponent>(
                        Framework::ComponentTypeId::CT_TransformComponent))
                    {
                        tr->x = state.x;
                        tr->y = state.y;
                        tr->rot = state.rot;
                    }
                }

                if (state.hasRect)
                {
                    if (auto* rc = object.GetComponentType<Framework::RenderComponent>(
                        Framework::ComponentTypeId::CT_RenderComponent))
                    {
                        rc->w = state.width;
                        rc->h = state.height;
                        // NEW: Restore Color
                        rc->r = state.r; rc->g = state.g; rc->b = state.b; rc->a = state.a;
                    }
                }

                if (state.hasCircle)
                {
                    if (auto* cc = object.GetComponentType<Framework::CircleRenderComponent>(
                        Framework::ComponentTypeId::CT_CircleRenderComponent))
                    {
                        cc->radius = state.radius;
                        // NEW: Restore Color
                        cc->r = state.r; cc->g = state.g; cc->b = state.b; cc->a = state.a;
                    }
                }
            }
        } // anonymous namespace

        TransformSnapshot CaptureTransformSnapshot(const Framework::GOC& object)
        {
            TransformSnapshot state;

            if (auto* tr = object.GetComponentType<Framework::TransformComponent>(
                Framework::ComponentTypeId::CT_TransformComponent))
            {
                state.hasTransform = true;
                state.x = tr->x;
                state.y = tr->y;
                state.rot = tr->rot;
            }

            if (auto* rc = object.GetComponentType<Framework::RenderComponent>(
                Framework::ComponentTypeId::CT_RenderComponent))
            {
                state.hasRect = true;
                state.width = rc->w;
                state.height = rc->h;
                // NEW: Capture Color
                state.r = rc->r; state.g = rc->g; state.b = rc->b; state.a = rc->a;
            }

            if (auto* cc = object.GetComponentType<Framework::CircleRenderComponent>(
                Framework::ComponentTypeId::CT_CircleRenderComponent))
            {
                state.hasCircle = true;
                state.radius = cc->radius;
                // NEW: Capture Color (Circle takes precedence if both exist)
                state.r = cc->r; state.g = cc->g; state.b = cc->b; state.a = cc->a;
            }

            return state;
        }

        void RecordTransformChange(const Framework::GOC& object,
            const TransformSnapshot& before)
        {
            if (!Framework::FACTORY)
                return;

            UndoAction action;
            action.kind = UndoKind::Transform;
            action.objectId = object.GetId();
            action.before = before;
            action.after = CaptureTransformSnapshot(object);
            PushAction(std::move(action));
        }

        void RecordObjectCreated(const Framework::GOC& object)
        {
            if (!Framework::FACTORY)
                return;

            UndoAction action;
            action.kind = UndoKind::Created;
            action.objectId = object.GetId();
            action.snapshot = Framework::FACTORY->SnapshotGameObject(object);
            PushAction(std::move(action));
        }

        void RecordObjectDeleted(const Framework::GOC& object)
        {
            if (!Framework::FACTORY)
                return;

            UndoAction action;
            action.kind = UndoKind::Deleted;
            action.objectId = object.GetId();
            action.snapshot = Framework::FACTORY->SnapshotGameObject(object);

            // NEW: Manually capture the exact state (pos, size, color) right now
            action.before = CaptureTransformSnapshot(object);

            PushAction(std::move(action));
        }

        bool UndoLastAction()
        {
            if (!Framework::FACTORY)
                return false;
            if (gUndoStack.empty())
                return false;

            UndoAction action = gUndoStack.back();

            bool requiresFactorySweep = false;
            bool undoApplied = false;

            switch (action.kind)
            {
            case UndoKind::Transform:
            {
                Framework::GOC* obj =
                    Framework::FACTORY->GetObjectWithId(action.objectId);
                if (obj)
                {
                    ApplyTransformSnapshot(*obj, action.before);
                    undoApplied = true;
                }
                break;
            }

            case UndoKind::Created:
            {
                Framework::GOC* obj =
                    Framework::FACTORY->GetObjectWithId(action.objectId);
                if (obj)
                {
                    Framework::FACTORY->Destroy(obj);
                    requiresFactorySweep = true;
                    undoApplied = true;
                }
                break;
            }

            case UndoKind::Deleted:
            {
                // Undo a deletion: resurrect the object from its snapshot.
                if (action.snapshot.is_object())
                {
                    Framework::GOC* restored =
                        Framework::FACTORY->InstantiateFromSnapshot(action.snapshot);

                    if (restored)
                    {
                        mygame::SetSelectedObjectId(restored->GetId());

                        // NEW: Force-apply the state we captured (Pos, Size, Color)
                        // This overwrites any default values the Prefab might have loaded with.
                        ApplyTransformSnapshot(*restored, action.before);

                        undoApplied = true;
                        requiresFactorySweep = true;
                    }
                }
                break;
            }
            }

            if (!undoApplied)
                return false;

            gUndoStack.pop_back();

            if (requiresFactorySweep)
            {
                Framework::FACTORY->Update(0.0f);
            }

            return true;
        }

        bool CanUndo()
        {
            return !gUndoStack.empty();
        }

        std::size_t StackDepth()
        {
            return gUndoStack.size();
        }

        std::size_t StackCapacity()
        {
            return kMaxUndoDepth;
        }
    } // namespace editor
}     // namespace mygame