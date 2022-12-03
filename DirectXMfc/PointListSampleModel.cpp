#include "pch.h"
#include "PointListSampleModel.h"
#include "D3DModelColorUtil.h"

using namespace std;
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
