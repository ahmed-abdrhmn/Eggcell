#include <string>
#include <vector>
#include <iostream>
#include "Formula.h"


class Lexxer {
	const wchar_t* pointer;
	bool FirstCallToNext;
	Token prevToken;


	inline bool isnum(const Token& arg) {
		return (arg.type == Token::type::value);
	}

	inline bool isop(const Token& arg) { // operations excluding brackets
		return (arg.type == Token::type::op && 
			!(arg.op == Token::op::opbrac || arg.op == Token::op::clbrac));
	}

	inline bool isbinaryop(const Token& arg) {
		return (arg.op == Token::op::badd) ||
			(arg.op == Token::op::bsub) ||
			(arg.op == Token::op::mul) ||
			(arg.op == Token::op::div) ||
			(arg.op == Token::op::exp) ||
			(arg.op == Token::op::mod);
	}

	inline bool isunaryop(const Token& arg) {
		return (arg.op == Token::op::usub);
	}

	inline bool isopbrac(const Token& arg) {
		return (arg.op == Token::op::opbrac);
	}

	inline bool isclbrac(const Token& arg) {
		return (arg.op == Token::op::clbrac);
	}

	inline bool isstart(const Token& arg) {
		return (arg.op == Token::op::opbrac) ||
			(arg.type == Token::type::null);
	}

	inline bool isend(const Token& arg) {
		return (arg.op == Token::op::clbrac) ||
			(arg.type == Token::type::null);
	}

public:
	Lexxer(const std::wstring& input) :pointer(input.c_str()), FirstCallToNext(true), prevToken{Token::type::null}{};
	Token next(void) {
		FirstCallToNext = false; //now it is no longer the first time next is called

		char32_t CChar; //Current Character
		unsigned char Clength; //Length of the Codepoint (1 or 2 uint16_t's)
		
		if (0xD800 <= *pointer && *pointer <= 0xDFFF) {
			CChar = ((*pointer & 0x03FF) << 16) | (*(pointer + 1) & 0x03FF);
			CChar = CChar - 0x10000;
			Clength = 2;
		}
		else {
			CChar = *pointer;
			Clength = 1;
		}

		//Token prevToken; //the token to return
		while (std::isspace(CChar)) { //skip spaces
			pointer += Clength;
		}

		//get symbol/number
		switch (CChar) {
		case L'+': {
			if (isstart(prevToken) || isop(prevToken)) { // + is unary
				pointer += Clength;
				prevToken = next(); //done to ignore unary plusses as they are effectless
				return prevToken;
			}
			else if(isclbrac(prevToken) || isnum(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::badd }; //+ is binary
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null }; //+ is error
			}
			pointer += Clength;
			break;
		}
		case L'-': {
			if (isstart(prevToken) || isop(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::usub }; // - is unary
			}
			else if(isclbrac(prevToken) || isnum(prevToken)){
				prevToken = Token{ Token::type::op,0,Token::op::bsub }; //- is binary
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null }; //- is error
			}
			pointer += Clength; //the reason for this only being here is that strol automatically points to first char after the number
			break;
		}
		case L'*': {
			if (isclbrac(prevToken) || isnum(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::mul };
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null };
			}
			pointer += Clength; //the reason for this only being here is that strol automatically points to first char after the number
			break;
		}
		case L'/': {
			if (isclbrac(prevToken) || isnum(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::div };
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null };
			}
			pointer += Clength; //the reason for this only being here is that strol automatically points to first char after the number
			break;
		}
		case L'^': {
			if (isclbrac(prevToken) || isnum(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::exp };
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null };
			}
			pointer += Clength; //the reason for this only being here is that strol automatically points to first char after the number
			break;
		}
		case L'%': {
			if (isclbrac(prevToken) || isnum(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::mod };
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null };
			}
			pointer += Clength; //the reason for this only being here is that strol automatically points to first char after the number
			break;
		}
		case L'(': {
			if (isstart(prevToken) ||isop(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::opbrac };
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null };
			}
			pointer += Clength; //the reason for this only being here is that strol automatically points to first char after the number
			break;
		}
		case L')': {
			if (isclbrac(prevToken) || isopbrac(prevToken) || isnum(prevToken)) {
				prevToken = Token{ Token::type::op,0,Token::op::clbrac };
			}
			else {
				prevToken = Token{ Token::type::err,0,Token::op::null };
			}
			pointer += Clength; //the reason for this only being here is that strol automatically points to first char after the number
			break;
		}
		case L'\"': {
			std::wstring temp;
			while ( *(++pointer) != L'\"') {
				if (*pointer == L'\0') { //unmatched quotation
					prevToken = Token{ Token::type::err,Value(),Token::op::null };
					return prevToken;
				}
				temp.push_back(*pointer);
			}
			prevToken = Token{Token::type::value,Value(temp),Token::op::null};
			pointer += Clength;; //so that pointer points to first char AFTER the "
			break;
		}
		//if end of string
		case L'\0': {
			if (isnum(prevToken) || isclbrac(prevToken)) {
				return Token{ Token::type::null,0,Token::op::null };
			}
			else {
				return Token{ Token::type::err,0,Token::op::null };
			}
			break;
		}
		// if not end of string or an operator then (for now) it must be a number or a referece(eg. A0) or something invalid
		default: {
		if (isstart(prevToken) || isop(prevToken)) {
			const wchar_t* previous = pointer;
			
			//Code that runs if The current token is an Index
			if(*pointer >= L'A' && *pointer <= L'Z'){
				while (*++pointer >= L'A' && *pointer <= L'Z');//skip the letters
				if (*pointer < L'0' || *pointer > L'9') {
					prevToken = Token{ Token::type::err,Value(),Token::op::null };
					return prevToken;
				};
				previous = pointer;
				//extract the row
				unsigned row = std::wcstol(pointer, (wchar_t**)&pointer, 0);
				//extract the column
				unsigned digitmul = 1;
				unsigned column = 0;
				while (*--previous >= L'A' && *previous <= L'Z') {
					column += (*previous - L'A' + 1) * digitmul;
					digitmul *= 26;
				}
				column--;//from one base to zero base
				prevToken = Token{ Token::type::value,Value(Index{column,row}),Token::op::null };
				break;
			}


			//Code that runs if The current token is a Number
			double number = std::wcstod(pointer, (wchar_t**)&pointer);
			if (pointer != previous) { //the token is number
				prevToken = Token{ Token::type::value,Value(number),Token::op::null };
			}
			else { //the token is unknown
				prevToken = Token{ Token::type::err,0,Token::op::null };
			}
			break;
		}
		else {
				prevToken = Token{ Token::type::err,0,Token::op::null };
				break;
		}
		}
		}
		
		return prevToken;
	} 
};

