

#include "Vector_2D.h"


#ifndef MATRIX_3X3_H
#define MATRIX_3X3_H


class Matrix_3x3 {
private:
    float m2[3][3]; // Internal representation of the matrix

    // Private constructors


public:
    Matrix_3x3();
    Matrix_3x3(const float* pArr);
    Matrix_3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i);
    Matrix_3x3(const Matrix_3x3& rhs) = default;

    // Getter functions
    const float(&getMatrix() const)[3][3]{ return m2; }
    float* getData() { return &m2[0][0]; }

    // Operator overloads
    Matrix_3x3& operator=(const Matrix_3x3& rhs);
    Matrix_3x3 operator*(const Matrix_3x3& rhs) const;
    Matrix_3x3& operator*=(const Matrix_3x3& rhs);
  

  

    // Static utility functions
    static void Mtx33Identity(Matrix_3x3& pResult);
    static void Mtx33Translate(Matrix_3x3& pResult, float x, float y);
    static void Mtx33Scale(Matrix_3x3& pResult, float x, float y);
    static void Mtx33RotRad(Matrix_3x3& pResult, float angle);
    static void Mtx33Transpose(Matrix_3x3& pResult, const Matrix_3x3& pMtx);




};

#endif
