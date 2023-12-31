/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "xmms.h"

static const gfloat vis_afalloff_speeds[] = {0.34, 0.5, 1.0, 1.3, 1.6};
static const gfloat vis_pfalloff_speeds[] = {1.2, 1.3, 1.4, 1.5, 1.6};
static const gint vis_redraw_delays[] = {1, 2, 4, 8};
static const guint8 vis_scope_colors[] =
{21, 21, 20, 20, 19, 19, 18, 19, 19, 20, 20, 21, 21};

void vis_timeout_func(Vis * vis, guchar * data)
{
	static GTimer *timer = NULL;
	gulong micros = 9999999;
	gboolean falloff = FALSE;
	gint i;

	if (!timer)
	{
		timer = g_timer_new();
		g_timer_start(timer);
	}
	else
	{
		g_timer_elapsed(timer, &micros);
		if (micros > 14000)
			g_timer_reset(timer);

	}

	if (cfg.vis_type == VIS_ANALYZER)
	{
		if (micros > 14000)
			falloff = TRUE;
		if (data || falloff)
		{
			for (i = 0; i < 75; i++)
			{
				if (data && data[i] > vis->vs_data[i])
				{
					vis->vs_data[i] = data[i];
					if (vis->vs_data[i] > vis->vs_peak[i])
					{
						vis->vs_peak[i] = vis->vs_data[i];
						vis->vs_peak_speed[i] = 0.01;

					}
					else if (vis->vs_peak[i] > 0.0)
					{
						vis->vs_peak[i] -= vis->vs_peak_speed[i];
						vis->vs_peak_speed[i] *= vis_pfalloff_speeds[cfg.peaks_falloff];
						if (vis->vs_peak[i] < vis->vs_data[i])
							vis->vs_peak[i] = vis->vs_data[i];
						if (vis->vs_peak[i] < 0.0)
							vis->vs_peak[i] = 0.0;
					}
				}
				else if (falloff)
				{
					if (vis->vs_data[i] > 0.0)
					{
						vis->vs_data[i] -= vis_afalloff_speeds[cfg.analyzer_falloff];
						if (vis->vs_data[i] < 0.0)
							vis->vs_data[i] = 0.0;
					}
					if (vis->vs_peak[i] > 0.0)
					{
						vis->vs_peak[i] -= vis->vs_peak_speed[i];
						vis->vs_peak_speed[i] *= vis_pfalloff_speeds[cfg.peaks_falloff];
						if (vis->vs_peak[i] < vis->vs_data[i])
							vis->vs_peak[i] = vis->vs_data[i];
						if (vis->vs_peak[i] < 0.0)
							vis->vs_peak[i] = 0.0;
					}
				}
			}
		}
	}
	else if (data)
	{
		for (i = 0; i < 75; i++)
			vis->vs_data[i] = data[i];
	}

	if (micros > 14000)
	{
		if (!vis->vs_refresh_delay)
		{
			vis_draw((Widget *) vis);
			vis->vs_refresh_delay = vis_redraw_delays[cfg.vis_refresh];

		}
		vis->vs_refresh_delay--;
	}
}

static GdkBitmap *create_vis_clip_mask(GdkWindow *win, guchar *rgb_data, guchar bg, gboolean double_sized)
{
	GdkBitmap *mask_bitmap;
	GdkGC *gc;
	GdkImage *mask_image;
	GdkColor pattern;
	guchar *mask, *p;
	gint x, y, i, b;
	gint w, h;

	p = mask = g_malloc(double_sized ? 19 * 32 : 10 * 16);
	if (double_sized) {
		w = 152;
		h = 32;
		for (y = 0; y < 32; y += 2) {
			for (x = 0; x < 152; x += 8) {
				b = 0;
				for (i = 0; i < 8; i += 2) {
					b <<= 2;
					b |= (rgb_data[y * 152 + x + i] == bg) ? 3 : 0;
				}
				*(p +  0) = b;
				*(p + 19) = b;
				p++;
			}
			p += 19;
		}
	} else {
		w = 76;
		h = 16;
		for (y = 0; y < 16; y++) {
			for (x = 0; x < 72; x += 8) {
				b = 0;
				for (i = 0; i < 8; i++) {
					b <<= 1;
					b |= (rgb_data[y * 76 + x + i] == bg) ? 1 : 0;
				}
				*p++ = b;
			}
			b = 0;
			for (i = 0; i < 4; i++) {
				b <<= 1;
				b |= (rgb_data[y * 76 + x + i] == bg) ? 1 : 0;
			}
			b <<= 4;
			*p++ = b;
		}
	}

	mask_image = gdk_image_new_bitmap(gdk_visual_get_best(), mask, w, h);
	mask_bitmap = gdk_pixmap_new(win, w, h, 1);
	gc = gdk_gc_new(mask_bitmap);
	pattern.pixel = 0;
	gdk_gc_set_foreground(gc, &pattern);
	pattern.pixel = 1;
	gdk_gc_set_background(gc, &pattern);
	gdk_draw_image(mask_bitmap, gc, mask_image, 0, 0, 0, 0, w, h);
	gdk_gc_destroy(gc);
	gdk_image_destroy(mask_image);

	return mask_bitmap;
}

