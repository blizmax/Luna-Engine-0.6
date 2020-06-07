// Copyright 2018-2020 JXMaster. All rights reserved.
/*
* @file IViewSet.hpp
* @author JXMaster
* @date 2019/7/17
*/
#pragma once
#include "GraphicDefines.hpp"
#include "IGraphicDeviceChild.hpp"
#include "Views.hpp"

namespace luna
{
	namespace gfx
	{
		struct ViewSetDesc
		{
			uint32 num_cbvs;
			uint32 num_srvs;
			uint32 num_uavs;
			uint32 num_samplers;

			ViewSetDesc() = default;
			ViewSetDesc(uint32 _num_cbvs,
				uint32 _num_srvs,
				uint32 _num_uavs,
				uint32 _num_samplers) :
				num_cbvs(_num_cbvs),
				num_srvs(_num_srvs),
				num_uavs(_num_uavs),
				num_samplers(_num_samplers) {}
		};

		//! @interface IViewSet
		//! Describes which views and samples are bound to the pipeline.
		//! This can be set at any time before the draw call or dispatch call
		//! is submitted.
		struct IViewSet : public IGraphicDeviceChild
		{
			luiid("{f12bc4b0-2aad-42bb-8b8c-237ed0593aa3}");

			virtual ViewSetDesc desc() = 0;
			virtual void set_cbv(uint32 index, IResource* res, const ConstantBufferViewDesc& cbv) = 0;
			virtual void set_srv(uint32 index, IResource* res, const ShaderResourceViewDesc* srv = nullptr) = 0;
			virtual void set_uav(uint32 index, IResource* res, IResource* counter_resource = nullptr, const UnorderedAccessViewDesc* uav = nullptr) = 0;
			virtual void set_sampler(uint32 index, const SamplerDesc& sampler) = 0;
			virtual void reset_cbv(uint32 index) = 0;
			virtual void reset_srv(uint32 index) = 0;
			virtual void reset_uav(uint32 index) = 0;
			virtual void reset_all() = 0;
		};
	}
}