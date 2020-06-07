// Copyright 2018-2020 JXMaster. All rights reserved.
/*
* @file AssetBrowser.cpp
* @author JXMaster
* @date 2020/4/29
*/
#include "AssetBrowser.hpp"
#include "MainEditor.hpp"

namespace luna
{
	namespace editor
	{
		struct AssetThumbnail
		{
			P<IName> m_filename;	// Without extension.
			bool m_is_dir;
		};

		R<Vector<AssetThumbnail>> get_assets_in_folder(IPath* folder_path)
		{
			Vector<AssetThumbnail> assets(get_module_allocator());
			lutry
			{
				lulet(iter, open_dir(folder_path));
				while (iter->valid())
				{
					if ((iter->attribute() & EFileAttributeFlag::directory) != EFileAttributeFlag::none)
					{
						// This is a directory.
						if (!strcmp(iter->filename(), ".") || !strcmp(iter->filename(), ".."))
						{
							iter->move_next();
							continue;
						}
						AssetThumbnail t;
						t.m_filename = intern_name(iter->filename());
						t.m_is_dir = true;
						assets.push_back(t);
					}
					else
					{
						// Ends with ".meta.la" or ".meta.lb"
						const char* name = iter->filename();
						size_t name_len = strlen(name);
						if (name_len > 8)
						{
							if ((!strcmp(name + name_len - 8, ".meta.la")) || (!strcmp(name + name_len - 8, ".meta.lb")))
							{
								AssetThumbnail t;
								t.m_filename = intern_name(name, name_len - 8);
								t.m_is_dir = false;
								assets.push_back(t);
							}
						}
					}
					iter->move_next();
				}
			}
			lucatchret;
			return assets;
		}

		void AssetBrowser::change_path(IPath* path)
		{
			if (m_current_location_in_histroy_path != m_histroy_paths.size() - 1)
			{
				// Clear forwards.
				m_histroy_paths.resize(m_current_location_in_histroy_path + 1);
			}
			m_path->assign(path);
			m_histroy_paths.push_back(m_path->clone());
			++m_current_location_in_histroy_path;
		}

