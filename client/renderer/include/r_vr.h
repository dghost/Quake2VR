#ifndef __R_VR_H
#define __R_VR_H

#include "../../vr/include/vr.h"
#include "r_local.h"

typedef struct {
	uint32_t x, y;
	uint32_t width, height;
} vr_rect_t;

typedef struct {
	float x, y;
	float width, height;
} vr_rectf_t;

typedef struct {
	float viewFovY;
	float viewFovX;
	float pixelScale;
	float aspect;
	fbo_t* eyeFBO[2];
	fbo_t* offscreen;
	eye_param_t renderParams[2];
} vr_param_t;

extern vr_param_t vrState;

typedef struct {
	hmd_t type;
	int32_t (*init)(void);
	int32_t (*enable)(void);
	void (*disable)(void);
	void (*frameStart)(int32_t changeBackBuffers);
	void (*getState)(vr_param_t *state);
	void (*present)(qboolean loading);
	void (*postPresent)(void);
} hmd_render_t;

typedef struct {
	r_shaderobject_t *shader;
	struct {
		GLuint texture;
		GLuint distortion;
		GLuint texture_size;
	} uniform;
} vr_distort_shader_t;

void R_VR_Init();
void R_VR_Shutdown();
void R_VR_Enable();
void R_VR_Disable();
void R_VR_StartFrame();
void R_VR_EndFrame();
void R_VR_PostGammaPresent();
void R_VR_GetFOV(float *fovx, float *fovy);
fbo_t* R_VR_GetHUDFBO();
fbo_t* R_VR_GetFrameFBO();

#endif //__R_VR_H