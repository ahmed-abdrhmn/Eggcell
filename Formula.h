#pragma once
#include <string>
#include <vector>

struct Index {
	unsigned column; unsigned row;
};

struct IndexRange {
	unsigned column1,column2;
	unsigned row1, row2;
};

inline unsigned GetIndex(unsigned column, unsigned row) {
	return row | (column << 16);
}


class Value {
public:
	enum class type { Text, Number, Index, IndexRange, ValueList, err };

private:
	type _type;
	union {
		std::wstring _text;
		double _number;
		Index _index;
		IndexRange _indexrange;
		std::vector<Value> _valuelist;
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
	const IndexRange& indrange() const {
		return _indexrange;
	}
	std::vector<Value>& valuelist() { //we make an exception of constness for value list because...easier to code I guess?
		return _valuelist;
	}
	//ctors
	Value(void);
	Value(double num);
	Value(std::wstring str);
	Value(Index ind);
	Value(IndexRange indrange);
	Value(std::vector<Value>& valuelist);

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
	enum class op { badd, usub, bsub, div, mul, exp, mod, opbrac, clbrac, fclb, fopb, comma, null } op;
	static const char prec[];
	std::wstring function;
	char paramcnt; //number of parameters

	bool isindex(void) const {
		return type == type::value && lit.type() == Value::type::Index;
	}

	bool isindexrange(void) const {
		return type == type::value && lit.type() == Value::type::IndexRange;
	}

	const Index& index(void) const {
		return lit.ind();
	}

	const IndexRange& indexrange(void) const {
		return lit.indrange();
	}
};

std::vector<Token> genIR(const std::wstring& input);
std::vector<Index> extractIndicesFromIR(const std::vector<Token>& IRparam);
Value evalIR(const std::vector<Token>& RPN, Value(&)(const Value&));