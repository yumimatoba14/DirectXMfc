#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace D3D11Graphics {

typedef Microsoft::WRL::ComPtr<ID3D11InputLayout> D3DInputLayoutPtr;
typedef Microsoft::WRL::ComPtr<ID3D11VertexShader> D3DVertexShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11GeometryShader> D3DGeometryShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11PixelShader> D3DPixelShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11Buffer> D3DBufferPtr;

class D3DShaderContext
{
public:
	void Init(
		D3DInputLayoutPtr pInputLayout,
		D3DVertexShaderPtr pVS, D3DBufferPtr pVSConstBuffer,
		D3DGeometryShaderPtr pGS, D3DBufferPtr pGSConstBuffer,
		D3DPixelShaderPtr pPS
	);

	const D3DInputLayoutPtr& GetIAInputLayout() const { return m_pIAInputLayout; }
	const D3DVertexShaderPtr& GetVertexShader() const { return m_pVS; }
	const D3DBufferPtr& GetVSConstantBuffer() const { return m_pVSConstantBuffer; }
	const D3DGeometryShaderPtr& GetGeometryShader() const { return m_pGS; }
	const D3DBufferPtr& GetGSConstantBuffer() const { return m_pGSConstantBuffer; }
	const D3DPixelShaderPtr& GetPixelShader() const { return m_pPS; }

private:
	D3DInputLayoutPtr m_pIAInputLayout;
	D3DVertexShaderPtr m_pVS;
	D3DBufferPtr m_pVSConstantBuffer;
	D3DGeometryShaderPtr m_pGS;
	D3DBufferPtr m_pGSConstantBuffer;
	D3DPixelShaderPtr m_pPS;
};

}
