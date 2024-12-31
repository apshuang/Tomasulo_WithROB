#include "../Headers/ReorderBuffer.h"
#include "../Headers/CommonDataBus.h"

void ReorderBufferLine::Reset() {
	busy = 0;
	inst = "";
	compareType = "";
	compareLeft = "";
	compareRight = "";
	branchLabel = "";
	targetReg = "";
	state = FREE;
	valueString = "";
}

string ReorderBufferLine::GetName() {
	string name = "ROB";
	name += (char)(id + 1 + '0');
	return name;
}

string ReorderBufferLine::GetInstruction() {
	return inst;
}

string ReorderBufferLine::getValue() {
	return valueString;
}

string ReorderBufferLine::GetTargetReg() {
	return targetReg;
}

ReorderBufferLine::ReorderBufferLine(int ID, FloatRegisters* floatRegs, IntegerRegisters* integerRegs) {
	Reset();
	id = ID;
	floatRegisters = floatRegs;
	integerRegisters = integerRegs;
}

int ReorderBufferLine::IsBusy() {
	return busy;
}

bool ReorderBufferLine::IsBranchInstruction() {
	if (compareType == "") return false;
	return true;
}

int ReorderBufferLine::isBranch() {
	if (!checkReady({ compareLeft, compareRight })) return -1;
	if (compareType == "bne") {
		if (compareLeft != compareRight) {
			return 1;
		}
		else return 0;
	}
	else {
		cout << "Unknown branch type" << endl;
		abort();
	}
}

string ReorderBufferLine::ROBIssue(string instruction, string unit, string target) {
	Reset();
	busy = 1;
	inst = instruction;
	valueString = unit;
	targetReg = target;
	state = ISSUE;
	return GetName();
}

string ReorderBufferLine::ROBIssueBranch(string instruction, string op, string left, string right, string label) {
	Reset();
	busy = 1;
	state = ISSUE;
	compareType = op;
	compareLeft = left;
	compareRight = right;
	branchLabel = label;
	inst = instruction;
	return GetName();
}

void ReorderBufferLine::ReceiveData(string unitName, string value) {
	if (valueString == unitName) {
		valueString = value;
	}
	if (compareLeft == unitName) {
		compareLeft = value;
	}
	if (compareRight == unitName) {
		compareRight = value;
	}
}

void ReorderBufferLine::ReceiveExecuteSignal(string unitName) {
	if (valueString == unitName) state = EXEC;
}

void ReorderBufferLine::Commit(int cycle) {
	// ����ROB��˵��commitֻ��Ӱ�쵽register�ľ�ֵ����������Ӱ��RS������ִ�е��κζ�������ΪRSֻ������ģ�飬��������ROB��
	// ��register�ľ�ֵʵ������һ���ȶ�ֵ��������������˵Ļ����ͽ�����ֵ���ó�����Ϊregister��ֵ�������������ں�����ROB��

	string commitMessage = "";
	if (branchLabel == "") {
		char label = targetReg[0];
		label = tolower(label);
		if (label == 'f' || label == 'r') {
			// ����Ĵ���
			int index = stoi(targetReg.substr(1));
			floatRegisters->SetStableValue(valueString, index);
		}
		else if (label == 'x') {
			int index = stoi(targetReg.substr(1));
			integerRegisters->SetStableValue(valueString, index);
		}
	}
	else {
		int branchResult = isBranch();
		if (branchResult == 0) {
			commitMessage = "not jump:" + inst;
		}
		else if (branchResult == 1) {
			commitMessage = "jump:" + inst;
		}
		else {
			cout << "Cannot commit when any compare item is not ready." << endl;
			abort();
		}
	}
	state = COMMIT;
	CommonDataBus::TransmissCommitSignal(GetName(), commitMessage);  // ������費�Ǵ�ֵ�����Ǵ�commit��ʱ��
}

int ReorderBufferLine::GetState() {
	return state;
}

void ReorderBufferLine::SetState(int newState) {
	state = newState;
}


ReorderBuffer::ReorderBuffer(FloatRegisters* floatRegs, IntegerRegisters* integerRegs) {
	head = 0;
	tail = 0;
	size = 0;
	for (int i = 0; i < ENTRYNUM; i++) {
		entry[i] = new ReorderBufferLine(i, floatRegs, integerRegs);
	}
}

int ReorderBuffer::IsFree() {
	if (size == ENTRYNUM) {
		return -1; //��������
	}
	return tail;
}

