#pragma once

#include "D3DVector.h"

namespace D3D11Graphics {

/// <summary>
/// Axis-aligned-bounding-box.
/// </summary>
/// <typeparam name="COORD"></typeparam>
template<typename V>
class D3DAabBox
{
public:
	typedef V VectorType;
	typedef typename D3DVectorTraits<V>::CoordType CoordType;
	enum { DIM = D3DVectorTraits<V>::DIM };

	D3DAabBox()
	{
		m_minPoint[0] = 0;
		m_maxPoint[0] = -1;
	}

	bool IsInitialized() const { return m_minPoint[0] <= m_maxPoint[0]; }

	VectorType GetMinPoint() const { return m_minPoint; }
	VectorType GetMaxPoint() const { return m_maxPoint; }

	bool IsInclude(const CoordType aCoord[DIM])
	{
		if (!IsInitialized()) {
			return false;
		}
		for (int i = 0; i < DIM; ++i) {
			if (aCoord[i] < m_minPoint[i]) {
				return false;
			}
			else if (m_maxPoint[i] < aCoord[i]) {
				return false;
			}
		}
		return true;
	}

	void Extend(const CoordType aCoord[DIM])
	{
		if (!IsInitialized()) {
			m_minPoint = m_maxPoint = VectorType(aCoord);
		}
		else {
			for (int i = 0; i < DIM; ++i) {
				if (aCoord[i] < m_minPoint[i]) {
					m_minPoint[i] = aCoord[i];
				}
				else if (m_maxPoint[i] < aCoord[i]) {
					m_maxPoint[i] = aCoord[i];
				}
			}
		}
	}
private:
	VectorType m_minPoint;
	VectorType m_maxPoint;
};

typedef D3DAabBox<D3DVector3d> D3DAabBox3d;

} // namespace D3D11Graphics
