#pragma once
#include <string>
#include <vector>

struct Index {
	unsigned column; unsigned row;
	bool operator ==(const Index& rhs) const noexcept {
		return column == rhs.column && row == rhs.row;
	}
};

inline unsigned GetIndex(unsigned column, unsigned row) {
	return row | (column << 16);
}

namespace std {
	template<>
	class hash<Index> {
	public:
		size_t operator ()(const Index& arg) const {
			return GetIndex(arg.column, arg.row);
		}
	};
};

class Value {
public:
	enum class type { Text, Number, Index, err };

private:
	type _type;
	union {
		std::wstring _text;
		double _number;
		Index _index;
	};
public:

	//accessors
	const std::wstring& text()const {
		return _text;
	}
	const double& num()const {
		return _number;
	}
	const enum type& type()const {
		return _type;
	}
	const Index& ind()const {
		return _index;
	}
	//ctors
	Value(void);
	Value(double num);
	Value(std::wstring str);
	Value(Index ind);

	Value(const Value& other);

	Value& operator=(const Value& rhs);

	Value operator+(const Value& rhs);
	Value operator-(const Value& rhs);
	Value operator-(void); //unary minus
	Value operator/(const Value& rhs);
	Value operator*(const Value& rhs);
	Value operator%(const Value& rhs);
	Value operator^(const Value& rhs);

	~Value();
};



struct IR;

IR* evalexpr(const std::wstring& input);
std::vector<Index> extractIndicesFromIR(const IR* IRparam);
Value evalIR(const IR* RPN, Value(&)(const Value&));
