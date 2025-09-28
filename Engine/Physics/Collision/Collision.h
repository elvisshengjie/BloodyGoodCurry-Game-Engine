#pragma once
#ifndef COLLISION_H
#define COLLISION_H
#include "Math/Vector_2D.h"

namespace Framework {

    struct AABB
    {
        Vector2D<float> min;
        Vector2D<float> max;

        AABB(float x, float y, float width, float height) : 
            min(x, y), max(x + width, y + height) {}
    };

    struct Circle
    {
        Vector2D<float> center;
        float radius;

        Circle(float x, float y, float rad) : center(x, y), radius(rad) {}
    };

    class Collision
    {
    public:
        // Rect-Rect collision
        static bool CheckCollisionRectToRect(const AABB& a, const AABB& b)
        {
            return (a.min.getX() < b.max.getX() && a.max.getX() > b.min.getX() &&
                    a.min.getY() < b.max.getY() && a.max.getY() > b.min.getY());
        }

        // Circle-Rect collision
        static bool CheckCollisionRectToCircle(const Circle& c, const AABB& r)
        {
            float closestX = std::max(r.min.getX(), std::min(c.center.getX(), r.max.getX()));
            float closestY = std::max(r.min.getY(), std::min(c.center.getY(), r.max.getY()));

            float dx = c.center.getX() - closestX;
            float dy = c.center.getY() - closestY;

            return (dx * dx + dy * dy) < (c.radius * c.radius);
        }

        // Static collision check
        //bool CollisionIntersection_RectToRect_Static(const AABB& aabb1, const AABB& aabb2); 
        
        // Dynamic Collision check
        //bool CollisionIntersection_RectToRect_Dynamic(const AABB& aabb1, const AEVec2& vel1, const AABB& aabb2, const AEVec2& vel2, float& firstTimeOfCollision);
        //Change AEVec2 to our own math or velocity thing
    };

} // namespace Framework
#endif //COLLISION_H
