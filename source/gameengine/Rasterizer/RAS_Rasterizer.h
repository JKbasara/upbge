/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file RAS_Rasterizer.h
 *  \ingroup bgerast
 */

#ifndef __RAS_RASTERIZER_H__
#define __RAS_RASTERIZER_H__

#ifdef _MSC_VER
#  pragma warning (disable:4786)
#endif

#include "MT_Matrix4x4.h"

#include "RAS_DebugDraw.h"
#include "RAS_Rect.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

class RAS_OpenGLRasterizer;
class RAS_FrameBuffer;
class RAS_ICanvas;
class RAS_MeshSlot;
class RAS_IDisplayArray;
class SCA_IScene;
class RAS_ISync;
struct KX_ClientObjectInfo;
class KX_RayCast;
struct GPUShader;
struct GPUTexture;
struct GPUViewport;
struct DRWShadingGroup;
struct EEVEE_SceneLayerData;

/**
 * 3D rendering device context interface. 
 */
class RAS_Rasterizer
{
public:

	enum FrameBufferType {
		RAS_FRAMEBUFFER_FILTER0 = 0,
		RAS_FRAMEBUFFER_FILTER1,
		RAS_FRAMEBUFFER_EYE_LEFT0,
		RAS_FRAMEBUFFER_EYE_RIGHT0,
		RAS_FRAMEBUFFER_EYE_LEFT1,
		RAS_FRAMEBUFFER_EYE_RIGHT1,
		RAS_FRAMEBUFFER_BLIT_DEPTH,
		RAS_FRAMEBUFFER_MAX,

		RAS_FRAMEBUFFER_CUSTOM,
	};

	/**
	 * Drawing types
	 */
	enum DrawType {
		RAS_WIREFRAME = 0,
		RAS_TEXTURED,
		RAS_RENDERER,
		RAS_SHADOW,
		RAS_DEPTH_PASS,
		RAS_DEPTH_PASS_CLIP,
		RAS_DRAW_MAX,
	};

	/**
	 * Valid SetDepthMask parameters
	 */
	enum DepthMask {
		RAS_DEPTHMASK_ENABLED = 1,
		RAS_DEPTHMASK_DISABLED,
	};

	/**
	 */
	enum {
		RAS_BACKCULL = 16, // GEMAT_BACKCULL
	};

	/**
	 * Stereo mode types
	 */
	enum StereoMode {
		RAS_STEREO_NOSTEREO = 1,
		// WARNING: Not yet supported.
		RAS_STEREO_QUADBUFFERED,
		RAS_STEREO_ABOVEBELOW,
		RAS_STEREO_INTERLACED,
		RAS_STEREO_ANAGLYPH,
		RAS_STEREO_SIDEBYSIDE,
		RAS_STEREO_VINTERLACE,
		RAS_STEREO_3DTVTOPBOTTOM,

		RAS_STEREO_MAXSTEREO
	};

	/**
	 * Render pass identifiers for stereo.
	 */
	enum StereoEye {
		RAS_STEREO_LEFTEYE = 0,
		RAS_STEREO_RIGHTEYE,
	};

	/**
	 * Mipmap options
	 */
	enum MipmapOption {
		RAS_MIPMAP_NONE,
		RAS_MIPMAP_NEAREST,
		RAS_MIPMAP_LINEAR,

		RAS_MIPMAP_MAX,  /* Should always be last */
	};

	enum ShadowType {
		RAS_SHADOW_NONE,
		RAS_SHADOW_SIMPLE,
		RAS_SHADOW_VARIANCE,
	};

	enum EnableBit {
		RAS_DEPTH_TEST = 0,
		RAS_ALPHA_TEST,
		RAS_SCISSOR_TEST,
		RAS_TEXTURE_2D,
		RAS_TEXTURE_CUBE_MAP,
		RAS_BLEND,
		RAS_COLOR_MATERIAL,
		RAS_CULL_FACE,
		RAS_FOG,
		RAS_LIGHTING,
		RAS_MULTISAMPLE,
		RAS_POLYGON_STIPPLE,
		RAS_POLYGON_OFFSET_FILL,
		RAS_POLYGON_OFFSET_LINE
	};

