

#include "Matrix_3x3.h"

// Constructors
Matrix_3x3::Matrix_3x3() {
    Mtx33Identity(*this);
}

Matrix_3x3::Matrix_3x3(const float* pArr) {
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            m2[row][col] = pArr[col * 3 + row]; // Column-major order
        }
    }
}

Matrix_3x3::Matrix_3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i) {
    m2[0][0] = a; m2[1][0] = b; m2[2][0] = c;
    m2[0][1] = d; m2[1][1] = e; m2[2][1] = f;
    m2[0][2] = g; m2[1][2] = h; m2[2][2] = i;
}

Matrix_3x3& Matrix_3x3::operator=(const Matrix_3x3& rhs) {
    if (this != &rhs) {
        for (int col = 0; col < 3; ++col) {
            for (int row = 0; row < 3; ++row) {
                m2[row][col] = rhs.m2[row][col];
            }
        }
    }
    return *this;
}

Matrix_3x3 Matrix_3x3::operator*(const Matrix_3x3& rhs) const {
    Matrix_3x3 result;
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            result.m2[row][col] =
                m2[row][0] * rhs.m2[0][col] +
                m2[row][1] * rhs.m2[1][col] +
                m2[row][2] * rhs.m2[2][col];
        }
    }
    return result;
}

Matrix_3x3& Matrix_3x3::operator*=(const Matrix_3x3& rhs) {
    *this = *this * rhs;
    return *this;
}






void Matrix_3x3::Mtx33Identity(Matrix_3x3& pResult) {
    pResult = Matrix_3x3(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}


void Matrix_3x3::Mtx33Translate(Matrix_3x3& pResult, float x, float y) {
    pResult = Matrix_3x3(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        x, y, 1.0f
    );
}


void Matrix_3x3::Mtx33Scale(Matrix_3x3& pResult, float x, float y) {
    pResult = Matrix_3x3(
        x, 0.0f, 0.0f,
        0.0f, y, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

void Matrix_3x3::Mtx33RotRad(Matrix_3x3& pResult, float angle) {
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    pResult = Matrix_3x3(
        cosA, -sinA, 0.0f,
        sinA, cosA, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

void Matrix_3x3::Mtx33Transpose(Matrix_3x3& pResult, const Matrix_3x3& pMtx) {
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            pResult.m2[row][col] = pMtx.m2[col][row];
        }
    }
}
