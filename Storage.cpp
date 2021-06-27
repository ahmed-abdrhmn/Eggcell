#include <stdexcept>
#define INC_FORMULA_FUNCS
#include "Storage.h"

bool WorkSheet::SearchSelfRef(unsigned rootindex, const _cell& root) {
	struct stackpair { _cell cell; std::unordered_set<unsigned>::const_iterator cabptr; };
	std::stack<stackpair> sStack;

	sStack.push(stackpair{ root });

	sStack.top().cabptr = sStack.top().cell.affectby.cbegin();
	while (true) {
		if (sStack.top().cabptr != sStack.top().cell.affectby.cend()) {
			if (*sStack.top().cabptr == rootindex) { return true; }
			stackpair topush;
			sStack.push(stackpair{ _GetCellVal(*sStack.top().cabptr) });
			sStack.top().cabptr = sStack.top().cell.affectby.cbegin();
		}
		else {
			sStack.pop();

			if (sStack.size()) {
				sStack.top().cabptr++;
			}
			else {
				break;
			}
		}
	}
	return false;
}

void WorkSheet::UpdateDependentCells(_cell& rootCell, std::unordered_set<Index>& UpdatedCells) {
	for (auto i : rootCell.affecton) {
		UpdatedCells.insert(Index{ 0x0000ffffu & i ,0xffff0000u & i });
		_UpdateCell(WorkSheetData[i]);
		UpdateDependentCells(WorkSheetData[i], UpdatedCells);
	}
	return;
}

WorkSheet::_cell& WorkSheet::_GetCellRef(unsigned column, unsigned row) {
	return WorkSheetData[GetIndex(column, row)];
}

WorkSheet::_cell& WorkSheet::_GetCellRef(unsigned index) {
	return WorkSheetData[index];
}

WorkSheet::_cell WorkSheet::_GetCellVal(unsigned index) {
	try {
		return WorkSheetData.at(index);
	}
	catch (std::out_of_range) {
		_cell toreturn;
		toreturn.type = _cell::type::null;
		return toreturn;
	}
}

WorkSheet::_cell WorkSheet::_GetCellVal(unsigned column, unsigned row) {
	try {
		return WorkSheetData.at(GetIndex(column, row));
	}
	catch (std::out_of_range) {
		_cell toreturn;
		toreturn.type = _cell::type::null;
		return toreturn;
	}
}


Value WorkSheet::DrefIndexValue(const Value& value) {
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

void WorkSheet::_UpdateCell(_cell& ToUpdate) {
	Value Result = evalIR(ToUpdate.RPN, DrefIndexValue);
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

void WorkSheet::_UpdateCell(unsigned index) {
	_cell& ToUpdate(WorkSheetData[index]);
	Value Result = evalIR(ToUpdate.RPN, DrefIndexValue);
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


WorkSheet::StClInfo WorkSheet::SetCell(const wchar_t* input, unsigned column, unsigned row) {
	unsigned thisindex = GetIndex(column, row);
	_cell ToSet(_GetCellRef(thisindex));

	ToSet.ExactInput = input;

	//erase dependencies between the cell being edited and ones it refrences
	for (auto i : ToSet.affectby) {
		_cell& precedor(_GetCellRef(i));
		precedor.affecton.erase(thisindex);
	}
	ToSet.affectby.clear();
	//delete ToSet.RPN;

	if (*input == L'\0') /*if arg is an empty string*/ {
		ToSet.type = _cell::type::null;
		ToSet.tt.clear();
	}
	else if (input[0] == L'=' && input[1] != L'\0') /*if arg is a formula*/ {
		//formula handling
		ToSet.RPN = evalexpr(input + 1);
		std::vector<Index> Indices = extractIndicesFromIR(ToSet.RPN);
		//redecide dependencies
		for (auto i : Indices) {

			_cell& indexedcell(_GetCellRef(i.column, i.row));

			if (SearchSelfRef(thisindex, indexedcell)) {
				return StClInfo{ SET_CELL_ERR_CIRCULAR_REF };
			}

			indexedcell.affecton.insert(thisindex);
			ToSet.affectby.insert(GetIndex(i.column, i.row));

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
	std::unordered_set<Index> UpdatedCells;
	UpdateDependentCells(ToSet, UpdatedCells);
	return StClInfo{ SET_CELL_OK,UpdatedCells };
}

const WorkSheet::cell WorkSheet::GetCell(unsigned column, unsigned row) {
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

