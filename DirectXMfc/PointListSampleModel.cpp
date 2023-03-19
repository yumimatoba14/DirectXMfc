#include "pch.h"
#include "PointListSampleModel.h"
#include "D3DModelColorUtil.h"
#include "D3DExclusiveLodPointListBuilder.h"
#include "D3DPointBlockListBuilder.h"
#include "D3DWin32File.h"

using namespace std;
using namespace DirectX;
using namespace D3D11Graphics;
using namespace D3D11Graphics::D3DModelColorUtil;

void PointListSampleModel::OnCreateBuffer(
	D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g,
	D3D11Graphics::D3DBufferPtr* ppVB, size_t* pnVertex
)
{
	const size_t nX = 1000;
	const size_t nY = 1000;
	const size_t nZ = 1;
	const double x0 = 0;
	const double y0 = 0;
	const double z0 = 0;
	const double aDist[3] = { 1.0 / nX, 1.0 / nY, -0.1 };
	const UINT aColor[3] = {
		RgbaF(0.0f, 1.0f, 1.0f, 1),
		RgbaF(0.0f, 0.0f, 1.0f, 1),
		RgbaF(1.0f, 0.0f, 0.5f, 1)
	};
	const UINT color2 = RgbaF(0.5f, 0.0f, 1.0f, 1);
	const bool useAnotherColor = false;
	vector<Vertex> vertices;
	for (size_t iZ = 0; iZ < nZ; ++iZ) {
		const float z = float(z0 + iZ * aDist[2]);
		const UINT color = aColor[iZ % 3];
		for (size_t iY = 0; iY < nY; ++iY) {
			for (size_t iX = 0; iX < nX; ++iX) {
				Vertex vtx{ XMFLOAT3(float(x0 + iX * aDist[0]), float(y0 + iY * aDist[1]), float(z)), color };
				if (useAnotherColor && iX % 2 == 1) {
					vtx.rgba = color2;
				}
				vertices.push_back(vtx);
			}
		}
	}

	*pnVertex = vertices.size();
	*ppVB = g.CreateVertexBuffer(vertices.data(), static_cast<UINT>(*pnVertex), false);
}

////////////////////////////////////////////////////////////////////////////////

PointListEnumeratorSampleModel::PointListEnumeratorSampleModel()
{
	m_nVertexInUnit = 1000 * 1000 * 1;
}

void PointListEnumeratorSampleModel::OnPreDraw(D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g)
{
	if (g3D.IsProgressiveViewMode()) {
		m_nMaxDrawnPointInFrame = m_nVertexInUnit;
		if (!g3D.IsProgressiveViewFollowingFrame()) {
			m_nextVertex = 0;
		}
		m_firstVertexInFrame = m_nextVertex;
	} else {
		m_nextVertex = 0;
		m_firstVertexInFrame = 0;
		m_nMaxDrawnPointInFrame = 0;
	}
	m_pGraphics = &g;
	m_isViewMoving = g3D.IsViewMoving();
	if (m_pVertexBuffer.Get() == nullptr) {
		PrepareVertices(g);
		P_ASSERT(m_pVertexBuffer);
	}
	GoNext();
}

void PointListEnumeratorSampleModel::OnPostDraw()
{
	m_pGraphics = nullptr;
}

D3DVertexBufferEnumerator::VertexBufferData PointListEnumeratorSampleModel::OnGetNext()
{
	size_t endVertex = min(m_nextVertex + m_nVertexInUnit, m_vertices.size());

	D3DVertexBufferEnumerator::VertexBufferData data;
	data.pVertexBuffer = m_pVertexBuffer;
	data.nVertex = UINT(endVertex - m_nextVertex);

	if (m_isViewMoving && 0 < m_nextVertex) {
		data.nVertex = 0;
	}
	else if (0 < m_nMaxDrawnPointInFrame) {
		if (m_firstVertexInFrame + m_nMaxDrawnPointInFrame <= m_nextVertex) {
			// This frame should be ended when m_nMaxDrawnPointInFrame points had been drawn.
			data.nVertex = 0;
		}
	}
		
	if (data.nVertex == 0) {
		data.pVertexBuffer = nullptr;
		return data;
	}

	{
		D3DMappedSubResource mappedMemory = m_pGraphics->MapDyamaicBuffer(m_pVertexBuffer);
		mappedMemory.Write(m_vertices.data() + m_nextVertex, (data.nVertex)*sizeof(Vertex));
	}

	m_nextVertex = endVertex;
	return data;
}

