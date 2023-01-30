#include "pch.h"
#include "D3DExclusiveLodPointListCreator.h"
#include "D3DExclusiveLodPointListHeader.h"
#include "D3DMemoryMappedFile.h"
#include "D3DWin32File.h"
#include "D3DWin32FileStreamBuf.h"

using namespace std;
using namespace DirectX;
using namespace D3D11Graphics;

const double MIN_BASE_LENGTH = 1e-6;

////////////////////////////////////////////////////////////////////////////////

namespace {
	class VectorToLatticeIndex
	{
	public:
		VectorToLatticeIndex(const D3DAabBox3d& range, double baseLength)
			: m_basePoint(range.GetMinPoint()), m_baseLength(baseLength)
		{
			D3DVector3d length = range.GetMaxPoint() - m_basePoint;
			for (int i = 0; i < 3; ++i) {
				m_anDivision[i] = static_cast<int>(ceil(length[i] / m_baseLength));
				if (m_anDivision[i] < 1) {
					m_anDivision[i] = 1;
				}
			}
		}

		D3DVector3d GetLatticeCenter(const XMINT3& indices) const
		{
			double dHalfLength = 0.5 * m_baseLength;
			double aCoord[3] = {
				m_basePoint[0] + indices.x * m_baseLength + dHalfLength,
				m_basePoint[1] + indices.y * m_baseLength + dHalfLength,
				m_basePoint[2] + indices.z * m_baseLength + dHalfLength
			};
			return D3DVector3d(aCoord);
		}

		DirectX::XMINT3 GetIndices(const D3DVector3d& v) const
		{
			DirectX::XMINT3 ret;
			ret.x = ToInt(v[0], m_basePoint[0], m_anDivision[0]);
			ret.y = ToInt(v[1], m_basePoint[1], m_anDivision[1]);
			ret.z = ToInt(v[2], m_basePoint[2], m_anDivision[2]);
			return ret;
		}

		size_t GetLatticeCount() const
		{
			return size_t(m_anDivision[0]) * m_anDivision[1] * m_anDivision[2];
		}

		size_t GetLatticeIndex(const DirectX::XMINT3& indices) const
		{
			size_t ret = indices.z;
			ret *= m_anDivision[1];
			ret += indices.y;
			ret *= m_anDivision[0];
			ret += indices.x;
			return ret;
		}

	private:
		int ToInt(double v, double v0, int n) const
		{
			int i = static_cast<int>((v - v0) / m_baseLength);
			if (i < 0) {
				return 0;
			}
			else if (n <= i) {
				return n - 1;
			}
			return i;
		}
	private:
		D3DVector3d m_basePoint;
		double m_baseLength;
		int m_anDivision[3];
	};

	class LatticeInfo
	{
	public:
		bool hasPoint = false;
		double sqDistanceFromCenter;
	};

	class RemainingPointStorage
	{
	public:
		typedef D3DExclusiveLodPointListCreator::Vertex Point;

		RemainingPointStorage(D3DWin32File* pFile)
			: m_nWrittenPoint(0), m_pFile(pFile)
		{
			P_ASSERT(m_pFile && m_pFile->IsOpend());
			m_pointBuffer.reserve(1024 * 4);
			m_outputByteBegin = pFile->GetFilePointer();
		}

		~RemainingPointStorage()
		{
			Flush();
		}

		void AddPoint(const Point& vertex)
		{
			if (m_pointBuffer.size() == m_pointBuffer.capacity()) {
				Flush();
			}
			P_ASSERT(m_pointBuffer.size() < m_pointBuffer.capacity());
			m_pointBuffer.push_back(vertex);
		}

		void Flush()
		{
			if (m_pointBuffer.empty()) {
				return;
			}
			int64_t orgPos = m_pFile->GetFilePointer();
			P_IS_TRUE((m_nWrittenPoint + m_pointBuffer.size()) * sizeof(Point) < (uint64_t)orgPos);

			m_pFile->SetFilePointer(m_outputByteBegin + m_nWrittenPoint * sizeof(Point), FILE_BEGIN);
			DWORD nWrittenByte = 0;
			m_pFile->Write(m_pointBuffer.data(), m_pointBuffer.size() * sizeof(Point));
			m_nWrittenPoint += m_pointBuffer.size();
			m_pointBuffer.clear();

			m_pFile->SetFilePointer(orgPos, FILE_BEGIN);
		}

