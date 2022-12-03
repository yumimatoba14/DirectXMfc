#pragma once

#include "D3D11Graphics.h"

namespace D3D11Graphics {
namespace D3DModelColorUtil {
	inline UINT FloatToUInt(float f) { return (UINT)(f * 255 + 0.5); }

	inline UINT Rgba(UINT r, UINT g, UINT b, UINT a)
	{
		return (0xFF & r) | ((0xFF & g) << 8) | ((0xFF & b) << 16) | ((0xFF & a) << 24);
	}

	inline UINT RgbaF(float r, float g, float b, float a)
	{
		return Rgba(FloatToUInt(r), FloatToUInt(g), FloatToUInt(b), FloatToUInt(a));
	}

	inline UINT Rgb(UINT r, UINT g, UINT b) { return Rgba(r, g, b, 255); }
}
}