	enum DepthFunc {
		RAS_NEVER = 0,
		RAS_LEQUAL,
		RAS_LESS,
		RAS_ALWAYS,
		RAS_GEQUAL,
		RAS_GREATER,
		RAS_NOTEQUAL,
		RAS_EQUAL
	};

	enum BlendFunc {
		RAS_ZERO = 0,
		RAS_ONE,
		RAS_SRC_COLOR,
		RAS_ONE_MINUS_SRC_COLOR,
		RAS_DST_COLOR,
		RAS_ONE_MINUS_DST_COLOR,
		RAS_SRC_ALPHA,
		RAS_ONE_MINUS_SRC_ALPHA,
		RAS_DST_ALPHA,
		RAS_ONE_MINUS_DST_ALPHA,
		RAS_SRC_ALPHA_SATURATE
	};

	enum MatrixMode {
		RAS_PROJECTION = 0,
		RAS_MODELVIEW,
		RAS_TEXTURE,
		RAS_MATRIX_MODE_MAX
	};

	enum ClearBit {
		RAS_COLOR_BUFFER_BIT = 0x2,
		RAS_DEPTH_BUFFER_BIT = 0x4,
		RAS_STENCIL_BUFFER_BIT = 0x8
	};

	enum HdrType {
		RAS_HDR_NONE = 0,
		RAS_HDR_HALF_FLOAT,
		RAS_HDR_FULL_FLOAT,
		RAS_HDR_MAX
	};

	/** Return the output frame buffer normally used for the input frame buffer
	 * index in case of filters render.
	 * \param index The input frame buffer, can be a non-filter frame buffer.
	 * \return The output filter frame buffer.
	 */
	static RAS_Rasterizer::FrameBufferType NextFilterFrameBuffer(FrameBufferType index);

	/** Return the output frame buffer normally used for the input frame buffer
	 * index in case of simple render.
	 * \param index The input render frame buffer, can be a eye frame buffer.
	 * \return The output render frame buffer.
	 */
	static RAS_Rasterizer::FrameBufferType NextRenderFrameBuffer(FrameBufferType index);

private:

	class FrameBuffers
	{
	private:

		RAS_FrameBuffer *m_frameBuffers[RAS_FRAMEBUFFER_MAX];

		/* We need to free all textures at ge exit so we do member variables */
		GPUTexture *m_colorTextureList[RAS_FRAMEBUFFER_MAX];
		GPUTexture *m_depthTextureList[RAS_FRAMEBUFFER_MAX];
		unsigned int m_width;
		unsigned int m_height;
		int m_samples;
		HdrType m_hdr;

	public:
		FrameBuffers();
		~FrameBuffers();

		void Update(RAS_ICanvas *canvas);
		RAS_FrameBuffer *GetFrameBuffer(FrameBufferType type);
	};

	// All info used to compute the ray cast transform matrix.
	struct RayCastTranform
	{
		/// The object scale.
		MT_Vector3 scale;
		/// The original object matrix.
		float *origmat;
		/// The output matrix.
		float *mat;
	};

	struct ScreenShaders
	{
		DRWShadingGroup *normal;
		DRWShadingGroup *anaglyph;
		DRWShadingGroup *interlace;
		DRWShadingGroup *vinterlace;
	} m_screenShaders;

	struct Matrices
	{
		MT_Matrix4x4 view;
		MT_Matrix4x4 viewinv;
		MT_Matrix4x4 proj;
		MT_Matrix4x4 projinv;
		MT_Matrix4x4 pers;
		MT_Matrix4x4 persinv;
	} m_matrices;

	// We store each debug shape by scene.
	std::map<SCA_IScene *, RAS_DebugDraw> m_debugDraws;

	/* fogging vars */
	bool m_fogenabled;

	double m_time;
	MT_Vector3 m_ambient;
	MT_Matrix4x4 m_viewmatrix;
	MT_Matrix4x4 m_viewinvmatrix;
	MT_Vector3 m_campos;
	bool m_camortho;
	bool m_camnegscale;

