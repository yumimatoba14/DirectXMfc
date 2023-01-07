#pragma once

#include "D3DPointListBuilder.h"
#include "D3DAabBox.h"
#include "D3DWin32File.h"

namespace D3D11Graphics {

class D3DMemoryMappedFile;

class D3DExclusiveLodPointListBuilder : public D3DPointListBuilder
{
public:
	D3DExclusiveLodPointListBuilder(LPCTSTR pFilePath, double latticeLength);
	virtual ~D3DExclusiveLodPointListBuilder();

public:
	void BuildPointListFile();

protected:
	virtual void OnAddVertex(const Vertex& vertex);
	virtual uint64_t OnGetVertexCount() const { return m_nVertex; }

private:
	uint64_t m_nVertex = 0;
	CString m_resultFilePath;
	D3DAabBox3d m_pointAabb;
	double m_latticeLength;
	D3DWin32File m_tempFile;
};

} //namespace D3D11Graphics
