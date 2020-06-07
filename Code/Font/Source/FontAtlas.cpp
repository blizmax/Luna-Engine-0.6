// Copyright 2018-2020 JXMaster. All rights reserved.
/*
* @file FontAtlas.cpp
* @author JXMaster
* @date 2019/10/6
*/
#include "FontAtlas.hpp"
#include "RectPack/IRectPackContext.hpp"
#include "../IFontFile.hpp"
#include <Base/Unicode.hpp>

namespace luna
{
	namespace font
	{
		bool FontAtlas::init(IFontFile* fontfile, uint32 font_index, float32 char_height)
		{
			m_font_file = fontfile;
			m_font_index = font_index;
			m_font_height = char_height;
			if (m_font_index >= m_font_file->count_fonts())
			{
				return false;
			}
			return true;
		}

		void FontAtlas::set_font(IFontFile* fontfile, uint32 font_index)
		{
			if (fontfile != m_font_file || font_index != m_font_index)
			{
				m_is_dirty = true;
			}
			m_font_file = fontfile;
			m_font_index = font_index;
		}

		void FontAtlas::set_font_height(float32 font_height)
		{
			if (font_height != m_font_height)
			{
				m_is_dirty = true;
			}
			m_font_height = font_height;
		}

		void FontAtlas::set_scale_factor(float32 scale_factor)
		{
			if (scale_factor != m_pixel_scale)
			{
				m_is_dirty = true;
			}
			m_pixel_scale = scale_factor;
		}

		void FontAtlas::add_glyph_ranges(uint32 num_ranges, const FontAtlasRange* ranges)
		{
			for (uint32 i = 0; i < num_ranges; ++i)
			{
				for (uint32 j = 0; j < ranges[i].num_chars; ++j)
				{
					uint32 ch = ranges[i].start_codepoint + j;
					auto p = m_glyphs.insert(make_pair(ch, FontChar()));
					if (p.second == true)
					{
						m_is_dirty = true;
					}
				}
			}
		}

		void FontAtlas::add_glyphs_from_string(const char* str)
		{
			luassert_usr(str);
			while (*str)
			{
				char32_t ch = utf8_decode_char(str);
				str += utf8_charlen(str);
				auto p = m_glyphs.insert(make_pair(ch, FontChar()));
				if (p.second == true)
				{
					m_is_dirty = true;
				}
			}
		}

		void FontAtlas::add_glyphs_from_lstring(const char* str, size_t len)
		{
			size_t i = 0;
			while (i < len)
			{
				char32_t ch = utf8_decode_char(str);
				i += utf8_charlen(str);
				str += utf8_charlen(str);
				auto p = m_glyphs.insert(make_pair(ch, FontChar()));
				if (p.second == true)
				{
					m_is_dirty = true;
				}
			}
		}

		void FontAtlas::remove_glyph_ranges(uint32 num_ranges, const FontAtlasRange* ranges)
		{
			for (uint32 i = 0; i < num_ranges; ++i)
			{
				for (uint32 j = 0; j < ranges[i].num_chars; ++j)
				{
					uint32 ch = ranges[i].start_codepoint + j;
					size_t erased = m_glyphs.erase(ch);
					if (erased)
					{
						m_is_dirty = true;
					}
				}
			}
		}

		void FontAtlas::remove_glyphs_from_string(const char* str)
		{
			while (*str)
			{
				char32_t ch = utf8_decode_char(str);
				str += utf8_charlen(str);
				size_t erased = m_glyphs.erase(ch);
				if (erased)
				{
					m_is_dirty = true;
				}
			}
		}

		void FontAtlas::remove_glyphs_from_lstring(const char* str, size_t len)
		{
			size_t i = 0;
			while (i < len)
			{
				char32_t ch = utf8_decode_char(str);
				i += utf8_charlen(str);
				str += utf8_charlen(str);
				size_t erased = m_glyphs.erase(ch);
				if (erased)
				{
					m_is_dirty = true;
				}
			}
		}