		uint64_t GetPointCount() const { return m_nWrittenPoint + m_pointBuffer.size(); }

	private:
		int64_t m_outputByteBegin;
		uint64_t m_nWrittenPoint;
		vector<Point> m_pointBuffer;
		D3DWin32File* m_pFile;
	};
}

void D3DExclusiveLodPointListCreator::CreateImage(
	D3DWin32File& inputVertexFile, int64_t nInputVertex,
	const D3DAabBox3d& vertexAabb,
	LPCTSTR pResultFilePath
)
{
	class CreatorImpl : public MmFileCreator {
	public:
		explicit CreatorImpl(LPCTSTR pPath) : m_pResultFilePath(pPath) {}
	protected:
		virtual D3DMemoryMappedFile Create(uint64_t fileSize)
		{
			D3DMemoryMappedFile resultFile;
			bool isOk = resultFile.OpenNewFile(m_pResultFilePath, fileSize);
			if (!isOk) {
				P_THROW_ERROR("OpenNewFile");
			}
			return resultFile;
		}
	private:
		LPCTSTR m_pResultFilePath;
	};

	CreateImageImpl(
		inputVertexFile, 0, nInputVertex, vertexAabb, CreatorImpl(pResultFilePath), 0
	);
}

/// <summary>
/// Create a point list image with exclusive LOD at the end of result file.
/// </summary>
/// <param name="inputVertexFile"></param>
/// <param name="inputVertexFileByteBegin"></param>
/// <param name="nInputVertex"></param>
/// <param name="vertexAabb"></param>
/// <param name="hResultFile"></param>
/// <param name="resultFileByteBegin"></param>
/// <returns>image byte size</returns>
int64_t D3DExclusiveLodPointListCreator::CreateImage(
	D3DWin32File& inputVertexFile, int64_t inputVertexFileByteBegin, int64_t nInputVertex,
	const D3DAabBox3d& vertexAabb,
	HANDLE hResultFile, int64_t resultFileByteBegin
)
{
	class CreatorImpl : public MmFileCreator {
	public:
		CreatorImpl(HANDLE hFile, int64_t byteBegin)
			: m_hResultFile(hFile), m_resultFileByteBegin(byteBegin)
		{}
	protected:
		virtual D3DMemoryMappedFile Create(uint64_t fileSize)
		{
			D3DMemoryMappedFile resultFile;
			resultFile.AttachFileToWrite(m_hResultFile, m_resultFileByteBegin + fileSize);
			return resultFile;
		}
	private:
		HANDLE m_hResultFile;
		int64_t m_resultFileByteBegin;
	};

	// TODO : CreateImageImpl() modifies inputVertexFile. Fix it.
	return CreateImageImpl(
		inputVertexFile, inputVertexFileByteBegin, nInputVertex, vertexAabb,
		CreatorImpl(hResultFile, resultFileByteBegin), resultFileByteBegin
	);
}

