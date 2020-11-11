#include <iostream>

#include <Utilities/Source/Benchmarking.h>

#define SEPARATOR(X) std::cout << "-------- " << X << " --------" << std::endl

enum Test
{
	Naive,
	Expr
};

class StackAllocatedVector3D
{
public:
	StackAllocatedVector3D(float x, float y, float z) :
		m_x(x),
		m_y(y),
		m_z(z)
	{}

	float x() const { return m_x; }
	float y() const { return m_y; }
	float z() const { return m_z; }

	float& x() { return m_x; }
	float& y() { return m_y; }
	float& z() { return m_z; }

protected:
	union
	{
		struct
		{
			float m_x;
			float m_y;
			float m_z;
		};

		float m_Data[3];
	};
};

class HeapAllocatedVector3D
{
public:
	HeapAllocatedVector3D(float x, float y, float z)
	{
		m_Data = new float[3];

		m_Data[0] = x;
		m_Data[1] = y;
		m_Data[2] = z;
	}

	virtual ~HeapAllocatedVector3D()
	{
		if(m_Data)
			delete[] m_Data;

		m_Data = nullptr;
	}

	float x() const { return m_Data[0]; }
	float y() const { return m_Data[1]; }
	float z() const { return m_Data[2]; }

	float& x() { return m_Data[0]; }
	float& y() { return m_Data[1]; }
	float& z() { return m_Data[2]; }

protected:
	float* m_Data = nullptr;
};

template<Test test, class StorageKing>
class Vector3D {};

///////////////
//-- Naive --//
///////////////

template<class StorageKind>
class Vector3D<Naive, StorageKind> : public StorageKind
{
public:
	Vector3D(float x, float y, float z) :
		StorageKind(x, y, z)
	{}

private:
	template<class StorageKind>
	friend Vector3D<Naive, StorageKind> operator+(const Vector3D<Naive, StorageKind>& a, const Vector3D<Naive, StorageKind>& b);
};

template<class StorageKind>
Vector3D<Naive, StorageKind> operator+(const Vector3D<Naive, StorageKind>& a, const Vector3D<Naive, StorageKind>& b)
{
	return Vector3D<Naive, StorageKind>(a.m_Data[0] + b.m_Data[0], a.m_Data[1] + b.m_Data[1], a.m_Data[2] + b.m_Data[2]);
}

//////////////
//-- Expr --//
//////////////

template<class E>
class Vector3DExpr
{
public:
	inline float operator[](size_t i) const { return Derived()[i]; }

protected:
	inline E& Derived() { return static_cast<E&>(*this); }
	inline const E& Derived() const { return static_cast<const E&>(*this); }
};

template<class StorageKind>
class Vector3D<Expr, StorageKind> : public Vector3DExpr<Vector3D<Expr, StorageKind>>, StorageKind
{
public:
	Vector3D(float x, float y, float z) :
		StorageKind(x, y, z)
	{}

	template<class E>
	Vector3D(const Vector3DExpr<E>& e) :
		StorageKind(e[0], e[1], e[2])
	{}

	inline float operator[](size_t i) const { return this->m_Data[i]; }
};

template<class EL, class ER>
class AddOp : public Vector3DExpr<AddOp<EL, ER>>
{
public:
	inline float operator[](size_t i) const { return el[i] + er[i]; }

private:
	const EL& el;
	const ER& er;

	AddOp(const EL& el, const ER& er) : 
		el(el),
		er(er)
	{}

	template<class _EL, class _ER>
	friend AddOp<_EL, _ER> operator+(const Vector3DExpr<_EL>& el, const Vector3DExpr<_ER>& er);
};

template<class _EL, class _ER>
AddOp<_EL, _ER> operator+(const Vector3DExpr<_EL>& el, const Vector3DExpr<_ER>& er)
{
	return AddOp<_EL, _ER>(static_cast<const _EL&>(el), static_cast<const _ER&>(er));
}

//////////////
//-- Main --//
//////////////