string ReorderBuffer::ROBIssue(int idx, string instruction, string unit, string target) {
	string name = entry[idx]->ROBIssue(instruction, unit, target);
	tail = (tail + 1) % ENTRYNUM;
	size++;
	return name;
}
string ReorderBuffer::ROBIssueBranch(int idx, string instruction, string op, string left, string right, string label) {
	string name = entry[idx]->ROBIssueBranch(instruction, op, left, right, label);
	tail = (tail + 1) % ENTRYNUM;
	size++;
	return name;
}

void ReorderBuffer::ReceiveData(string unitName, string value) {
	for (int i = 0; i < ENTRYNUM; i++) {
		entry[i]->ReceiveData(unitName, value);
	}
}

void ReorderBuffer::ReceiveExecuteSignal(string unitName) {
	for (int i = 0; i < ENTRYNUM; i++) {
		entry[i]->ReceiveExecuteSignal(unitName);
	}
}

void ReorderBuffer::BackTrack() {
	// ����branchָ���жϴ���������Ҫ����
	// �������ROB�����������صĹ��ܵ�Ԫ����register�ָ�Ϊ�ȶ�ֵ

	int index = head;
	int nowSize = size;
	for (int i = 0; i < nowSize; i++) {
		// ���ڻ���ISSUE��EXEC�ģ��������
		if (entry[index]->GetState() == ISSUE || entry[index]->GetState() == EXEC) {
			CommonDataBus::TransmissClearSignal(entry[index]->getValue());  // ��ʱ���entry��value����һ������ֵ
		}
		
		// ����ISSUE��EXEC��WRITE�ģ���Ҫ�����ǵ�targetRegister�ָ����ȶ�ֵ��������������������
		CommonDataBus::TransmissClearSignal(entry[index]->GetTargetReg());
		entry[index]->Reset();
		head++;
		size--;
		head %= ENTRYNUM;
		index++;
		index %= ENTRYNUM;
	}
}

void ReorderBuffer::Tick(int cycle) {
	// ReorderBuffer����ֻ�������ؽ���write��commit̬�����ã�����issue��exec̬�����ö��Ǳ����ģ�
	int index = head;
	int nowSize = size;
	for (int i = 0; i < nowSize; i++) {
		// ���жϵ�ǰ���δCOMMIT��ָ���Ƿ�Ϊbranchָ��
		if (head == index) {
			if (entry[index]->IsBranchInstruction()) {
				int branchResult = entry[index]->isBranch();
				if (branchResult == -1) {
					// ��δ׼���ã����Ժ�����һ�У���commit������head����
					continue;
				}
				else if (branchResult == 0) {
					// ��ת��������Ҫ����������е�ROBLine
					entry[index]->Commit(cycle);  // �Ȱѵ�ǰָ���ύ��
					head++;
					size--;
					head %= ENTRYNUM;
					BackTrack();
				}
				else if (branchResult == 1) {
					// ��ת��ȷ������commit��
					entry[index]->Commit(cycle);
					head++;
					size--;
					head %= ENTRYNUM;
				}
			}
		}
		
		if (entry[index]->GetState() == WRITE) {
			// ֻҪ��write�����������δCOMMIT��ָ��Ϳ����ύ��
			if (head == index) {
				entry[index]->Commit(cycle);
				head++;
				size--;
				head %= ENTRYNUM;
			}
		}

		if (entry[index]->GetState() == EXEC) {
			if (checkReady({ entry[index]->getValue() })) {
				entry[index]->SetState(WRITE);
			}
		}
		index++;
		index %= ENTRYNUM;
	}
}


void ReorderBuffer::InsertOutput(vector<string>& table) {
	const int tableWidth = 80;
	const int colWidth[] = { 10, 8, 20, 70}; // ���п��

	// ��ӡ��ͷ
	printHeader("Reorder Buffer", tableWidth);

	// ��ӡ�б���
	cout << "|"
		<< centerString("Entry", colWidth[0]) << "|"
		<< centerString("State", colWidth[1]) << "|"
		<< centerString("Instruction", colWidth[2]) << "|"
		<< centerString("Value", colWidth[3]) << "|\n";
	cout << string(tableWidth, '-') << "\n";

	// ��ӡROB����
	for (size_t i = 0; i < ENTRYNUM; ++i) {
		cout << "|"
			<< centerString("ROB" + to_string(i + 1), colWidth[0]) << "|"
			<< centerString(StateOutput[entry[i]->GetState()], colWidth[1]) << "|"
			<< centerString(entry[i]->GetInstruction(), colWidth[2]) << "|"
			<< centerString(entry[i]->getValue(), colWidth[3]) << "|\n";
	}

	// ��ӡ��β��
	cout << string(tableWidth, '-') << "\n";
}


bool ReorderBuffer::isAllFree() {
	return size == 0;
}