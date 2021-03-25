/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hello_cardboard_app.h"
#include "myLogic.h"

namespace ndk_hello_cardboard {

namespace {

// The objects are about 1 meter in radius, so the min/max target distance are
// set so that the objects are always within the room (which is about 5 meters
// across) and the reticle is always closer than any objects.
constexpr float kMinTargetDistance = 2.5f;
constexpr float kMaxTargetDistance = 3.5f;
constexpr float kMinTargetHeight = 0.5f;
constexpr float kMaxTargetHeight = kMinTargetHeight + 3.0f;

constexpr float kDefaultFloorHeight = -1.7f;

constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

// Angle threshold for determining whether the controller is pointing at the
// object.
constexpr float kAngleLimit = 0.2f;

// Number of different possible targets
constexpr int kTargetMeshCount = 3;

float angle = 5.0f;
float angle_cat = 5.0f;

// Simple shaders to render .obj files without any lighting.
constexpr const char* kObjVertexShader =
    R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    attribute vec2 a_UV;
    varying vec2 v_UV;

    void main() {
      v_UV = a_UV;
      gl_Position = u_MVP * a_Position;
    })glsl";

constexpr const char* kObjFragmentShader =
    R"glsl(
    precision mediump float;

    uniform sampler2D u_Texture;
    varying vec2 v_UV;

    void main() {
      // The y coordinate of this sample's textures is reversed compared to
      // what OpenGL expects, so we invert the y coordinate.
      gl_FragColor = texture2D(u_Texture, vec2(v_UV.x, 1.0 - v_UV.y));
    })glsl";

//⭐️❤️🌈 XION
constexpr constexpr char* xObjectVertexShader =
            R"glsl(
      layout (location = 0) in vec3 aPos;
      layout (location = 1) in vec3 aNormal;
      layout (location = 2) in vec2 aTexCoords;
      out vec2 TexCoords;
      uniform mat4 model;
      uniform mat4 view;
      uniform mat4 projection;

      void main() {
          gl_Position = projection * view * model * vec4(aPos, 1.0);
          TexCoords = aTexCoords;
      }
      )glsl";

constexpr constexpr char* xObjectFragmentShader =
        R"glsl(
        out vec4 FragColor;
        in vec2 TexCoords;
        uniform sampler2D texture_diffuse1;

        void main() {
            FragColor = texture(texture_diffuse1, TexCoords);
        }
        )glsl";

}  // anonymous namespace

HelloCardboardApp::HelloCardboardApp(JavaVM* vm, jobject obj, jobject asset_mgr_obj)
    : head_tracker_(nullptr),
      lens_distortion_(nullptr),
      distortion_renderer_(nullptr),
      screen_params_changed_(false),
      device_params_changed_(false),
      screen_width_(0),
      screen_height_(0),
      depthRenderBuffer_(0),
      framebuffer_(0),
      texture_(0),
      obj_program_(0),
      obj_position_param_(0),
      obj_uv_param_(0),
      obj_modelview_projection_param_(0),
      target_object_meshes_(kTargetMeshCount),
      target_object_not_selected_textures_(kTargetMeshCount),
      target_object_selected_textures_(kTargetMeshCount),
      cur_target_object_(RandomUniformInt(kTargetMeshCount)) {
  JNIEnv* env;
  vm->GetEnv((void**)&env, JNI_VERSION_1_6);
  java_asset_mgr_ = env->NewGlobalRef(asset_mgr_obj);
  asset_mgr_ = AAssetManager_fromJava(env, asset_mgr_obj);

  Cardboard_initializeAndroid(vm, obj);
  head_tracker_ = CardboardHeadTracker_create();
}

HelloCardboardApp::~HelloCardboardApp() {
  CardboardHeadTracker_destroy(head_tracker_);
  CardboardLensDistortion_destroy(lens_distortion_);
  CardboardDistortionRenderer_destroy(distortion_renderer_);
}

