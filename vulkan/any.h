#pragma once

#include <type_traits>
#include <stdexcept>
#include <typeinfo>

class any;

template<typename T>
T& any_cast(any& a);
template<typename T>
const T& any_cast(const any& a);

namespace std
{
	void swap(any& a, any& b) noexcept;
}

class any
{
	template<typename T>
	friend T& any_cast(any&);
	template<typename T>
	friend const T& any_cast(const any&);
	friend void ::std::swap(any&, any&) noexcept;

	struct any_base
	{
		virtual ~any_base() = default;

		virtual any_base* clone() const = 0;

		virtual const std::type_info& type_info() const = 0;
	};

	template<typename T>
	struct any_value : any_base
	{
		any_value(const T& t)
			: value(t) {}
		any_value(T&& t)
			: value(std::move(t)) {}
		virtual ~any_value() = default;

		any_base* clone() const override
		{
			return new any_value<T>(value);
		}

		const std::type_info& type_info() const override
		{
			return typeid(T);
		}

		T value;
	};
public:

	any()
		: _value(nullptr) {}
	any(const any& a)
		: _value(a._value->clone()) {}
	any(any& a)
		: _value(a._value->clone()) {}
	template<typename T>
	any(const T& t)
		: _value(new any_value<T>(t)) {}
	any(any&& a)
		: _value(a._value)
	{
		a._value = nullptr;
	}
	template<typename T>
	any(T&& t)
		: _value(new any_value<T>(std::move(t))) {}
	~any()
	{
		delete _value;
	}

	any& operator=(const any& a)
	{
		delete _value;
		_value = a._value->clone();
		return *this;
	}
	any& operator=(any& a)
	{
		delete _value;
		_value = a._value->clone();
		return *this;
	}
	template<typename T>
	any& operator=(const T& t)
	{
		delete _value;
		_value = new any_value<T>(t);
		return *this;
	}
	any& operator=(any&& a)
	{
		delete _value;
		_value = a._value;
		a._value = nullptr;
		return *this;
	}
	template<typename T>
	any& operator=(T&& t)
	{
		delete _value;
		_value = new any_value<T>(std::move(t));
		return *this;
	}

	const std::type_info& type_info() const
	{
		return _value->type_info();
	}

	bool empty() const
	{
		return _value == nullptr;
	}

	void clear()
	{
		delete _value;
		_value = nullptr;
	}

private:

	any_base* _value;
};

class bad_any_cast : public std::bad_cast
{
public:
	const char* what() const noexcept override
	{
		return "bad any cast";
	}
};

template <typename T>
T& any_cast(any& a)
{
	any::any_value<T>* value = dynamic_cast<any::any_value<T>*>(a._value);
	if (value == nullptr)
		throw bad_any_cast();
	return value->value;
}

template <typename T>
const T& any_cast(const any& a)
{
	any::any_value<T>* value = dynamic_cast<any::any_value<T>*>(a._value);
	if (value == nullptr)
		throw bad_any_cast();
	return value->value;
}

namespace std
{
	inline void swap(any& a, any& b) noexcept
	{
		if (&a == &b)
			return;
		std::swap(a._value, b._value);
	}
}
