#include "pch.h"
#include "D3DPointBlockListBuilder.h"
#include "D3DExclusiveLodPointListCreator.h"
#include "D3DBinaryFormatter.h"
#include "D3DWin32FileStreamBuf.h"

using namespace D3D11Graphics;
using namespace DirectX;
using namespace std;

const uint8_t CURRENT_FILE_VERSION = 0;

////////////////////////////////////////////////////////////////////////////////

D3DPointBlockListBuilder::D3DPointBlockListBuilder(LPCTSTR pFilePath)
	: m_resultFilePath(pFilePath)
{
	m_inputVertexFile.OpenTempFile();
	m_inputVertexStreamBuf.AttachHandle(m_inputVertexFile.GetHandle());
}

D3DPointBlockListBuilder::~D3DPointBlockListBuilder()
{

}

////////////////////////////////////////////////////////////////////////////////

void D3DPointBlockListBuilder::AddVertex(const Vertex& vertex)
{
	m_inputVertexStreamBuf.Write(&vertex, sizeof(Vertex));
	m_pointAabb.Extend(D3DVector3d(vertex.pos));
	++m_nVertex;
}

////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// Calculate division max so that one lattie would have number of points less than targetVertexCountInLattice.
/// This function doesn't assure that the returned counts would satisfied the condition.
/// </summary>
/// <param name="aabb"></param>
/// <param name="nVertex"></param>
/// <param name="targetVertexCountInLattice"></param>
/// <param name="aResultCount">(output) result division counts</param>
static void CalculateLatticeDivisionCount(
	const D3DAabBox3d& aabb, uint64_t nVertex, uint64_t targetVertexCountInLattice,
	int aResultCount[3]
)
{
	P_IS_TRUE(aabb.IsInitialized());
	const uint64_t nLatticeRequired = (nVertex + targetVertexCountInLattice - 1) / targetVertexCountInLattice;
	P_IS_TRUE(nLatticeRequired < (1ui64 << 31));

	aResultCount[0] = aResultCount[1] = aResultCount[2] = 1;

	if (nLatticeRequired == 1) {
		return;
	}

	D3DVector3d aabbSize = aabb.GetMaxPoint() - aabb.GetMinPoint();

	int minAxis = 0;
	int maxAxis = 0;
	for (int i = 1; i < 3; ++i) {
		if (aabbSize[i] < aabbSize[minAxis]) {
			minAxis = i;
		}
		else if (aabbSize[maxAxis] < aabbSize[i]) {
			maxAxis = i;
		}
	}
	if (minAxis == maxAxis) {
		// all axies are same.
		P_ASSERT(minAxis == 0);
		maxAxis = (minAxis + 2) % 3;
	}

	const double tolZero = 1e-6;
	if (aabbSize[maxAxis] < tolZero) {
		// cannnot dividied.
		return;
	}

	int maxDivision = 1024;	// TODO: update this value.
	if (nLatticeRequired < maxDivision) {
		maxDivision = (int)nLatticeRequired;
	}
	int aSortedAxis[3] = { minAxis, 3 - minAxis - maxAxis, maxAxis };
	double shorterAxisLengthRatio[2] = { -1, -1 };
	int nZeroLengthAxis = 0;
	for (int i = 0; i < 2; ++i) {
		int axis = aSortedAxis[i];
		shorterAxisLengthRatio[i] = aabbSize[axis] / aabbSize[maxAxis];
		if (aabbSize[axis] * maxDivision < aabbSize[maxAxis]) {
			++nZeroLengthAxis;
		}
	}
	if (nZeroLengthAxis == 2) {
		aResultCount[maxAxis] = static_cast<int>(nLatticeRequired);
		return;
	}

	double cubicRatio = 1;
	for (int i = nZeroLengthAxis; i < 2; ++i) {
		cubicRatio *= shorterAxisLengthRatio[i];
	}
	double latticeLength = aabbSize[maxAxis] / pow(nLatticeRequired / cubicRatio, 1.0 / (3 - nZeroLengthAxis));

	for (int i = nZeroLengthAxis; i < 3; ++i) {
		int axis = aSortedAxis[i];
		aResultCount[axis] = static_cast<int>(ceil(aabbSize[axis] / latticeLength));
	}
}

namespace {
	class Vector3dToLatticeIndex
	{
	public:
		Vector3dToLatticeIndex(const D3DAabBox3d& range, const int aDivisionCount[3])
			: m_basePoint(range.GetMinPoint())
		{
			D3DVector3d length = range.GetMaxPoint() - m_basePoint;
			for (int i = 0; i < 3; ++i) {
				P_IS_TRUE(0 < aDivisionCount[i]);
				m_anDivision[i] = aDivisionCount[i];
				m_aBaseLength[i] = length[i] / aDivisionCount[i];
			}
		}