void PointListEnumeratorSampleModel::PrepareVertices(D3D11Graphics::D3DGraphics& g)
{
	const size_t nX = 1000;
	const size_t nY = 1000;
	const size_t nZ = 10;
	const double x0 = 0;
	const double y0 = 0;
	const double z0 = 0;
	const double aDist[3] = { 1.0 / nX, 1.0 / nY, -0.1 };
	const UINT aColor[3] = {
		RgbaF(0.0f, 1.0f, 1.0f, 1),
		RgbaF(0.0f, 0.0f, 1.0f, 1),
		RgbaF(1.0f, 0.0f, 0.5f, 1)
	};
	const UINT color2 = RgbaF(0.5f, 0.0f, 1.0f, 1);
	const bool useAnotherColor = false;
	m_vertices.clear();
	for (size_t iZ = 0; iZ < nZ; ++iZ) {
		const float z = float(z0 + iZ * aDist[2]);
		const UINT color = aColor[iZ % 3];
		for (size_t iY = 0; iY < nY; ++iY) {
			for (size_t iX = 0; iX < nX; ++iX) {
				Vertex vtx{ XMFLOAT3(float(x0 + iX * aDist[0]), float(y0 + iY * aDist[1]), float(z)), color };
				if (useAnotherColor && iX % 2 == 1) {
					vtx.rgba = color2;
				}
				m_vertices.push_back(vtx);
			}
		}
	}

	m_pVertexBuffer = g.CreateVertexBufferWithSize(
		static_cast<UINT>(m_nVertexInUnit * sizeof(Vertex)), nullptr, true);
}

////////////////////////////////////////////////////////////////////////////////

MemoryMappedPointListEnumeratorSampleModel::MemoryMappedPointListEnumeratorSampleModel(LPCTSTR pFilePath, bool useHeader)
{
	m_nVertexInUnit = 1 << 20;
	m_filePath = pFilePath ? pFilePath : _T("PlainPointCloud.bin");
	m_isUseHeader = useHeader;
}

void MemoryMappedPointListEnumeratorSampleModel::OnPreDraw(D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g)
{
	uint64_t nVertexInFile = PrepareFile();
	PrepareVertexBuffer(g);
	P_ASSERT(m_pVertexBuffer);

	bool isOk = m_mmFile.OpenToRead(m_filePath);
	if (!isOk) {
		P_THROW_ERROR("Open");
	}
	if (!m_pointListHeader.IsInitialized()) {
		if (m_isUseHeader) {
			size_t dataSize = 32;
			auto pSrc = m_mmFile.MapView(0, dataSize);
			size_t nHeaderByte = D3DExclusiveLodPointListHeader::ReadHeaderSize(
				pSrc.ToConstArray<char>(), dataSize);
			if (dataSize < nHeaderByte) {
				pSrc = m_mmFile.MapView(0, nHeaderByte);
			}
			m_pointListHeader.ReadFromBinaryData(pSrc.ToConstArray<char>(), nHeaderByte);
			m_pointByteBegin = nHeaderByte;
		} else {
			m_pointListHeader.ClearLevelInfo();
			m_pointListHeader.SetLevelInfo(0, nVertexInFile);
			m_pointByteBegin = 0;
		}
	}

	if (g3D.IsProgressiveViewMode()) {
		m_nMaxDrawnPointInFrame = m_nVertexInUnit;
		if (!g3D.IsProgressiveViewFollowingFrame()) {
			m_nextVertex = 0;
			m_drawingLength = m_pointListHeader.GetFirstLevelLength();
		}
		else {
			if (m_drawingPrecision < m_drawingLength) {
				m_drawingLength = m_pointListHeader.GetNextLevelLength(m_drawingLength);
			}
		}
		m_firstVertexInFrame = m_nextVertex;
	}
	else {
		m_nextVertex = 0;
		m_firstVertexInFrame = 0;
		m_nMaxDrawnPointInFrame = 0;
		m_drawingLength = 0;
	}

	m_pGraphics = &g;

	GoNext();
}

