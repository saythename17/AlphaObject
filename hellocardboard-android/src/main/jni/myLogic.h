////
//// Created by 박시온 on 1/28/21.
////

#ifndef CARDBOARD_1_4_1_MYLOGIC_H
#define CARDBOARD_1_4_1_MYLOGIC_H

#include "hello_cardboard_app.h"

using namespace ndk_hello_cardboard;

void SetIdentityMatrix(float *m);

void ScaleXX(Matrix4x4 &mat, float scale);

void TranslateM(float *m, float x, float y, float z);

void TranslateX(Matrix4x4 &mat, float x, float y, float z);

void SetRotateM(float *m, float a, float x, float y, float z);

#endif //CARDBOARD_1_4_1_MYLOGIC_H