	StereoMode m_stereomode;
	StereoEye m_curreye;
	float m_eyeseparation;
	float m_focallength;
	bool m_setfocallength;
	int m_noOfScanlines;

	/* motion blur */
	unsigned short m_motionblur;
	float m_motionblurvalue;

	/* Render tools */
	void *m_clientobject;
	void *m_auxilaryClientInfo;
	int m_lastlightlayer;
	bool m_lastlighting;
	void *m_lastauxinfo;
	unsigned int m_numgllights;

	/// Class used to manage off screens used by the rasterizer.
	FrameBuffers m_frameBuffers;

	DrawType m_drawingmode;
	ShadowType m_shadowMode;

	bool m_invertFrontFace;
	bool m_last_frontface;

	std::unique_ptr<RAS_OpenGLRasterizer> m_impl;

	void InitScreenShaders();
	void ExitScreenShaders();

	/*void EnableLights();
	void DisableLights();*/

public:
	RAS_Rasterizer();
	virtual ~RAS_Rasterizer();

	/**
	 * Enable capability
	 * \param bit Enable bit
	 */
	void Enable(EnableBit bit);

	/**
	 * Disable capability
	 * \param bit Enable bit
	 */
	void Disable(EnableBit bit);

	/**
	 * Set the value for Depth Buffer comparisons
	 * \param func Depth comparison function
	 */
	void SetDepthFunc(DepthFunc func);

	/** 
	 * Set the blending equation.
	 * \param src The src value.
	 * \param dst The destination value.
	 */
	void SetBlendFunc(BlendFunc src, BlendFunc dst);

	/**
	 * Takes a screenshot
	 */
	unsigned int *MakeScreenshot(int x, int y, int width, int height);

	/**
	 * SetDepthMask enables or disables writing a fragment's depth value
	 * to the Z buffer.
	 */
	void SetDepthMask(DepthMask depthmask);

	/**
	 * Init initializes the renderer.
	 */
	void Init();

	/**
	 * Exit cleans up the renderer.
	 */
	void Exit();

	/**
	 * BeginFrame is called at the start of each frame.
	 */
	void BeginFrame(double time);

	/**
	 * EndFrame is called at the end of each frame.
	 */
	void EndFrame();

	/**
	 * Clears a specified set of buffers
	 * \param clearbit What buffers to clear (separated by bitwise OR)
	 */
	void Clear(int clearbit);

	/**
	 * Set background color
	 */
	void SetClearColor(float r, float g, float b, float a=1.0f);

	/**
	 * Set background depth
	 */
	void SetClearDepth(float d);

	/**
	 * Set background color mask.
	 */
	void SetColorMask(bool r, bool g, bool b, bool a);

	/**
	 * Draw screen overlay plane with basic uv coordinates.
	 */
	void DrawOverlayPlane();

	/* ToneMap GPUTexture (ref: DRW_transform_to_display) */
	void ToneMapGpuTex(GPUTexture *gputex);

	/// Update dimensions of all off screens.
	void UpdateOffScreens(RAS_ICanvas *canvas);

	/** Return the corresponding off screen to off screen type.
	 * \param type The off screen type to return.
	 */
	RAS_FrameBuffer *GetFrameBuffer(FrameBufferType type);

	/** Draw off screen without set viewport.
	 * Used to copy the frame buffer object to another.
	 * \param srcindex The input off screen index.
	 * \param dstindex The output off screen index.
	 */
	void DrawFrameBuffer(RAS_FrameBuffer *srcfb, RAS_FrameBuffer *dstfb);

	/** Draw off screen at the given index to screen.
	 * \param canvas The canvas containing the screen viewport.
	 * \param index The off screen index to read from.
	 */
	void DrawFrameBuffer(RAS_ICanvas *canvas, RAS_FrameBuffer *frameBuffer);

	/** Draw each stereo off screen to screen.
	 * \param canvas The canvas containing the screen viewport.
	 * \param lefteyeindex The left off screen index.
	 * \param righteyeindex The right off screen index.
	 */
	void DrawStereoFrameBuffer(RAS_ICanvas *canvas, RAS_FrameBuffer *leftFb, RAS_FrameBuffer *rightFb);