void MemoryMappedPointListEnumeratorSampleModel::OnPostDraw()
{
	m_pGraphics = nullptr;
}

D3DVertexBufferEnumerator::VertexBufferData MemoryMappedPointListEnumeratorSampleModel::OnGetNext()
{
	uint64_t endVertexInLevel = m_pointListHeader.GetEnoughPointCount(m_drawingLength);
	UINT nVertex = 0;

	uint64_t endVertex = min(m_nextVertex + m_nVertexInUnit, endVertexInLevel);

	nVertex = static_cast<UINT>(endVertex - m_nextVertex);

	if (0 < m_nMaxDrawnPointInFrame) {
		if (m_firstVertexInFrame + m_nMaxDrawnPointInFrame <= m_nextVertex) {
			// This frame should be ended since m_nMaxDrawnPointInFrame points had been drawn.
			nVertex = 0;
		}
	}

	if (nVertex == 0) {
		return D3DVertexBufferEnumerator::VertexBufferData{ nullptr, 0 };
	}

	{
		D3DMappedSubResource mappedMemory = m_pGraphics->MapDyamaicBuffer(m_pVertexBuffer);
		UINT dataSize = nVertex * sizeof(Vertex);
		auto pSrc = m_mmFile.MapView(m_pointByteBegin + m_nextVertex * sizeof(Vertex), dataSize);
		mappedMemory.Write(pSrc.Get(), dataSize);
	}

	m_nextVertex = endVertex;
	return D3DVertexBufferEnumerator::VertexBufferData{ m_pVertexBuffer, nVertex };
}

namespace {
	class PlainPointListBuilder : public D3DPointListBuilder
	{
	public:
		PlainPointListBuilder(LPCTSTR pFilePath)
		{
			m_outputFile.OpenNewFile(pFilePath);
		}

	protected:
		virtual void OnAddVertex(const Vertex& vertex)
		{
			m_outputFile.Write(&vertex, sizeof(Vertex));
			++m_nAddedVertex;
		}

		virtual uint64_t OnGetVertexCount() const { return m_nAddedVertex; }

	private:
		D3DWin32File m_outputFile;
		uint64_t m_nAddedVertex = 0;
	};
}

static uint64_t ToUInt64(DWORD high, DWORD low) { return (uint64_t(high) << 32) + low; }

