// Copyright 2018-2020 JXMaster. All rights reserved.
/*
* @file ImGuiInstance.hpp
* @author JXMaster
* @date 2020/4/5
*/
#pragma once
#include "ImGuiHeader.hpp"
#include <ImGuiLib/imgui.h>
#include <ImGuizmo/ImGuizmo.h>
#include <Input/Input.hpp>

namespace luna
{
	namespace imgui
	{
		extern P<IMutex> m_mtx;
		extern P<IMutex> m_gizmo_mtx;
		extern P<IBlob> m_vs_blob;
		extern P<IBlob> m_ps_blob;
	}
}