#include "rasterizer_gx.h"

#include <wiiuse/wpad.h>

#include "core/os/os.h"

#define FIFO_SIZE (256*1024)
#define WII_CONSOLE // REMOVE WHEN DONE DEBUGGING

void RasterizerGX::_blit_tex_to_efb(const Rect2& p_rect, RasterizerStorageGX::Texture *p_tex, Color bgcolor)
{
    ERR_FAIL_COND(!p_tex);

    GX_ClearVtxDesc();

    Mtx44 proj;
    guOrtho(proj, 0, screenMode->xfbHeight, 0, screenMode->fbWidth, 0, 300);
    GX_SetNumTevStages(1);
    GX_SetNumTexGens(1);
    GX_SetNumChans(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_LoadTexObj(&p_tex->tex_obj, GX_TEXMAP0);
    GX_SetCullMode(GX_CULL_NONE);
    Mtx mv;
    guMtxIdentity(mv);
    GX_LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);
    GXColor bg = {bgcolor.r * 255, bgcolor.g * 255, bgcolor.b * 255, bgcolor.a * 255};
    GX_SetTevKColor(GX_KCOLOR0, bg);
    GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);

    GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_KONST, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
    GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

    GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
        GX_Position2f32(p_rect.position.x + p_rect.size.x, p_rect.position.y + p_rect.size.y); // Top right
        GX_TexCoord2f32(1.0f,1.0f);
        GX_Position2f32(p_rect.position.x, p_rect.position.y + p_rect.size.y); // Top left
        GX_TexCoord2f32(0,1.0f);
        GX_Position2f32(p_rect.position.x + p_rect.size.x, p_rect.position.y); // Bottom right
        GX_TexCoord2f32(1.0f,0);
        GX_Position2f32(p_rect.position.x, p_rect.position.y); // bottom left
        GX_TexCoord2f32(0,0);
    GX_End();
}

bool RasterizerGX::is_wii_texture_format(Image::Format format)
{
    return (format >= Image::FORMAT_GX_I4 && format <= Image::FORMAT_GX_CMPR);
}

u8 RasterizerGX::convert_to_gx_format(Image::Format format)
{
    ERR_FAIL_COND_V(!is_wii_texture_format(format), 0);
    
    return format - Image::FORMAT_GX_I4;
}

Image::Format RasterizerGX::get_gx_convert_format(Image::Format format)
{
    if(is_wii_texture_format(format))
        return format;
    
    switch(format)
    { // TODO
        case Image::FORMAT_RGBA8:
        default:
        return Image::FORMAT_GX_RGBA8;
    }
}

void RasterizerGX::set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter) {
    if(p_image.is_null() || p_image->empty())
        return;
    
    RID tex_rid = storage.texture_create();
    storage.texture_set_data(tex_rid, p_image, 0);
    RasterizerStorageGX::Texture *tex = storage.texture_owner.get(tex_rid);

    printf("boot image format: %d\n", p_image->get_format());

    printf("GX buffer size needed: %d\n", GX_GetTexBufferSize(p_image->get_width(), p_image->get_height(), GX_TF_RGBA8, GX_FALSE, 1));
    printf("Image buffer size: %d\n", p_image->get_data().size());

    Ref<Image> image;
    if(is_wii_texture_format(p_image->get_format()))
        image = p_image;
    else
    {
        image.instance();
        image->create(p_image->get_width(), p_image->get_height(), false, p_image->get_format(), p_image->get_data());
        image->decompress(); // Just in case
        image->convert(get_gx_convert_format(p_image->get_format()));
    }

    printf("image format: %s\n", Image::format_names[image->get_format()]);

    u8 fmt = convert_to_gx_format(image->get_format()); // TODO
    #define IS_32_ALIGNED(ptr) ((reinterpret_cast<intptr_t>(ptr) % 32) == 0)
    if(!IS_32_ALIGNED(image->get_data().read().ptr()))
        printf("BOOT IMAGE DATA NOT 32 ALIGNED!\n");
    GX_InitTexObj(&tex->tex_obj, (void*)image->get_data().read().ptr(), image->get_width(), image->get_height(), fmt, GX_CLAMP, GX_CLAMP, GX_FALSE);
    Rect2 rect(0,0,OS::get_singleton()->get_window_size().width, OS::get_singleton()->get_window_size().height);
    /* TODO
    Rect2 rect;
    Size2 window_size = OS::get_singleton()->get_window_size();
    if(p_scale || p_image->get_width() > window_size.x || p_image->get_height() > window_size.y)
    {
        float aspect = (float)p_image->get_width() / (float)p_image->get_height();
        if(p_image->get_width() > window_size.x)
        {

        }
        else
        {

        }
    }
    else
    {

    }
    */
    _blit_tex_to_efb(rect, tex, p_color);
    GX_CopyDisp(framebuffer, GX_TRUE);
    GX_Flush();
    VIDEO_SetNextFramebuffer(framebuffer);
    console_visible = false;
    VIDEO_Flush();
    VIDEO_WaitVSync();
    storage.texture_owner.free(tex_rid);
}

