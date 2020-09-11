#include "rasterizer_storage_gx.h"

TexCacheRegion::TexCacheRegion() : _is_initialized(false) {
}

void TexCacheRegion::invalidate_cache() {
    ERR_FAIL_COND(!_is_initialized);

    GX_InvalidateTexRegion(&region);
}

GXTexRegion *TexCacheRegion::get_region()
{
    ERR_FAIL_COND_V(!_is_initialized, NULL);

    return &region;
}

TexCacheRegion::TexCacheSize TexCacheRegion::get_even_size() const
{
    ERR_FAIL_COND_V(!_is_initialized, TEXCACHE_NONE);

    return even_size;
}

TexCacheRegion::TexCacheSize TexCacheRegion::get_odd_size() const
{
    ERR_FAIL_COND_V(!_is_initialized, TEXCACHE_NONE);

    return odd_size;
}

u32 TexCacheRegion::get_even_pos() const
{
    ERR_FAIL_COND_V(!_is_initialized, 0);

    return tmem_even;
}

u32 TexCacheRegion::get_odd_pos() const
{
    ERR_FAIL_COND_V(!_is_initialized, 0);

    return tmem_odd;
}

bool TexCacheRegion::is_mipmap_cache() const
{
    ERR_FAIL_COND_V(!_is_initialized, false);

    return is_mipmap;
}

void TexCacheRegion::initialize(bool is_mipmap, u32 tmem_even, TexCacheSize size_even, u32 tmem_odd, TexCacheSize size_odd)
{
    this->is_mipmap = is_mipmap;
    this->tmem_even = tmem_even;
    this->tmem_odd = tmem_odd;
    this->even_size = even_size;
    this->odd_size = odd_size;

    GX_InitTexCacheRegion(&region, is_mipmap ? GX_TRUE : GX_FALSE, tmem_even, size_even, tmem_odd, size_odd);
    _is_initialized = true;
}

static Ref<TexCacheRegion> regions[8];

GXTexRegion *tex_region_callback(GXTexObj *tex, u8 map_id)
{
    // TODO: Dynamic texture region allocator?
    return regions[map_id]->get_region();
}

void RasterizerStorageGX::_setup_texture_regions() {
    u32 evens[] = {
        0x00000000, 0x00008000,
        0x00010000, 0x00018000,
        0x00020000, 0x00028000,
        0x00030000, 0x00038000,
    };
    u32 odds[] = {
        0x00040000, 0x00048000,
        0x00050000, 0x00058000,
        0x00060000, 0x00068000,
        0x00070000, 0x00078000
    };

    for(int i = 0; i < 8; i++)
    {
        regions[i].instance();
        regions[i]->initialize(false, evens[i], TexCacheRegion::TEXCACHE_32K, odds[i], TexCacheRegion::TEXCACHE_32K);
    }
    GX_SetTexRegionCallback(tex_region_callback);
}

/* TEXTURE API */

RID RasterizerStorageGX::texture_create() {
    Texture *texture = memnew(Texture);
    ERR_FAIL_COND_V(!texture, RID());
    return texture_owner.make_rid(texture);
}

void RasterizerStorageGX::texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, VisualServer::TextureType p_type, uint32_t p_flags) {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND(!t);
    if(t->image.is_null())
        t->image.instance();
    t->image->create(p_width, p_height, false, p_format);
}

#define IS_32_ALIGNED(ptr) ((reinterpret_cast<intptr_t>(ptr) % 32) == 0)

void RasterizerStorageGX::texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_level) {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND(!t);
    ERR_FAIL_COND(!IS_32_ALIGNED(p_image->get_data().read().ptr()));
    if(t->image.is_null())
        t->image.instance();
    t->image->create(p_image->get_width(), p_image->get_height(), false, p_image->get_format(), p_image->get_data());
}

void RasterizerStorageGX::texture_set_data_partial(RID p_texture, const Ref<Image> &p_image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int p_dst_mip, int p_level) {
    Texture *t = texture_owner.get(p_texture);

    ERR_FAIL_COND(!t);
    ERR_FAIL_COND_MSG(p_image.is_null(), "It's not a reference to a valid Image object.");
    ERR_FAIL_COND(src_w <= 0 || src_h <= 0);
    ERR_FAIL_COND(src_x < 0 || src_y < 0 || src_x + src_w > p_image->get_width() || src_y + src_h > p_image->get_height());

    t->image->blit_rect(p_image, Rect2(src_x, src_y, src_w, src_h), Vector2(dst_x, dst_y));
}

Ref<Image> RasterizerStorageGX::texture_get_data(RID p_texture, int p_level) const {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND_V(!t, Ref<Image>());
    return t->image;
}

void RasterizerStorageGX::texture_set_flags(RID p_texture, uint32_t p_flags) {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND(!t);
    t->flags = p_flags;
}

uint32_t RasterizerStorageGX::texture_get_flags(RID p_texture) const {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND_V(!t, 0);
    return t->flags;
}

Image::Format RasterizerStorageGX::texture_get_format(RID p_texture) const {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND_V(!t, Image::FORMAT_RGB8);
    ERR_FAIL_COND_V(t->image.is_null(), Image::FORMAT_RGB8);
    return t->image->get_format();
}

void RasterizerStorageGX::texture_set_path(RID p_texture, const String &p_path) {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND(!t);
    t->path = p_path;
}

String RasterizerStorageGX::texture_get_path(RID p_texture) const {
    Texture *t = texture_owner.getornull(p_texture);
    ERR_FAIL_COND_V(!t, String());
    return t->path;
}

/* RENDER TARGET API */

RID RasterizerStorageGX::render_target_create()
{
    RenderTarget *rt = memnew(RenderTarget);
    ERR_FAIL_COND_V(!rt, RID());
    rt->texture = texture_create();
    texture_allocate(rt->texture, 1, 1, 0, Image::FORMAT_RGBA8);
    return render_target_owner.make_rid(rt);
}

RID RasterizerStorageGX::render_target_get_texture(RID p_render_target) const
{
    RenderTarget *rt = render_target_owner.getornull(p_render_target);
    ERR_FAIL_COND_V(!rt, RID());
    return rt->texture;
}

void RasterizerStorageGX::render_target_set_size(RID p_render_target, int p_width, int p_height)
{
    RenderTarget *rt = render_target_owner.getornull(p_render_target);
    ERR_FAIL_COND(!rt);
    Ref<Image> img = texture_get_data(rt->texture, 0);
    ERR_FAIL_COND(img.is_null());
    img->resize(p_width, p_height, Image::INTERPOLATE_NEAREST);
}