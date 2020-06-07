// Copyright 2018-2020 JXMaster. All rights reserved.
/*
* @file StudioHeader.hpp
* @author JXMaster
* @date 2020/4/20
*/
#pragma once
#include <Input/Input.hpp>
#include <Gfx/Gfx.hpp>
#include <Renderer/Renderer.hpp>
#include <ImGui/ImGui.hpp>
#include <RectPack/RectPack.hpp>
#include <Image/Image.hpp>
#include <Font/Font.hpp>
#include <Asset/Asset.hpp>
#include <Texture/Texture.hpp>
#include <ObjLoader/ObjLoader.hpp>
#include <3DEngine/3DEngine.hpp>
#include <Scene/Scene.hpp>

namespace luna
{
	inline IAllocator* get_module_allocator()
	{
		return get_global_heap();
	}
	namespace editor
	{
		extern ILogger* g_logger;
	}
	inline ILogger* get_logger()
	{
		return editor::g_logger;
	}

	inline void input_asset_entry(imgui::IContext* ctx, const char* label, IStringBuffer* asset_name, Guid& asset_guid, IName* required_type)
	{
		ctx->input_text(label, asset_name);
		bool editing = ctx->is_item_active();
		if (!editing)
		{
			// Check if the asset path is valid.
			auto ass = asset::fetch_asset(new_path(asset_name->c_str()));
			if (succeeded(ass) && (ass.get()->meta()->type() == required_type))
			{
				// If valid, change the GUID to a new one.
				asset_guid = ass.get()->meta()->guid();
			}
			else
			{
				// The reset behavior.
				if (asset_guid == Guid(0, 0))
				{
					asset_name->clear();
				}
				else
				{
					// Use the valid asset to change the string.
					auto ass2 = asset::fetch_asset(asset_guid);
					if (succeeded(ass2))
					{
						asset_name->assign(ass2.get()->meta()->meta_path()->encode()->c_str());
					}
					else
					{
						asset_name->clear();
					}
				}
			}
		}
		ctx->same_line();
		ctx->push_id(label);
		if (ctx->button("Clear"))
		{
			asset_guid = Guid(0, 0);
			asset_name->clear();
		}
		ctx->pop_id();
	}
}