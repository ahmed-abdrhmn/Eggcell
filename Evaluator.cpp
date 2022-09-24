#include <vector>
#include "Functions.h"

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

Value evalIR(const std::vector<Token>& RPN , Value(& DrefIndex)(const Value&)) {
	std::vector<Value> litstack;
	const Token* currenttoken = &RPN.front();
	while(currenttoken->type != Token::type::null && currenttoken->type != Token::type::err) {
		if (currenttoken->type == Token::type::value) {
			if (currenttoken->lit.type() != Value::type::Index) {
				litstack.push_back(currenttoken->lit);
			}
			else {
				litstack.push_back(DrefIndex(currenttoken->lit));
			}
		}
		else if (currenttoken->type == Token::type::op){
			switch (currenttoken->op) {
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
		else { //token is a function
			functions_map.at(currenttoken->function)(litstack); //the token MUST contain a function that is in the map so no need to check
		}
		currenttoken++;
	}

	if (litstack.size() == 1)return litstack.front();
	else return Value(); //error
}