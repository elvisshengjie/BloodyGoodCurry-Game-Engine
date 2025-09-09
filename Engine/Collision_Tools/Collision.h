/*********************************************************************************************
 \file      Collision.h
 \par		Cat Feeders
 \author    Choo Jian Wei (jianwei.c@digipen.edu) 100%


\copyright All content   2024 DigiPen Institute of Technology Singapore. All rights reserved.
**********************************************************************************************/

#pragma once
#ifndef COLLISION_H
#define COLLISION_H
#include "../Math/Vector_2D.h"
#include "Collide_Elements.h"



namespace Framework {


    /**************************************************************************
        \brief Checks if the distance between two points is less than the threshold.
        \param point1 The first point.
        \param point2 The second point.
        \param threshold The maximum allowable distance between the two points.
        \return `true` if the distance between the points is less than the threshold,
                `false` otherwise.
     **************************************************************************/
    bool CheckPointPointDistance(const Point& point1, const Point& point2, float threshold);

    /**************************************************************************
    \brief Checks if two rectangles overlap.
    \param aabb1 The first rectangle, represented as an axis-aligned bounding box (AABB).
    \param aabb2 The second rectangle, represented as an axis-aligned bounding box (AABB).
    \return `true` if the two rectangles overlap, `false` otherwise.
    **************************************************************************/
    bool CheckOverlapRectRect(const AABB& aabb1, const AABB& aabb2);

    /**************************************************************************
    \brief Checks if two rectangles collide, taking their velocities into account.
    \param aabb1 The first rectangle, represented as an axis-aligned bounding box (AABB).
    \param vel1 The velocity vector of the first rectangle.
    \param aabb2 The second rectangle, represented as an axis-aligned bounding box (AABB).
    \param vel2 The velocity vector of the second rectangle.
    \param firstTimeOfCollision Reference to a float where the first time of collision
                                will be stored if a collision is detected.
    \return `true` if a collision is detected, `false` otherwise.
    \details This function calculates the time at which the two rectangles first
             collide, if at all, considering their initial positions and velocities.
    **************************************************************************/
    bool CollisionIntersection_RectRect(
        const AABB& aabb1, const Vector2D<float>& vel1,
        const AABB& aabb2, const Vector2D<float>& vel2,
        float& firstTimeOfCollision);

    /**************************************************************************
    \brief Checks if a point is within a certain distance from a rectangle.
    \param point The point to check.
    \param aabb The rectangle, represented as an axis-aligned bounding box (AABB).
    \param threshold The maximum allowable distance between the point and the rectangle.
    \return `true` if the point is within the specified distance from the rectangle,
            `false` otherwise.
    **************************************************************************/
    bool CheckPointRecDistance(const Point& point, const AABB& aabb, float threshold);

} // namespace Framework
#endif //COLLISION_H
