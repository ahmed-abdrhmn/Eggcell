#include <math.h>
#include "Formula.h"

const char Token::prec[]{ 0,3,0,1,1,2,1,4,4 };

Value::Value(void):_type(type::err) {}

Value::Value(double num):_type(type::Number),_number(num){}

Value::Value(std::wstring str):_type(type::Text),_text(str){}

Value::Value(Index ind):_type(type::Index),_index(ind){}

Value::Value(IndexRange indrange): _type(type::IndexRange), _indexrange(indrange) {};

Value::Value(std::vector<Value>& valuelist) : _type(type::ValueList), _valuelist(valuelist) {};

Value::Value(const Value& other) : _type(other._type) {
	switch (other._type) {
	case type::Text: {
		new(&_text) std::wstring(other._text);
		break;
	}
	case type::Number: {
		_number = other._number;
		break;
	}
	case type::Index: {
		_index = other._index;
		break;
	}
	case type::IndexRange: {
		_indexrange = other._indexrange;
		break;
	}
	case type::ValueList: {
		new (&_valuelist) std::vector<Value>(other._valuelist);
		break;
	}
	}
}

Value& Value::operator=(const Value& rhs) {
	//handling construction or destrucion of the text subobject
	if (_type == type::Text && rhs._type != type::Text) {
		_text.~basic_string();
	}
	else if (_type != type::Text && rhs._type == type::Text) {
		new(&_text) std::wstring(rhs._text);
	}
	else if (_type == type::Text && rhs._type == type::Text) {
		_text = rhs._text;
	}
	//handling construction or destruction of the valuelist subobject
	if (_type == type::ValueList && rhs._type != type::ValueList) {
		_valuelist.~vector();
	}
	else if (_type != type::ValueList && rhs._type == type::ValueList) {
		new(&_valuelist) std::vector<Value>(rhs._valuelist);
	}
	else if (_type == type::ValueList && rhs._type == type::ValueList) {
		_text = rhs._text;
	}

	switch (rhs._type)
	{
	case type::Number: {
		_number = rhs._number;
		break;
	}
	case type::Index: {
		_index = rhs._index;
		break;
	}
	case type::IndexRange: {
		_indexrange = rhs._indexrange;
		break;
	}
	}

	_type = rhs._type;

	return *this;
}

Value::~Value() {
	if (_type == type::Text) {
		_text.~basic_string();
	}
	else if (_type == type::ValueList) {
		_valuelist.~vector();
	}
}

Value Value::operator+(const Value& rhs) {
	/*return error if atleast one of the sides is error or if types are not equal each other*/
	if (_type == type::err || rhs._type == type::err  || _type != rhs._type) {	return Value();	}
	
	switch (_type)
	{
	case Value::type::Text: {
		return Value(_text + rhs._text); //concatenated text
	}
	case Value::type::Number: {
		return Value(_number + rhs._number); //add numbers
	}
	default: {
		return Value(); //for now just return error
	}
	}
}
Value Value::operator-(const Value& rhs) {
	/*return error if atleast one of the sides is error or if types are not equal each other*/
	if (_type == type::err || rhs._type == type::err || _type != rhs._type) { return Value(); }

	switch (_type)
	{
	case Value::type::Text: {
		return Value(); //error
	}
	case Value::type::Number: {
		return Value(_number - rhs._number); //add numbers
	}
	default: {
		return Value(); //for now just return error
	}
	}
}
Value Value::operator-(void) { //unary minus
	if (_type == type::Number) {
		return Value(-_number);
	}
	else {
		return Value();
	}
}
Value Value::operator/(const Value& rhs){
	if (_type == type::err || rhs._type == type::err || _type != rhs._type) { return Value(); }

	if (_type == type::Number) {
		return Value(_number / rhs._number);
	}
	else if (_type == type::Index) {
		return Value();
	}
	else {
		return Value();
	}
}
Value Value::operator*(const Value& rhs){
	if (_type == type::err || rhs._type == type::err || _type != rhs._type) { return Value(); }

	if (_type == type::Number) {
		return Value(_number * rhs._number);
	}
	else if (_type == type::Index) {
		return Value();
	}
	else {
		return Value();
	}
}
Value Value::operator%(const Value& rhs){
	if (_type == type::err || rhs._type == type::err || _type != rhs._type) { return Value(); }

	if (_type == type::Number) {
		return Value(fmod(_number,rhs._number));
	}
	else if (_type == type::Index) {
		return Value();
	}
	else {
		return Value();
	}
}
Value Value::operator^(const Value& rhs){
	if (_type == type::err || rhs._type == type::err || _type != rhs._type) { return Value(); }

	if (_type == type::Number) {
		return Value((int)pow((double)_number ,(double)rhs._number));
	}
	else if (_type == type::Index) {
		return Value();
	}
	else {
		return Value();
	}
}