#include "pch.h"
#include "D3DViewOp.h"
#include <DirectXMath.h>

using namespace DirectX;

const double ZERO_TOL = 1e-6;

static XMVECTOR MakeXmVector(float x, float y, float z)
{
	XMFLOAT3 vec(x, y, z);
	return XMLoadFloat3(&vec);
}

static XMVECTOR MakeXmVector(int x, int y, int z)
{
	return MakeXmVector(float(x), float(y), float(z));
}

static XMVECTOR MakeXmVector(double x, double y, double z)
{
	return MakeXmVector(float(x), float(y), float(z));
}

static double CalcXmVectorLength(const XMVECTOR& vec)
{
	XMVECTOR res = XMVector3LengthSq(vec);
	return sqrt(res.m128_f32[0]);
}

static bool NormalizeXmVector(double zeroTol, XMVECTOR vec, XMVECTOR* pResult)
{
	P_IS_TRUE(pResult);
	double len = CalcXmVectorLength(vec);
	if (len < zeroTol) {
		return false;
	}
	*pResult = float(1.0 / len) * vec;
	return true;
}

static XMVECTOR OuterProduct(const XMVECTOR& lhs, const XMVECTOR& rhs)
{
	return XMVector3Cross(lhs, rhs);
}

////////////////////////////////////////////////////////////////////////////////

D3DViewOp::D3DViewOp()
	: m_eyePoint(MakeXmVector(0,0,0)), m_eyeDirection(MakeXmVector(0, 0, -1)),
	m_upDirection(MakeXmVector(0, 1, 0)),
	m_isMouseMoving(false)
{
}

////////////////////////////////////////////////////////////////////////////////

void D3DViewOp::SetEyePoint(double x, double y, double z)
{
	m_eyePoint = MakeXmVector(x, y, z);
}

void D3DViewOp::SetEyeDirection(const XMVECTOR& dir)
{
	XMVECTOR newEyeDir;
	if (!NormalizeXmVector(ZERO_TOL, dir, &newEyeDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}
	m_eyeDirection = newEyeDir;
}

void D3DViewOp::RoateHorizontally(double rightAngleRad)
{
	XMVECTOR rightDir = OuterProduct(m_eyeDirection, m_upDirection);
	if (!NormalizeXmVector(ZERO_TOL, rightDir, &rightDir)) {
		P_IGNORE_ERROR("NormalizeXmVector");
		return;
	}
	float c = (float)cos(rightAngleRad);
	float s = (float)sin(rightAngleRad);
	XMVECTOR newDir = c * m_eyeDirection + s * rightDir;
	SetEyeDirection(newDir);
}

void D3DViewOp::GoForward(double length)
{
	m_eyePoint = m_eyePoint + float(length) * m_eyeDirection;
}

XMFLOAT4X4 D3DViewOp::GetViewMatrix() const
{
	XMFLOAT4X4 viewMatrix;
	XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(
		XMMatrixLookToRH(m_eyePoint, m_eyeDirection, m_upDirection)
	));
	return viewMatrix;
}

////////////////////////////////////////////////////////////////////////////////

void D3DViewOp::StartMouseMove(const CPoint& point)
{
	m_isMouseMoving = true;
	m_mouseLastPoint = point;
}

void D3DViewOp::MouseMove(const CPoint& point)
{
	constexpr float baseAngleRad = XMConvertToRadians(45)*0.5f;
	if (m_isMouseMoving) {
		CPoint diff = point - m_mouseLastPoint;
		RoateHorizontally(-1.0 * diff.x / 100.0 * baseAngleRad);
		m_mouseLastPoint = point;
	}
}

void D3DViewOp::EndMouseMove(const CPoint& point)
{
	MouseMove(point);
	m_isMouseMoving = false;
}

////////////////////////////////////////////////////////////////////////////////