		void AssetBrowser::render()
		{
			auto gui = m_editor->m_ctx;

			char title[64];
			sprintf_s(title, "Asset Browser");

			gui->set_next_window_size(Float2(1000.0f, 500.0f), imgui::ECondition::first_use_ever);
			gui->begin(title, nullptr, imgui::EWindowFlag::no_collapse | imgui::EWindowFlag::menu_bar);

			if (gui->begin_menu_bar())
			{
				if (gui->begin_menu("New"))
				{
					for (auto& i : m_editor->m_importer_types)
					{
						auto name = i.first;
						if (gui->menu_item(name->c_str()))
						{
							auto editor = i.second->new_importer(m_path);
							m_editor->m_editors.push_back(editor);
						}
					}
					gui->end_menu();
				}
				gui->end_menu_bar();
			}

			navbar();

			tile_context();

			gui->end();
		}
		void AssetBrowser::navbar()
		{
			auto gui = m_editor->m_ctx;
			// Draw back/forward/pop arrow.
			bool back_disabled = (m_current_location_in_histroy_path == 0);
			if (back_disabled)
			{
				gui->push_style_var(imgui::EStyle::alpha, gui->get_style()->Alpha * 0.5f);
			}
			if (gui->arrow_button("back", imgui::EDirection::left) && !back_disabled)
			{
				--m_current_location_in_histroy_path;
				m_path->assign(m_histroy_paths[m_current_location_in_histroy_path]);
			}
			if (back_disabled)
			{
				gui->pop_style_var();
			}
			gui->same_line();
			bool forward_disabled = (m_current_location_in_histroy_path == m_histroy_paths.size() - 1);
			if (forward_disabled)
			{
				gui->push_style_var(imgui::EStyle::alpha, gui->get_style()->Alpha * 0.5f);
			}
			if (gui->arrow_button("forward", imgui::EDirection::right) && !forward_disabled)
			{
				++m_current_location_in_histroy_path;
				m_path->assign(m_histroy_paths[m_current_location_in_histroy_path]);
			}
			if (forward_disabled)
			{
				gui->pop_style_var();
			}
			gui->same_line();
			bool pop_disabled = (m_path->count_nodes() == 0);
			if (pop_disabled)
			{
				gui->push_style_var(imgui::EStyle::alpha, gui->get_style()->Alpha * 0.5f);
			}
			if (gui->arrow_button("pop", imgui::EDirection::up) && !pop_disabled)
			{
				auto path = new_path();
				path->assign(m_path);
				path->pop_back();
				change_path(path);
			}
			if (pop_disabled)
			{
				gui->pop_style_var();
			}
			gui->same_line();
			// Draw path.
			{
				auto pos = gui->get_cursor_screen_pos();
				auto frame_padding = gui->get_style()->FramePadding;

				auto region_min = pos;
				auto region_max = pos + frame_padding * 2 + Float2(gui->get_content_region_avail().x - gui->get_style()->WindowPadding.x,
					gui->get_text_line_height());
				if (!m_is_text_editing)
				{
					auto dl = gui->get_window_draw_list();
					imgui::dl_add_rect_filled(dl, region_min, region_max, 0xFF202020);

					imgui::dl_add_rect(dl, region_min, region_max,
						color_to_abgr8(gui->get_style()->Colors[(uint32)imgui::EColor::border]));

					bool btn_clicked = false;
					if ((m_path->flags() & EPathFlag::absolute) != EPathFlag::none)
					{
						gui->text("/");
						if (m_path->count_nodes())
						{
							gui->same_line();
						}
					}
					auto mouse_pos = imgui::io_get_mouse_pos(gui->get_io());
					P<IMutPath> changed_path;
					for (uint32 i = 0; i < m_path->count_nodes(); ++i)
					{
						auto node = m_path->node(i);
						gui->push_id(i);
						if (gui->button(node->c_str()) && i != (m_path->count_nodes() - 1))
						{
							changed_path = new_path();
							changed_path->assign(m_path);
							for (uint32 j = i; j < m_path->count_nodes() - 1; ++j)
							{
								changed_path->pop_back();
							}
						}
						auto btn_min = gui->get_item_rect_min();
						auto btn_max = gui->get_item_rect_max();
						if (in_rect(mouse_pos, btn_min, btn_max) && gui->is_mouse_clicked(input::EMouseKey::lb))
						{
							btn_clicked = true;
						}
						gui->pop_id();
						gui->same_line();
						gui->text("/");
						if ((m_path->count_nodes() > 1) && (i != m_path->count_nodes() - 1))
						{
							gui->same_line();
						}
					}
					if (changed_path)
					{
						change_path(changed_path);
					}

					
					if (in_rect(mouse_pos, region_min, region_max) && gui->is_mouse_clicked(input::EMouseKey::lb) && !btn_clicked)
					{
						// Switch to text mode.
						m_is_text_editing = true;
						m_path_edit_text = m_path->encode(EPathSeparator::slash, true);
					}
				}
				else
				{
					gui->set_next_item_width(region_max.x - region_min.x);
					gui->input_text("##PathTextEditing", m_path_edit_text);
					auto mouse_pos = imgui::io_get_mouse_pos(gui->get_io());
					if (!in_rect(mouse_pos, region_min, region_max) && gui->is_mouse_clicked(input::EMouseKey::lb))
					{
						// Switch to normal mode.
						m_is_text_editing = false;
						auto new_p = new_path();
						new_p->parse(m_path_edit_text->c_str());
						if (succeeded(dir_exists(new_p)))
						{
							m_path->assign(new_p);
						}
					}
				}
			}
			
		}

