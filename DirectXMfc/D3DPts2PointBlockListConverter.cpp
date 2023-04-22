#include "pch.h"
#include "D3DPts2PointBlockListConverter.h"
#include "D3DPointBlockListBuilder.h"
#include "D3DModelColorUtil.h"
#include <fstream>

using namespace D3D11Graphics;
using namespace std;

void D3DPts2PointBlockListConverter::ConvertFile(LPCTSTR pPtsFilePath, LPCTSTR pOutputFilePath)
{
	ifstream ptsInput(wstring(CT2W(pPtsFilePath)), ios_base::in);
	if (!ptsInput.is_open()) {
		P_THROW_ERROR("Failed to open pts file.");
	}
	const size_t nBufferByte = 1024;
	char aBufferByte[nBufferByte];
	auto readToBuffer = [&aBufferByte, nBufferByte](istream& input) {
		aBufferByte[nBufferByte - 1] = '\0';
		input.getline(aBufferByte, nBufferByte);
		if (aBufferByte[0] == '\0') {
			P_THROW_ERROR("Failed to read a line.");
		}
		if (nBufferByte - 1 <= strlen(aBufferByte)) {
			P_THROW_ERROR("Filed to read a long line.");
		}
	};

	D3DPointBlockListBuilder builder(pOutputFilePath);

	readToBuffer(ptsInput);
	int64_t nPoint = _atoi64(aBufferByte);
	for (int64_t iPoint = 0; iPoint < nPoint; ++iPoint) {
		readToBuffer(ptsInput);
		double aCoord[3];
		double intencity;
		int rgb[3];
		int numRead = sscanf_s(aBufferByte, "%lg %lg %lg %lg %i %i %i",
			aCoord + 0, aCoord + 1, aCoord + 2, &intencity, rgb + 0, rgb + 1, rgb + 2);
		P_IS_TRUE(numRead == 7);

		D3DPointBlockListBuilder::Vertex vertex;
		vertex.pos.x = static_cast<float>(aCoord[0]);
		vertex.pos.y = static_cast<float>(aCoord[1]);
		vertex.pos.z = static_cast<float>(aCoord[2]);
		vertex.rgba = D3DModelColorUtil::Rgb(rgb[0], rgb[1], rgb[2]);

		builder.AddVertex(vertex);
	}

	builder.BuildPointBlockFile();
}
