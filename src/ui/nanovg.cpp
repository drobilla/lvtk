// Copyright 2022 Michael Fisher <mfisher@lvtk.org>
// SPDX-License-Identifier: ISC


#include <lvtk/ui/nanovg.hpp>
#include "../nanovg/nanovg_gl.h"
#include "Roboto-Regular.ttf.h"

using Surface = lvtk::nvg::Surface;

namespace lvtk {
namespace nvg {
namespace detail {
static constexpr const char* default_font_face = "sans";

#if defined(NANOVG_GL2)
static constexpr auto create  = nvgCreateGL2;
static constexpr auto destroy = nvgDeleteGL2;
#elif defined(NANOVG_GL3)
static constexpr auto create  = nvgCreateGL3;
static constexpr auto destroy = nvgDeleteGL3;
#else
#    error "No GL version specified for NanoVG"
#endif

} // namespace detail

class Surface::Context {
    int _font = 0;

public:
    Context() : ctx (detail::create (NVG_ANTIALIAS | NVG_STENCIL_STROKES)) {
        _font = nvgCreateFontMem (ctx, detail::default_font_face, (uint8_t*) Roboto_Regular_ttf, Roboto_Regular_ttf_size, 0);
    }

    ~Context() {
        if (ctx)
            detail::destroy (ctx);
    }

    void save() {
        stack.push_back (state);
        nvgSave (ctx);
    }

    void restore() {
        if (stack.empty())
            return;
        state = stack.back();
        stack.pop_back();
        nvgRestore (ctx);
    }

private:
    friend class Surface;
    NVGcontext* ctx { nullptr };
    struct State {
        NVGcolor color;
        Point<float> origin;
        Rectangle<float> clip;

        State& operator= (const State& o) {
            color  = o.color;
            origin = o.origin;
            clip   = o.clip;
            return *this;
        }
    };

    float scale = 1.f;
    State state;
    std::vector<State> stack;
};

Surface::Surface (float scale)
    : ctx (std::make_unique<Context>()) {
    ctx->scale = scale;
    set_fill (Color (0.f, 0.f, 0.f, 1.f));
}

Surface::~Surface() {
    ctx.reset();
}

float Surface::scale_factor() const noexcept {
    return ctx->scale;
}

void Surface::translate (const Point<int>& pt) {
    nvgTranslate (ctx->ctx, (float) pt.x, (float) pt.y);
}

void Surface::transform (const Affine& mat) {
    nvgTransform (ctx->ctx, mat.m00, mat.m01, mat.m02, mat.m10, mat.m11, mat.m12);
}

void Surface::clip (const Rectangle<int>& r) {
    ctx->state.clip = r.as<float>();
    nvgScissor (ctx->ctx,
                ctx->state.clip.x * ctx->scale,
                ctx->state.clip.y * ctx->scale,
                ctx->state.clip.width * ctx->scale,
                ctx->state.clip.height * ctx->scale);
}

void Surface::intersect_clip (const Rectangle<int>& r) {
    nvgIntersectScissor (ctx->ctx, r.x * ctx->scale, r.y * ctx->scale, r.width * ctx->scale, r.height * ctx->scale);
}

Rectangle<int> Surface::last_clip() const {
    return (ctx->state.clip).as<int>();
}

void Surface::set_fill (const Fill& fill) {
    auto c      = fill.color();
    auto& color = ctx->state.color;
    color.r     = c.fred();
    color.g     = c.fgreen();
    color.b     = c.fblue();
    color.a     = c.falpha();
}

void Surface::save() { ctx->save(); }
void Surface::restore() { ctx->restore(); }

void Surface::begin_frame (int width, int height, float pixel_ratio) {
    ctx->scale = 1.0;
    nvgBeginFrame (ctx->ctx, ctx->scale * (float) width, ctx->scale * (float) height, pixel_ratio);
}

void Surface::end_frame() {
    nvgEndFrame (ctx->ctx);
}

void Surface::fill_rect (const Rectangle<float>& r) {
    nvgBeginPath (ctx->ctx);

    nvgRect (ctx->ctx,
             (r.x * ctx->scale) + ctx->state.origin.x,
             (r.y * ctx->scale) + ctx->state.origin.y,
             r.width * ctx->scale,
             r.height * ctx->scale);

    nvgFillColor (ctx->ctx, ctx->state.color);
    nvgFill (ctx->ctx);
}

void Surface::__text_top_left (const std::string& text, float x, float y) {
    nvgFontSize (ctx->ctx, 15.f * ctx->scale);
    nvgFontFace (ctx->ctx, detail::default_font_face);
    nvgTextAlign (ctx->ctx, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
    nvgFillColor (ctx->ctx, ctx->state.color);

    nvgText (ctx->ctx,
             ctx->state.origin.x + (x * ctx->scale),
             ctx->state.origin.y + (y * ctx->scale),
             text.c_str(),
             nullptr);
}

} // namespace nvg
} // namespace lvtk
