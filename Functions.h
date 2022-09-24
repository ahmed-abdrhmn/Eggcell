#pragma once

#include <unordered_map>
#include <vector>
#include <math.h>
#include "Formula.h"

typedef void (*funcptr) (std::vector<Value>&);


const std::unordered_map<std::wstring, funcptr > functions_map = {
	{L"SIN", [](std::vector<Value>& numstack)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(sin(num.num()));
	}},
	{L"COS",[](std::vector<Value>& numstack)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(cos(num.num()));
	}},
	{L"TAN",[](std::vector<Value>& numstack)->void {
		Value& num = *(numstack.end() - 1); //set the topmost value in the stack to its sin value
		num = Value(tan(num.num()));
	}}
};
