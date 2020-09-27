#include <vector>
#include "Token.h"

inline void mul(std::vector<Value> &numstack) {
	Value rhs = *(numstack.cend() - 1);
	Value lhs = *(numstack.cend() - 2);
	
	numstack.pop_back();
	numstack.pop_back();
	
	Value result = lhs * rhs;
	numstack.push_back(result);
	
	return;
}

inline void add(std::vector<Value>& numstack) {
	Value rhs = *(numstack.cend() - 1);
	Value lhs = *(numstack.cend() - 2);

	numstack.pop_back();
	numstack.pop_back();

	Value result = lhs + rhs;
	numstack.push_back(result);

	return;
}

inline void bsub(std::vector<Value>& numstack) {
	Value rhs = *(numstack.cend() - 1);
	Value lhs = *(numstack.cend() - 2);

	numstack.pop_back();
	numstack.pop_back();

	Value result = lhs - rhs;
	numstack.push_back(result);

	return;
}

inline void div(std::vector<Value>& numstack) {
	Value rhs = *(numstack.cend() - 1);
	Value lhs = *(numstack.cend() - 2);

	numstack.pop_back();
	numstack.pop_back();

	Value result = lhs / rhs;
	numstack.push_back(result);

	return;
}

inline void usub(std::vector<Value>& numstack) {
	Value lhs = numstack.back();

	numstack.pop_back();

	Value result = - lhs;
	numstack.push_back(result);

	return;
}

inline void mod(std::vector<Value>& numstack) {
	Value rhs = *(numstack.cend() - 1);
	Value lhs = *(numstack.cend() - 2);

	numstack.pop_back();
	numstack.pop_back();

	Value result = lhs % rhs;
	numstack.push_back(result);

	return;
}

inline void exp(std::vector<Value>& numstack) {
	Value rhs = *(numstack.cend() - 1);
	Value lhs = *(numstack.cend() - 2);

	numstack.pop_back();
	numstack.pop_back();


	Value result = lhs ^ rhs;
	numstack.push_back(result);

	return;
}

Value evalRPN(const std::vector<Token> &RPN , Value(& DrefIndex)(const Value&)) {
	std::vector<Value> litstack;
	for (auto currenttoken : RPN) {
		if (currenttoken.type == Token::type::value) {
			if (currenttoken.lit.type() != Value::type::Index) {
				litstack.push_back(currenttoken.lit);
			}
			else {
				litstack.push_back(DrefIndex(currenttoken.lit));
			}
		}
		else{
			switch (currenttoken.op) {
			case Token::op::badd: {
				add(litstack);
				break;
			}
			case Token::op::bsub: {
				bsub(litstack);
				break;
			}
			case Token::op::mul: {
				mul(litstack);
				break;
			}
			case Token::op::div: {
				div(litstack);
				break;
			}
			case Token::op::usub: {
				usub(litstack);
				break;
			}
			case Token::op::mod: {
				mod(litstack);
				break;
			}
			case Token::op::exp: {
				exp(litstack);
				break;
			}
			}
		}
	}

	if (litstack.size() == 1)return litstack.front();
	else return Value();
}