void HelloCardboardApp::OnSurfaceCreated(JNIEnv* env) {
  const int obj_vertex_shader =
      LoadGLShader(GL_VERTEX_SHADER, kObjVertexShader);
  const int obj_fragment_shader =
      LoadGLShader(GL_FRAGMENT_SHADER, kObjFragmentShader);

  obj_program_ = glCreateProgram();
  glAttachShader(obj_program_, obj_vertex_shader);
  glAttachShader(obj_program_, obj_fragment_shader);
  glLinkProgram(obj_program_);
  glUseProgram(obj_program_);

  CHECKGLERROR("Obj program");

  obj_position_param_ = glGetAttribLocation(obj_program_, "a_Position");
  obj_uv_param_ = glGetAttribLocation(obj_program_, "a_UV");
  obj_modelview_projection_param_ = glGetUniformLocation(obj_program_, "u_MVP");

  CHECKGLERROR("Obj program params");

  HELLOCARDBOARD_CHECK(room_.Initialize(env, asset_mgr_, "CubeRoom.obj",
                                 obj_position_param_, obj_uv_param_));
  HELLOCARDBOARD_CHECK(
      room_tex_.Initialize(env, java_asset_mgr_, "CubeRoom_BakedDiffuse.png"));
    /*
   *
   *
   */
    HELLOCARDBOARD_CHECK(dog_.Initialize(env, asset_mgr_, "dog.obj",
                         obj_position_param_, obj_uv_param_));
    HELLOCARDBOARD_CHECK(dog_tex_.Initialize(env, java_asset_mgr_, "dog_diffuse.png"));
    HELLOCARDBOARD_CHECK(cat_.Initialize(env, asset_mgr_, "cat.obj",
                         obj_position_param_, obj_uv_param_));
    HELLOCARDBOARD_CHECK(cat_tex_.Initialize(env, java_asset_mgr_, "cat_diffuse.png"));

    HELLOCARDBOARD_CHECK(alpha_.Initialize(env, asset_mgr_, "QuadSphere.obj",
                                         obj_position_param_, obj_uv_param_));
    HELLOCARDBOARD_CHECK(alpha_tex_.Initialize(env, java_asset_mgr_, "sky.png"));



  HELLOCARDBOARD_CHECK(target_object_meshes_[0].Initialize(
      env, asset_mgr_, "Icosahedron.obj", obj_position_param_, obj_uv_param_));
  HELLOCARDBOARD_CHECK(target_object_not_selected_textures_[0].Initialize(
      env, java_asset_mgr_, "Icosahedron_Blue_BakedDiffuse.png"));
  HELLOCARDBOARD_CHECK(target_object_selected_textures_[0].Initialize(
      env, java_asset_mgr_, "Icosahedron_Pink_BakedDiffuse.png"));
  HELLOCARDBOARD_CHECK(target_object_meshes_[1].Initialize(
      env, asset_mgr_, "QuadSphere.obj", obj_position_param_, obj_uv_param_));
  HELLOCARDBOARD_CHECK(target_object_not_selected_textures_[1].Initialize(
      env, java_asset_mgr_, "QuadSphere_Blue_BakedDiffuse.png"));
  HELLOCARDBOARD_CHECK(target_object_selected_textures_[1].Initialize(
      env, java_asset_mgr_, "QuadSphere_Pink_BakedDiffuse.png"));
  HELLOCARDBOARD_CHECK(target_object_meshes_[2].Initialize(
      env, asset_mgr_, "TriSphere.obj", obj_position_param_, obj_uv_param_));
  HELLOCARDBOARD_CHECK(target_object_not_selected_textures_[2].Initialize(
      env, java_asset_mgr_, "TriSphere_Blue_BakedDiffuse.png"));
  HELLOCARDBOARD_CHECK(target_object_selected_textures_[2].Initialize(
      env, java_asset_mgr_, "TriSphere_Pink_BakedDiffuse.png"));


  // Target object first appears directly in front of user.
  model_target_ = GetTranslationMatrix({1.0f, 1.5f, kMinTargetDistance});
  model_dog_ = GetTranslationMatrix({1.0f,  kDefaultFloorHeight - 0.01f , 1.0f - kMaxTargetDistance});
  model_cat_ = GetTranslationMatrix({1.0f, kDefaultFloorHeight, 1.0f - kMaxTargetDistance});
  model_alpha_ = GetTranslationMatrix({1.0f, 1.5f, kMaxTargetDistance});

  CHECKGLERROR("OnSurfaceCreated");
}

void HelloCardboardApp::SetScreenParams(int width, int height) {
  screen_width_ = width;
  screen_height_ = height;
  screen_params_changed_ = true;
}

