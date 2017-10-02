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
* Contributor(s): Pierluigi Grassi, Porteries Tristan.
*
* ***** END GPL LICENSE BLOCK *****
*/

/** \file gameengine/Rasterizer/RAS_EeveeEffectsManager.cpp
*  \ingroup bgerast
*/

#include "RAS_ICanvas.h"
#include "RAS_Rasterizer.h"
#include "GPU_framebuffer.h"
#include "RAS_EeveeEffectsManager.h"
#include "RAS_SceneLayerData.h"

#include "BLI_math.h"
#include "DNA_world_types.h"

#include "KX_Scene.h"  // For DOF,
#include "KX_Camera.h" // motion blur and AO

extern "C" {
#  include "DRW_render.h"
}

RAS_EeveeEffectsManager::RAS_EeveeEffectsManager(EEVEE_Data *vedata, RAS_ICanvas *canvas, IDProperty *props, KX_Scene *scene):
m_canvas(canvas),
m_props(props),
m_scene(scene),
m_dofInitialized(false)
{
	m_stl = vedata->stl;
	m_psl = vedata->psl;
	m_txl = vedata->txl;
	m_fbl = vedata->fbl;
	m_effects = m_stl->effects;

	static const DRWTextureFormat dataTypeEnums[] = {
		DRW_TEX_RGB_11_11_10, // RAS_HDR_NONE
		DRW_TEX_RGBA_16, // RAS_HDR_HALF_FLOAT
		DRW_TEX_RGBA_32 // RAS_HDR_FULL_FLOAT
	};

	// Bloom
	m_bloomColorTex = DRW_texture_create_2D(m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1, dataTypeEnums[m_canvas->GetHdrType()], DRW_TEX_FILTER, nullptr);
	m_bloomDepthTex = DRW_texture_create_2D(m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1, DRW_TEX_DEPTH_24, DRWTextureFlag(0), NULL);
	DRWFboTexture fbbloomtex[2] = { { &m_bloomColorTex, dataTypeEnums[m_canvas->GetHdrType()], DRWTextureFlag(DRW_TEX_FILTER) },
									{ &m_bloomDepthTex, DRW_TEX_DEPTH_24, DRWTextureFlag(0) } };
	DRW_framebuffer_init_bge(&m_bloomTarget, &draw_engine_eevee_type,
		m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1, fbbloomtex, ARRAY_SIZE(fbbloomtex));

	// Camera Motion Blur
	m_shutter = BKE_collection_engine_property_value_get_float(m_props, "motion_blur_shutter");
	m_effects->motion_blur_samples = BKE_collection_engine_property_value_get_int(m_props, "motion_blur_samples");
	m_blurColorTex = DRW_texture_create_2D(m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1, dataTypeEnums[m_canvas->GetHdrType()], DRW_TEX_FILTER, nullptr);
	m_blurDepthTex = DRW_texture_create_2D(m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1, DRW_TEX_DEPTH_24, DRWTextureFlag(0), NULL);
	DRWFboTexture fbblurtex[2] = { { &m_blurColorTex, dataTypeEnums[m_canvas->GetHdrType()], DRWTextureFlag(DRW_TEX_FILTER) },
									{ &m_blurDepthTex, DRW_TEX_DEPTH_24, DRWTextureFlag(0) } };
	DRW_framebuffer_init_bge(&m_blurTarget, &draw_engine_eevee_type,
		m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1, fbblurtex, ARRAY_SIZE(fbblurtex));

	// Depth of field
	m_dofColorTex = DRW_texture_create_2D(m_canvas->GetWidth() / 2, m_canvas->GetHeight() / 2, dataTypeEnums[m_canvas->GetHdrType()], DRW_TEX_FILTER, nullptr);
	m_dofDepthTex = DRW_texture_create_2D(m_canvas->GetWidth() / 2, m_canvas->GetHeight() / 2, DRW_TEX_DEPTH_24, DRWTextureFlag(0), NULL);
	DRWFboTexture fbdoftex[2] = { { &m_dofColorTex, dataTypeEnums[m_canvas->GetHdrType()], DRWTextureFlag(DRW_TEX_FILTER) },
								  { &m_dofDepthTex, DRW_TEX_DEPTH_24, DRWTextureFlag(0) } };
	DRW_framebuffer_init_bge(&m_dofTarget, &draw_engine_eevee_type,
		m_canvas->GetWidth() / 2, m_canvas->GetHeight() / 2, fbdoftex, ARRAY_SIZE(fbdoftex));

	// Ambient occlusion
	m_useAO = m_effects->use_ao;

	// Volumetrics
	World *world = m_scene->GetBlenderScene()->world;
	m_useVolumetricNodes = (world && world->use_nodes && world->nodetree);
}

RAS_EeveeEffectsManager::~RAS_EeveeEffectsManager()
{
	/* Free textures */
	DRW_TEXTURE_FREE_SAFE(m_bloomColorTex);
	DRW_TEXTURE_FREE_SAFE(m_bloomDepthTex);
	DRW_TEXTURE_FREE_SAFE(m_blurColorTex);
	DRW_TEXTURE_FREE_SAFE(m_blurDepthTex);
	DRW_TEXTURE_FREE_SAFE(m_dofColorTex);
	DRW_TEXTURE_FREE_SAFE(m_dofDepthTex);
}

