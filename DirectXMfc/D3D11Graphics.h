#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>

#if defined(_WIN64)
#define D3D_IS_32BIT_MODULE (0)
#else
#define D3D_IS_32BIT_MODULE (1)
#endif

namespace D3D11Graphics {

typedef Microsoft::WRL::ComPtr<IDXGIAdapter> DXGIAdapterPtr;
typedef Microsoft::WRL::ComPtr<ID3D11Device> D3DDevicePtr;
typedef Microsoft::WRL::ComPtr<ID3D11DeviceContext> D3DDeviceContextPtr;

typedef Microsoft::WRL::ComPtr<ID3D11InputLayout> D3DInputLayoutPtr;
typedef Microsoft::WRL::ComPtr<ID3D11VertexShader> D3DVertexShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11GeometryShader> D3DGeometryShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11PixelShader> D3DPixelShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11DepthStencilState> D3DDepthStencilStatePtr;
typedef Microsoft::WRL::ComPtr<ID3D11RenderTargetView> D3DRenderTargetViewPtr;
typedef Microsoft::WRL::ComPtr<ID3D11Texture2D> D3DTexture2DPtr;
typedef Microsoft::WRL::ComPtr<ID3D11Resource> D3DResourcePtr;
typedef Microsoft::WRL::ComPtr<ID3D11Buffer> D3DBufferPtr;

struct D3DHandleCloser {
	void operator ()(HANDLE h) noexcept {
		if (h != INVALID_HANDLE_VALUE) {
			::CloseHandle(h);
		}
	}
};
struct D3DFindFileCloser {
	void operator ()(HANDLE h) noexcept {
		if (h != INVALID_HANDLE_VALUE) {
			::FindClose(h);
		}
	}
};
typedef std::unique_ptr<void, D3DHandleCloser> D3DUniqueHandle;
typedef D3DUniqueHandle D3DUniqueFileHandle;
typedef std::unique_ptr<void, D3DFindFileCloser> D3DUniqueFindFileHandle;

// Include std::swap() since D3D11Graphics::swap() will be defined.
// They oppose to call std::swap() without namespace.
using std::swap;

const uint8_t D3D_FILE_HEADER_EXCLUSIVE_LOD_POINT_LIST = 1;
const uint8_t D3D_FILE_HEADER_POINT_BLOCK_LIST = 2;

class D3DGraphics3D;

typedef uint64_t D3DSelectionTargetId;
#define D3D_SELECTION_TARGET_NULL D3DSelectionTargetId(0)

} // namespace D3D11Graphics
