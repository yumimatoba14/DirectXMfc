#pragma once

#include "D3D11Graphics.h"

namespace D3D11Graphics {

class D3DPts2PointBlockListConverter
{
public:
	void ConvertFile(LPCTSTR pPtsFilePath, LPCTSTR pOutputFilePath);
};

}