std::vector<Token> genIR(const std::wstring& input) {
	std::vector<Token> tokens; 
	std::vector<Token> operators; //used as a stack

	/*const wchar_t* current = input.c_str();*/
	/* read number or operator and add to tokens*/

	Lexxer lex(input);
	
	Token temp;

	while (1) {
		temp = lex.next();
		
		if (temp.type == Token::type::null) { 
			
			while(operators.size()){ //empty operators
				if (operators.back().op == Token::op::opbrac) { //unmatched open bracket
					tokens = std::vector<Token>{ Token{Token::type::err} };
					goto end; 
				}
				tokens.push_back(operators.back());
				operators.pop_back();
			};

			break;
		}
		
		if (temp.type == Token::type::value) {
			tokens.push_back(temp);
		}

		if (temp.type == Token::type::op) {
			if (temp.op == Token::op::opbrac) {
				operators.push_back(temp);
				//std::cout << "open bracket";
			}
			else if (temp.op == Token::op::clbrac) {
				while (operators.size() && (operators.back().op != Token::op::opbrac)) {
					tokens.push_back(operators.back());
					operators.pop_back();
					//std::cout << "close bracket";
				}

				if (operators.size()) { //discard the open bracket from operator stack without adding it to the output list
					operators.pop_back();
				}
				else {
					//std::wcout << L"*Invalid Input*\n"; //unmatched close bracket
					tokens = std::vector<Token>{ Token{Token::type::err} };
					goto end;
				}
			}
			else {
				char CurrPrec = Token::prec[(int)temp.op];
				while (operators.size() != 0 && ((Token::prec[(int)operators.back().op] > CurrPrec) || (Token::prec[(int)operators.back().op] == CurrPrec && (temp.op != Token::op::usub && temp.op != Token::op::exp))/*unary minus and exponents are the only right assoc operator*/ )) {
					if (operators.back().op == Token::op::opbrac)break;
					tokens.push_back(operators.back());
					operators.pop_back();
				} //keep popping operators until one of less precedence is found
				operators.push_back(temp);
			}
		}

		if (temp.type == Token::type::err) {
			//std::wcout << L"*Invalid Input*\n";
			tokens = std::vector<Token>{ Token{Token::type::err} };
			goto end;;
		}

	};

end: 
	tokens.push_back(Token{ Token::type::null });
	return tokens;
};

std::vector<Index> extractIndicesFromIR(const std::vector<Token>& IRparam) {
	const Token* tptr = &IRparam.front();
	std::vector<Index> toret;
	while (tptr->type != Token::type::null && tptr->type != Token::type::err) {
		if (tptr->isindex()) {
			toret.push_back(tptr->index());
		}
		tptr++;
	}
	return toret;
}