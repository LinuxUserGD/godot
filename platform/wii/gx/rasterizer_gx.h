#ifndef RASTERIZER_GX_H
#define RASTERIZER_GX_H

#include "core/math/camera_matrix.h"
#include "core/self_list.h"
#include "scene/resources/mesh.h"
#include "servers/visual/rasterizer.h"
#include "servers/visual_server.h"

#include "rasterizer_scene_gx.h"
#include "rasterizer_canvas_gx.h"
#include "rasterizer_storage_gx.h"

#include <wiiuse/wpad.h>

#define FIFO_SIZE (256*1024)

class RasterizerGX : public Rasterizer {
protected:
	RasterizerCanvasGX canvas;
	RasterizerStorageGX storage;
	RasterizerSceneGX scene;

	void *fifo_buf;
	void *framebuffer;

	GXRModeObj *screenMode;

public:
	RasterizerStorage *get_storage() { return &storage; }
	RasterizerCanvas *get_canvas() { return &canvas; }
	RasterizerScene *get_scene() { return &scene; }

	void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true) {}
	void set_shader_time_scale(float p_scale) {}

	void initialize()
	{
		// Initialize the VI
		VIDEO_Init();

		// Configure the VI
		screenMode = VIDEO_GetPreferredMode(NULL);
		VIDEO_Configure(screenMode);

		// Allocate and setup the framebuffer
		framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenMode));
		#define WII_CONSOLE
		#ifdef WII_CONSOLE
		console_init(framebuffer,20,20,screenMode->fbWidth,screenMode->xfbHeight,screenMode->fbWidth*VI_DISPLAY_PIX_SZ);
		printf("Hello, World!\n");
		#endif
		VIDEO_SetNextFramebuffer(framebuffer);
		VIDEO_SetBlack(false);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		if(screenMode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

		// Setup FIFO
		fifo_buf = memalign(32, FIFO_SIZE);
		memset(fifo_buf, 0, FIFO_SIZE);
		GX_Init(fifo_buf, FIFO_SIZE);

		storage._setup_texture_regions();
		GX_InvalidateTexAll();

		// Configure GX
		GX_SetViewport(0,0,screenMode->fbWidth,screenMode->efbHeight,0,1); // TODO: Environment options
		f32 yscale = GX_GetYScaleFactor(screenMode->efbHeight,screenMode->xfbHeight);
		u32 xfbHeight = GX_SetDispCopyYScale(yscale);
		GX_SetDispCopySrc(0,0,screenMode->fbWidth,screenMode->efbHeight);
		GX_SetDispCopyDst(screenMode->fbWidth,xfbHeight);
		GX_SetCopyFilter(screenMode->aa,screenMode->sample_pattern,GX_TRUE,screenMode->vfilter);
		//

		GX_SetCullMode(GX_CULL_BACK);
		GX_CopyDisp(framebuffer,GX_TRUE);
		GX_SetDispCopyGamma(GX_GM_1_0);

		GX_InvVtxCache();
		GX_ClearVtxDesc();
	}
	void begin_frame(double frame_step) {}
	void set_current_render_target(RID p_render_target)
	{
		if(p_render_target.is_valid()) {
			RasterizerStorageGX::RenderTarget *rt = storage.render_target_owner.getornull(p_render_target);
			storage.current_rt = rt;
			ERR_FAIL_COND(!rt);

			RasterizerStorageGX::Texture *tex = storage.texture_owner.getornull(rt->texture);
			ERR_FAIL_COND(!tex);
			ERR_FAIL_COND(tex->image.is_null());
			GX_SetViewport(0,0,tex->image->get_width(),tex->image->get_height(),0,1);
		}
		else
		{
			storage.current_rt = NULL;
			GX_SetViewport(0,0,screenMode->fbWidth,screenMode->efbHeight,0,1);
		}
	}
	void restore_render_target(bool p_3d_was_drawn) {}
	void clear_render_target(const Color &p_color)
	{
		uint32_t col_32 = p_color.to_rgba32();
		uint8_t *col_bytes = reinterpret_cast<uint8_t*>(&col_32);
		GXColor gxcol = {col_bytes[0], col_bytes[1], col_bytes[2], col_bytes[3]};
		GX_SetCopyClear(gxcol, 0x00ffffff);
	}
	void blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen = 0)
	{
		f32 vertices[8] = {
			p_screen_rect.position.x, p_screen_rect.position.y + p_screen_rect.size.y,
			p_screen_rect.position.x + p_screen_rect.size.x, p_screen_rect.position.y + p_screen_rect.size.y,
			p_screen_rect.position.x + p_screen_rect.size.x, p_screen_rect.position.y,
			p_screen_rect.position.x, p_screen_rect.position.y
		};

		RasterizerStorageGX::RenderTarget *rt = storage.render_target_owner.getornull(p_render_target);
		ERR_FAIL_COND(!rt);
		RasterizerStorageGX::Texture *tex = storage.texture_owner.getornull(rt->texture);
		ERR_FAIL_COND(!tex);
		ERR_FAIL_COND(tex->image.is_null());

		Mtx44 proj;
		guOrtho(proj, 0, screenMode->efbHeight, 0, screenMode->fbWidth, 0, 300);
		GX_SetNumTevStages(1);
		GX_SetNumTexGens(1);
		GX_SetNumChans(1);
		GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
		GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_SetArray(GX_VA_POS, vertices, sizeof(f32)*2);
		GX_LoadTexObj(&tex->tex_obj, GX_TEXMAP0);
		GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, 4);
			GX_Position1x8(0);
			GX_TexCoord2f32(0,0);
			GX_Position1x8(1);
			GX_TexCoord2f32(1,0);
			GX_Position1x8(2);
			GX_TexCoord2f32(1,1);
			GX_Position1x8(3);
			GX_TexCoord2f32(0,1);
		GX_End();
	}
	void output_lens_distorted_to_screen(RID p_render_target, const Rect2 &p_screen_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample) {}
	void end_frame(bool p_swap_buffers)
	{
		GX_CopyDisp(framebuffer, GX_TRUE);
		GX_Flush();
		VIDEO_WaitVSync();
	}
	void finalize() {}

	static Error is_viable() {
		return OK;
	}

	static void make_current() {
		_create_func = _create_current;
	}

	virtual bool is_low_end() const { return true; }

	RasterizerGX() {}
	~RasterizerGX() {}

	static Rasterizer *_create_current() {
		return memnew(RasterizerGX);
	}
};

#endif // RASTERIZER_GX_H