void vis_draw(Widget * w)
{
	Vis *vis = (Vis *) w;
	gint x, y, h = 0, h2;
	guchar vis_color[24][3];
	guchar rgb_data[152 * 32], *ptr, c;
	guint32 colors[24];
	GdkRgbCmap *cmap;

	if (!vis->vs_widget.visible)
		return;

	GDK_THREADS_ENTER();

	get_skin_viscolor(vis_color);
	for (y = 0; y < 24; y++)
	{
		colors[y] = vis_color[y][0] << 16 | vis_color[y][1] << 8 | vis_color[y][2];
	}
	cmap = gdk_rgb_cmap_new(colors, 24);

	if (!vis->vs_doublesize)
	{
		memset(rgb_data, 0, 76 * 16);
		if (!cfg.vis_transparent)
		{
			for (y = 1; y < 16; y += 2)
			{
				ptr = rgb_data + (y * 76);
				for (x = 0; x < 76; x += 2, ptr += 2)
					*ptr = 1;
			}
		}
		if (cfg.vis_type == VIS_ANALYZER)
		{
			for (x = 0; x < 75; x++)
			{
				if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
					h = vis->vs_data[x >> 2];
				else if (cfg.analyzer_type == ANALYZER_LINES)
					h = vis->vs_data[x];
				if (h && (cfg.analyzer_type == ANALYZER_LINES ||
					  (x % 4) != 3))
				{
					ptr = rgb_data + ((16 - h) * 76) + x;
					switch (cfg.analyzer_mode)
					{
						case ANALYZER_NORMAL:
							for (y = 0; y < h; y++, ptr += 76)
								*ptr = 18 - h + y;
							break;
						case ANALYZER_FIRE:
							for (y = 0; y < h; y++, ptr += 76)
								*ptr = y + 2;
							break;
						case ANALYZER_VLINES:
							for (y = 0; y < h; y++, ptr += 76)
								*ptr = 18 - h;
							break;
					}
				}
			}
			if (cfg.analyzer_peaks)
			{
				for (x = 0; x < 75; x++)
				{
					if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
						h = vis->vs_peak[x >> 2];
					else if(cfg.analyzer_type == ANALYZER_LINES)
						h = vis->vs_peak[x];
					if (h && (cfg.analyzer_type == ANALYZER_LINES || (x % 4) != 3))
						rgb_data[(16 - h) * 76 + x] = 23;
				}
			}
		}
		else if (cfg.vis_type == VIS_SCOPE)
		{
			for (x = 0; x < 75; x++)
			{
				switch (cfg.scope_mode)
				{
					case SCOPE_DOT:
						h = vis->vs_data[x];
						ptr = rgb_data + ((15 - h) * 76) + x;
						*ptr = vis_scope_colors[h];
						break;
					case SCOPE_LINE:
						if (x != 74)
						{
							h = 15 - vis->vs_data[x];
							h2 = 15 - vis->vs_data[x + 1];
							if (h > h2)
							{
								y = h;
								h = h2;
								h2 = y;
							}
							ptr = rgb_data + (h * 76) + x;
							for (y = h; y <= h2; y++, ptr += 76)
								*ptr = vis_scope_colors[y - 3];

						}
						else
						{
							h = 15 - vis->vs_data[x];
							ptr = rgb_data + (h * 76) + x;
							*ptr = vis_scope_colors[h];
						}
						break;
					case SCOPE_SOLID:
						h = 15 - vis->vs_data[x];
						h2 = 9;
						c = vis_scope_colors[(gint) vis->vs_data[x]];
						if (h > h2)
						{
							y = h;
							h = h2;
							h2 = y;
						}
						ptr = rgb_data + (h * 76) + x;
						for (y = h; y <= h2; y++, ptr += 76)
							*ptr = c;
						break;
				}
			}
		}

		if (cfg.vis_transparent)
		{
			GdkBitmap *mask;

			mask = create_vis_clip_mask(vis->vs_window, rgb_data, 0, 0);
			gdk_gc_set_clip_mask(vis->vs_widget.gc, mask);
			gdk_gc_set_clip_origin(vis->vs_widget.gc, vis->vs_widget.x, vis->vs_widget.y);
			vis_clear(vis);
			gdk_draw_indexed_image(vis->vs_window, vis->vs_widget.gc, vis->vs_widget.x, vis->vs_widget.y, vis->vs_widget.width, vis->vs_widget.height, GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data, 76, cmap);
			gdk_gc_set_clip_mask(vis->vs_widget.gc, NULL);
			gdk_bitmap_unref(mask);
		}
		else
		{
			gdk_draw_indexed_image(vis->vs_window, vis->vs_widget.gc, vis->vs_widget.x, vis->vs_widget.y, vis->vs_widget.width, vis->vs_widget.height, GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data, 76, cmap);
		}
	}
	else
	{
		memset(rgb_data, 0, 152 * 32);
		if (!cfg.vis_transparent)
		{
			for (y = 1; y < 16; y += 2)
			{
				ptr = rgb_data + (y * 304);
				for (x = 0; x < 76; x += 2, ptr += 4)
				{
					*ptr = 1;
					*(ptr + 1) = 1;
					*(ptr + 152) = 1;
					*(ptr + 153) = 1;
				}
			}
		}
		if (cfg.vis_type == VIS_ANALYZER)
		{
			for (x = 0; x < 75; x++)
			{
				if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
					h = vis->vs_data[x >> 2];	
				else if (cfg.analyzer_type == ANALYZER_LINES)
					h = vis->vs_data[x];
				if (h && (cfg.analyzer_type == ANALYZER_LINES || (x % 4) != 3))
				{
					ptr = rgb_data + ((16 - h) * 304) + (x << 1);
					switch (cfg.analyzer_mode)
					{
						case ANALYZER_NORMAL:
							for (y = 0; y < h; y++, ptr += 304)
							{
								*ptr = 18 - h + y;
								*(ptr + 1) = 18 - h + y;
								*(ptr + 152) = 18 - h + y;
								*(ptr + 153) = 18 - h + y;
							}
							break;
						case ANALYZER_FIRE:
							for (y = 0; y < h; y++, ptr += 304)
							{
								*ptr = y + 2;
								*(ptr + 1) = y + 2;
								*(ptr + 152) = y + 2;
								*(ptr + 153) = y + 2;
							}
							break;
						case ANALYZER_VLINES:
							for (y = 0; y < h; y++, ptr += 304)
							{
								*ptr = 18 - h;
								*(ptr + 1) = 18 - h;
								*(ptr + 152) = 18 - h;
								*(ptr + 153) = 18 - h;
							}

							break;
					}

				}
			}
			if (cfg.analyzer_peaks)
			{

				for (x = 0; x < 75; x++)
				{
					if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
						h = vis->vs_peak[x >> 2];
					else if (cfg.analyzer_type == ANALYZER_LINES)
						h = vis->vs_peak[x];

					if (h && (cfg.analyzer_type == ANALYZER_LINES || (x % 4) != 3))
					{
						ptr = rgb_data + (16 - h) * 304 + (x << 1);
						*ptr = 23;
						*(ptr + 1) = 23;
						*(ptr + 152) = 23;
						*(ptr + 153) = 23;
					}
				}
			}
		}
		else if (cfg.vis_type == VIS_SCOPE)
		{
			for (x = 0; x < 75; x++)
			{
				switch (cfg.scope_mode)
				{
					case SCOPE_DOT:
						h = vis->vs_data[x];
						ptr = rgb_data + ((15 - h) * 304) + (x << 1);
						*ptr = vis_scope_colors[h];
						*(ptr + 1) = vis_scope_colors[h];
						*(ptr + 152) = vis_scope_colors[h];
						*(ptr + 153) = vis_scope_colors[h];
						break;
					case SCOPE_LINE:
						if (x != 74)
						{
							h = 15 - vis->vs_data[x];
							h2 = 15 - vis->vs_data[x + 1];
							if (h > h2)
							{
								y = h;
								h = h2;
								h2 = y;
							}
							ptr = rgb_data + (h * 304) + (x << 1);
							for (y = h; y <= h2; y++, ptr += 304)
							{
								*ptr = vis_scope_colors[y - 3];
								*(ptr + 1) = vis_scope_colors[y - 3];
								*(ptr + 152) = vis_scope_colors[y - 3];
								*(ptr + 153) = vis_scope_colors[y - 3];
							}
						}
						else
						{
							h = 15 - vis->vs_data[x];
							ptr = rgb_data + (h * 304) + (x << 1);
							*ptr = vis_scope_colors[h];
							*(ptr + 1) = vis_scope_colors[h];
							*(ptr + 152) = vis_scope_colors[h];
							*(ptr + 153) = vis_scope_colors[h];
						}
						break;
					case SCOPE_SOLID:
						h = 15 - vis->vs_data[x];
						h2 = 9;
						c = vis_scope_colors[(gint) vis->vs_data[x]];
						if (h > h2)
						{
							y = h;
							h = h2;
							h2 = y;
						}
						ptr = rgb_data + (h * 304) + (x << 1);
						for (y = h; y <= h2; y++, ptr += 304)
						{
							*ptr = c;
							*(ptr + 1) = c;
							*(ptr + 152) = c;
							*(ptr + 153) = c;
						}
						break;
				}
			}
		}
		if (cfg.vis_transparent)
		{
			GdkBitmap *mask;

			mask = create_vis_clip_mask(vis->vs_window, rgb_data, 0, 1);
			gdk_gc_set_clip_mask(vis->vs_widget.gc, mask);
			gdk_gc_set_clip_origin(vis->vs_widget.gc, vis->vs_widget.x << 1, vis->vs_widget.y << 1);
			vis_clear(vis);
			gdk_draw_indexed_image(vis->vs_window, vis->vs_widget.gc, vis->vs_widget.x << 1, vis->vs_widget.y << 1, vis->vs_widget.width << 1, vis->vs_widget.height << 1, GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data, 152, cmap);
			gdk_gc_set_clip_mask(vis->vs_widget.gc, NULL);
			gdk_bitmap_unref(mask);
		}
		else
		{
			gdk_draw_indexed_image(vis->vs_window, vis->vs_widget.gc, vis->vs_widget.x << 1, vis->vs_widget.y << 1, vis->vs_widget.width << 1, vis->vs_widget.height << 1, GDK_RGB_DITHER_NONE, (guchar *) rgb_data, 152, cmap);
		}
	}
	gdk_rgb_cmap_free(cmap);
	GDK_THREADS_LEAVE();

}

