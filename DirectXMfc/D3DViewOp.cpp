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

static float InnerProduct(const XMVECTOR& lhs, const XMVECTOR& rhs)
{
	return XMVectorGetX(XMVector3Dot(lhs, rhs));
}

static XMVECTOR OuterProduct(const XMVECTOR& lhs, const XMVECTOR& rhs)
{
	return XMVector3Cross(lhs, rhs);
}

////////////////////////////////////////////////////////////////////////////////

D3DViewOp::D3DViewOp()
	: m_eyePoint(MakeXmVector(0,0,0)), m_eyeDirection(MakeXmVector(0, 0, -1)),
	m_upDirection(MakeXmVector(0, 1, 0)), m_verticalDirection(MakeXmVector(0, 0, 0)),
	m_mode(MODE_NONE)
{
}

////////////////////////////////////////////////////////////////////////////////

void D3DViewOp::SetEyePoint(double x, double y, double z)
{
	m_eyePoint = MakeXmVector(x, y, z);
}

void D3DViewOp::SetUpDirection(double x, double y, double z)
{
	XMVECTOR newUpDir;
	if (!NormalizeXmVector(ZERO_TOL, MakeXmVector(x, y, z), &newUpDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}
	XMVECTOR rightDir = OuterProduct(m_eyeDirection, newUpDir);
	XMVECTOR newEyeDir;
	if (!NormalizeXmVector(ZERO_TOL, OuterProduct(newUpDir, rightDir), &newEyeDir)) {
		rightDir = OuterProduct(m_eyeDirection, m_upDirection);
		if (!NormalizeXmVector(ZERO_TOL, OuterProduct(newUpDir, rightDir), &newEyeDir)) {
			P_THROW_ERROR("NormalizeXmVector");
		}
	}
	m_eyeDirection = newEyeDir;
	m_upDirection = newUpDir;
}

void D3DViewOp::SetVerticalDirection(double x, double y, double z)
{
	if (!NormalizeXmVector(ZERO_TOL, MakeXmVector(x, y, z), &m_verticalDirection)) {
		m_verticalDirection = MakeXmVector(0, 0, 0);
	}
}

void D3DViewOp::RotateHorizontally(double rightAngleRad)
{
	XMVECTOR rightDir = OuterProduct(m_eyeDirection, m_upDirection);
	if (!NormalizeXmVector(ZERO_TOL, rightDir, &rightDir)) {
		P_IGNORE_ERROR("NormalizeXmVector");
		return;
	}
	float c = (float)cos(rightAngleRad);
	float s = (float)sin(rightAngleRad);
	XMVECTOR newDir = c * m_eyeDirection + s * rightDir;

	XMVECTOR newEyeDir;
	if (!NormalizeXmVector(ZERO_TOL, newDir, &newEyeDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}
	m_eyeDirection = newEyeDir;
}

void D3DViewOp::RotateAroundVerticalDirection(double rightAngleRad)
{
	const double PERP_ANGLE_TOL_COS = 0.5;
	double upAngleCos = InnerProduct(m_verticalDirection, m_upDirection);
	if (fabs(upAngleCos) < PERP_ANGLE_TOL_COS) {
		RotateHorizontally(rightAngleRad);
		return;
	}

	XMVECTOR rightDir = OuterProduct(m_eyeDirection, m_upDirection);
	double rightDirAngleCos = InnerProduct(m_verticalDirection, rightDir);
	double eyeDirAngleCos = InnerProduct(m_verticalDirection, m_eyeDirection);
	if (fabs(rightDirAngleCos) > PERP_ANGLE_TOL_COS || fabs(eyeDirAngleCos) > PERP_ANGLE_TOL_COS) {
		RotateHorizontally(rightAngleRad);
		return;
	}

	XMVECTOR refRightDir = OuterProduct(m_eyeDirection, m_verticalDirection);
	if (!NormalizeXmVector(ZERO_TOL, refRightDir, &refRightDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}
	XMVECTOR refDir = OuterProduct(m_verticalDirection, refRightDir);

	double rotAngleRad = rightAngleRad * (upAngleCos < 0 ? -1 : 1);
	float c = (float)cos(rotAngleRad);
	float s = (float)sin(rotAngleRad);
	XMVECTOR newRefDir = c * refDir + s * refRightDir;
	XMVECTOR newRightDir = c * refRightDir - s * refDir;

	XMVECTOR newEyeDir = float(eyeDirAngleCos) * m_verticalDirection + float(sqrt(1 - eyeDirAngleCos * eyeDirAngleCos)) * newRefDir;
	XMVECTOR newUpDir = OuterProduct(newRightDir, newEyeDir);
	if (upAngleCos < 0) {
		newUpDir *= -1;
	}

	if (!NormalizeXmVector(ZERO_TOL, newEyeDir, &newEyeDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}
	if (!NormalizeXmVector(ZERO_TOL, newUpDir, &newUpDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}

	m_eyeDirection = newEyeDir;
	m_upDirection = newUpDir;
}

void D3DViewOp::RotateVertically(double upAngleRad)
{
	float c = (float)cos(upAngleRad);
	float s = (float)sin(upAngleRad);
	XMVECTOR newEyeDir;
	if (!NormalizeXmVector(ZERO_TOL, c * m_eyeDirection + s * m_upDirection, &newEyeDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}
	XMVECTOR newUpDir;
	if (!NormalizeXmVector(ZERO_TOL, c * m_upDirection - s * m_eyeDirection, &newUpDir)) {
		P_THROW_ERROR("NormalizeXmVector");
	}
	m_eyeDirection = newEyeDir;
	m_upDirection = newUpDir;
}

void D3DViewOp::Pan(double distanceToRight, double distanceToUp)
{
	m_eyePoint += float(distanceToRight) * GetRightDir() + float(distanceToUp) * m_upDirection;
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

void D3DViewOp::StartMouseMove(const CPoint& point, MouseStartOption option)
{
	m_isMouseMoved = false;
	m_mouseLastPoint = point;
	switch (option) {
	case MOUSE_L_BUTTON:
		m_mode = MODE_ROTATE;
		break;
	case MOUSE_R_BUTTON:
		m_mode = MODE_PAN;
		break;
	default:
		m_mode = MODE_NONE;
	}
}

void D3DViewOp::MouseMove(const CPoint& point)
{
	m_isMouseMoved = true;
	if (m_mode == MODE_ROTATE) {
		constexpr float baseAngleRad = XMConvertToRadians(45) * 0.5f;
		CPoint diff = point - m_mouseLastPoint;
		//RotateHorizontally(-1.0 * diff.x / 100.0 * baseAngleRad);
		RotateAroundVerticalDirection(-1.0 * diff.x / 100.0 * baseAngleRad);
		RotateVertically(diff.y / 100.0 * baseAngleRad);
	}
	else if (m_mode == MODE_PAN) {
		const double velocity = 0.01;
		CPoint diff = point - m_mouseLastPoint;
		Pan(-diff.x * velocity, diff.y * velocity);
	}
	m_mouseLastPoint = point;
}

void D3DViewOp::EndMouseMove(const CPoint& point)
{
	MouseMove(point);
	m_mode = MODE_NONE;
}

////////////////////////////////////////////////////////////////////////////////

XMVECTOR D3DViewOp::GetRightDir() const
{
	return OuterProduct(m_eyeDirection, m_upDirection);
}

////////////////////////////////////////////////////////////////////////////////
