#pragma once

/// <summary>
/// This class implements view operations.
/// </summary>
class D3DViewOp
{
private:
	typedef DirectX::XMVECTOR XMVECTOR;
public:
	enum MouseStartOption {
		MOUSE_L_BUTTON = 0x01,
		MOUSE_R_BUTTON = 0x02,
		MOUSE_M_BUTTON = 0x04,
	};
	enum Mode {
		MODE_NONE,
		MODE_ROTATE,
		MODE_PAN
	};
public:
	D3DViewOp();

	void SetEyePoint(double x, double y, double z);
	void SetUpDirection(double x, double y, double z);
	void SetVerticalDirection(double x, double y, double z);

	void RotateHorizontally(double rightAngleRad);
	void RotateAroundVerticalDirection(double rightAngleRad);
	void RotateVertically(double upAngleRad);
	void Pan(double distanceToRight, double distanceToUp);
	void GoForward(double length);

	DirectX::XMFLOAT4X4 GetViewMatrix() const;
	CPoint GetMouseLastPoint() const { return m_mouseLastPoint; }

public:
	void StartMouseMove(const CPoint& point, MouseStartOption option);
	void MouseMove(const CPoint& point);
	void EndMouseMove() { m_mode = MODE_NONE; }
	void EndMouseMove(const CPoint& point);
	bool IsMouseMoving() const { return m_mode != MODE_NONE; }
	bool IsMouseMoved() const { return m_isMouseMoved; }

	void SetViewSize(const CSize& size) { m_viewSize = size; }
private:
	XMVECTOR GetRightDir() const;
private:
	XMVECTOR m_eyePoint;
	XMVECTOR m_eyeDirection;
	XMVECTOR m_upDirection;
	XMVECTOR m_verticalDirection;

	Mode m_mode;
	bool m_isMouseMoved;
	CPoint m_mouseLastPoint;
	CSize m_viewSize;
};

