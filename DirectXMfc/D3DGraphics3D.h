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

	// Remarks:
	// XMLFLAT4x4 is row majar where float values are ordred like the following
	// XMLFLAT4x4 = [  f0,  f1,  f2,  f3 ]
	//              [  f4,  f5,  f6,  f7 ]
	//              [  f8,  f9, f10, f11 ]
	//              [ f12, f13, f14, f15 ]
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

	XMFLOAT4X4 GetModelMatrix() const { return m_modelMatrix; }
	void SetModelMatrix(const XMFLOAT4X4& matrix) { m_modelMatrix = matrix; OnModelToViewMatrixModified(); }

	XMFLOAT4X4 GetViewMatrix() const { return m_viewMatrix; }
	void SetViewMatrix(const XMFLOAT4X4& matrix) { m_viewMatrix = matrix; OnModelToViewMatrixModified(); }

	XMFLOAT4X4 GetModelToViewMatrix() { return PrepareModelToViewMatrix(); }

	/// <summary>
	/// Get field-of-view angle of Y direction in degree.
	/// </summary>
	/// <returns></returns>
	double GetFovAngleYInDegree() const { return m_fovAngleYDeg; }
	void SetFovAngleYInDegree(double degree) { m_fovAngleYDeg = degree; OnShaderParamModified(); }

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
	void SetPointSize(double size) { m_pointSize = size; OnShaderParamModified(); }

	void DrawBegin();
	void DrawEnd() { m_graphics.DrawEnd(); }

	void DrawPointList(D3DModelPointList* pModel);
	void DrawPointListEnumerator(D3DModelPointListEnumerator* pModel);
	void DrawPointArray(const PointListVertex aVertex[], int64_t nVertex);
	void DrawTriangleList(D3DModelTriangleList* pModel);

	void ResizeBuffers(const SIZE& newSize);
	bool SaveViewToFile(REFGUID targetFormat, LPCTSTR targetFilePath)
	{
		return m_graphics.SaveViewToFile(targetFormat, targetFilePath);
	}

	int64_t GetDrawnPointCount() const { return m_graphics.GetDrawnPointCount(); }
private:
	void InitializeShaderContexts();

	void OnModelToViewMatrixModified() {
		m_isNeedToUpdateModelToViewMatrix = true;
		OnShaderParamModified();
	}
	XMFLOAT4X4 PrepareModelToViewMatrix();

	void OnShaderParamModified() { m_isNeedToUpdateShaderParameter = true; }
	void PrepareShaderParam() {
		if (m_isNeedToUpdateShaderParameter) {
			UpdateShaderParam();
		}
	}
	void UpdateShaderParam();

private:
	D3DGraphics m_graphics;
	/// <summary>
	/// (model point) = modelMatrix * (local point)
	/// </summary>
	XMFLOAT4X4 m_modelMatrix;
	/// <summary>
	/// A matrix to transform model coordinates to view coordinates.
	/// (view point) = viewMatrix * (model point)
	/// </summary>
	XMFLOAT4X4 m_viewMatrix;
	/// <summary>
	/// A matrix of viewMatrix * modelMatrix.
	/// </summary>
	XMFLOAT4X4 m_modelToViewMatrix;
	SIZE m_viewSize;
	float m_viewNearZ;
	float m_viewFarZ;
	double m_fovAngleYDeg;
	double m_pointSize;
	D3DBufferPtr m_pShaderParamConstBuf;
	D3DShaderContext m_triangleListSc;
	D3DShaderContext m_pointListSc;
	D3DBufferPtr m_pTempVertexBuffer;
	UINT m_tempVertexBufferSize = (1 << 20) * sizeof(PointListVertex);
	bool m_isViewMoving = false;
	bool m_isProgressiveViewMode = false;
	bool m_isProgressiveViewFollowingFrame = false;
	bool m_isNeedToUpdateModelToViewMatrix = true;
	bool m_isNeedToUpdateShaderParameter = true;
};

}   // end of namespace D3D11Graphics
