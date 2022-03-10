#include <stdexcept>
#define INC_FORMULA_FUNCS
#include "Storage.h"

#include <queue>

bool WorkSheet::SearchSelfRef(unsigned selfindex, unsigned rootindex) {
	std::stack<unsigned> CellStack; //Depth first Search
	CellStack.push(rootindex);
	while (CellStack.size()) {
		unsigned CurrentCell = CellStack.top();
		
		if (CurrentCell == selfindex)
			return true;
		
		CellStack.pop();
		for (unsigned DependentCell : _GetCellRef(CurrentCell).affectby) {
			CellStack.push(DependentCell);
		}
	}
	return false;
}

void WorkSheet::UpdateDependentCells(_cell& rootCell) {
	for (auto i : rootCell.affecton) {
		_UpdateCell(WorkSheetData[i]);
		UpdateDependentCells(WorkSheetData[i]);
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


unsigned WorkSheet::SetCell(const wchar_t* input, unsigned column, unsigned row) {
	unsigned SetIndex = GetIndex(column, row);
	_cell ToSet(_GetCellRef(SetIndex));

	ToSet.ExactInput = input;

	//erase dependencies between the cell being edited and ones it refrences
	for (auto i : ToSet.affectby) {
		_cell& precedor(_GetCellRef(i));
		precedor.affecton.erase(SetIndex);
		if (precedor.type == _cell::type::null && precedor.affecton.empty()) {
			WorkSheetData.erase(i);
		}
	}
	ToSet.affectby.clear();
	ToSet.RPN.clear();

	if (*input == L'\0') /*if arg is an empty string*/ {
		ToSet.type = _cell::type::null;
		ToSet.tt.clear();
		if (ToSet.affecton.empty()) {
			WorkSheetData.erase(SetIndex);
		}
	}
	else if (input[0] == L'=' && input[1] != L'\0') /*if arg is a formula*/ {
		//formula handling
		ToSet.RPN = genIR(input + 1);
		std::vector<Index> Indices = extractIndicesFromIR(ToSet.RPN);
		//redecide dependencies
		for (auto i : Indices) {
			unsigned CurrentIndex = GetIndex(i.column, i.row);

			if (SearchSelfRef(SetIndex, CurrentIndex)) {
				return SET_CELL_ERR_CIRCULAR_REF;
			}

			_GetCellRef(i.column, i.row).affecton.insert(SetIndex);
			ToSet.affectby.insert(CurrentIndex);

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

	WorkSheetData[SetIndex] = ToSet;
	UpdateDependentCells(ToSet);
	return SET_CELL_OK;
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

std::vector<WorkSheet::CellIndexPair> WorkSheet::SerializeCells(void) {
	std::vector<CellIndexPair> toret;
	for (const auto& i : WorkSheetData) {
		if (i.second.ExactInput.size() > 0u) { //to avoid serialising empty cells that affect other cells or werent deleted from the unordered_map
			toret.push_back(
				CellIndexPair{ (i.first & 0xffff000u) >> 16 ,i.first & 0xffffu,i.second.ExactInput }
			);
		}
	}
	return toret;
}

void WorkSheet::DeSerializeCells(const std::vector<WorkSheet::CellIndexPair>& cip) {
	WorkSheetData.clear();
	for (const auto& i : cip) {
		SetCell(i.cellitem.data(), i.column, i.row);
	}
}