		void FontAtlas::render()
		{
			constexpr uint32 char_gap = 2;	// Preserve some space between chars to prevent bilinear sample incorrect.
			constexpr uint32 half_gap = char_gap / 2;
			if (m_image && !m_is_dirty)
			{
				return;
			}
			// Packing.
			Vector<rpack::PackRect> packs(get_module_allocator());
			Vector<glyph_t> glyphs(get_module_allocator());
			packs.reserve(m_glyphs.size());
			glyphs.reserve(m_glyphs.size());
			float32 pixel_scale = m_font_file->scale_for_pixel_height(m_font_index, m_font_height * m_pixel_scale);
			float32 origin_scale = m_font_file->scale_for_pixel_height(m_font_index, m_font_height);
			int32 ascent, descent, line_gap;
			m_font_file->get_vmetrics(m_font_index, &ascent, &descent, &line_gap);
			m_ascent = ascent * origin_scale;
			m_descent = descent * origin_scale;
			m_line_gap = line_gap * origin_scale;
			for (auto& i : m_glyphs)
			{
				glyph_t glyph = m_font_file->find_glyph(m_font_index, i.first);
				int32 x0, y0, x1, y1;
				m_font_file->get_glyph_box(m_font_index, glyph, &x0, &y0, &x1, &y1);
				FontChar c;
				int32 advance_width, left_side_bearing;
				m_font_file->get_glyph_hmetrics(m_font_index, glyph, &advance_width, &left_side_bearing);
				c.m_advance_width = advance_width * origin_scale;
				c.m_left_side_bearing = left_side_bearing * origin_scale;
				c.m_sf.x = (float32)(x1 - x0) * origin_scale;
				c.m_sf.y = (float32)(y1 - y0) * origin_scale;
				c.m_sf.z = c.m_left_side_bearing;
				c.m_sf.w = -(float32)y1 * origin_scale;

				i.second = c;
				rpack::PackRect r;
				m_font_file->get_glyph_bitmap_box(m_font_index, glyph, pixel_scale, pixel_scale, 0, 0, &x0, &y0, &x1, &y1);
				r.width = (uint32)(x1 - x0) + char_gap;	// 2 for margin between chars.
				r.height = (uint32)(y1 - y0) + char_gap;
				packs.push_back(r);
				glyphs.push_back(glyph);
			}
			// Pack texture size starts from 128x128.
			uint32 tex_size = 1 << 7;
			while (true)
			{
				auto pack_ctx = rpack::new_context(tex_size, tex_size);
				result_t r = pack_ctx->pack_rects(packs.data(), (uint32)packs.size());
				if (succeeded(r))
				{
					break;
				}
				tex_size <<= 1;
			}
			// Grab the image and render the atlas to the image.
			image::ImageDesc img_desc;
			img_desc.format = image::EImagePixelFormat::r8_unorm;
			img_desc.height = tex_size;
			img_desc.width = tex_size;
			auto img = image::new_image(img_desc);

			memset(img->buffer(), 0, img->size());

			for (uint32 i = 0; i < (uint32)m_glyphs.size(); ++i)
			{
				void* dest = pixel_offset(img->buffer(), packs[i].x + half_gap, packs[i].y + half_gap, 0,
					1, img_desc.width * image::size_per_pixel(img_desc.format), img->size());
				m_font_file->render_glyph_bitmap(m_font_index, glyphs[i], dest,
					packs[i].width - char_gap, packs[i].height - char_gap, img_desc.width * image::size_per_pixel(img_desc.format), pixel_scale, pixel_scale, 0.0f, 0.0f);
			}
			glyphs.clear();
			glyphs.shrink_to_fit();
			// Copy the bitmap to RGBA format.
			img_desc.format = image::EImagePixelFormat::rgba8_unorm;
			auto img_rgba = image::new_image(img_desc);
			for (uint32 i = 0; i < tex_size; ++i)
			{
				for (uint32 j = 0; j < tex_size; ++j)
				{
					uint8* src = (uint8*)pixel_offset(img->buffer(), i, j, 0, 1, img_desc.width * image::size_per_pixel(image::EImagePixelFormat::r8_unorm), img->size());
					uint8* dest = (uint8*)pixel_offset(img_rgba->buffer(), i, j, 0, 4, img_desc.width * image::size_per_pixel(img_desc.format), img_rgba->size());
					dest[0] = 255;
					dest[1] = 255;
					dest[2] = 255;
					dest[3] = src[0];
				}
			}
			// free origin image early.
			img = nullptr;

			m_image = img_rgba;
			m_size = tex_size;
			// Copy rect info.
			uint32 i = 0;
			for (auto& ch : m_glyphs)
			{
				ch.second.m_rect.x = (float32)(packs[i].x + half_gap) / tex_size;
				ch.second.m_rect.y = (float32)(packs[i].y + half_gap) / tex_size;
				ch.second.m_rect.z = (float32)(packs[i].x + packs[i].width - half_gap) / tex_size;
				ch.second.m_rect.w = (float32)(packs[i].y + packs[i].height - half_gap) / tex_size;
				++i;
			}
			m_scale = origin_scale;
			m_is_dirty = false;
		}