		void AssetBrowser::tile_context()
		{
			auto gui = m_editor->m_ctx;
			// Draw content.
			gui->push_style_var(imgui::EStyle::child_rounding, 5.0f);
			gui->begin_child("ctx", Float2(0.0f, 0.0f), true, imgui::EWindowFlag::no_move);
			auto assets = get_assets_in_folder(m_path);
			Float2 mouse_pos = imgui::io_get_mouse_pos(gui->get_io());
			if (succeeded(assets))
			{
				if (assets.get().empty())
				{
					auto region = gui->get_content_region_avail();
					auto region_center = region / 2;
					const char* text = "Empty Directory";
					auto text_size = gui->calc_text_size(text);
					gui->set_cursor_pos(region_center - Float2(text_size.x / 2, text_size.y / 2));
					gui->text(text);
				}
				else
				{
					// Draw asset tiles.

					size_t num_assets = assets.get().size();

					constexpr uint32 padding = 5;

					uint32 tile_width = (uint32)(m_tile_size + padding * 2);
					uint32 tile_height = (uint32)(m_tile_size + padding * 2 + gui->get_text_line_height());
					auto window_pos = gui->get_window_pos();

					float32 woff = 0;
					float32 hoff = 0;
					auto origin_pos = gui->get_cursor_screen_pos();

					for (size_t i = 0; i < num_assets; ++i)
					{
						auto dl = gui->get_window_draw_list();

						auto tile_min = gui->get_cursor_screen_pos() + padding;
						auto tile_max = tile_min + Float2((float32)tile_width, (float32)tile_height);

						auto siter = m_selections.find(assets.get()[i].m_filename);
						if (siter != m_selections.end())
						{
							// Draw selection background.
							imgui::dl_add_rect_filled(dl, tile_min - padding, tile_max - padding, color_to_abgr8(gui->get_style()->Colors[(uint32)imgui::EColor::button]));
						}

						if (assets.get()[i].m_is_dir)
						{
							auto folder_icon_begin_pos = gui->get_cursor_screen_pos() + Float2(padding, padding);
							// Draw an folder icon.
							Float2 shape1[4] = {
								{41.1f, 21.0f},
								{85.3f, 21.0f},
								{91.5f, 33.5f},
								{35.5f, 33.5f},
							};
							Float2 shape2[4] = {
								{9.9f, 36.3f},
								{91.5f, 36.3f},
								{80.5f, 90.4f},
								{19.6f, 90.4f}
							};
							imgui::dl_path_clear(dl);
							imgui::dl_path_line_to(dl, shape1[0] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_line_to(dl, shape1[1] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_line_to(dl, shape1[2] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_line_to(dl, shape1[3] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_fill_convex(dl, 0xFFCCCCCC);
							imgui::dl_path_clear(dl);
							imgui::dl_path_line_to(dl, shape2[0] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_line_to(dl, shape2[1] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_line_to(dl, shape2[2] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_line_to(dl, shape2[3] * m_tile_size / 100.0f + folder_icon_begin_pos);
							imgui::dl_path_fill_convex(dl, 0xFFCCCCCC);

							if (mouse_pos.x > tile_min.x && mouse_pos.y > tile_min.y && mouse_pos.x < tile_max.x && mouse_pos.y < tile_max.y)
							{
								if (gui->is_mouse_double_clicked(input::EMouseKey::lb))
								{
									// Change path.
									auto path = new_path();
									path->assign(m_path);
									path->push_back(assets.get()[i].m_filename);
									change_path(path);
								}
							}
						}
						else
						{
							auto meta_path = new_path();
							meta_path->assign(m_path);
							meta_path->push_back(assets.get()[i].m_filename);
							meta_path->set_flags(meta_path->flags() & ~EPathFlag::diretory);
							auto ass = asset::fetch_asset(meta_path);
							if (succeeded(ass))
							{
								// Editor logic.
								auto iter = m_editor->m_editor_types.find(ass.get()->meta()->type());
								
								if (iter != m_editor->m_editor_types.end())
								{
									iter->second->on_draw_tile(gui, ass.get(), RectF(tile_min.x - window_pos.x, tile_min.y - window_pos.y, tile_min.x + m_tile_size - window_pos.x, tile_min.y + m_tile_size - window_pos.y));

									if (mouse_pos.x > tile_min.x && mouse_pos.y > tile_min.y && mouse_pos.x < tile_max.x && mouse_pos.y < tile_max.y)
									{
										if (gui->is_mouse_double_clicked(input::EMouseKey::lb))
										{
											// Open Editor.
											auto edit = iter->second->new_editor(ass.get());
											m_editor->m_editors.push_back(edit);
										}
									}
								}
								else
								{
									auto text_sz = gui->calc_text_size(ass.get()->meta()->type()->c_str());
									RectF draw_rect = RectF(tile_min.x - window_pos.x, tile_min.y - window_pos.y, tile_min.x + m_tile_size - window_pos.x, tile_min.y + m_tile_size - window_pos.y);
									Float2 center = Float2((draw_rect.right + draw_rect.left) / 2.0f, (draw_rect.top + draw_rect.bottom) / 2.0f);
									gui->set_cursor_pos(center - text_sz / 2.0f);
									gui->text(ass.get()->meta()->type()->c_str());
								}

								// Load the data if not loaded.
								if (ass.get()->meta()->state() == asset::EAssetState::unloaded)
								{
									if ((ass.get()->meta()->flags() & asset::EAssetFlag::loading_error) != asset::EAssetFlag::none)
									{
										// Output error.
										log_error(ass.get()->meta()->error_object()->err_msg()->c_str());
									}
									ass.get()->meta()->load();
								}

								// Draw status circle.
								if (ass.get()->meta()->state() == asset::EAssetState::loaded)
								{
									// Draw green circle.
									imgui::dl_add_circle_filled(dl, tile_min + Float2(m_tile_size, m_tile_size) - 5.0f, 10.0f, color_to_abgr8(color::green));
								}
								else
								{
									// Draw yellow circle.
									imgui::dl_add_circle_filled(dl, tile_min + Float2(m_tile_size, m_tile_size) - 5.0f, 10.0f, color_to_abgr8(color::yellow));
								}
							}
							else
							{
								auto text_sz = gui->calc_text_size("Unknown");
								Float2 center = tile_min / 2.0f + (tile_min + Float2(m_tile_size, m_tile_size) / 2.0f);
								gui->set_cursor_pos(center - text_sz / 2.0f);
								gui->text("Unknown");
								imgui::dl_add_circle_filled(dl, tile_min + Float2(m_tile_size, m_tile_size) - 5.0f, 10.0f, color_to_abgr8(color::red));
							}
						}

						// Draw asset name.
						gui->set_cursor_pos(Float2(tile_min.x - window_pos.x, tile_min.y - window_pos.y + m_tile_size));
						gui->text(assets.get()[i].m_filename->c_str());

						// Check if the asset is clicked / double clicked.
						
						if (mouse_pos.x > tile_min.x && mouse_pos.y > tile_min.y && mouse_pos.x < tile_max.x && mouse_pos.y < tile_max.y)
						{
							if (gui->is_mouse_clicked(input::EMouseKey::lb))
							{
								m_selections.clear();
								m_selections.insert(assets.get()[i].m_filename);
							}
						}


						// Update woff and hoff.
						woff += tile_width;
						if (woff + tile_width > gui->get_window_width())
						{
							woff = 0;
							hoff += tile_height;
						}

						// Set cursor pos for next tile.
						gui->set_cursor_screen_pos(origin_pos + Float2(woff, hoff));
					}
				}
			}
			else
			{
				auto region = gui->get_content_region_avail();
				auto region_center = region / 2;
				const char* text_fail = "Failed to display assets in this directory.";
				const char* text_reason = explain(assets.result());
				auto text_fail_size = gui->calc_text_size(text_fail);
				auto text_reason_size = gui->calc_text_size(text_reason);
				gui->set_cursor_pos(region_center - Float2(text_fail_size.x / 2, text_fail_size.y));
				gui->text(text_fail);
				gui->set_cursor_pos(region_center - Float2(text_reason_size.x / 2, 0.0f));
				gui->text(text_reason);
			}

			auto tile_ctx_min = gui->get_window_pos();
			auto tile_ctx_max = gui->get_window_pos() + gui->get_window_size();

			if (gui->is_mouse_clicked(input::EMouseKey::lb) && in_rect(mouse_pos, tile_ctx_min, tile_ctx_max))
			{
				m_ms_mouse_begin_pos = mouse_pos;
				m_ms_is_dragging = true;
			}

			if (m_ms_is_dragging)
			{
				// Draw drag rect.
				auto dl = gui->get_window_draw_list();
				auto rect_min = min(mouse_pos, m_ms_mouse_begin_pos);
				auto rect_max = max(mouse_pos, m_ms_mouse_begin_pos);

				auto color = gui->get_style()->Colors[(uint32)imgui::EColor::button];
				color.a *= 0.5f;
				imgui::dl_add_rect_filled(dl, rect_min, rect_max, color_to_abgr8(color));

				if (gui->is_mouse_released(input::EMouseKey::lb))
				{
					m_ms_is_dragging = false;
				}
			}

			gui->end_child();
			gui->pop_style_var();
		}
	}
}