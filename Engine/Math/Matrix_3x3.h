/*********************************************************************************************
 \file       Matrix_3x3.h
 \par       SofaSpuds
 \author    jianwei.c (jianwei.c@digipen.edu) - Primary Author, 100%

 \brief     Declaration and some inline immeplementations of the Matrix_3x3 class, 
            which provides 3x3 matrix operations for 2D transformations. 
            Includes constructors, operator overloads, and static 
            utility functions for identity, translation, scaling, rotation, 
            and transposition.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
**********************************************************************************************/
#include "Vector_2D.h"
#ifndef MATRIX_3X3_H
#define MATRIX_3X3_H
class Matrix_3x3 {
private:
    float m2[3][3]; /// Internal representation of the matrix
public:
    /*****************************************************************************************
      \brief Default constructor. Initializes the matrix to the identity matrix.
    *****************************************************************************************/
    Matrix_3x3();
    /*****************************************************************************************
      \brief Constructor from a flat array of 9 floats.
      \param pArr Pointer to an array of 9 floats representing the matrix elements.
    *****************************************************************************************/
    Matrix_3x3(const float* pArr);
    /*****************************************************************************************
      \brief Parameterized constructor.
      \param a,b,c,d,e,f,g,h,i Elements of the matrix in row-major order.
    *****************************************************************************************/
    Matrix_3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i);
    /*****************************************************************************************
      \brief Copy constructor (defaulted).
      \param rhs Matrix to copy from.
    *****************************************************************************************/
    Matrix_3x3(const Matrix_3x3& rhs) = default;
    /*****************************************************************************************
      \brief Returns a const reference to the internal 3x3 array.
      \return Const reference to the matrix array.
    *****************************************************************************************/
    const float(&getMatrix() const)[3][3]{ return m2; }
    /*****************************************************************************************
      \brief Returns a pointer to the first element of the internal matrix array.
      \return Pointer to the first element of the matrix.
    *****************************************************************************************/
    float* getData() { return &m2[0][0]; }
    /*****************************************************************************************
      \brief Assignment operator.
      \param rhs Matrix to assign from.
      \return Reference to this matrix.
    *****************************************************************************************/
    Matrix_3x3& operator=(const Matrix_3x3& rhs);
    /*****************************************************************************************
      \brief Multiplies this matrix with another matrix.
      \param rhs Matrix to multiply with.
      \return Resulting matrix after multiplication.
    *****************************************************************************************/
    Matrix_3x3 operator*(const Matrix_3x3& rhs) const;
    /*****************************************************************************************
      \brief Multiplies this matrix with another matrix in-place.
      \param rhs Matrix to multiply with.
      \return Reference to this matrix after multiplication.
    *****************************************************************************************/
    Matrix_3x3& operator*=(const Matrix_3x3& rhs);
    /*****************************************************************************************
      \brief Sets a matrix to the identity matrix.
      \param pResult Matrix to store the result.
    *****************************************************************************************/
    static void Mtx33Identity(Matrix_3x3& pResult);
    /*****************************************************************************************
      \brief Sets a matrix to a translation matrix.
      \param pResult Matrix to store the result.
      \param x Translation along the X-axis.
      \param y Translation along the Y-axis.
    *****************************************************************************************/
    static void Mtx33Translate(Matrix_3x3& pResult, float x, float y);
    /*****************************************************************************************
      \brief Sets a matrix to a scale matrix.
      \param pResult Matrix to store the result.
      \param x Scale factor along the X-axis.
      \param y Scale factor along the Y-axis.
    *****************************************************************************************/
    static void Mtx33Scale(Matrix_3x3& pResult, float x, float y);
    /*****************************************************************************************
      \brief Sets a matrix to a rotation matrix using radians.
      \param pResult Matrix to store the result.
      \param angle Rotation angle in radians.
    *****************************************************************************************/
    static void Mtx33RotRad(Matrix_3x3& pResult, float angle);
    /*****************************************************************************************
      \brief Computes the transpose of a matrix.
      \param pResult Matrix to store the result.
      \param pMtx Matrix to transpose.
    *****************************************************************************************/
    static void Mtx33Transpose(Matrix_3x3& pResult, const Matrix_3x3& pMtx);
};

#endif