/// <summary>
/// Create a file if it does not exist.
/// </summary>
/// <returns>number of vertex in file.</returns>
uint64_t MemoryMappedPointListEnumeratorSampleModel::PrepareFile()
{
	WIN32_FIND_DATA findFileData;
	D3DUniqueFindFileHandle hFind(::FindFirstFile(m_filePath, &findFileData));
	if (hFind.get() != INVALID_HANDLE_VALUE) {
		if (m_isUseHeader) {
			return 0;		// This value is not used.
		}
		else {
			uint64_t fileSize = ToUInt64(findFileData.nFileSizeHigh, findFileData.nFileSizeLow);
			return fileSize / sizeof(Vertex);
		}
	}

	unique_ptr<D3DPointListBuilder> pBuilder;
	if (this->m_isUseHeader) {
		pBuilder = make_unique<D3DExclusiveLodPointListBuilder>(m_filePath, 0.01);
	}
	else {
		pBuilder = make_unique<PlainPointListBuilder>(m_filePath);
	}

	const size_t nX = 1000;
	const size_t nY = 1000;
	const size_t nZ = 10;
	const double x0 = 0;
	const double y0 = 0;
	const double z0 = 0;
	const double aDist[3] = { 1.0 / nX, 1.0 / nY, -0.1 };
	const UINT aColor[3] = {
		RgbaF(0.0f, 1.0f, 1.0f, 1),
		RgbaF(0.0f, 0.0f, 1.0f, 1),
		RgbaF(1.0f, 0.0f, 0.5f, 1)
	};
	const UINT color2 = RgbaF(0.5f, 0.0f, 1.0f, 1);
	const bool useAnotherColor = false;
	for (size_t iZ = 0; iZ < nZ; ++iZ) {
		const float z = float(z0 + iZ * aDist[2]);
		const UINT color = aColor[iZ % 3];
		for (size_t iY = 0; iY < nY; ++iY) {
			for (size_t iX = 0; iX < nX; ++iX) {
				Vertex vtx{ XMFLOAT3(float(x0 + iX * aDist[0]), float(y0 + iY * aDist[1]), float(z)), color };
				if (useAnotherColor && iX % 2 == 1) {
					vtx.rgba = color2;
				}
				pBuilder->AddVertex(vtx);
			}
		}
	}

	return pBuilder->GetVertexCount();
}

void MemoryMappedPointListEnumeratorSampleModel::PrepareVertexBuffer(D3D11Graphics::D3DGraphics& g)
{
	if (!m_pVertexBuffer) {
		m_pVertexBuffer = g.CreateVertexBufferWithSize(
			static_cast<UINT>(m_nVertexInUnit * sizeof(Vertex)), nullptr, true);
	}
}

////////////////////////////////////////////////////////////////////////////////

MultiPointListSampleModel1::MultiPointListSampleModel1(LPCTSTR pFilePath)
{
	D3DVector3d pointListSize = D3DVector::Make(1, 1, 1);
	for (int i = 0; i < 5; ++i) {
		Instance inst;
		inst.pPointList = make_unique<MemoryMappedPointListEnumeratorSampleModel>(pFilePath, true);
		inst.offset = D3DVector::Make<double>(double(i) * 2, 0, 0);
		inst.aabb.Extend(inst.offset);
		inst.aabb.Extend(inst.offset + pointListSize);
		m_instances.push_back(move(inst));
	}
}

static double CalcPointListEnumerationPrecision(
	const D3DGraphics3D::XMFLOAT4X4& modelToViewMatrix, const D3DAabBox3d& aabb
)
{
	double minDistance = 1e128;
	for (int i = 0; i < 8; ++i) {
		D3DVector3d pnt;
		pnt[0] = ((i & 0x01) == 0 ? aabb.GetMinPoint()[0] : aabb.GetMaxPoint()[0]);
		pnt[1] = ((i & 0x02) == 0 ? aabb.GetMinPoint()[1] : aabb.GetMaxPoint()[1]);
		pnt[2] = ((i & 0x04) == 0 ? aabb.GetMinPoint()[2] : aabb.GetMaxPoint()[2]);
		double value = modelToViewMatrix.m[2][3];
		for (int j = 0; j < 3; ++j) {
			value += modelToViewMatrix.m[2][j] * pnt[j];
		}
		value *= -1;
		if (value < minDistance) {
			minDistance = value;
			if (minDistance < 0) {
				minDistance = 0;
				break;
			}
		}
	}
	return  0.001 * minDistance;  // TODO: fix me
}

static void SetPoinstListEnumerationPrecision(
	const D3DGraphics3D::XMFLOAT4X4& modelToViewMatrix, const D3DAabBox3d& aabb,
	MemoryMappedPointListEnumeratorSampleModel* pPointList
)
{
	double precision = CalcPointListEnumerationPrecision(modelToViewMatrix, aabb);
	pPointList->SetDrawingPrecision(precision);
}