	/**
	 * GetRenderArea computes the render area from the 2d canvas.
	 */
	RAS_Rect GetRenderArea(RAS_ICanvas *canvas, StereoEye eye);

	// Stereo Functions
	/**
	 * SetStereoMode will set the stereo mode
	 */
	void SetStereoMode(const StereoMode stereomode);

	/**
	 * Stereo can be used to query if the rasterizer is in stereo mode.
	 * \return true if stereo mode is enabled.
	 */
	bool Stereo();
	StereoMode GetStereoMode();

	/**
	 * Sets which eye buffer subsequent primitives will be rendered to.
	 */
	void SetEye(const StereoEye eye);
	StereoEye GetEye();

	/**
	 * Sets the distance between eyes for stereo mode.
	 */
	void SetEyeSeparation(const float eyeseparation);
	float GetEyeSeparation();

	/**
	 * Sets the focal length for stereo mode.
	 */
	void SetFocalLength(const float focallength);
	float GetFocalLength();

	/**
	 * Create a sync object
	 * For use with offscreen render
	 */
	RAS_ISync *CreateSync(int type);

	/// Render primitives using a derived mesh drawing.
	void IndexPrimitivesDerivedMesh(RAS_MeshSlot *ms);
	/// Render text mesh slot using BLF functions.
	void IndexPrimitivesText(RAS_MeshSlot *ms);
 
	/// Get the modelview matrix according to the stereo settings.
	MT_Matrix4x4 GetViewMatrix(StereoEye eye, const MT_Transform &camtrans, bool perspective);
	/**
	 * Sets the modelview matrix.
	 */
	void SetMatrix(const MT_Matrix4x4 &viewmat, const MT_Matrix4x4& projmat, const MT_Vector3 &pos, const MT_Vector3 &scale);

	/**
	 * Get/Set viewport area
	 */
	void SetViewport(int x, int y, int width, int height);

	/**
	 * Set scissor mask
	 */
	void SetScissor(int x, int y, int width, int height);

	/**
	 */
	const MT_Vector3& GetCameraPosition();
	bool GetCameraOrtho();

	/**
	 * Fog
	 */
	/*void SetFog(short type, float start, float dist, float intensity, const MT_Vector3& color);
	void DisplayFog();
	void EnableFog(bool enable);*/
	
	/**
	 * \param drawingmode = RAS_WIREFRAME, RAS_SOLID, RAS_SHADOW or RAS_TEXTURED.
	 */
	void SetDrawingMode(DrawType drawingmode);

	/**
	 * \return the current drawing mode: RAS_WIREFRAME, RAS_SOLID RAS_SHADOW or RAS_TEXTURED.
	 */
	DrawType GetDrawingMode();

	/// \param shadowmode = RAS_SHADOW_SIMPLE, RAS_SHADOW_VARIANCE.
	void SetShadowMode(ShadowType shadowmode);

	/// \return the current drawing mode: RAS_SHADOW_SIMPLE, RAS_SHADOW_VARIANCE.
	ShadowType GetShadowMode();

	/**
	 * Sets face culling
	 */
	void SetCullFace(bool enable);

	/// Set and enable clip plane.
	void EnableClipPlane(int numplanes);
	/// Disable clip plane
	void DisableClipPlane(int numplanes);

	/**
	 * Sets wireframe mode.
	 */
	void SetLines(bool enable);

	/**
	 */
	double GetTime();

	/**
	 * Generates a projection matrix from the specified frustum.
	 * \param left the left clipping plane
	 * \param right the right clipping plane
	 * \param bottom the bottom clipping plane
	 * \param top the top clipping plane
	 * \param frustnear the near clipping plane
	 * \param frustfar the far clipping plane
	 * \return a 4x4 matrix representing the projection transform.
	 */
	MT_Matrix4x4 GetFrustumMatrix(
			StereoEye eye,
	        float left, float right, float bottom, float top,
	        float frustnear, float frustfar,
	        float focallength = 0.0f, bool perspective = true);

