#pragma once
#include "Formula.h"

struct Token {
	enum class type { value, op, null, err } type;
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