
#pragma once
#include <cmath>
template <typename T = float>
class Vector2D
{
    private:
        T x, y;
    public:
    // Constructors
    Vector2D() : x(0), y(0) {}
    Vector2D(T _x, T _y) : x(_x), y(_y) {}
    Vector2D& operator=(const Vector2D& rhs)
    {
        if (this != &rhs) {
            x = rhs.x;
            y = rhs.y;
        }
        return *this;
    }
 

    // Arithmetic operators
    Vector2D& operator += (const Vector2D& rhs) { x += rhs.x; y += rhs.y; return *this; }
    Vector2D& operator -= (const Vector2D& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    Vector2D operator-(const Vector2D& rhs) const { return Vector2D(x - rhs.x, y - rhs.y); }
    Vector2D& operator *= (T rhs) { x *= rhs; y *= rhs; return *this; }
    Vector2D& operator /= (T rhs) { x /= rhs; y /= rhs; return *this; }
    Vector2D operator -() const { return Vector2D(-x, -y); }

    // Getters
    T getX() const { return x; }
    T getY() const { return y; }

    // Setters
    void setX(T _x) { x = _x; }
    void setY(T _y) { y = _y; }

    // Static Methods for Vector Operations
    static Vector2D Add(const Vector2D& lhs, const Vector2D& rhs) { return Vector2D(lhs.x + rhs.x, lhs.y + rhs.y); }
    static Vector2D Subtract(const Vector2D& lhs, const Vector2D& rhs) { return Vector2D(lhs.x - rhs.x, lhs.y - rhs.y); }
    static Vector2D Multiply(const Vector2D& lhs, T rhs) { return Vector2D(lhs.x * rhs, lhs.y * rhs); }
    static Vector2D Divide(const Vector2D& lhs, T rhs) { return Vector2D(lhs.x / rhs, lhs.y / rhs); }

    static Vector2D Normalize(const Vector2D& pVec)
    {
        T len = Length(pVec);
        return (len != 0) ? Vector2D(pVec.x / len, pVec.y / len) : Vector2D(0, 0);
    }
    static T Length(const Vector2D& pVec) { return std::sqrt(pVec.x * pVec.x + pVec.y * pVec.y); }
    static T SquareLength(const Vector2D& pVec) { return pVec.x * pVec.x + pVec.y * pVec.y; }
    static T Distance(const Vector2D& pVec0, const Vector2D& pVec1) { return length(subtract(pVec0, pVec1)); }
    static T SquareDistance(const Vector2D& pVec0, const Vector2D& pVec1) { return squareLength(subtract(pVec0, pVec1)); }
    static T DotProduct(const Vector2D& pVec0, const Vector2D& pVec1) { return pVec0.x * pVec1.x + pVec0.y * pVec1.y; }
    static Vector2D Scale(T scalar, const Vector2D& vec) { return Vector2D(vec.x * scalar, vec.y * scalar); }
    static T CrossProductMag(const Vector2D& pVec0, const Vector2D& pVec1) { return pVec0.x * pVec1.y - pVec0.y * pVec1.x; }

    // Unity-like Methods
    static Vector2D MoveTowards(const Vector2D& current, const Vector2D& target, T maxDistanceDelta)
    {
        Vector2D direction = subtract(target, current);
        T dist = length(direction);
        if (dist <= maxDistanceDelta || dist == 0.0f)
            return target;
        return add(current, scale(maxDistanceDelta / dist, direction));
    }

    static T AngleClockwise(const Vector2D& pVec0, const Vector2D& pVec1)
    {
        T dot = dotProduct(pVec0, pVec1);
        T det = crossProductMag(pVec0, pVec1);
        return std::atan2(det, dot);
    }



};






