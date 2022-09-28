#pragma once

#include <unordered_map>
#include <vector>
#include <math.h>
#include "Formula.h"


typedef void (*funcptr) (std::vector<Value>&,char);


const std::unordered_map<std::wstring, funcptr > functions_map = {
	{L"SIN", [](std::vector<Value>& numstack, char paramcnt)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(sin(num.num()));
	}},
	{L"COS",[](std::vector<Value>& numstack, char paramcnt)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(cos(num.num()));
	}},
	{L"TAN",[](std::vector<Value>& numstack, char paramcnt)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(tan(num.num()));
	}},
	{L"ASIN",[](std::vector<Value>& numstack, char paramcnt)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(asin(num.num()));
	}},
	{L"ACOS",[](std::vector<Value>& numstack, char paramcnt)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(acos(num.num()));
	}},
	{L"ATAN",[](std::vector<Value>& numstack, char paramcnt)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(atan(num.num()));
	}},
	{L"ADD",[](std::vector<Value>& numstack, char paramcnt)->void {
		Value num1 = *(numstack.cend() - 1);
		Value num2 = *(numstack.cend() - 2);

		numstack.pop_back();
		numstack.pop_back();

		Value result = num1 + num2;
		numstack.push_back(result);
	}},
	{L"AVG",[](std::vector<Value>& numstack, char paramcnt)->void {	
		if (paramcnt < 1) {
			numstack.push_back(Value()); //push an error to the top of the stack, note that this is a very hacky way to signal an error but it SHOULD work 🤔
			return;
		}

		double total = 0;
		double count = 0;
		for (int i = 0; i < paramcnt; i++) {
			Value& top(numstack.back());
			if (top.type() == Value::type::Number) {
				total += top.num();
				count += 1;
			}
			else if (top.type() == Value::type::ValueList) {
				for (Value i : top.valuelist()) {
					if (i.type() == Value::type::Number) {
						total += i.num();
						count += 1;
					}
				}
			}
			else {
				numstack.push_back(Value()); //again, this effectively means returning an error
				return;
			}
			numstack.pop_back();
		}

		Value result(total / count);
		numstack.push_back(result);
	}},
	{L"SUM",[](std::vector<Value>& numstack, char paramcnt)->void {
		if (paramcnt < 1) {
			numstack.push_back(Value()); //push an error to the top of the stack, note that this is a very hacky way to signal an error but it SHOULD work 🤔
			return;
		}

		double total = 0;
		for (int i = 0; i < paramcnt; i++) {
			Value& top(numstack.back());
			if (top.type() == Value::type::Number) {
				total += top.num();
			}
			else if (top.type() == Value::type::ValueList) {
				for (Value i : top.valuelist()) {
					if (i.type() == Value::type::Number) {
						total += i.num();
					}
				}
			}
			else {
				numstack.push_back(Value()); //again, this effectively means returning an error
				return;
			}
			numstack.pop_back();
		}

		Value result(total);
		numstack.push_back(result);
	}}
};