int main()
{
	const int iterations = 2000000;

#pragma region stack_allocation
	SEPARATOR("Naive stack allocated");
	{
		Vector3D<Naive, StackAllocatedVector3D> na = { 1, 2, 3 };
		Vector3D<Naive, StackAllocatedVector3D> nb = { 4, 5, 6 };
		Vector3D<Naive, StackAllocatedVector3D> nc = { 7, 8, 9 };

		TimePerformance(iterations, [&]() {
			Vector3D<Naive, StackAllocatedVector3D> nsum = na + nb + nc + na + nb + nc;
		});
	}

	SEPARATOR("Expr 1 stack allocated");
	{
		Vector3D<Expr, StackAllocatedVector3D> ea = { 1, 2, 3 };
		Vector3D<Expr, StackAllocatedVector3D> eb = { 4, 5, 6 };
		Vector3D<Expr, StackAllocatedVector3D> ec = { 7, 8, 9 };

		TimePerformance(iterations, [&]() {
			Vector3D<Expr, StackAllocatedVector3D> esum = ea + eb + ec + ea + eb + ec;
		});
	}

	SEPARATOR("Expr 2 stack allocated");
	{
		Vector3D<Expr, StackAllocatedVector3D> ea = { 1, 2, 3 };
		Vector3D<Expr, StackAllocatedVector3D> eb = { 4, 5, 6 };
		Vector3D<Expr, StackAllocatedVector3D> ec = { 7, 8, 9 };

		auto esum = ea + eb + ec + ea + eb + ec;

		TimePerformance(iterations, [&]() {
			Vector3D<Expr, StackAllocatedVector3D> result = esum;
		});
	}

	SEPARATOR("Expr 3 stack allocated");
	{
		Vector3D<Expr, StackAllocatedVector3D> ea = { 1, 2, 3 };
		Vector3D<Expr, StackAllocatedVector3D> eb = { 4, 5, 6 };
		Vector3D<Expr, StackAllocatedVector3D> ec = { 7, 8, 9 };

		auto esum = (ea + eb) + (ec + ea) + (eb + ec);

		TimePerformance(iterations, [&]() {
			Vector3D<Expr, StackAllocatedVector3D> result = esum;
		});
	}
#pragma endregion

#pragma region heap_allocation
	SEPARATOR("Naive heap allocated");
	{
		Vector3D<Naive, HeapAllocatedVector3D> na = { 1, 2, 3 };
		Vector3D<Naive, HeapAllocatedVector3D> nb = { 4, 5, 6 };
		Vector3D<Naive, HeapAllocatedVector3D> nc = { 7, 8, 9 };

		TimePerformance(iterations, [&]() {
			Vector3D<Naive, HeapAllocatedVector3D> nsum = na + nb + nc + na + nb + nc;
		});
	}

	SEPARATOR("Expr 1 heap allocated");
	{
		Vector3D<Expr, HeapAllocatedVector3D> ea = { 1, 2, 3 };
		Vector3D<Expr, HeapAllocatedVector3D> eb = { 4, 5, 6 };
		Vector3D<Expr, HeapAllocatedVector3D> ec = { 7, 8, 9 };

		TimePerformance(iterations, [&]() {
			Vector3D<Expr, HeapAllocatedVector3D> esum = ea + eb + ec + ea + eb + ec;
		});
	}

	SEPARATOR("Expr 2 heap allocated");
	{
		Vector3D<Expr, HeapAllocatedVector3D> ea = { 1, 2, 3 };
		Vector3D<Expr, HeapAllocatedVector3D> eb = { 4, 5, 6 };
		Vector3D<Expr, HeapAllocatedVector3D> ec = { 7, 8, 9 };

		auto esum = ea + eb + ec + ea + eb + ec;

		TimePerformance(iterations, [&]() {
			Vector3D<Expr, HeapAllocatedVector3D> result = esum;
			});
	}

	SEPARATOR("Expr 3 heap allocated");
	{
		Vector3D<Expr, HeapAllocatedVector3D> ea = { 1, 2, 3 };
		Vector3D<Expr, HeapAllocatedVector3D> eb = { 4, 5, 6 };
		Vector3D<Expr, HeapAllocatedVector3D> ec = { 7, 8, 9 };

		auto esum = (ea + eb) + (ec + ea) + (eb + ec);

		TimePerformance(iterations, [&]() {
			Vector3D<Expr, HeapAllocatedVector3D> result = esum;
		});
	}
#pragma endregion

	std::cin.get();
}