	/**
	 * Generates a orthographic projection matrix from the specified frustum.
	 * \param left the left clipping plane
	 * \param right the right clipping plane
	 * \param bottom the bottom clipping plane
	 * \param top the top clipping plane
	 * \param frustnear the near clipping plane
	 * \param frustfar the far clipping plane
	 * \return a 4x4 matrix representing the projection transform.
	 */
	MT_Matrix4x4 GetOrthoMatrix(
	        float left, float right, float bottom, float top,
	        float frustnear, float frustfar);

	///**
	// * Sets the specular color component of the lighting equation.
	// */
	//void SetSpecularity(float specX, float specY, float specZ, float specval);
	//
	///**
	// * Sets the specular exponent component of the lighting equation.
	// */
	//void SetShinyness(float shiny);

	///**
	// * Sets the diffuse color component of the lighting equation.
	// */
	//void SetDiffuse(float difX,float difY, float difZ, float diffuse);

	///**
	// * Sets the emissive color component of the lighting equation.
	// */ 
	//void SetEmissive(float eX, float eY, float eZ, float e);
	//
	void SetAmbientColor(const MT_Vector3& color);
	//void SetAmbient(float factor);

	/**
	 * Sets a polygon offset.  z depth will be: z1 = mult*z0 + add
	 */
	void	SetPolygonOffset(float mult, float add);

	RAS_DebugDraw& GetDebugDraw(SCA_IScene *scene);
	void FlushDebugDraw(SCA_IScene *scene, RAS_ICanvas *canvas);

	const MT_Matrix4x4 &GetViewMatrix() const;
	const MT_Matrix4x4 &GetViewInvMatrix() const;
	const MT_Matrix4x4& GetProjMatrix() const;
	const MT_Matrix4x4& GetProjInvMatrix() const;
	const MT_Matrix4x4& GetPersMatrix() const;
	const MT_Matrix4x4& GetPersInvMatrix() const;

	/*void EnableMotionBlur(float motionblurvalue);
	void DisableMotionBlur();
	void SetMotionBlur(unsigned short state);*/

	void SetAlphaBlend(int alphablend);
	void SetFrontFace(bool ccw);

	void SetInvertFrontFace(bool invert);

	void SetAnisotropicFiltering(short level);
	short GetAnisotropicFiltering();

	void SetMipmapping(MipmapOption val);
	MipmapOption GetMipmapping();

	/// \see KX_RayCast
	bool RayHit(KX_ClientObjectInfo *client, KX_RayCast *result, RayCastTranform *raytransform);
	/// \see KX_RayCast
	bool NeedRayCast(KX_ClientObjectInfo *info, void *data);

	/**
	 * Render Tools
	 */
	void GetTransform(float *origmat, int objectdrawmode, float mat[16]);

	void DisableForText();
	/**
	 * Renders 3D text string using BFL.
	 * \param fontid	The id of the font.
	 * \param text		The string to render.
	 * \param size		The size of the text.
	 * \param dpi		The resolution of the text.
	 * \param color		The color of the object.
	 * \param mat		The Matrix of the text object.
	 * \param aspect	A scaling factor to compensate for the size.
	 */
	void RenderText3D(
	        int fontid, const std::string& text, int size, int dpi,
	        const float color[4], const float mat[16], float aspect);

	void ProcessLighting(bool uselights, const MT_Transform &trans, GPUShader *shader);

	void PushMatrix();
	void PopMatrix();
	void MultMatrix(const float mat[16]);
	void SetMatrixMode(MatrixMode mode);
	void LoadMatrix(const float mat[16]);
	void LoadIdentity();

	/** Set the current off screen depth to the global depth texture used by materials.
	 * In case of mutlisample off screen a blit to RAS_FrameBuffer_BLIT_DEPTH is procceed.
	 */
	void UpdateGlobalDepthTexture(RAS_FrameBuffer *frameBuffer);
	/// Set the global depth texture to an empty texture.
	void ResetGlobalDepthTexture();

	//void MotionBlur();

	void SetClientObject(void *obj);

	void SetAuxilaryClientInfo(void *inf);

	/**
	 * Prints information about what the hardware supports.
	 */
	void PrintHardwareInfo();
};

#endif  /* __RAS_RASTERIZER_H__ */