void vis_clear_data(Vis * vis)
{
	gint i;

	for (i = 0; i < 75; i++)
	{
		vis->vs_data[i] = (cfg.vis_type == VIS_SCOPE) ? 6 : 0;
		vis->vs_peak[i] = 0;
	}
}

void vis_set_doublesize(Vis * vis, gboolean doublesize)
{
	vis->vs_doublesize = doublesize;
}

void vis_clear(Vis * vis)
{
	if (!vis->vs_doublesize)
		gdk_window_clear_area(vis->vs_window, vis->vs_widget.x, vis->vs_widget.y, vis->vs_widget.width, vis->vs_widget.height);
	else
		gdk_window_clear_area(vis->vs_window, vis->vs_widget.x << 1, vis->vs_widget.y << 1, vis->vs_widget.width << 1, vis->vs_widget.height << 1);
}

void vis_set_window(Vis * vis, GdkWindow * window)
{
	vis->vs_window = window;
}

Vis *create_vis(GList ** wlist, GdkPixmap * parent, GdkWindow * window, GdkGC * gc, gint x, gint y, gint width, gboolean doublesize)
{
	Vis *vis;

	vis = (Vis *) g_malloc0(sizeof (Vis));
	vis->vs_widget.parent = parent;
	vis->vs_window = window;
	vis->vs_widget.gc = gc;
	vis->vs_widget.x = x;
	vis->vs_widget.y = y;
	vis->vs_widget.width = width;
	vis->vs_widget.height = 16;
	vis->vs_widget.visible = 1;
	vis->vs_doublesize = doublesize;
	add_widget(wlist, vis);
	return vis;
}
