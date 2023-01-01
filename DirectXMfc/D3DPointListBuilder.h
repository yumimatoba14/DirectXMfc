#pragma once

#include "D3DGraphics3D.h"

namespace D3D11Graphics {

class D3DPointListBuilder
{
public:
	typedef D3DGraphics3D::PointListVertex Vertex;
public:
	virtual ~D3DPointListBuilder();
	void AddVertex(const Vertex& vertex) { OnAddVertex(vertex); }

	uint64_t GetVertexCount() const { return OnGetVertexCount(); }

protected:
	virtual void OnAddVertex(const Vertex& vertex) = 0;
	virtual uint64_t OnGetVertexCount() const = 0;
};

} // namespace D3D11Graphics

