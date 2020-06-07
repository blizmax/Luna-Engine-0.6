// Copyright 2018-2020 JXMaster. All rights reserved.
/*
* @file TextureEditor.hpp
* @author JXMaster
* @date 2020/5/9
*/
#pragma once
#include "../IAssetEditor.hpp"

namespace luna
{
	namespace editor
	{
		class TextureEditor : public IAssetEditor
		{
		public:
			lucid("{5ef18c5d-5729-4f92-b1b6-2239a5a9aa8d}");
			luiimpl(TextureEditor, IAssetEditor, IObject);

			WP<texture::ITexture> m_tex;

			bool m_open;

			TextureEditor(IAllocator* alloc) :
				luibind(alloc) {}

			virtual void on_render(imgui::IContext* ctx) override;
			virtual bool closed() override
			{
				return !m_open;
			}
		};
	}
}