int64_t D3DExclusiveLodPointListCreator::CreateImageImpl(
	D3DWin32File& inputVertexFile, int64_t inputVertexFileByteBegin, int64_t nInputVertex,
	const D3DAabBox3d& vertexAabb,
	MmFileCreator& resultFileCreator, int64_t resultFileByteBegin
)
{
	VectorToLatticeIndex toLatticeIndex(vertexAabb, m_latticeLength);
	size_t nLattice = toLatticeIndex.GetLatticeCount();

	P_IS_TRUE(nLattice < (uint64_t)nInputVertex);
	P_IS_TRUE(MIN_BASE_LENGTH < m_latticeLength);

	D3DExclusiveLodPointListHeader header;
	const int nLevelOfLattice = 4;
	const double aLength[] = {
		m_latticeLength, m_latticeLength * 0.5, m_latticeLength * 0.25, m_latticeLength * 0.125
	};
	for (int i = 0; i < nLevelOfLattice; ++i) {
		header.SetLevelInfo(aLength[i], nInputVertex);
	}
	header.SetLevelInfo(MIN_BASE_LENGTH, nInputVertex);	// centinel to draw all vertices.
	size_t nHeaderByte = header.GetBinarySize();

	const uint64_t nResultFileByte = nHeaderByte + nInputVertex * sizeof(Vertex);
	D3DMemoryMappedFile resultFile = resultFileCreator.Create(nResultFileByte);

	uint64_t nCurrVertex = nInputVertex;
	uint64_t resultFileBegin = resultFileByteBegin + nHeaderByte;
	double latticeLength = header.GetFirstLevelLength();
	for (int i = 0; i < nLevelOfLattice; ++i) {
		inputVertexFile.SetFilePointer(inputVertexFileByteBegin, FILE_BEGIN);
		uint64_t nAddedLattice = Build1Level(
			inputVertexFile, nCurrVertex, vertexAabb, latticeLength, resultFile, resultFileBegin
		);
		if (nAddedLattice == 0) {
			// vertices are less than making lattices.
			break;
		}
		P_IS_TRUE(nAddedLattice <= nCurrVertex);
		nCurrVertex -= nAddedLattice;
		header.SetLevelInfo(latticeLength, nInputVertex - nCurrVertex);

		resultFileBegin += nAddedLattice * sizeof(Vertex);
		latticeLength = header.GetNextLevelLength(latticeLength);
	}

	if (0 < nCurrVertex) {
		inputVertexFile.SetFilePointer(inputVertexFileByteBegin, FILE_BEGIN);
		uint64_t resultFileEnd = resultFileByteBegin + nResultFileByte;
		CopyVertices(inputVertexFile, nCurrVertex, resultFile, resultFileBegin, resultFileEnd);
	}

	P_IS_TRUE(header.GetBinarySize() == nHeaderByte);
	{
		auto pHeaderMemory = resultFile.MapView(resultFileByteBegin, nHeaderByte);
		header.WriteToBinaryData(pHeaderMemory.ToArray<char>(), nHeaderByte);
	}

	return (int64_t)nResultFileByte;
}

