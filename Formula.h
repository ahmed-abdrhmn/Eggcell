#pragma once
#include <string>
#include <vector>

struct Index {
	unsigned column; unsigned row;
};

inline unsigned GetIndex(unsigned column, unsigned row) {
	return row | (column << 16);
}

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


struct Token {
	enum class type { value, op, null, func ,err } type;
	Value lit;
	enum class op { badd, usub, bsub, div, mul, exp, mod, opbrac, clbrac, null } op;
	static const char prec[];

	bool isindex(void) const {
		return type == type::value && lit.type() == Value::type::Index;
	}

	const Index& index(void) const {
		return lit.ind();
	}
};

std::vector<Token> genIR(const std::wstring& input);
std::vector<Index> extractIndicesFromIR(const std::vector<Token>& IRparam);
Value evalIR(const std::vector<Token>& RPN, Value(&)(const Value&));
