#pragma once
#include <cstdint>

template <typename T>
struct ResourceHandle
{
	uint32_t id = 0;
	bool IsValid() const { return id != 0; }
};