void RAS_EeveeEffectsManager::InitDof()
{
	if (m_effects->enabled_effects & EFFECT_DOF) {
		/* Depth Of Field */
		KX_Camera *cam = m_scene->GetActiveCamera();
		float sensorSize = cam->GetCameraData()->m_sensor_x;
		/* Only update params that needs to be updated */
		float scaleCamera = 0.001f;
		float sensorScaled = scaleCamera * sensorSize;
		m_effects->dof_params[2] = m_canvas->GetWidth() / (1.0f * sensorScaled);
	}
}

GPUFrameBuffer *RAS_EeveeEffectsManager::RenderBloom(RAS_Rasterizer *rasty, GPUFrameBuffer *inputfb)
{
	/* Bloom */
	if ((m_effects->enabled_effects & EFFECT_BLOOM) != 0) {
		struct GPUTexture *last;

		m_effects->source_buffer = GPU_framebuffer_depth_texture(inputfb);

		/* Extract bright pixels */
		copy_v2_v2(m_effects->unf_source_texel_size, m_effects->source_texel_size);
		m_effects->unf_source_buffer = m_effects->source_buffer;

		DRW_framebuffer_bind(m_fbl->bloom_blit_fb);
		DRW_draw_pass(m_psl->bloom_blit);

		/* Downsample */
		copy_v2_v2(m_effects->unf_source_texel_size, m_effects->blit_texel_size);
		m_effects->unf_source_buffer = m_txl->bloom_blit;

		DRW_framebuffer_bind(m_fbl->bloom_down_fb[0]);
		DRW_draw_pass(m_psl->bloom_downsample_first);

		last = m_txl->bloom_downsample[0];

		for (int i = 1; i < m_effects->bloom_iteration_ct; ++i) {
			copy_v2_v2(m_effects->unf_source_texel_size, m_effects->downsamp_texel_size[i - 1]);
			m_effects->unf_source_buffer = last;

			DRW_framebuffer_bind(m_fbl->bloom_down_fb[i]);
			DRW_draw_pass(m_psl->bloom_downsample);

			/* Used in next loop */
			last = m_txl->bloom_downsample[i];
		}

		/* Upsample and accumulate */
		for (int i = m_effects->bloom_iteration_ct - 2; i >= 0; --i) {
			copy_v2_v2(m_effects->unf_source_texel_size, m_effects->downsamp_texel_size[i]);
			m_effects->unf_source_buffer = m_txl->bloom_downsample[i];
			m_effects->unf_base_buffer = last;

			DRW_framebuffer_bind(m_fbl->bloom_accum_fb[i]);
			DRW_draw_pass(m_psl->bloom_upsample);

			last = m_txl->bloom_upsample[i];
		}

		/* Resolve */
		copy_v2_v2(m_effects->unf_source_texel_size, m_effects->downsamp_texel_size[0]);
		m_effects->unf_source_buffer = last;
		m_effects->unf_base_buffer = m_effects->source_buffer;

		rasty->SetViewport(0, 0, m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1);

		DRW_framebuffer_bind(m_bloomTarget);
		DRW_draw_pass(m_psl->bloom_resolve);

		return m_bloomTarget;
	}
	return inputfb;
}

GPUFrameBuffer *RAS_EeveeEffectsManager::RenderMotionBlur(RAS_Rasterizer *rasty, GPUFrameBuffer *inputfb)
{
	/* Motion Blur */
	if (BKE_collection_engine_property_value_get_bool(m_props, "motion_blur_enable")) {

		KX_Camera *cam = m_scene->GetActiveCamera();

		m_effects->source_buffer = GPU_framebuffer_depth_texture(inputfb);
		DRW_viewport_texture_list_get()->depth = GPU_framebuffer_depth_texture(inputfb);
		float camToWorld[4][4];
		cam->GetCameraToWorld().getValue(&camToWorld[0][0]);
		camToWorld[3][0] *= m_shutter;
		camToWorld[3][1] *= m_shutter;
		camToWorld[3][2] *= m_shutter;
		copy_m4_m4(m_effects->current_ndc_to_world, camToWorld);

		rasty->SetViewport(0, 0, m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1);

		DRW_framebuffer_bind(m_blurTarget);
		DRW_draw_pass(m_psl->motion_blur);

		float worldToCam[4][4];
		cam->GetWorldToCamera().getValue(&worldToCam[0][0]);
		worldToCam[3][0] *= m_shutter;
		worldToCam[3][1] *= m_shutter;
		worldToCam[3][2] *= m_shutter;
		copy_m4_m4(m_effects->past_world_to_ndc, worldToCam);

		return m_blurTarget;
	}
	return inputfb;
}

