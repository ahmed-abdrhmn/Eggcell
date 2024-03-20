#pragma once
#include <unordered_set>
#include <unordered_map>
#include "Formula.h"
#include <stack>
#include <string>

#define SET_CELL_OK 0
#define SET_CELL_ERR_CIRCULAR_REF 1


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

	struct CellIndexPair {
		unsigned column;
		unsigned row;
		std::wstring cellitem;
	};

	struct MergedCell {
		unsigned indX1;
		unsigned indX2;
		unsigned indY1;
		unsigned indY2;
		unsigned posX;
		unsigned posY;
	};

	std::vector<MergedCell> MergedCells;

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

	bool SearchSelfRef(unsigned selfindex, unsigned rootindex);
	void UpdateDependentCells(_cell& rootCell);
	_cell& _GetCellRef(unsigned column, unsigned row);
	_cell& _GetCellRef(unsigned index);
	_cell _GetCellVal(unsigned index);
	_cell _GetCellVal(unsigned column, unsigned row);
	static Value DrefIndexValue(const Value& value);
	void _UpdateCell(_cell& ToUpdate);
	void _UpdateCell(unsigned index);
public:
	unsigned SetCell(const wchar_t* input, unsigned column, unsigned row);
	const cell GetCell(unsigned column, unsigned row);
	std::vector<CellIndexPair> SerializeCells(void);
	void DeSerializeCells(const std::vector<CellIndexPair>& cip);
};
