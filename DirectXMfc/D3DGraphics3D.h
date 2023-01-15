#pragma once

#include "D3DGraphics.h"

class D3DViewOp;

namespace D3D11Graphics {

class D3DModelPointList;
class D3DModelPointListEnumerator;
class D3DModelTriangleList;

/// <summary>
/// D3DGraphics class which implements specific utility functions for 3D viewing.
/// </summary>
class D3DGraphics3D
{
public:
	typedef DirectX::XMFLOAT3 XMFLOAT3;
	typedef DirectX::XMFLOAT4X4 XMFLOAT4X4;

	struct ShaderParam {
		XMFLOAT4X4 viewMatrix;
		XMFLOAT4X4 projectionMatrix;
		float pointSizeX;
		float pointSizeY;
		float pixelSizeX;
		float pixelSizeY;
	};

	struct PointListVertex
	{
		XMFLOAT3 pos;
		UINT rgba;
	};

public:
	D3DGraphics3D();
	D3DGraphics3D(const D3DGraphics&) = delete;

	void Initialize(HWND hWnd);
	bool IsInitialized() const { return m_graphics.HasDevice(); }

	bool IsViewMoving() const { return m_isViewMoving; }
	void SetViewMoving(bool isMoving) { m_isViewMoving = isMoving; }

	bool IsProgressiveViewMode() const { return m_isProgressiveViewMode; }
	bool IsProgressiveViewFollowingFrame() const { return m_isProgressiveViewFollowingFrame; }
	void SetProgressiveViewMode(bool enableProgressiveView, bool isFollowingFrame = false);

	/// <summary>
	/// Get field-of-view angle of Y direction in degree.
	/// </summary>
	/// <returns></returns>
	double GetFovAngleYInDegree() const { return m_fovAngleYDeg; }
	void SetFovAngleYInDegree(double degree) { m_fovAngleYDeg = degree; }

	/// <summary>
	/// Return size of point.
	/// A negative value is used when it represents relative size.
	/// </summary>
	/// <returns></returns>
	double GetPointSize() const { return m_pointSize; }

	/// <summary>
	/// Set a size of drawn point.
	/// It is roughly a length in model space if a positive value is set.
	/// Set a negative value to specify relative size in pixel.
	/// </summary>
	/// <param name="size"></param>
	void SetPointSize(double size) { m_pointSize = size; }

	/// <summary>
	/// 
	/// </summary>
	/// <param name="viewMatrix">viewMatrix is used as the followin;
	/// viewCoord = viewMatrix * modelCoord
	/// </param>
	void UpdateShaderParam(const XMFLOAT4X4& viewMatrix);

	void DrawBegin();
	void DrawEnd() { m_graphics.DrawEnd(); }

	void DrawPointList(D3DModelPointList* pModel);
	void DrawPointListEnumerator(D3DModelPointListEnumerator* pModel);
	void DrawTriangleList(D3DModelTriangleList* pModel);

	void ResizeBuffers(const SIZE& newSize);
	bool SaveViewToFile(REFGUID targetFormat, LPCTSTR targetFilePath)
	{
		return m_graphics.SaveViewToFile(targetFormat, targetFilePath);
	}

	int64_t GetDrawnPointCount() const { return m_graphics.GetDrawnPointCount(); }
private:
	void InitializeShaderContexts();
private:
	D3DGraphics m_graphics;
	SIZE m_viewSize;
	float m_viewNearZ;
	float m_viewFarZ;
	double m_fovAngleYDeg;
	double m_pointSize;
	D3DBufferPtr m_pShaderParamConstBuf;
	D3DShaderContext m_triangleListSc;
	D3DShaderContext m_pointListSc;
	bool m_isViewMoving = false;
	bool m_isProgressiveViewMode = false;
	bool m_isProgressiveViewFollowingFrame = false;
};

}   // end of namespace D3D11Graphics
