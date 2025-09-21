#include "Collision.h"
#include <cmath>
#include <algorithm> 

namespace Framework 
{
    // Simple collision check
    bool Collision::CheckCollision(const AABB& a, const AABB& b)
    {
        return (a.minX < b.maxX && a.maxX > b.minX && a.minY < b.maxY && a.maxY > b.minY);
    }

    bool Collision::CollisionIntersection_RectToRect_Static(const AABB& aabb1, const AABB& aabb2)
    {
        // Check the different min and max X Y for collision. Return false on any if there arent any collision
        // If (max X of A < min X of B)
        if (aabb1.maxX < aabb2.minX)
            return false;
        // If (min X of A > max X of B)
        if (aabb1.minX > aabb2.maxX)
            return false;
        // If (max Y of A < min Y of B)
        if (aabb1.maxY < aabb2.minY)
            return false;
        // If (min Y of A > max Y of B)
        if (aabb1.minY > aabb2.maxY)
            return false;

        // If all checks are true, then return true, meaning there is a collision
        return true;
    }

    //bool CollisionIntersection_RectToRect_Dynamic(const AABB& aabb1, const AEVec2& vel1, const AABB& aabb2, const AEVec2& vel2, float& firstTimeOfCollision)
    
} // namespace Framework
