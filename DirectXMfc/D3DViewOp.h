#pragma once

/// <summary>
/// This class implements view operations.
/// </summary>
class D3DViewOp
{
private:
	typedef DirectX::XMVECTOR XMVECTOR;
public:
	D3DViewOp();

	void SetEyePoint(double x, double y, double z);
	void SetEyeDirection(const XMVECTOR& dir);

	void RoateHorizontally(double rightAngleRad);
	void GoForward(double length);

	DirectX::XMFLOAT4X4 GetViewMatrix() const;

public:
	void StartMouseMove(const CPoint& point);
	void MouseMove(const CPoint& point);
	void EndMouseMove(const CPoint& point);
	bool IsMouseMoving() const { return m_isMouseMoving; }

private:
	XMVECTOR m_eyePoint;
	XMVECTOR m_eyeDirection;
	XMVECTOR m_upDirection;

	bool m_isMouseMoving;
	CPoint m_mouseLastPoint;
};

