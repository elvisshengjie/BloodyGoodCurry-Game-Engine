/*********************************************************************************************
 \file      UndoStack.h
 \brief     Lightweight editor undo system for transforms and object create/delete.
*********************************************************************************************/

#pragma once

#include <cstddef>
#include "Factory/Factory.h"   // brings in Framework::GOC, Framework::GOCId, Framework::json

namespace mygame
{
    namespace editor
    {
        /**
         * \brief Minimal snapshot of an object's spatial data used for cheap transform undo.
         */
        struct TransformSnapshot
        {
            bool  hasTransform = false;
            float x = 0.0f;
            float y = 0.0f;
            float rot = 0.0f;

            bool  hasRect = false;
            float width = 1.0f;
            float height = 1.0f;

            bool  hasCircle = false;
            float radius = 0.0f;

            // NEW: Add color storage to restore visual state
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float a = 1.0f;
        };

        /**
         * \brief Capture the current transform/render/circle data from an object.
         */
        TransformSnapshot CaptureTransformSnapshot(const Framework::GOC& object);

        /**
         * \brief Record a before/after transform change into the undo stack.
         */
        void RecordTransformChange(const Framework::GOC& object,
            const TransformSnapshot& before);

        /**
         * \brief Record that an object was created (so undo will delete it).
         */
        void RecordObjectCreated(const Framework::GOC& object);

        /**
         * \brief Record that an object is about to be deleted (so undo will resurrect it).
         */
        void RecordObjectDeleted(const Framework::GOC& object);

        /**
         * \brief Undo the most recent recorded editor action.
         */
        bool UndoLastAction();

        /**
         * \brief Query if there is at least one undoable action.
         */
        bool CanUndo();

        /**
         * \brief Current number of stored undo steps.
         */
        std::size_t StackDepth();

        /**
         * \brief Maximum number of undo steps retained.
         */
        std::size_t StackCapacity();
    }
}