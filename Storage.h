#pragma once
#include <unordered_set>
#include <unordered_map>
#include "Token.h"
#include <stdexcept>
#include <stack>
#include <string>
#define ERR_CIRCULAR_REF 1


inline unsigned GetIndex(unsigned column, unsigned row) {
	return row | (column << 16);
}



class WorkSheet {
public:
	//This is the cell struct that is public
	struct cell {
		//relating to value of the cell
		enum class type { tt, rl, null ,err} type; //tt = t_ex_t ,		rl = r_ea_l
		std::wstring tt; //text that is rendered to screen 
								//and simultaneously used to store the cell's data if its type is text
		double rl; //number stored(if type is number)
		std::wstring ExactInput;
	};
private:
	//This is the cell struct interally managed by the Worksheet class
	struct _cell {
		//relating to value of the cell 
		enum class type { tt, rl, null, err } type = type::null;//note cells have null type by default
		 //tt = t_ex_t ,		rl = r_ea_l
		std::wstring tt; //text that is rendered to screen 
								//and simultaneously used to store the cell's data if its type is text

		double rl; //number stored(if type is number)

		//Structures used to calculate the value of the cell
		std::wstring ExactInput;
		std::vector<Token> RPN;

		std::unordered_set<unsigned> affecton;
		std::unordered_set<unsigned> affectby;
	};


	std::unordered_map<unsigned, _cell> WorkSheetData;
	static WorkSheet* CurrentWorksheet;

	bool SearchSelfRef(unsigned rootindex, const _cell& root) {


		struct stackpair { _cell cell; std::unordered_set<unsigned>::const_iterator cabptr; };
		std::stack<stackpair> sStack;

		sStack.push(stackpair{ root });
		//std::string ToPrint = "root cell affect by size = " + std::to_string(root.affectby.size());
		//MessageBoxA(NULL, ToPrint.c_str(), "CABSIZE", MB_OK);

		sStack.top().cabptr = sStack.top().cell.affectby.cbegin();
		while (sStack.size()) {
			if (sStack.top().cabptr != sStack.top().cell.affectby.cend()) {
				
				//MessageBoxA(NULL, "Checking Reference", "Checking Reference", MB_OK);
				
				if (*sStack.top().cabptr == rootindex) { return true; }
				stackpair topush;
				sStack.push(stackpair{ _GetCellVal(*sStack.top().cabptr) });
				sStack.top().cabptr = sStack.top().cell.affectby.cbegin();
				continue;
			}
			sStack.pop();
			if (sStack.size()) { sStack.top().cabptr++; }
		}
		return false;
	}

	std::vector<cell> UpdateDependentCells(_cell& rootCell) {
		for (auto i : rootCell.affecton) {
			_UpdateCell(WorkSheetData[i]);
			//MessageBoxA(NULL, "Dependent Cell Updated", "Test", MB_OK);
			UpdateDependentCells(WorkSheetData[i]);
		}

		return std::vector<cell>();
	}

	_cell& _GetCellRef(unsigned column, unsigned row) {
		return WorkSheetData[GetIndex(column, row)];
	}

	_cell& _GetCellRef(unsigned index) {
		return WorkSheetData[index];
	}

	_cell _GetCellVal(unsigned index) {
		try {
			return WorkSheetData.at(index);
		}
		catch (std::out_of_range) {
			_cell toreturn;
			toreturn.type = _cell::type::null;
			return toreturn;
		}
	}

	_cell _GetCellVal(unsigned column,unsigned row) {
		try {
			return WorkSheetData.at(GetIndex(column,row));
		}
		catch (std::out_of_range) {
			_cell toreturn;
			toreturn.type = _cell::type::null;
			return toreturn;
		}
	}

	void _UpdateCell(_cell& ToUpdate) {
		Value Result = evalRPN(ToUpdate.RPN, DrefIndexValue);
		switch (Result.type())
		{
		case Value::type::Number: {
			ToUpdate.type = _cell::type::rl;
			ToUpdate.rl = Result.num();
			ToUpdate.tt = std::to_wstring(Result.num());
			break;
		}
		case Value::type::Text: {
			ToUpdate.type = _cell::type::tt;
			ToUpdate.tt = Result.text();
			break;
		}
		default: {
			ToUpdate.type = _cell::type::err;
			ToUpdate.tt = L"*ERROR*";
			break;
		}
		}
	}