GPUFrameBuffer *RAS_EeveeEffectsManager::RenderDof(RAS_Rasterizer *rasty, GPUFrameBuffer *inputfb)
{
	/* Depth Of Field */
	if ((m_effects->enabled_effects & EFFECT_DOF) != 0) {

		if (!m_dofInitialized) {
			/* We need to initialize at runtime (not in constructor)
			 * to access m_scene->GetActiveCamera()
			 */
			InitDof();
			m_dofInitialized = true;
		}

		float clear_col[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		m_effects->source_buffer = GPU_framebuffer_depth_texture(inputfb);
		DRW_viewport_texture_list_get()->depth = GPU_framebuffer_depth_texture(inputfb);

		/* Downsample */
		DRW_framebuffer_bind(m_fbl->dof_down_fb);
		DRW_draw_pass(m_psl->dof_down);

		/* Scatter Far */
		m_effects->unf_source_buffer = m_txl->dof_down_far;
		copy_v2_fl2(m_effects->dof_layer_select, 0.0f, 1.0f);
		DRW_framebuffer_bind(m_fbl->dof_scatter_far_fb);
		DRW_framebuffer_clear(true, false, false, clear_col, 0.0f);
		DRW_draw_pass(m_psl->dof_scatter);

		/* Scatter Near */
		if ((m_effects->enabled_effects & EFFECT_BLOOM) != 0) {
			/* Reuse bloom half res buffer */
			m_effects->unf_source_buffer = m_txl->bloom_downsample[0];
		}
		else {
			m_effects->unf_source_buffer = m_txl->dof_down_near;
		}
		copy_v2_fl2(m_effects->dof_layer_select, 1.0f, 0.0f);
		DRW_framebuffer_bind(m_fbl->dof_scatter_near_fb);
		DRW_framebuffer_clear(true, false, false, clear_col, 0.0f);
		DRW_draw_pass(m_psl->dof_scatter);

		/* Resolve */
		DRW_framebuffer_bind(m_dofTarget);
		DRW_draw_pass(m_psl->dof_resolve);

		return m_dofTarget;
	}
	return inputfb;
}

void RAS_EeveeEffectsManager::UpdateAO(GPUFrameBuffer *inputfb)
{
	if (m_useAO) {
		/* Create stl->g_data->minmaxz from our depth texture.
		 * This texture is used as uniform if AO is enabled.
		 * See: DRW_shgroup_uniform_buffer(shgrp, "minMaxDepthTex", &vedata->stl->g_data->minmaxz);
		 */
		EEVEE_create_minmax_buffer(m_scene->GetEeveeData(), GPU_framebuffer_depth_texture(inputfb), -1);
	}
}

GPUFrameBuffer *RAS_EeveeEffectsManager::RenderVolumetrics(RAS_Rasterizer *rasty, GPUFrameBuffer *inputfb)
{
	if ((m_effects->enabled_effects & EFFECT_VOLUMETRIC) != 0 && m_useVolumetricNodes) {

		DRW_viewport_texture_list_get()->depth = GPU_framebuffer_depth_texture(inputfb);
		EEVEE_effects_replace_e_data_depth(GPU_framebuffer_depth_texture(inputfb));

		/* Compute volumetric integration at halfres. */
		DRW_framebuffer_texture_attach(m_fbl->volumetric_fb, m_stl->g_data->volumetric, 0, 0);
		EEVEE_SceneLayerData *sldata = (EEVEE_SceneLayerData *)(&m_scene->GetSceneLayerData()->GetData());
		if (sldata->volumetrics->use_colored_transmit) {
			DRW_framebuffer_texture_attach(m_fbl->volumetric_fb, m_stl->g_data->volumetric_transmit, 1, 0);
		}
		DRW_framebuffer_bind(m_fbl->volumetric_fb);
		DRW_draw_pass(m_psl->volumetric_integrate_ps);

		/* Resolve at fullres */
		rasty->SetViewport(0, 0, m_canvas->GetWidth() + 1, m_canvas->GetHeight() + 1);
		DRW_framebuffer_bind(inputfb);
		if (sldata->volumetrics->use_colored_transmit) {
			DRW_draw_pass(m_psl->volumetric_resolve_transmit_ps);
		}
		DRW_draw_pass(m_psl->volumetric_resolve_ps);

		///* Restore */
		DRW_framebuffer_texture_detach(m_stl->g_data->volumetric);
		if (sldata->volumetrics->use_colored_transmit) {
			DRW_framebuffer_texture_detach(m_stl->g_data->volumetric_transmit);
		}

		return inputfb;
	}
	return inputfb;
}


GPUFrameBuffer *RAS_EeveeEffectsManager::RenderEeveeEffects(RAS_Rasterizer *rasty, GPUFrameBuffer *inputfb)
{
	rasty->Disable(RAS_Rasterizer::RAS_DEPTH_TEST);

	UpdateAO(inputfb);

	inputfb = RenderVolumetrics(rasty, inputfb);

	inputfb = RenderMotionBlur(rasty, inputfb);

	inputfb = RenderDof(rasty, inputfb);

	inputfb = RenderBloom(rasty, inputfb);

	rasty->Enable(RAS_Rasterizer::RAS_DEPTH_TEST);
	
	return inputfb;
}