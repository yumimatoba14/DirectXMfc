#pragma once

#include "D3D11Graphics.h"

namespace D3D11Graphics {

class D3DVertexBufferEnumerator
{
public:
	struct VertexBufferData
	{
		D3DBufferPtr pVertexBuffer;
		UINT nVertex;
	};
protected:
	~D3DVertexBufferEnumerator();

public:
	bool HasCurrent() const { return m_current.pVertexBuffer.Get() != nullptr; }

	VertexBufferData GetCurrent() const { return m_current; }

	/// <summary>
	/// 
	/// </summary>
	/// <returns>HasCurrent()</returns>
	bool GoNext()
	{
		m_current = OnGetNext();
		return HasCurrent();
	}

protected:
	virtual VertexBufferData OnGetNext() = 0;

private:
	VertexBufferData m_current;
};

}
