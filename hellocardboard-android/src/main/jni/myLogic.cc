#include "myLogic.h"

void TransposeM(Matrix4x4 mat) {

}

void SetIdentityMatrix(float *m) {
    memset((void*)m, 0, 16*sizeof(float));
    m[0] = m[5] = m [10] = m[15] = 1.0f;
}

void ScaleXX(Matrix4x4 &mat, float scale) {
    auto& _matData = mat.m;
    for(int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; ++j) {
            _matData[i][j] *= scale;
        }
    }
}

void TranslateM(float *m, float x, float y, float z) {
    for (int i = 0; i < 4; i++)
    {
        m[12+i] += m[i]*x + m[4+i]*y + m[8+i]*z;
    }
}

void TranslateX(Matrix4x4 &mat, float x, float y, float z) {
    auto& _matData = mat.m;
    _matData[0][3] = x;
    _matData[1][3] = y;
    _matData[2][3] = z;
}

void SetRotateM(float *m, float a, float x, float y, float z) {
    float s,c;
    memset((void*)m, 0, 15*sizeof(float));
    m[15] = 1.0f;

    a *= PI/180.0f;
    s = sin(a);
    c = cos(a);

    if(1.0f == x && 0.0f == y && 0.0f == z) {
        m[5] = c; m[10] = c;
        m[6] = s; m[9]  = -s;
        m[0] = 1;
    } else if (0.0f == x && 1.0f == y && 0.0f == z) {
        m[0] = c; m[10] = c;
        m[8] = s; m[2]  = -s;
        m[5] = 1;
    } else if (0.0f == x && 0.0f == y && 1.0f == z) {
        m[0] = c; m[5] = c;
        m[1] = s; m[4] = -s;
        m[10] = 1;
    } else {
        normalize(x, y, z);
        float nc = 1.0f - c;
        float xy = x * y;
        float yz = y * z;
        float zx = z * x;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        m[ 0] = x*x*nc +  c;
        m[ 4] =  xy*nc - zs;
        m[ 8] =  zx*nc + ys;
        m[ 1] =  xy*nc + zs;
        m[ 5] = y*y*nc +  c;
        m[ 9] =  yz*nc - xs;
        m[ 2] =  zx*nc - ys;
        m[ 6] =  yz*nc + xs;
        m[10] = z*z*nc +  c;
    }
}

void RotateX(Matrix4x4 &mat, float a, float x, float y, float z) {
    float s,c;
    auto& _mat = mat.m;
    memset((void*)&mat, 0, 15*sizeof(float));
    _mat[3][3] = 1.0f;

    a *= PI/180.0f;
    s = sin(a);
    c = cos(a);

    if(1.0f == x && 0.0f == y && 0.0f == z) {
        _mat[1][1] = c; _mat[2][2] = c;
        _mat[2][1] = s; _mat[1][2]  = -s;
        _mat[0][0] = 1;
    } else if (0.0f == x && 1.0f == y && 0.0f == z) {
        _mat[0][0] = c; _mat[2][2] = c;
        _mat[0][2] = s; _mat[2][0]  = -s;
        _mat[1][1] = 1;
    } else if (0.0f == x && 0.0f == y && 1.0f == z) {
        _mat[0][0] = c; _mat[1][1] = c;
        _mat[1][0] = s; _mat[0][1] = -s;
        _mat[2][2] = 1;
    } else {
        normalize(x, y, z);
        float nc = 1.0f - c;
        float xy = x * y;
        float yz = y * z;
        float zx = z * x;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        _mat[0][0] = x*x*nc + c;
        _mat[0][1] =  xy*nc - zs;
        _mat[0][2] =  zx*nc + ys;
        _mat[1][0] =  xy*nc + zs;
        _mat[1][1] = y*y*nc +  c;
        _mat[1][2] =  yz*nc - xs;
        _mat[2][0] =  zx*nc - ys;
        _mat[2][1] =  yz*nc + xs;
        _mat[2][2] = z*z*nc +  c;
    }
}

void RotateXX(Matrix4x4 &mat, float a, float x, float y, float z) {
    float s, c;
    auto &_mat = mat.m;
    memset((void *) &mat, 0, 15 * sizeof(float));
    _mat[3][3] = 1.0f;

    a *= PI / 180.0f;
    s = sin(a);
    c = cos(a);

    if (1.0f == x && 0.0f == y && 0.0f == z) {
        _mat[1][1] = c;
        _mat[2][2] = c;
        _mat[2][1] = s;
        _mat[1][2] = -s;
        _mat[0][0] = 1;
    } else if (0.0f == x && 1.0f == y && 0.0f == z) {
        _mat[0][0] = c;
        _mat[2][2] = c;
        _mat[0][2] = s;
        _mat[2][0] = -s;
        _mat[1][1] = 1;
    } else if (0.0f == x && 0.0f == y && 1.0f == z) {
        _mat[0][0] = c;
        _mat[1][1] = c;
        _mat[1][0] = s;
        _mat[0][1] = -s;
        _mat[2][2] = 1;
    } else {
        normalize(x, y, z);
        float nc = 1.0f - c;
        float xy = x * y;
        float yz = y * z;
        float zx = z * x;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        _mat[0][0] = x * x * nc + c;
        _mat[0][1] = xy * nc - zs;
        _mat[0][2] = zx * nc + ys;
        _mat[1][0] = xy * nc + zs;
        _mat[1][1] = y * y * nc + c;
        _mat[1][2] = yz * nc - xs;
        _mat[2][0] = zx * nc - ys;
        _mat[2][1] = yz * nc + xs;
        _mat[2][2] = z * z * nc + c;
    }
}



