#pragma once

#include <vulkan/vulkan.hpp>

/// Gets a function pointer for an instance function (useful for extensions)
template<typename T>
T getProcAddress(const vk::Instance& instance, const char* name)
{
	auto func = instance.getProcAddr(name);
	if (!func)
		throw std::runtime_error("Can't get proc address");
	return (T)func;
}