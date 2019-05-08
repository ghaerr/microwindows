#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_p.h"
#include "agg_renderer_scanline.h"
#include "agg_pixfmt_rgb.h"

#include "agg_ellipse.h"
#include "agg_rounded_rect.h"
#include "agg_conv_curve.h"
#include "agg_conv_stroke.h"

#include "agg_font_freetype.h"
#include "agg_scanline_u.h"
#include "agg_scanline_bin.h"
#include "agg_conv_contour.h"
#include "agg_renderer_primitives.h"

#include "platform/agg_platform_support.h"
#include "ctrl/agg_slider_ctrl.h"
#include "ctrl/agg_cbox_ctrl.h"
#include "pixel_formats.h"

//#define FONT	"../fonts/truetype/timesi.ttf"
//#define FONT	"../../../nuklear/extra_font/Roboto-Light.ttf"
#define FONT	"../../../nuklear/extra_font/Roboto-Regular.ttf"
//#define FONT	"../../../nuklear/extra_font/ProggyClean.ttf"

enum flip_y_e { flip_y = true };
bool font_flip_y = !flip_y;


class the_application : public agg::platform_support
{
    double m_x[2];
    double m_y[2];
    double m_dx;
    double m_dy;
    int    m_idx;
	double m_offset;
    agg::slider_ctrl<color_type> m_radius;
    agg::slider_ctrl<color_type> m_thickness;
    agg::cbox_ctrl<color_type>   m_white_on_black;

    typedef agg::renderer_base<pixfmt> base_ren_type;
    typedef agg::renderer_scanline_aa_solid<base_ren_type> renderer_solid;
    typedef agg::renderer_scanline_bin_solid<base_ren_type> renderer_bin;
    typedef agg::font_engine_freetype_int32 font_engine_type;
    typedef agg::font_cache_manager<font_engine_type> font_manager_type;
    font_engine_type             m_feng;
	font_manager_type            m_fman;
    // Pipeline to process the vectors glyph paths (curves + contour)
    agg::conv_curve<font_manager_type::path_adaptor_type> m_curves;
    agg::conv_contour<agg::conv_curve<font_manager_type::path_adaptor_type> > m_contour;


public:
    the_application(agg::pix_format_e format, bool flip_y) :
        agg::platform_support(format, flip_y),
        m_idx(-1),
		m_offset(0.5),
        m_radius(10, 10, 600-10,   19,    !flip_y),
        m_thickness(10, 10+20, 600-10, 19+20, !flip_y),
        m_white_on_black(10, 10+40, "White on black"),
		m_feng(),
		m_fman(m_feng),
        m_curves(m_fman.path_adaptor()),
        m_contour(m_curves)
    {
        m_x[0] = 100;   m_y[0] = 100;
        m_x[1] = 500;   m_y[1] = 350;
        add_ctrl(m_radius);
        add_ctrl(m_thickness);
        add_ctrl(m_white_on_black);

        m_radius.label("radius=%4.3f");
        m_radius.range(0.0, 50.0);
        m_radius.value(25.0);

        m_thickness.label("thickness=%4.3f");
        m_thickness.range(0.1, 20.0);
        m_thickness.value(1.0);

        m_white_on_black.text_color(agg::srgba8(127, 127, 127));
        m_white_on_black.inactive_color(agg::srgba8(127, 127, 127));

        m_curves.approximation_scale(2.0);
        m_contour.auto_detect_orientation(false);
    }