void HelloCardboardApp::OnDrawFrame() {
  if (!UpdateDeviceParams()) {
    return;
  }

  // Update Head Pose.
  head_view_ = GetPose();

  // Incorporate the floor height into the head_view
  head_view_ =
      head_view_ * GetTranslationMatrix({0.0f, kDefaultFloorHeight, 0.0f});
  head_view_dog_ =
          head_view_ * GetTranslationMatrix({0.0f, kDefaultFloorHeight + 1.66f, -3.0f});
  head_view_cat_ =
          head_view_ * GetTranslationMatrix({-1.7f, kDefaultFloorHeight + 1.66f, -3.0f});
  head_view_alpha_ =
          head_view_ * GetTranslationMatrix({1.0f, 2.0f, -1.0f});

  //⭐️❤️🌈
  angle += 0.7f;
  if(angle_cat > 5.0f) angle_cat+=0.7f;
  else if(angle_cat < 10.1f) angle_cat -=0.7f;
  auto mat = model_dog_.m;
  LOGD("(BEFORE)XION_START");
  for(int i = 0;i < 4;++i) {
    std::string str = "XION__\n\n" +
                      std::to_string(mat[i][0]) + ",\t" +
                      std::to_string(mat[i][1]) + ",\t"+
                      std::to_string(mat[i][2]) + ",\t"+
                      std::to_string(mat[i][3]) + ",\t" + "\n\n";
    LOGD("%d|%s",i,str.c_str());
  }
  LOGD("(BEFORE)XION_END");

  RotateX(model_dog_, angle, 0.0f, 1.0f, 0.0f);
  RotateXX(model_cat_, angle_cat, 0.0f, 1.0f, 0.0f);
  RotateX(model_alpha_, angle_cat, 0.0f, 1.0f, 0.0f);
  LOGD("???|%f",angle);

  mat = model_dog_.m;
  LOGD("XION_START");
  for(int i = 0;i < 4;++i) {
    std::string str = "XION__\n\n" +
                      std::to_string(mat[i][0]) + ",\t" +
                      std::to_string(mat[i][1]) + ",\t"+
                      std::to_string(mat[i][2]) + ",\t"+
                      std::to_string(mat[i][3]) + ",\t" + "\n\n";
    LOGD("%d|%s",i,str.c_str());
  }
  LOGD("XION_END");
  //⭐️❤️🌈

  // Bind buffer
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Draw eyes views
  for (int eye = 0; eye < 2; ++eye) {
    glViewport(eye == kLeft ? 0 : screen_width_ / 2, 0, screen_width_ / 2,
               screen_height_);

    Matrix4x4 eye_matrix = GetMatrixFromGlArray(eye_matrices_[eye]);
    Matrix4x4 eye_view = eye_matrix * head_view_;
    /*
     *
     */
    Matrix4x4 eye_view_dog = eye_matrix * head_view_dog_;
    Matrix4x4 eye_view_cat = eye_matrix * head_view_cat_;
    Matrix4x4 eye_view_alpha = eye_matrix * head_view_alpha_;

    Matrix4x4 projection_matrix =
        GetMatrixFromGlArray(projection_matrices_[eye]);
    Matrix4x4 modelview_target = eye_view * model_target_;
    Matrix4x4 modelview_dog_ = eye_view_dog * model_dog_;
    Matrix4x4 modelview_cat_ = eye_view_cat * model_cat_;
    Matrix4x4 modelview_alpha_ = eye_view_alpha * model_alpha_;

    auto mat = modelview_target.m;
//    LOGD("(BEFORE)XION_START");
//    for(int i = 0;i < 4;++i) {
//      std::string str = "XION__\n\n" +
//                        std::to_string(mat[i][0]) + ",\t" +
//                        std::to_string(mat[i][1]) + ",\t"+
//                        std::to_string(mat[i][2]) + ",\t"+
//                        std::to_string(mat[i][3]) + ",\t" + "\n\n";
//      LOGD("%d|%s",i,str.c_str());
//    }
//    LOGD("(BEFORE)XION_END");

    const float SCALE_SIZE = 2.0f;
    const float SCALE_SIZE_DOG = 0.025f;
    const float SCALE_SIZE_ALPHA = 0.55f;
    // 🌈️️ Dog Model Scaling and Logging
    ScaleXX(modelview_target, SCALE_SIZE);
    ScaleXX(modelview_dog_,SCALE_SIZE_DOG);
    ScaleXX(modelview_cat_,SCALE_SIZE_DOG);
    ScaleXX(modelview_alpha_, SCALE_SIZE_ALPHA);

    //ScaleX(matrix, SCALE_SIZE, SCALE_SIZE, SCALE_SIZE);
    // 🌈️️ Dog Model Scaling and Logging
//    for(int i = 0;i < 4; ++i){
//      const float SCALE_SIZE = 3.0f;
//      ScaleM(modelview_dog_.m[i], SCALE_SIZE, SCALE_SIZE, SCALE_SIZE );
//      LOGD("DOG____________________________START");
//      for(int j = 0; j < 4; ++j)
//        LOGD("DOG_%d|%d|%f",i,j,modelview_dog_.m[i][j]);
//      LOGD("DOG___%d",i);
//      LOGD("DOG____________________________END");
//    }

    modelview_projection_target_ = projection_matrix * modelview_target;
    modelview_projection_room_ = projection_matrix * eye_view;
    modelview_projection_dog_ = projection_matrix * modelview_dog_;
    modelview_projection_cat_ = projection_matrix * modelview_cat_;
    modelview_projection_alpha_ = projection_matrix * modelview_alpha_;

    // Draw room and target
    DrawWorld();
    LOGD("DRAW");
  }

  // Render
  CardboardDistortionRenderer_renderEyeToDisplay(
      distortion_renderer_, /* target_display = */ 0, /* x = */ 0, /* y = */ 0,
      screen_width_, screen_height_, &left_eye_texture_description_,
      &right_eye_texture_description_);

  CHECKGLERROR("onDrawFrame");
}

