#pragma once
#ifndef COLLISION_H
#define COLLISION_H
#include "Math/Vector_2D.h"
#include "Collide_Elements.h"

namespace Framework {

    struct AABB
    {
        Vector2D<float> min;
        Vector2D<float> max;

        AABB(float x, float y, float width, float height) : 
            min(x, y), max(x + width, y + height) {}
    };

    class Collision
    {
    public:
        // Overlap test
        static bool CheckCollision(const AABB& a, const AABB& b);

        // Static collision check
        bool CollisionIntersection_RectToRect_Static(const AABB& aabb1, const AABB& aabb2); 
        
        // Dynamic Collision check
        //bool CollisionIntersection_RectToRect_Dynamic(const AABB& aabb1, const AEVec2& vel1, const AABB& aabb2, const AEVec2& vel2, float& firstTimeOfCollision);
        //Change AEVec2 to our own math or velocity thing
    };

} // namespace Framework
#endif //COLLISION_H