    virtual void on_draw()
    {
        typedef agg::renderer_base<pixfmt> renderer_base;
        typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_solid;

        pixfmt pixf(rbuf_window());
        renderer_base rb(pixf);
        renderer_solid ren(rb);
        renderer_bin ren_bin(rb);

        rb.clear(m_white_on_black.status() ? agg::rgba(0,0,0) : agg::rgba(1,1,1));

        agg::rasterizer_scanline_aa<> ras;
        agg::scanline_p8 sl;

        // Render two "control" circles
        agg::ellipse c;
        ren.color(agg::srgba8(127,127,127));
        c.init(m_x[0], m_y[0], 3, 3, 16);
        ras.add_path(c);
        agg::render_scanlines(ras, sl, ren);
        c.init(m_x[1], m_y[1], 3, 3, 16);
        ras.add_path(c);
        agg::render_scanlines(ras, sl, ren);

		agg::path_storage path;

		// Create a line
		path.move_to(400, 200);
		path.line_to(450, 250);

        // Fill line
        agg::conv_stroke<agg::path_storage> l(path);
        l.width(m_thickness.value());
        ras.add_path(l);

		// Create a polygon
		path.remove_all();
		path.move_to(220, 60);
		path.line_to(469, 170);
		path.line_to(243, 310);
		path.close_polygon();

        // Fill polygon
        agg::conv_stroke<agg::path_storage> o(path);
        o.width(m_thickness.value());
        ras.add_path(o);

        // Creating a rounded rectangle
        double d = m_offset;
        agg::rounded_rect r(m_x[0]+d, m_y[0]+d, m_x[1]+d, m_y[1]+d, m_radius.value());
        r.normalize_radius();

        // Drawing as an outline
        agg::conv_stroke<agg::rounded_rect> p(r);
        p.width(m_thickness.value());
        ras.add_path(p);

		// Create ellipse
		agg::ellipse e;
		e.init(225, 225, 100, 100);
		agg::conv_stroke<agg::ellipse> q(e);
        q.width(m_thickness.value());
		ras.add_path(q);

		// Create bezier curve
		agg::curve4 curve;
		path.remove_all();
		curve.approximation_method(agg::curve_inc);
		curve.approximation_scale(1.0);
		curve.angle_tolerance(15);
		curve.cusp_limit(0);
		//curve.init(300, 225, 350, 250, 250, 200, 400, 250);
		curve.init(270, 424, 113, 86, 588, 423, 126, 333);
		path.concat_path(curve);

		agg::conv_stroke<agg::path_storage> stroke(path);
		stroke.width(m_thickness.value());
		stroke.line_cap(agg::butt_cap);
		stroke.line_join(agg::miter_join);
		stroke.inner_join(agg::inner_miter);
		stroke.inner_miter_limit(1.01);
		ras.add_path(stroke);

		// Draw text
		agg::glyph_rendering gren = agg::glyph_ren_native_gray8;
		double m_height = 12.0;
		double m_width = 12.0;
		double m_weight = 0.0;
		int m_kerning = 1;

        m_contour.width(-m_weight * m_height * 0.05);

        if(m_feng.load_font(full_file_name(FONT), 0, gren))
        {
            m_feng.hinting(1);
            m_feng.height(m_height);
            m_feng.width(m_width);
            m_feng.flip_y(font_flip_y);

            //agg::trans_affine mtx;
            //mtx *= agg::trans_affine_rotation(agg::deg2rad(-4.0));
            ////mtx *= agg::trans_affine_skewing(-0.4, 0);
            ////mtx *= agg::trans_affine_translation(1, 0);
            //m_feng.transform(mtx);

            double x  = 10.0;
            double y0 = height() - m_height - 10.0;
            double y  = y0;
			const char *p = "NodeEdit Source";

            while(*p)
			{
                const agg::glyph_cache* glyph = m_fman.glyph(*p);
                if(glyph)
                {
                    if(m_kerning)
                    {
                        m_fman.add_kerning(&x, &y);
                    }

                    if(x >= width() - m_height)
                    {
                        x = 10.0;
                        y0 -= m_height;
                        //if(y0 <= 120) break;
                        y = y0;
                    }

                    m_fman.init_embedded_adaptors(glyph, x, y);

                    switch(glyph->data_type)
                    {
                    default: break;
                    case agg::glyph_data_mono:
                        //ren_bin.color(agg::srgba8(0, 0, 0));
						ren_bin.color(agg::rgba(0.75,0.75,0.75));
                        agg::render_scanlines(m_fman.mono_adaptor(), 
                                              m_fman.mono_scanline(), 
                                              ren_bin);
                        break;

                    case agg::glyph_data_gray8:
                        //ren.color(agg::srgba8(0, 0, 0));
						ren.color(agg::rgba(0.75,0.75,0.75));
                        agg::render_scanlines(m_fman.gray8_adaptor(), 
                                              m_fman.gray8_scanline(), 
                                              ren);
                        break;

                    case agg::glyph_data_outline:
                        ras.reset();
                        if(fabs(m_weight) <= 0.01)
                        {
                            // For the sake of efficiency skip the
                            // contour converter if the weight is about zero.
                            //-----------------------
                            ras.add_path(m_curves);
                        }
                        else
                        {
                            ras.add_path(m_contour);
                        }
                        //ren.color(agg::srgba8(0, 0, 0));
						ren.color(agg::rgba(0.75,0.75,0.75));
                        agg::render_scanlines(ras, sl, ren);
//dump_path(m_fman.path_adaptor());
                        break;
                    }

                    // increment pen position
                    x += glyph->advance_x;
                    y += glyph->advance_y;
                    //++num_glyphs;
                }
                ++p;
			}
		}

		// Render paths
        //ren.color(m_white_on_black.status() ? agg::rgba(1,1,1) : agg::rgba(0,0,0));
		ren.color(agg::rgba(0.75,0.75,0.75));
        agg::render_scanlines(ras, sl, ren);

        // Render the controls
        agg::render_ctrl(ras, sl, rb, m_radius);
        agg::render_ctrl(ras, sl, rb, m_thickness);
        agg::render_ctrl(ras, sl, rb, m_white_on_black);
    }


    virtual void on_mouse_button_down(int x, int y, unsigned flags)
    {
        if(flags & agg::mouse_left)
        {
            unsigned i;
            for (i = 0; i < 2; i++)
            {
                if(sqrt( (x-m_x[i]) * (x-m_x[i]) + (y-m_y[i]) * (y-m_y[i]) ) < 5.0)
                {
                    m_dx = x - m_x[i];
                    m_dy = y - m_y[i];
                    m_idx = i;
                    break;
                }
            }
        }
    }


    virtual void on_mouse_move(int x, int y, unsigned flags)
    {
        if(flags & agg::mouse_left)
        {
            if(m_idx >= 0)
            {
                m_x[m_idx] = x - m_dx;
                m_y[m_idx] = y - m_dy;
                force_redraw();
            }
        }
        else
        {
            on_mouse_button_up(x, y, flags);
        }
    }

    virtual void on_mouse_button_up(int x, int y, unsigned flags)
    {
        m_idx = -1;
    }


};


int agg_main(int argc, char* argv[])
{
    the_application app(pix_format, flip_y);
    app.caption("Nuklear Shapes");

    if(app.init(600, 400, agg::window_resize))
    {
        return app.run();
    }
    return 1;
}
