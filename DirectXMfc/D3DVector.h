#pragma once

#include "D3D11Graphics.h"

namespace D3D11Graphics {

/// <summary>
/// This class implicitly implements traits for D3DVectorN.
/// </summary>
/// <typeparam name="V"></typeparam>
template<typename V>
struct D3DVectorTraits
{
	typedef V VectorType;
	typedef typename V::CoordType CoordType;
	enum { DIM = V::DIM };

	static CoordType GetAt(const VectorType& v, int i) { return v[i]; }
	static void SetAt(VectorType& v, int i, CoordType coord) { v[i] = coord; }
};

////////////////////////////////////////////////////////////////////////////////

template<>
struct D3DVectorTraits<DirectX::XMFLOAT3>
{
	typedef DirectX::XMFLOAT3 VectorType;
	typedef float CoordType;
	enum { DIM = 3 };

	static CoordType GetAt(const VectorType& v, int i)
	{
		switch (i) {
		case 0: return v.x;
		case 1: return v.y;
		case 2: return v.z;
		}
		return 0;
	}
	static void SetAt(VectorType& v, int i, CoordType coord)
	{
		switch (i) {
		case 0: v.x = coord;
		case 1: v.y = coord;
		case 2: v.z = coord;
		}
	}
};

////////////////////////////////////////////////////////////////////////////////

template<int N, typename COORD = double>
class D3DVectorN
{
public:
	typedef COORD CoordType;
	enum { DIM = N };

	// This default constructor doesn't initialize this members
	// so as to omit initialization. It is not so harmfull for this class.
	D3DVectorN() {}

	D3DVectorN(const D3DVectorN&) = default;

	template<typename C>
	explicit D3DVectorN(const C aCoord[DIM])
	{
		for (int i = 0; i < DIM; ++i) {
			m_aCoord[i] = aCoord[i];
		}
	}

	template<typename V>
	D3DVectorN(const V& v)
	{
		for (int i = 0; i < DIM; ++i) {
			m_aCoord[i] = D3DVectorTraits<V>::GetAt(v, i);
		}
	}

	D3DVectorN& operator = (const D3DVectorN&) = default;

	operator const CoordType* () const { return m_aCoord; }

	CoordType& operator [](int i) { return m_aCoord[i]; }
	CoordType operator [](int i) const { return m_aCoord[i]; }

	D3DVectorN& operator += (const D3DVectorN& rhs)
	{
		for (int i = 0; i < DIM; ++i) {
			m_aCoord[i] += rhs[i];
		}
		return *this;
	}

	D3DVectorN& operator -= (const D3DVectorN& rhs)
	{
		for (int i = 0; i < DIM; ++i) {
			m_aCoord[i] -= rhs[i];
		}
		return *this;
	}

	D3DVectorN& operator *= (COORD scalar)
	{
		for (int i = 0; i < DIM; ++i) {
			m_aCoord[i] *= scalar;
		}
		return *this;
	}


	CoordType Dot(const D3DVectorN& rhs) const
	{
		CoordType ret = 0;
		for (int i = 0; i < DIM; ++i) {
			ret += m_aCoord[i] * rhs.m_aCoord[i];
		}
		return ret;
	}

	CoordType GetSqLength() const { return Dot(*this); }

	static D3DVectorN MakeZero()
	{
		D3DVectorN vec;
		for (int i = 0; i < DIM; ++i) {
			vec[i] = 0;
		}
		return vec;
	}

private:
	CoordType m_aCoord[DIM];
};

template<int N, typename COORD>
D3DVectorN<N, COORD> operator + (const D3DVectorN<N, COORD>& lhs, const D3DVectorN<N, COORD>& rhs)
{
	return D3DVectorN<N, COORD>(lhs) += rhs;
}

template<int N, typename COORD>
D3DVectorN<N, COORD> operator - (const D3DVectorN<N, COORD>& lhs, const D3DVectorN<N, COORD>& rhs)
{
	return D3DVectorN<N, COORD>(lhs) -= rhs;
}

template<int N, typename COORD>
D3DVectorN<N, COORD> operator * (COORD s, const D3DVectorN<N, COORD>& v)
{
	return D3DVectorN<N, COORD>(v) *= s;
}

template<int N, typename COORD>
D3DVectorN<N, COORD> operator * (const D3DVectorN<N, COORD>& v, COORD s)
{
	return D3DVectorN<N, COORD>(v) *= s;
}

////////////////////////////////////////////////////////////////////////////////

typedef D3DVectorN<3, double> D3DVector3d;
typedef D3DVectorN<3, float> D3DVector3f;

////////////////////////////////////////////////////////////////////////////////

// Utility class for D3DVectorN.
class D3DVector
{
public:
	template<class COORD>
	static D3DVectorN<3, COORD> Make(COORD x, COORD y, COORD z)
	{
		COORD aCoord[3] = { x, y, z };
		return D3DVectorN<3, COORD>(aCoord);
	}

	template<class V>
	static void CopyToArray(const V& vec, typename D3DVectorTraits<V>::CoordType aCoord[])
	{
		for (int i = 0; i < D3DVectorTraits<V>::DIM; ++i) {
			aCoord[i] = D3DVectorTraits<V>::GetAt(vec, i);
		}
	}
};

} // namespace D3D11Graphics