void MultiPointListSampleModel1::OnDrawTo(D3D11Graphics::D3DGraphics3D& g)
{
	D3DGraphics3D::XMFLOAT4X4 orgModelMatrix = g.GetModelMatrix();
	D3DGraphics3D::XMFLOAT4X4 modelToViewMatrix = g.GetModelToViewMatrix();
	D3DGraphics3D::XMFLOAT4X4 localMtx = orgModelMatrix;
	for (size_t iContent = 0; iContent < m_instances.size(); ++iContent) {
		const Instance& inst = m_instances[iContent];
		for (int i = 0; i < 3; ++i) {
			localMtx(i, 3) = static_cast<float>(orgModelMatrix(i, 3) + inst.offset[i]);
		}
		g.SetModelMatrix(localMtx);
		SetPoinstListEnumerationPrecision(modelToViewMatrix, inst.aabb, inst.pPointList.get());
		g.DrawPointListEnumerator(inst.pPointList.get());
	}
	g.SetModelMatrix(orgModelMatrix);
}

////////////////////////////////////////////////////////////////////////////////

void MultiPointListSampleModel2::OnDrawTo(D3D11Graphics::D3DGraphics3D& g)
{
	PrepareBlockData();

	if (!g.IsProgressiveViewFollowingFrame()) {
		UpdateDrawnInstances(g);

		D3DGraphics3D::XMFLOAT4X4 modelToViewMatrix = g.GetModelToViewMatrix();

		for (auto iBlock : m_drawnInstanceIndices) {
			const InstanceData& instance = m_instanceList[iBlock];
			g.SetModelMatrix(instance.localToGlobalMatrix);
			double precision = CalcPointListEnumerationPrecision(modelToViewMatrix, instance.aabb);
			instance.pObject->SetDrawingPrecision(precision);
			instance.pObject->PrepareFirstDraw(g);
		}
	}
	bool isProgressiveMode = g.IsProgressiveViewMode();

	const size_t maxDrawnPointCountPerFrame = 2 << 20;
	for (auto iBlock : m_drawnInstanceIndices) {
		const InstanceData& instance = m_instanceList[iBlock];
		g.SetModelMatrix(instance.localToGlobalMatrix);
		instance.pObject->DrawTo(g);
		if (isProgressiveMode && maxDrawnPointCountPerFrame < g.GetDrawnPointCount()) {
			break;
		}
	}
}

static bool CalcBoxDistanceInProjection(const XMMATRIX& modelProjMatrix, const D3DAabBox3d& aabb, double* pDistance)
{
	const float tolZero = 1e-6f;
	*pDistance = 0;
	D3DAabBox3d projAabb;
	bool hasNegativeW = false;
	for (int i = 0; i < 8; ++i) {
		float coord[3];
		coord[0] = static_cast<float>((i & 0x01) ? aabb.GetMaxPoint()[0] : aabb.GetMinPoint()[0]);
		coord[1] = static_cast<float>((i & 0x02) ? aabb.GetMaxPoint()[1] : aabb.GetMinPoint()[1]);
		coord[2] = static_cast<float>((i & 0x04) ? aabb.GetMaxPoint()[2] : aabb.GetMinPoint()[2]);

		XMVECTOR moelVec = XMVectorSet(coord[0], coord[1], coord[2], 1);
		XMVECTOR projVecHomo = XMVector4Transform(moelVec, modelProjMatrix);
		float w = XMVectorGetW(projVecHomo);
		// w = -zv where zv is z in view coordinate system. Negative z shall be drawn in RH system.
		if (w < tolZero) {
			P_ASSERT(XMVectorGetZ(projVecHomo) < 0);
			w = tolZero;
			hasNegativeW = true;
		}
		double invW = 1.0 / w;
		D3DVector3d projVec;
		projVec[0] = XMVectorGetX(projVecHomo) * invW;
		projVec[1] = XMVectorGetY(projVecHomo) * invW;
		projVec[2] = XMVectorGetZ(projVecHomo) * invW;
		projAabb.Extend(projVec);
	}

	D3DVector3d minPoint = projAabb.GetMinPoint();
	D3DVector3d maxPoint = projAabb.GetMaxPoint();
	for (int i = 0; i < 2; ++i) {
		if (maxPoint[i] < -1 || 1 < minPoint[i]) {
			return false;
		}
	}
	if (maxPoint[2] < 0 || 1 < minPoint[2]) {
		return false;
	}
	if (!hasNegativeW && 0 < minPoint[2]) {
		*pDistance = minPoint[2];
	}
	return true;
}

