#pragma once

#include "D3DGraphics.h"

class D3DViewOp;

namespace D3D11Graphics {

class D3DModelTriangleList;

/// <summary>
/// D3DGraphics class which implements specific utility functions for 3D viewing.
/// </summary>
class D3DGraphics3D
{
public:
	typedef DirectX::XMFLOAT4X4 XMFLOAT4X4;

	struct ShaderParam {
		XMFLOAT4X4 viewMatrix;
		XMFLOAT4X4 projectionMatrix;
	};

public:
	D3DGraphics3D();
	D3DGraphics3D(const D3DGraphics&) = delete;

	void Initialize(HWND hWnd);
	bool IsInitialized() const { return m_graphics.HasDevice(); }

	/// <summary>
	/// 
	/// </summary>
	/// <param name="viewMatrix">viewMatrix is used as the followin;
	/// viewCoord = viewMatrix * modelCoord
	/// </param>
	void UpdateShaderParam(const XMFLOAT4X4& viewMatrix);

	void DrawBegin() { m_graphics.DrawBegin(); }
	void DrawEnd() { m_graphics.DrawEnd(); }

	void DrawTriangleList(D3DModelTriangleList* pModel);

	void ResizeBuffers(const SIZE& newSize);
private:
	void InitializeShaderContexts();
private:
	D3DGraphics m_graphics;
	SIZE m_viewSize;
	float m_viewNearZ;
	float m_viewFarZ;
	D3DBufferPtr m_pShaderParamConstBuf;
	D3DShaderContext m_triangleListSc;
};

}   // end of namespace D3D11Graphics
