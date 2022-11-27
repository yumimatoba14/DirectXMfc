#include "pch.h"
#include "D3DShaderContext.h"

using namespace std;
using namespace D3D11Graphics;

void D3DShaderContext::Init(
	D3DInputLayoutPtr pInputLayout,
	D3DVertexShaderPtr pVS, D3DBufferPtr pVSConstBuffer,
	D3DGeometryShaderPtr pGS, D3DBufferPtr pGSConstBuffer,
	D3DPixelShaderPtr pPS
)
{
	m_pIAInputLayout = pInputLayout;
	m_pVS = move(pVS);
	m_pVSConstantBuffer = move(pVSConstBuffer);
	m_pGS = move(pGS);
	m_pGSConstantBuffer = move(pGSConstBuffer);
	m_pPS = move(pPS);
}