void MultiPointListSampleModel2::UpdateDrawnInstances(D3D11Graphics::D3DGraphics3D& g)
{
	m_drawnInstanceIndices.clear();

	D3DGraphics3D::XMMATRIX modelProjMatrix = g.GetModelToProjectionMatrix();

	size_t nBlock = m_instanceList.size();
	multimap<double, size_t> distanceToBlock;
	for (size_t iBlock = 0; iBlock < nBlock; ++iBlock) {
		double distance = 0;
		bool isVisible = CalcBoxDistanceInProjection(modelProjMatrix, m_instanceList[iBlock].aabb, &distance);
		if (!isVisible) {
			continue;
		}
		distanceToBlock.emplace(distance, iBlock);
	}
	for (auto distToIndex : distanceToBlock) {
		m_drawnInstanceIndices.push_back(distToIndex.second);
	}
}

void MultiPointListSampleModel2::PrepareBlockData()
{
	if (!m_instanceList.empty()) {
		return;
	}

	bool isOk = m_mmFile.OpenToRead(m_filePath);
	if (!isOk) {
		PrepareFile();

		isOk = m_mmFile.OpenToRead(m_filePath);
		P_IS_TRUE(isOk);
	}

	D3DPointBlockListHeader header;
	header.ReadFromFile(m_mmFile);

	auto& imageList = header.GetBlockList();

	size_t nBlock = imageList.size();
	m_instanceList.reserve(nBlock);
	for (size_t iBlock = 0; iBlock < nBlock; ++iBlock) {
		const auto& image = imageList[iBlock];
		InstanceData instance;
		instance.localToGlobalMatrix = image.localToGlobalMatrix;
		instance.aabb.Extend(image.aAabbPoint[0]);
		instance.aabb.Extend(image.aAabbPoint[1]);
		instance.pObject = make_unique<D3DExclusiveLodPointListObject>(m_mmFile, image.firstBytePos);
		m_instanceList.push_back(move(instance));
	}
}

void MultiPointListSampleModel2::PrepareFile()
{
	auto pBuilder = make_unique<D3DPointBlockListBuilder>(m_filePath);

	const size_t nX = 1000;
	const size_t nY = 1000;
	const size_t nZ = 1;
	const double x0 = 0;
	const double y0 = 0;
	const double z0 = 0;
	const double aDist[3] = { 1.0 / nX, 1.0 / nY, -1 };
	const UINT aColor[3] = {
		RgbaF(0.0f, 1.0f, 1.0f, 1),
		RgbaF(0.0f, 0.0f, 1.0f, 1),
		RgbaF(1.0f, 0.0f, 0.5f, 1)
	};
	for (size_t iZ = 0; iZ < nZ; ++iZ) {
		const float z = float(z0 + iZ * aDist[2]);
		const UINT color = aColor[iZ % 3];
		for (size_t iY = 0; iY < nY; ++iY) {
			for (size_t iX = 0; iX < nX; ++iX) {
				D3DGraphics3D::PointListVertex vtx{ XMFLOAT3(float(x0 + iX * aDist[0]), float(y0 + iY * aDist[1]), float(z)), color };
				pBuilder->AddVertex(vtx);
			}
		}
	}

	pBuilder->BuildPointBlockFile();
}

////////////////////////////////////////////////////////////////////////////////