void HelloCardboardApp::OnTriggerEvent() {
  if (IsPointingAtTarget()) {
    HideTarget();
  }
}

void HelloCardboardApp::OnPause() { CardboardHeadTracker_pause(head_tracker_); }

void HelloCardboardApp::OnResume() {
  CardboardHeadTracker_resume(head_tracker_);

  // Parameters may have changed.
  device_params_changed_ = true;

  // Check for device parameters existence in external storage. If they're
  // missing, we must scan a Cardboard QR code and save the obtained parameters.
  uint8_t* buffer;
  int size;
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);
  if (size == 0) {
    SwitchViewer();
  }
  CardboardQrCode_destroy(buffer);
}

void HelloCardboardApp::SwitchViewer() {
  CardboardQrCode_scanQrCodeAndSaveDeviceParams();
}

bool HelloCardboardApp::UpdateDeviceParams() {
  // Checks if screen or device parameters changed
  if (!screen_params_changed_ && !device_params_changed_) {
    return true;
  }

  // Get saved device parameters
  uint8_t* buffer;
  int size;
  CardboardQrCode_getSavedDeviceParams(&buffer, &size);

  // If there are no parameters saved yet, returns false.
  if (size == 0) {
    return false;
  }

  CardboardLensDistortion_destroy(lens_distortion_);
  lens_distortion_ = CardboardLensDistortion_create(
      buffer, size, screen_width_, screen_height_);

  CardboardQrCode_destroy(buffer);

  GlSetup();

  CardboardDistortionRenderer_destroy(distortion_renderer_);
  distortion_renderer_ = CardboardOpenGlEs2DistortionRenderer_create();

  CardboardMesh left_mesh;
  CardboardMesh right_mesh;
  CardboardLensDistortion_getDistortionMesh(lens_distortion_, kLeft,
                                            &left_mesh);
  CardboardLensDistortion_getDistortionMesh(lens_distortion_, kRight,
                                            &right_mesh);

  CardboardDistortionRenderer_setMesh(distortion_renderer_, &left_mesh, kLeft);
  CardboardDistortionRenderer_setMesh(distortion_renderer_, &right_mesh,
                                      kRight);

  // Get eye matrices
  CardboardLensDistortion_getEyeFromHeadMatrix(
      lens_distortion_, kLeft, eye_matrices_[0]);
  CardboardLensDistortion_getEyeFromHeadMatrix(
      lens_distortion_, kRight, eye_matrices_[1]);
  CardboardLensDistortion_getProjectionMatrix(
      lens_distortion_, kLeft, kZNear, kZFar, projection_matrices_[0]);
  CardboardLensDistortion_getProjectionMatrix(
      lens_distortion_, kRight, kZNear, kZFar, projection_matrices_[1]);

  screen_params_changed_ = false;
  device_params_changed_ = false;

  CHECKGLERROR("UpdateDeviceParams");

  return true;
}

void HelloCardboardApp::GlSetup() {
  LOGD("GL SETUP");

  if (framebuffer_ != 0) {
    GlTeardown();
  }

  // Create render texture.
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screen_width_, screen_height_, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);

  left_eye_texture_description_.texture = texture_;
  left_eye_texture_description_.left_u = 0;
  left_eye_texture_description_.right_u = 0.5;
  left_eye_texture_description_.top_v = 1;
  left_eye_texture_description_.bottom_v = 0;

  right_eye_texture_description_.texture = texture_;
  right_eye_texture_description_.left_u = 0.5;
  right_eye_texture_description_.right_u = 1;
  right_eye_texture_description_.top_v = 1;
  right_eye_texture_description_.bottom_v = 0;

  // Generate depth buffer to perform depth test.
  glGenRenderbuffers(1, &depthRenderBuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width_,
                        screen_height_);
  CHECKGLERROR("Create Render buffer");

  // Create render target.
  glGenFramebuffers(1, &framebuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthRenderBuffer_);

  CHECKGLERROR("GlSetup");
}