void RasterizerGX::initialize()
{
    // Initialize the VI
    VIDEO_Init();

    // Configure the VI
    screenMode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(screenMode);

    // Allocate and setup the framebuffer
    framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenMode));
    #ifdef WII_CONSOLE
    console_fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenMode));
    console_init(console_fb,20,20,screenMode->fbWidth,screenMode->xfbHeight,screenMode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_SetNextFramebuffer(console_fb);
    printf("Hello, World!\n");
    console_visible = true;
    #else
    VIDEO_SetNextFramebuffer(framebuffer);
    console_visible = false;
    #endif
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

    GX_SetCullMode(GX_CULL_BACK);
    GX_CopyDisp(framebuffer,GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_InvVtxCache();
    GX_ClearVtxDesc();
}

void RasterizerGX::set_current_render_target(RID p_render_target)
{
    if(p_render_target.is_valid()) {
        RasterizerStorageGX::RenderTarget *rt = storage.render_target_owner.getornull(p_render_target);
        storage.current_rt = rt;
        ERR_FAIL_COND(!rt);

        RasterizerStorageGX::Texture *tex = storage.texture_owner.getornull(rt->texture);
        ERR_FAIL_COND(!tex);
        ERR_FAIL_COND(tex->image.is_null());
        GX_SetViewport(0,0,tex->image->get_width(),tex->image->get_height(),0,1);
        GX_SetScissor(0,0,tex->image->get_width(),tex->image->get_height());
    }
    else
    {
        storage.current_rt = NULL;
        GX_SetViewport(0,0,screenMode->fbWidth,screenMode->efbHeight,0,1);
    }
}

void RasterizerGX::clear_render_target(const Color &p_color)
{
    uint32_t col_32 = p_color.to_rgba32();
    uint8_t *col_bytes = reinterpret_cast<uint8_t*>(&col_32);
    GXColor gxcol = {col_bytes[0], col_bytes[1], col_bytes[2], col_bytes[3]};
    GX_SetCopyClear(gxcol, 0x00ffffff);
    GX_CopyDisp(framebuffer, GX_TRUE);
}

void RasterizerGX::blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen)
{
    RasterizerStorageGX::RenderTarget *rt = storage.render_target_owner.getornull(p_render_target);
    ERR_FAIL_COND(!rt);
    RasterizerStorageGX::Texture *tex = storage.texture_owner.getornull(rt->texture);
    ERR_FAIL_COND(!tex);
    ERR_FAIL_COND(tex->image.is_null());

    //GX_InitTexObj(&tex->tex_obj, (void*)tex->image->get_data().read().ptr(), tex->image->get_width(), tex->image->get_height(), GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    //_blit_tex_to_efb(p_screen_rect, tex);
}

void RasterizerGX::end_frame(bool p_swap_buffers)
{
    #ifdef WII_CONSOLE
    WPAD_ScanPads();
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
    {
        console_visible = !console_visible;
        VIDEO_SetNextFramebuffer(console_visible ? console_fb : framebuffer);
        VIDEO_Flush();
    }
    #endif
    GX_CopyDisp(framebuffer, GX_TRUE);
    GX_Flush();
    VIDEO_WaitVSync();
}