	void _UpdateCell(unsigned index) {
		_cell& ToUpdate(WorkSheetData[index]);
		Value Result = evalRPN(ToUpdate.RPN, DrefIndexValue);
		switch (Result.type())
		{
		case Value::type::Number: {
			ToUpdate.type = _cell::type::rl;
			ToUpdate.rl = Result.num();
			ToUpdate.tt = std::to_wstring(Result.num());
			break;
		}
		case Value::type::Text: {
			ToUpdate.type = _cell::type::tt;
			ToUpdate.tt = Result.text();
			break;
		}
		default: {
			ToUpdate.type = _cell::type::err;
			ToUpdate.tt = L"*ERROR*";
			break;
		}
		}
		return;
	}


	static Value DrefIndexValue(const Value& value) {
		_cell Indexed(CurrentWorksheet->_GetCellVal(value.ind().column, value.ind().row));

		switch (Indexed.type)
		{
		case _cell::type::tt: {
			return Value(Indexed.tt);
		}
		case _cell::type::rl: {
			return Value(Indexed.rl);
		}
		default: {
			return Value();
		}
		}
	};

public:
	unsigned SetCell(const wchar_t* input, unsigned column, unsigned row) {
		//check if arg is empty

		unsigned thisindex = GetIndex(column, row);
		_cell ToSet(_GetCellRef(thisindex));

		ToSet.ExactInput = input;

		//erase dependencies between the cell being edited and ones it refrences
		for (auto i : ToSet.affectby) {
			//MessageBoxA(NULL, "AffectBy Removed!", "hallo", MB_OK);
			_cell& precedor(_GetCellRef(i));
			precedor.affecton.erase(thisindex);
		}
		ToSet.affectby.clear();
		ToSet.RPN.clear();

		if (*input == L'\0') /*if arg is an empty string*/ {
			ToSet.type = _cell::type::null;
			ToSet.tt.clear();
		}
		else if (input[0] == L'=' && input[1] != L'\0') /*if arg is a formula*/ {
			//formula handling
			ToSet.RPN = evalexpr(input + 1);

			//redecide dependencies
			for (auto i : ToSet.RPN) {
				if (i.type == Token::type::value && i.lit.type() == Value::type::Index) {
					_cell& indexedcell(_GetCellRef(i.lit.ind().column, i.lit.ind().row));

					if (SearchSelfRef(thisindex, indexedcell)) {
						//MessageBoxA(NULL, "Formula refrences itself", "Self reference", MB_OK);
						return ERR_CIRCULAR_REF;
					}

					indexedcell.affecton.insert(thisindex);
					ToSet.affectby.insert(GetIndex(i.lit.ind().column, i.lit.ind().row));
				}
			}

			_UpdateCell(ToSet);
		}
		else {
			//check if arg is text or number
			wchar_t* lastptr;
			double number = std::wcstod(input, &lastptr); //attempt parse number
			/*store data accordingly*/
			if (lastptr == input) {  //if lastptr points to beginning it means nothing was read(there is no number)
				ToSet.tt = input;
				ToSet.type = _cell::type::tt;
			}
			else { //a number was read but we still have to check if it is followed by anything other than space
				while (1) {
					if (*lastptr != L' ') { //the input is text
						if (*lastptr == L'\0') { //the input is a number
							ToSet.rl = number;
							ToSet.tt = std::to_wstring(number);
							ToSet.type = _cell::type::rl;
							break;
						}
						else {
							ToSet.tt = input;
							ToSet.type = _cell::type::tt;
							break;
						}
					}
					lastptr++;
				}

			}

		}

		WorkSheetData[thisindex] = ToSet;
		UpdateDependentCells(ToSet);
		return 0;
	}
	const cell GetCell(unsigned column, unsigned row) {
		unsigned index = GetIndex(column, row);
		if (WorkSheetData.count(index)) {
			_cell& ToExtractfrom = WorkSheetData.at(index);
			cell ToReturn;
			ToReturn.type = (enum cell::type)ToExtractfrom.type;
			ToReturn.rl = ToExtractfrom.rl;
			ToReturn.tt = ToExtractfrom.tt;
			ToReturn.ExactInput = ToExtractfrom.ExactInput;
			return ToReturn;
		}
		else {
			cell ToReturn;
			ToReturn.type = cell::type::null;
			return ToReturn;
		}
	}

};