void HelloCardboardApp::GlTeardown() {
  if (framebuffer_ == 0) {
    return;
  }
  glDeleteRenderbuffers(1, &depthRenderBuffer_);
  depthRenderBuffer_ = 0;
  glDeleteFramebuffers(1, &framebuffer_);
  framebuffer_ = 0;
  glDeleteTextures(1, &texture_);
  texture_ = 0;

  CHECKGLERROR("GlTeardown");
}

Matrix4x4 HelloCardboardApp::GetPose() {
  std::array<float, 4> out_orientation;
  std::array<float, 3> out_position;
  long monotonic_time_nano = GetMonotonicTimeNano();
  monotonic_time_nano += kPredictionTimeWithoutVsyncNanos;
  CardboardHeadTracker_getPose(head_tracker_, monotonic_time_nano,
                               &out_position[0], &out_orientation[0]);
  return GetTranslationMatrix(out_position) *
         Quatf::FromXYZW(&out_orientation[0]).ToMatrix();
}

void HelloCardboardApp::DrawWorld() {
  DrawRoom();
  DrawTarget();
  DrawDog();
  DrawCat();
  DrawAlpha();
}

void HelloCardboardApp::DrawTarget() {
  glUseProgram(obj_program_);

  std::array<float, 16> target_array = modelview_projection_target_.ToGlArray();
  glUniformMatrix4fv(obj_modelview_projection_param_, 1, GL_FALSE,
                     target_array.data());

  if (IsPointingAtTarget()) {
    target_object_selected_textures_[cur_target_object_].Bind();
  } else {
    target_object_not_selected_textures_[cur_target_object_].Bind();
  }
  target_object_meshes_[cur_target_object_].Draw();

  CHECKGLERROR("DrawTarget");
}

void HelloCardboardApp::DrawRoom() {
  glUseProgram(obj_program_);

  std::array<float, 16> room_array = modelview_projection_room_.ToGlArray();
  glUniformMatrix4fv(obj_modelview_projection_param_, 1, GL_FALSE,
                     room_array.data());

  room_tex_.Bind();
  room_.Draw();

  CHECKGLERROR("DrawRoom");
}

/*
 *
 *
 */
void HelloCardboardApp::DrawDog() {
  glUseProgram(obj_program_);

  std::array<float, 16> dog_array = modelview_projection_dog_.ToGlArray();
  glUniformMatrix4fv(obj_modelview_projection_param_, 1, GL_FALSE, dog_array.data());


  dog_tex_.Bind();
  dog_.Draw();

  CHECKGLERROR("DrawDog");
}

void HelloCardboardApp::DrawCat() {
  glUseProgram(obj_program_);

  std::array<float, 16> cat_array = modelview_projection_cat_.ToGlArray();
  glUniformMatrix4fv(obj_modelview_projection_param_, 1, GL_FALSE, cat_array.data());

  cat_tex_.Bind();
  cat_.Draw();

  CHECKGLERROR("DrawCat");
}

void HelloCardboardApp::DrawAlpha() {
  glUseProgram(obj_program_);

  std::array<float, 16> alpha_array = modelview_projection_alpha_.ToGlArray();
  glUniformMatrix4fv(obj_modelview_projection_param_, 1, GL_FALSE, alpha_array.data());

  alpha_tex_.Bind();
  alpha_.Draw();

  CHECKGLERROR("DrawAlpha");
}

void HelloCardboardApp::HideTarget() {
  cur_target_object_ = RandomUniformInt(kTargetMeshCount);

  float angle = RandomUniformFloat(-M_PI, M_PI);
  float distance = RandomUniformFloat(kMinTargetDistance, kMaxTargetDistance);
  float height = RandomUniformFloat(kMinTargetHeight, kMaxTargetHeight);
  std::array<float, 3> target_position = {std::cos(angle) * distance, height,
                                          std::sin(angle) * distance};

  model_target_ = GetTranslationMatrix(target_position);
}

bool HelloCardboardApp::IsPointingAtTarget() {
  // Compute vectors pointing towards the reticle and towards the target object
  // in head space.
  Matrix4x4 head_from_target = head_view_ * model_target_;

  const std::array<float, 4> unit_quaternion = {0.f, 0.f, 0.f, 1.f};
  const std::array<float, 4> point_vector = {0.f, 0.f, -1.f, 0.f};
  const std::array<float, 4> target_vector = head_from_target * unit_quaternion;

  float angle = AngleBetweenVectors(point_vector, target_vector);
  return angle < kAngleLimit;
}

}  // namespace ndk_hello_cardboard