/// <summary>
/// 
/// </summary>
/// <param name="baseLength"></param>
/// <param name="isLastLayer"></param>
/// <returns>number of added lattice</returns>
uint64_t D3DExclusiveLodPointListCreator::Build1Level(
	D3DWin32File& inputVertexFile, uint64_t nInputVertex,
	const D3DAabBox3d& pointAabb, double latticeLength,
	D3DMemoryMappedFile& resultFile, uint64_t resultFileBeginPos
)
{
	VectorToLatticeIndex toLatticeIndex(pointAabb, latticeLength);
	const size_t nLattice = toLatticeIndex.GetLatticeCount();
	if (nInputVertex < nLattice) {
		// There is not enough space to allocate points of lattices.
		return 0;
	}

	// Assign vertices to the lattice.
	// Not assigned vertices are stored to nextLevelPoints.
	RemainingPointStorage nextLavelPoints(&inputVertexFile);

	D3DMemoryMappedFile::MappedPtr pLatticePointMemory = resultFile.MapView(resultFileBeginPos, nLattice * sizeof(Vertex));
	Vertex* aLatticePoint = pLatticePointMemory.ToArray<Vertex>();
	vector<LatticeInfo> latticeInfos;
	latticeInfos.resize(nLattice);

	D3DWin32FileStreamBuf inputVertexStreamBuf(inputVertexFile.GetHandle(), 1024 * 64);
	for (uint64_t iVertex = 0; iVertex < nInputVertex; ++iVertex) {
		Vertex vertex;
		inputVertexStreamBuf.Read(&vertex, sizeof(Vertex));

		D3DVector3d vertexPos = vertex.pos;
		XMINT3 vtxIndices = toLatticeIndex.GetIndices(vertexPos);
		size_t latticeIndex = toLatticeIndex.GetLatticeIndex(vtxIndices);
		double sqDistanceFromCenter = (vertexPos - toLatticeIndex.GetLatticeCenter(vtxIndices)).GetSqLength();

		auto& info = latticeInfos[latticeIndex];
		bool setToLattice = true;		// true to set vertex to the lattice.
		bool addToNextLevel = false;	// true when either of 2 vertices are used in the next level.
		if (info.hasPoint) {
			// Choose the vertex of which sqDistanceFromCenter is the minimum.
			setToLattice = (sqDistanceFromCenter < info.sqDistanceFromCenter);
			addToNextLevel = true;
		}
		if (setToLattice) {
			info.hasPoint = true;
			info.sqDistanceFromCenter = sqDistanceFromCenter;
			std::swap(aLatticePoint[latticeIndex], vertex);
		}

		if (addToNextLevel) {
			nextLavelPoints.AddPoint(vertex);
		}
	}

	bool isNeedStrip = nInputVertex < nLattice + nextLavelPoints.GetPointCount();

	size_t nAddedLattice = nLattice;
	if (isNeedStrip) {
		// There is more than 1 lattice which has no point.
		// Strip such lattices and compress memory area so that lattices in the former area
		// would have vertices and ones in the latter wouldn't.
		size_t emptyLattice = 0;
		size_t filledLatticeEnd = nLattice;
		while (emptyLattice < filledLatticeEnd) {
			while (emptyLattice < filledLatticeEnd && latticeInfos[emptyLattice].hasPoint) {
				++emptyLattice;
			}
			while (emptyLattice < filledLatticeEnd && !latticeInfos[filledLatticeEnd - 1].hasPoint) {
				P_ASSERT(0 < filledLatticeEnd);
				--filledLatticeEnd;
			}
			P_ASSERT(emptyLattice <= filledLatticeEnd);

			if (emptyLattice < filledLatticeEnd) {
				P_ASSERT(emptyLattice + 1 < filledLatticeEnd);
				swap(aLatticePoint[emptyLattice], aLatticePoint[filledLatticeEnd - 1]);
				++emptyLattice;
				--filledLatticeEnd;
				P_ASSERT(emptyLattice <= filledLatticeEnd);
			}
			else {
				break;
			}
		}
		P_ASSERT(emptyLattice == filledLatticeEnd);

		nAddedLattice = emptyLattice;
	}

	return nAddedLattice;
}

/// <summary>
/// Copy vertices from input file to result file.
/// </summary>
/// <param name="inputVertexFile"></param>
/// <param name="nInputVertex"></param>
/// <param name="resultFile"></param>
/// <param name="resultFileBeginPos"></param>
/// <param name="resultFileEndPos"></param>
void D3DExclusiveLodPointListCreator::CopyVertices(
	D3DWin32File& inputVertexFile, uint64_t nInputVertex,
	D3DMemoryMappedFile& resultFile, uint64_t resultFileBeginPos, uint64_t resultFileEndPos
)
{
	P_ASSERT(resultFileBeginPos + nInputVertex * sizeof(Vertex) <= resultFileEndPos);
	const size_t nPointInLatterBuf = 1024 * 4;
	const size_t latterBufSize = nPointInLatterBuf * sizeof(Vertex);
	D3DMemoryMappedFile::MappedPtr pLatterMemory;

	for (uint64_t iVertex = 0; iVertex < nInputVertex; ++iVertex) {
		Vertex vertex;
		inputVertexFile.Read(&vertex, sizeof(Vertex));

		size_t indexInBuf = iVertex % nPointInLatterBuf;
		if (indexInBuf == 0) {
			uint64_t bufferBegin = resultFileBeginPos + iVertex * sizeof(Vertex);
			size_t bufSize = latterBufSize;
			if (resultFileEndPos < bufferBegin + bufSize) {
				bufSize = static_cast<size_t>(resultFileEndPos - bufferBegin);
			}
			pLatterMemory = resultFile.MapView(bufferBegin, bufSize);
			P_IS_TRUE(!pLatterMemory.IsNull());
		}
		pLatterMemory.ToArray<Vertex>()[indexInBuf] = vertex;
	}
}

////////////////////////////////////////////////////////////////////////////////