		bool FontAtlas::contain_char(codepoint_t char_codepoint)
		{
			auto iter = m_glyphs.find(char_codepoint);
			return iter == m_glyphs.end() ? false : true;
		}

		float32 FontAtlas::get_kern_advance(codepoint_t ch1, codepoint_t ch2)
		{
			return m_scale * m_font_file->get_kern_advance(m_font_index, m_font_file->find_glyph(m_font_index, ch1), m_font_file->find_glyph(m_font_index, ch2));
		}

		RV FontAtlas::get_char(codepoint_t char_codepoint, FontAtlasChar& ch)
		{
			// find codepoint.
			auto iter = m_glyphs.find(char_codepoint);
			if (iter == m_glyphs.end())
			{
				return e_item_not_exist;
			}

			FontChar& r = iter->second;
			ch.uv_top_left.x = r.m_rect.x;
			ch.uv_top_left.y = r.m_rect.y;
			ch.uv_bottom_right.x = r.m_rect.z;
			ch.uv_bottom_right.y = r.m_rect.w;
			ch.size.x = r.m_sf.x;
			ch.size.y = r.m_sf.y;
			ch.pos_top_left.x = r.m_sf.z;
			ch.pos_top_left.y = r.m_sf.w;
			ch.advance_width = r.m_advance_width;
			ch.left_side_bearing = r.m_left_side_bearing;
			return s_ok;
		}
		float32 FontAtlas::get_line_width(const char* text, uint32 num_chars, float32 spacing)
		{
			float32 char_adv = 0.0f;
			const char* cur_char = text;
			uint32 char_index = 0;
			while (*cur_char)
			{
				if (num_chars && char_index >= num_chars)
				{
					break;
				}
				char32_t c = utf8_decode_char(cur_char);
				// Advance string.
				cur_char += utf8_charspan(c);
				++char_index;
				// Get char info.
				FontAtlasChar ch_info;
				if (failed(get_char(c, ch_info)))
				{
					c = '?';
					if (failed(get_char(c, ch_info)))
					{
						continue;
					}
				}
				char32_t nc = utf8_decode_char(cur_char);
				if (nc && (!num_chars || (char_index < num_chars)))
				{
					float32 kern = get_kern_advance(c, nc);
					char_adv += ch_info.advance_width + spacing + kern;
				}
				else
				{
					char_adv += ch_info.advance_width;
				}
			}
			return char_adv;
		}
		void FontAtlas::get_draw_region(float32 region_width, const char* text, float32* width, float32* height,uint32 num_chars, float32 spacing_x, float32 spacing_y)
		{
			float32 ascent, descent, line_gap;
			get_vmetrics(&ascent, &descent, &line_gap);
			float32 row_gap = ascent - descent + line_gap;
			float32 row_adv = 0.0f;
			float32 char_adv = 0.0f;
			*width = 0.0f;
			const char* cur_char = text;
			uint32 char_index = 0;
			// for each line.
			while (*cur_char)
			{
				row_adv += row_gap;
				while (*cur_char && char_adv < region_width)
				{
					if (num_chars && char_index >= num_chars)
					{
						// breaks directly.
						goto end;
					}
					char32_t c = utf8_decode_char(cur_char);
					// advance char string.
					cur_char += utf8_charspan(c);
					++char_index;
					// Get char info.
					FontAtlasChar ch_info;
					if (failed(get_char(c, ch_info)))
					{
						c = '?';
						if (failed(get_char(c, ch_info)))
						{
							// Skip this.
							continue;
						}
					}
					if (ch_info.size.x + ch_info.left_side_bearing + char_adv > region_width)
					{
						// Jump to next line.
						break;
					}
					char32_t nc = utf8_decode_char(cur_char);
					if (nc && (!num_chars || (char_index < num_chars)))
					{
						float32 kern;
						kern = get_kern_advance(c, nc);
						char_adv += ch_info.advance_width + spacing_x + kern;
					}
					else
					{
						char_adv += ch_info.advance_width;
					}
				}
				// Jump to next line.
				row_adv += spacing_y;
				*width = max(*width, char_adv);
				char_adv = 0.0f;
			}
		end:
			*height = row_adv;
		}
	}
}