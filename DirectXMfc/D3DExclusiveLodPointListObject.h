#pragma once
#include "D3DDrawingModel.h"
#include "D3DExclusiveLodPointListHeader.h"

namespace D3D11Graphics {

class D3DMemoryMappedFile;

class D3DExclusiveLodPointListObject : public D3DDrawingModel
{
public:
	typedef D3DGraphics3D::PointListVertex Vertex;
public:
	D3DExclusiveLodPointListObject(D3DMemoryMappedFile& imageFile, int64_t imagePos);

	void SetDrawingPrecision(double length) { m_drawingPrecision = length; }
	void SetMaxPointCountDrawnPerFrame(int64_t nPoint) { m_maxPointCountDrawnPerFrame = nPoint; }

	void PrepareFirstDraw(D3DGraphics3D& g);
protected:
	virtual void OnDrawTo(D3DGraphics3D& g);

private:
	D3DMemoryMappedFile& m_imageFile;
	int64_t m_imageByteBegin = 0;
	double m_drawingPrecision = 0;
	int64_t m_maxPointCountDrawnPerFrame = 1 << 20;	/// used only in case of progressive mode.

	// point list data initialized in OnDrawTo() once.
private:
	D3DExclusiveLodPointListHeader m_pointListHeader;
	int64_t m_pointByteBegin = 0;

	// Work variables used in OnDrawTo().
private:
	double m_precisionForFrame = 0;
	uint64_t m_nextVertex = 0;
};

typedef std::unique_ptr< D3DExclusiveLodPointListObject> D3DExclusiveLodPointListObjectPtr;

}	// end of namespace D3D11Graphics
