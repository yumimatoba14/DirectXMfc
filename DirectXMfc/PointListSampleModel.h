#pragma once

#include "D3DModelPointList.h"
#include "D3DModelPointListEnumerator.h"
#include "D3DDrawingModel.h"
#include "D3DMemoryMappedFile.h"
#include "D3DExclusiveLodPointListHeader.h"
#include "D3DExclusiveLodPointListObject.h"
#include "D3DPointBlockListHeader.h"
#include "D3DAabBox.h"

class PointListSampleModel : public D3D11Graphics::D3DModelPointList
{
protected:
	virtual void OnCreateBuffer(
		D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g,
		D3D11Graphics::D3DBufferPtr* ppVB, size_t* pnVertex
	);
};

////////////////////////////////////////////////////////////////////////////////

class PointListEnumeratorSampleModel : public D3D11Graphics::D3DModelPointListEnumerator
{
private:
	typedef DirectX::XMFLOAT3 XMFLOAT3;

public:
	PointListEnumeratorSampleModel();
protected:
	virtual void OnPreDraw(D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g);
	virtual void OnPostDraw();

	virtual D3DVertexBufferEnumerator::VertexBufferData OnGetNext();

private:
	void PrepareVertices(D3D11Graphics::D3DGraphics& g);

private:
	size_t m_nVertexInUnit;
	std::vector<Vertex> m_vertices;
	size_t m_nextVertex = 0;
	size_t m_nMaxDrawnPointInFrame = 0;
	size_t m_firstVertexInFrame = 0;
	D3D11Graphics::D3DBufferPtr m_pVertexBuffer;
	D3D11Graphics::D3DGraphics* m_pGraphics = nullptr;
	bool m_isViewMoving = false;
};

////////////////////////////////////////////////////////////////////////////////

class MemoryMappedPointListEnumeratorSampleModel : public D3D11Graphics::D3DModelPointListEnumerator
{
private:
	typedef DirectX::XMFLOAT3 XMFLOAT3;

public:
	MemoryMappedPointListEnumeratorSampleModel(LPCTSTR pFilePath, bool useHeader);

	void SetDrawingPrecision(double length) { m_drawingPrecision = length; }
protected:
	virtual void OnPreDraw(D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g);
	virtual void OnPostDraw();

	virtual D3DVertexBufferEnumerator::VertexBufferData OnGetNext();

private:
	uint64_t PrepareFile();
	void PrepareVertexBuffer(D3D11Graphics::D3DGraphics& g);

private:
	size_t m_nVertexInUnit;
	CString m_filePath;
	bool m_isUseHeader;
	D3D11Graphics::D3DMemoryMappedFile m_mmFile;
	D3D11Graphics::D3DExclusiveLodPointListHeader m_pointListHeader;
	uint64_t m_pointByteBegin = 0;
	uint64_t m_nextVertex = 0;
	uint64_t m_nMaxDrawnPointInFrame = 0;
	uint64_t m_firstVertexInFrame = 0;
	double m_drawingLength = 0;
	double m_drawingPrecision = 0;
	D3D11Graphics::D3DBufferPtr m_pVertexBuffer;
	D3D11Graphics::D3DGraphics* m_pGraphics = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

class MultiPointListSampleModel1 : public D3D11Graphics::D3DDrawingModel
{
public:
	typedef std::unique_ptr<MemoryMappedPointListEnumeratorSampleModel> ContentPtr;
	typedef D3D11Graphics::D3DVector3d D3DVector3d;
	typedef D3D11Graphics::D3DAabBox3d D3DAabBox3d;
	struct Instance {
		ContentPtr pPointList;
		D3DVector3d offset;
		D3DAabBox3d aabb;
	};
public:
	MultiPointListSampleModel1(LPCTSTR pFilePath);

protected:
	virtual void OnDrawTo(D3D11Graphics::D3DGraphics3D& g);

private:
	std::vector<Instance> m_instances;
};

////////////////////////////////////////////////////////////////////////////////

class MultiPointListSampleModel2 : public D3D11Graphics::D3DDrawingModel
{
public:
	typedef D3D11Graphics::D3DAabBox3d D3DAabBox3d;
	typedef DirectX::XMFLOAT4X4 XMFLOAT4X4;

	struct InstanceData {
		XMFLOAT4X4 localToGlobalMatrix;
		D3DAabBox3d aabb;
		D3D11Graphics::D3DExclusiveLodPointListObjectPtr pObject;
	};
public:
	MultiPointListSampleModel2(LPCTSTR pFilePath) : m_filePath(pFilePath) {}

protected:
	virtual void OnDrawTo(D3D11Graphics::D3DGraphics3D& g);

private:
	void UpdateDrawnInstances(D3D11Graphics::D3DGraphics3D& g);
	void PrepareBlockData();
	void PrepareFile();

private:
	CString m_filePath;
	D3D11Graphics::D3DMemoryMappedFile m_mmFile;
	std::vector<InstanceData> m_instanceList;
	std::vector<size_t> m_drawnInstanceIndices;
};

////////////////////////////////////////////////////////////////////////////////