//    /*
//     * Matrix Calculation
//     */
//
//

//
//    void MultiplyMM(float *m, float *lhs, float *rhs) {
//        float t[16];
//        for (int i = 0; i < 4; i++) {
//            const float rhs_i0 = rhs[I(i, 0)];
//            float ri0 = lhs[ I(0,0) ] * rhs_i0;
//            float ri1 = lhs[ I(0,1) ] * rhs_i0;
//            float ri2 = lhs[ I(0,2) ] * rhs_i0;
//            float ri3 = lhs[ I(0,3) ] * rhs_i0;
//            for (int j = 1; j < 4; j++) {
//                const float rhs_ij = rhs[ I(i,j) ];
//                ri0 += lhs[ I(j,0) ] * rhs_ij;
//                ri1 += lhs[ I(j,1) ] * rhs_ij;
//                ri2 += lhs[ I(j,2) ] * rhs_ij;
//                ri3 += lhs[ I(j,3) ] * rhs_ij;
//            }
//            t[ I(i,0) ] = ri0;
//            t[ I(i,1) ] = ri1;
//            t[ I(i,2) ] = ri2;
//            t[ I(i,3) ] = ri3;
//        }
//        memcpy(m, t, sizeof(t));
//    }
//
//    void ScaleM(float *m, float x, float y, float z) {
//        for (int i = 0; i < 4; i++)
//        {
//            m[i] *= x; m[4+i] *=y; m[8+i] *= z;
//        }
//    }
//
//    void ScaleX(float *m, float *m2, float *m3, float scale) {
//        for(int i = 0; i < 3; i++) {
//            m[i] *= scale;
//            m2[i] *= scale;
//            m3[i] *= scale;
//        }
//    }
//
//    void TranslateM(float *m, float x, float y, float z) {
//        for (int i = 0; i < 4; i++)
//        {
//            m[12+i] += m[i]*x + m[4+i]*y + m[8+i]*z;
//        }
//    }
//
//    void RotateM(float *m, float a, float x, float y, float z) {
//        float rot[16], res[16];
//        SetRotateM(rot, a, x, y, z);
//        MultiplyMM(res, m, rot);
//        memcpy(m, res, 16*sizeof(float));
//    }
//
//    void LookAtM(float *m,
//                 float eyeX, float eyeY, float eyeZ,
//                 float cenX, float cenY, float cenZ,
//                 float  upX, float  upY, float  upZ)
//    {
//        float fx = cenX - eyeX;
//        float fy = cenY - eyeY;
//        float fz = cenZ - eyeZ;
//        normalize(fx, fy, fz);
//        float sx = fy * upZ - fz * upY;
//        float sy = fz * upX - fx * upZ;
//        float sz = fx * upY - fy * upX;
//        normalize(sx, sy, sz);
//        float ux = sy * fz - sz * fy;
//        float uy = sz * fx - sx * fz;
//        float uz = sx * fy - sy * fx;
//
//        m[ 0] = sx;
//        m[ 1] = ux;
//        m[ 2] = -fx;
//        m[ 3] = 0.0f;
//        m[ 4] = sy;
//        m[ 5] = uy;
//        m[ 6] = -fy;
//        m[ 7] = 0.0f;
//        m[ 8] = sz;
//        m[ 9] = uz;
//        m[10] = -fz;
//        m[11] = 0.0f;
//        m[12] = 0.0f;
//        m[13] = 0.0f;
//        m[14] = 0.0f;
//        m[15] = 1.0f;
//        TranslateM(m, -eyeX, -eyeY, -eyeZ);
//    }
//
//    void matrixFrustumM(float *m, float left, float right, float bottom, float top, float near, float far) {
//        float r_width  = 1.0f / (right - left);
//        float r_height = 1.0f / (top - bottom);
//        float r_depth  = 1.0f / (near - far);
//        float x = 2.0f * (near * r_width);
//        float y = 2.0f * (near * r_height);
//        float A = 2.0f * ((right+left) * r_width);
//        float B = (top + bottom) * r_height;
//        float C = (far + near) * r_depth;
//        float D = 2.0f * (far * near * r_depth);
//
//        memset((void*)m, 0, 16*sizeof(float));
//        m[ 0] = x;
//        m[ 5] = y;
//        m[ 8] = A;
//        m[ 9] = B;
//        m[10] = C;
//        m[14] = D;
//        m[11] = -1.0f;
//    }
//    /*
//     *
//     */
