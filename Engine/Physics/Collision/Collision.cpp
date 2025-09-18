#include "Collision.h"
#include <cmath>
#include <algorithm> 

namespace Framework {

    // Function to check if the distance between two points is less than the threshold
    bool CheckPointPointDistance(const Point& point1, const Point& point2, float threshold) 
    {
        // Extract the Vector2D positions from the Point structures
        const Vector2D<float>& pos1 = point1.GetPosition(); // Position of point1
        const Vector2D<float>& pos2 = point2.GetPosition(); // Position of point2

        // Calculate the distance between the two points
        float dx = pos1.getX() - pos2.getX(); // Change in x
        float dy = pos1.getY() - pos2.getY(); // Change in y
        float distanceSquared = dx * dx + dy * dy; 

        return distanceSquared <= threshold * threshold; // Compare squared distances
    }

    // Function to check if two rectangles overlap
    bool CheckOverlapRectRect(const AABB& aabb1, const AABB& aabb2) 
    {
    
        // Check for overlap in the x-axis
        if (aabb1.getMax().getX() < aabb2.getMin().getX() || aabb1.getMin().getX()> aabb2.getMax().getX())
            return false;

        // Check for overlap in the y-axis
        if (aabb1.getMax().getY() < aabb2.getMin().getY() || aabb1.getMin().getY() > aabb2.getMax().getY())
            return false;
        return true;
    }
    // Function to check if two rectangles collide, taking their velocities into account
 // Function to check if two rectangles collide, taking their velocities into account
    bool CollisionIntersection_RectRect(
        const AABB& aabb1, const Vector2D<float>& vel1,
        const AABB& aabb2, const Vector2D<float>& vel2,
        float& firstTimeOfCollision) {



        if (CheckOverlapRectRect(aabb1, aabb2))
            return true;


        // Initialize and calculate the new velocity of Vb
        float tFirst = 0.0f;
        float tLast = firstTimeOfCollision;

        // Work with 1st dimension (x-axis)
        float vRelX = vel2.getX() - vel1.getX();

        if (vRelX < 0.0f)
        {
            if (aabb1.getMin().getX() > aabb2.getMax().getX())// Case 1
                return false;

            if (aabb1.getMax().getX() < aabb2.getMin().getX()) // Case 4
            {
                //dlast
                tFirst = std::max((aabb1.getMax().getX() - aabb2.getMin().getX()) / vRelX, tFirst);
            }
            if (aabb1.getMin().getX() < aabb2.getMax().getX())
            {
                //dfirst
                tLast = std::min((aabb1.getMin().getX() - aabb2.getMax().getX()) / vRelX, tLast);

            }


        }
        else if (vRelX > 0)
        {
            if (aabb1.getMax().getX() < aabb2.getMin().getX()) //Case 3
            {
                return false;
            }
            if (aabb1.getMin().getX() > aabb2.getMax().getX()) //Case2
            {

                tFirst = std::max((aabb1.getMin().getX() - aabb2.getMax().getX()) / vRelX, tFirst);
            }
            if (aabb1.getMax().getX() > aabb2.getMin().getX())
            {
                tLast = std::min((aabb1.getMax().getX() - aabb2.getMin().getX()) / vRelX, tLast);

            }

        }
        else if (vRelX == 0) // Case 5
        {
            if (aabb1.getMax().getX() < aabb2.getMin().getX())
                return false;

            else if (aabb1.getMin().getX() > aabb2.getMax().getX())
            {
                return false;
            }
        }

        if (tFirst > tLast)// Case 6
        {
            return false;
        }

        // Work with 2nd dimension (y-axis)
        float vRelY = vel2.getY() - vel1.getY();
        if (vRelY < 0.0f)
        {
            if (aabb1.getMin().getY() > aabb2.getMax().getY())// Case 1
            {
                return false;
            }

            if (aabb1.getMax().getY() < aabb2.getMin().getY()) // Case 4
            {
                //tFirst = AEMax((aabb1.max.y - aabb2.min.y) / vRelY, tFirst);
                tFirst = std::max((aabb1.getMax().getY() - aabb2.getMin().getY()) / vRelY, tFirst);
            }
            if (aabb1.getMin().getY() < aabb2.getMax().getY())
            {
                tLast = std::min((aabb1.getMin().getY() - aabb2.getMax().getY()) / vRelY, tLast);

            }

        }
        else if (vRelY > 0)
        {
            if (aabb1.getMax().getY() < aabb2.getMin().getY()) //Case 3
            {
                return false;
            }
            if (aabb1.getMin().getY() > aabb2.getMax().getY()) //Case2
            {
                tFirst = std::max((aabb1.getMin().getY() - aabb2.getMax().getY()) / vRelY, tFirst);
            }
            if (aabb1.getMax().getY() < aabb2.getMin().getY())
            {
                tLast = std::min((aabb1.getMax().getY() - aabb2.getMin().getY()) / vRelY, tLast);
            }

        }
        else if (vRelY == 0) // Case 5
        {
            if (aabb1.getMax().getY() < aabb2.getMin().getY())
            {
                return false;
            }
            else if (aabb1.getMin().getY() > aabb2.getMax().getY())
            {
                return false;
            }
        }
        if (tFirst > tLast)// Case 6
        {
            return false;
        }
        return true;
    }

    // Function to check if a point is within a certain distance from a rectangle
    bool CheckPointRecDistance(const Point& point, const AABB& aabb, float threshold) {
        
        const Vector2D<float>& pointPos = point.GetPosition(); // Access the Vector2D inside Point

        // Calculate the closest point on the rectangle to the point
        float closeX = myMax(aabb.getMin().getX(), myMin(pointPos.getX(), aabb.getMax().getX()));
        float closeY = myMax(aabb.getMin().getY(), myMin(pointPos.getY(), aabb.getMax().getY()));

        // Calculate the distance from the point to the closest point on the rectangle
        float dx = pointPos.getX() - closeX;
        float dy = pointPos.getY() - closeY;
        float distance = std::sqrt((dx * dx) + (dy * dy));

        // Return true if the distance is less than the threshold
        return distance < threshold;
    }
} // namespace Framework
