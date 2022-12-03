#include "pch.h"
#include "D3DDrawingModel.h"

using namespace D3D11Graphics;

D3DDrawingModel::~D3DDrawingModel()
{

}

void D3DDrawingModel::DrawTo(D3DGraphics3D& g)
{
	OnDrawTo(g);
}