		D3DVector3d GetLatticeCenter(const XMINT3& indices) const
		{
			double aCoord[3] = {
				m_basePoint[0] + (indices.x + 0.5) * m_aBaseLength[0],
				m_basePoint[1] + (indices.y + 0.5) * m_aBaseLength[1],
				m_basePoint[2] + (indices.z + 0.5) * m_aBaseLength[2]
			};
			return D3DVector3d(aCoord);
		}

		DirectX::XMINT3 GetIndices(const D3DVector3d& v) const
		{
			DirectX::XMINT3 ret;
			ret.x = ToInt(v[0], m_basePoint[0], m_aBaseLength[0], m_anDivision[0]);
			ret.y = ToInt(v[1], m_basePoint[1], m_aBaseLength[1], m_anDivision[1]);
			ret.z = ToInt(v[2], m_basePoint[2], m_aBaseLength[2], m_anDivision[2]);
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
		static int ToInt(double v, double v0, double baseLength, int maxValue)
		{
			int i = static_cast<int>((v - v0) / baseLength);
			if (i < 0) {
				return 0;
			}
			else if (maxValue <= i) {
				return maxValue - 1;
			}
			return i;
		}
	private:
		D3DVector3d m_basePoint;
		double m_aBaseLength[3] = { 0 };
		int m_anDivision[3] = { 0 };
	};
}

namespace {
	struct BlockImage {
		D3DGraphics3D::XMFLOAT4X4 localToGlobalMatrix;
		double aAabbPoint[2][3];	// aAabbPoint[0] : min point, aAabbPoint[1] : max point.
		int64_t firstBytePos = 0;
		int64_t nImageByte = 0;
	};

}

// TODO: This function is ad-hoc. To be fixed.
static double DecideLodBaseLatticeLength(const D3DAabBox3d& blockAabb, int64_t numBlockVertex)
{
	int targetPointNumInLattice = 100;
	const int64_t targetLatticeCountUB = 1ui64 << 20;
	if (targetPointNumInLattice * targetLatticeCountUB < numBlockVertex) {
		if (INT_MAX * targetLatticeCountUB < numBlockVertex) {
			P_THROW_ERROR("Not supported case. numBlockVertex is too large.");
		}
		targetPointNumInLattice = static_cast<int>(numBlockVertex / targetLatticeCountUB);
	}
	int aResultCount[3];
	CalculateLatticeDivisionCount(blockAabb, numBlockVertex, targetPointNumInLattice, aResultCount);

	D3DVector3d diagonalVec = blockAabb.GetMaxPoint() - blockAabb.GetMinPoint();
	const double baseLengthMin = 0.01;
	double baseLength = HUGE_VAL;
	for (int i = 0; i < 3; ++i) {
		P_IS_TRUE(0 < aResultCount[i]);
		double len = diagonalVec[i] / aResultCount[i];
		if (len < baseLengthMin) {
			len = baseLengthMin;
		}
		if (len < baseLength) {
			baseLength = len;
		}
	}
	return baseLength;
}

void D3DPointBlockListBuilder::BuildPointBlockFile()
{
	if (!m_inputVertexFile.IsOpend()) {
		return;
	}
	m_inputVertexStreamBuf.pubsync();

	D3DWin32File dividedFile1;
	dividedFile1.OpenTempFile();
	vector<BlockData> dividedBlocks;
	Build1Level(m_inputVertexFile, GetVertexCount(), m_pointAabb, 1024 * 1024, dividedFile1, 0, &dividedBlocks);

	size_t nBlock = dividedBlocks.size();
	P_IS_TRUE(nBlock < (size_t(1) << 31));

	D3DBinaryFormatter writer;
	writer.SetBitFlags(0);
	writer.WriteInt8(D3D_FILE_HEADER_POINT_BLOCK_LIST);
	writer.WriteInt8(CURRENT_FILE_VERSION);
	writer.WriteInt32(writer.GetBitFlags());
	writer.WriteInt32((int32_t)nBlock);
	int64_t nHeaderByte = writer.GetCurrentPosition();

	vector<BlockImage> blockImages;
	blockImages.reserve(dividedBlocks.size());
	P_ASSERT(sizeof(BlockImage) == 128);
	for(size_t iBlock = 0; iBlock < nBlock; ++iBlock) {
		const BlockData& inBlock = dividedBlocks[iBlock];
		BlockImage image;
		image.localToGlobalMatrix = XMFLOAT4X4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
		D3DVector::CopyToArray(inBlock.aabb.GetMinPoint(), image.aAabbPoint[0]);
		D3DVector::CopyToArray(inBlock.aabb.GetMaxPoint(), image.aAabbPoint[1]);
		image.firstBytePos = 0;
		image.nImageByte = 0;
		blockImages.push_back(image);
		writer.WriteByte(sizeof(BlockImage), (char*)&image);
	}

	int64_t blockListBegin = writer.GetCurrentPosition();

	D3DWin32File resultFile;
	resultFile.OpenNewFile(m_resultFilePath);
	resultFile.SetFilePointer(blockListBegin, FILE_BEGIN);

	int64_t endOfFilePos = resultFile.GetFilePointer();

	for (size_t iBlock = 0; iBlock < nBlock; ++iBlock) {
		const BlockData& inBlock = dividedBlocks[iBlock];
		BlockImage& image = blockImages[iBlock];

		image.firstBytePos = endOfFilePos;

		const double latticeLength = DecideLodBaseLatticeLength(inBlock.aabb, inBlock.nVertex);
		D3DExclusiveLodPointListCreator creator(latticeLength);
		image.nImageByte = creator.CreateImage(
			dividedFile1, inBlock.blockByteBegin, inBlock.nVertex,
			inBlock.aabb, resultFile.GetHandle(), image.firstBytePos
		);
		endOfFilePos += image.nImageByte;
	}

	writer.SetCurrentPosition(nHeaderByte);
	for (size_t iBlock = 0; iBlock < nBlock; ++iBlock) {
		BlockImage& image = blockImages[iBlock];
		writer.WriteByte(sizeof(BlockImage), (char*)&image);
	}
	P_IS_TRUE(writer.GetCurrentPosition() == blockListBegin);

	resultFile.SetFilePointer(0, FILE_BEGIN);
	resultFile.Write(writer.GetBinaryData().data(), blockListBegin);
}

////////////////////////////////////////////////////////////////////////////////

void D3DPointBlockListBuilder::Build1Level(
	D3DWin32File& inputVertexFile, uint64_t nInputVertex, const D3DAabBox3d& pointAabb,
	uint64_t targetVertexCount,
	D3DWin32File& resultFile, int64_t resultFileByteBegin, vector<BlockData>* pResultBlockList
)
{
	int aDivisionCount[3] = { 0 };
	CalculateLatticeDivisionCount(pointAabb, nInputVertex, targetVertexCount, aDivisionCount);

	Vector3dToLatticeIndex toLatticeIndex(pointAabb, aDivisionCount);
	size_t nLattice = toLatticeIndex.GetLatticeCount();

	vector<int64_t> latticePointCounts;
	latticePointCounts.resize(nLattice, 0);

	inputVertexFile.SetFilePointer(0, FILE_BEGIN);
	D3DWin32FileStreamBuf inputVertexStreamBuf(inputVertexFile.GetHandle(), 1024*64);
	for (uint64_t iVertex = 0; iVertex < nInputVertex; ++iVertex) {
		Vertex vertex;
		inputVertexStreamBuf.Read(&vertex, sizeof(Vertex));

		D3DVector3d vertexPos = vertex.pos;
		XMINT3 latticeIndices = toLatticeIndex.GetIndices(vertexPos);
		size_t latticeIndex = toLatticeIndex.GetLatticeIndex(latticeIndices);
		latticePointCounts[latticeIndex]++;
	}

	vector<BlockData> resultBlockList;
	resultBlockList.resize(nLattice);
	int64_t nextPointBegin = 0;
	for (size_t iLattice = 0; iLattice < nLattice; ++iLattice) {
		resultBlockList[iLattice].blockByteBegin = resultFileByteBegin + nextPointBegin * sizeof(Vertex);
		nextPointBegin += latticePointCounts[iLattice];
	}

	inputVertexStreamBuf.pubseekpos(0);
	for (uint64_t iVertex = 0; iVertex < nInputVertex; ++iVertex) {
		Vertex vertex;
		inputVertexStreamBuf.Read(&vertex, sizeof(Vertex));

		D3DVector3d vertexPos = vertex.pos;
		XMINT3 latticeIndices = toLatticeIndex.GetIndices(vertexPos);
		size_t latticeIndex = toLatticeIndex.GetLatticeIndex(latticeIndices);

		BlockData& currBlock = resultBlockList[latticeIndex];
		P_IS_TRUE(currBlock.nVertex < latticePointCounts[latticeIndex]);

		resultFile.SetFilePointer(currBlock.blockByteBegin + currBlock.nVertex * sizeof(Vertex), FILE_BEGIN);
		resultFile.Write(&vertex, sizeof(Vertex));

		currBlock.nVertex++;
		currBlock.aabb.Extend(vertexPos);
	}

	resultBlockList.erase(
		remove_if(
			resultBlockList.begin(), resultBlockList.end(),
			[](const BlockData& block) -> bool { return block.nVertex == 0; }
		),
		resultBlockList.end()
	);

	*pResultBlockList = move(resultBlockList);
}

////////////////////////////////////////////////////////////////////////////////
