// 8086Model.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//Reference page ~160 https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <map>
#include <cinttypes>

enum class Register : uint8_t
{
	AL,
	CL,
	DL,
	BL,
	AH,
	CH,
	DH,
	BH,
	AX,
	CX,
	DX,
	BX,
	SP,
	BP,
	SI,
	DI
};

class ByteStream
{
	ByteStream()
		: data(nullptr)
		, Offset(0)
		, Length(0)
	{}

public:
	bool LoadFromFilePath(const std::string& InFilePath)
	{
		std::ifstream StreamFile(InFilePath, std::ios::out | std::ios::binary);

		if (StreamFile.is_open() == false)
		{
			std::cout << "Cannot open file!" << std::endl;
			return false;
		}

		StreamFile.seekg(0, std::ios::end);
		Length = file.tellg();
		StreamFile.seekg(0, std::ios::beg);

		data = new char[Length];

		std::cout << "Size of bytes array: " << Length << std::endl;

		file.read(data, Length);

		file.close();
	}

	char* GetDataAtOffset() const 
	{
		return data[Offset];
	}

	uint32_t GetLength() const 
	{
		return Length;
	}

	uint32_t GetOffset() const
	{
		return Offset;
	}

	void StepForwardOffset(uint32_t Step)
	{
		Offset += Step;
	}

	void SetOffset(uint32_t InOffset)
	{
		Offset = InOffset;
	}

	bool DoesStreamHaveSpace(uint32_t BytesOfSpace)
	{
		return Length > (Offset + BytesOfSpace);
	}

private:
	char* data;
	uint32_t Offset;
	uint32_t Length;
};

struct Instruction
{
	Instruction()
		: OperationName("")
		, OpCodeByteMask()
		, DByteMask(0)
		, WByteMask(0)
		, SByteMask(0)
		, VByteMask(0)
		, RegByteMask(0)
		, HasModRMFields(0)
		, HasWContingentData(0)
		, AdditionalInstructionSize(0)
	{}

	Instruction(const std::string& InOperationName, const uint8_t InOpCodeByteMask)
		: OperationName(InOperationName)
		, OpCodeByteMask(InOpCodeByteMask)
		, DByteMask(0)
		, WByteMask(0)
		, SByteMask(0)
		, VByteMask(0)
		, RegByteMask(0)
		, HasModRMFields(0)
		, HasWContingentData(0)
		, AdditionalInstructionSize(0)
	{}

	Instruction(const std::string& InOperationName, const uint8_t InOpCodeByteMask, const uint8_t InDByteMask = 0, const uint8_t InWByteMask = 0,
		const uint8_t InSByteMask = 0, const uint8_t InVByteMask = 0, const uint8_t InRMByteMask = 0, const bool InHasModRMFields = false,
		const bool InHasWContingentData = false, const uint8_t InAdditionalInstructionSize = 0)
		: OperationName(InOperationName)
		, OpCodeByteMask(InOpCodeByteMask)
		, DByteMask(InDByteMask)
		, WByteMask(InWByteMask)
		, SByteMask(InSByteMask)
		, VByteMask(InVByteMask)
		, HasModRMFields(InHasModRMFields)
		, HasWContingentData(InHasWContingentData)
		, AdditionalInstructionSize(InAdditionalInstructionSize)
	{}

	void DecodeInstruction(ByteStream* DataStream)
	{
		char* Data = DataStream->GetDataAtOffset();

		if ((Data[0] & OpCodeByteMask) != OpCodeByteMask)
		{
			return 0;
		}

		std::string Source;
		std::string Dest;

		uint8_t StreamStepSize = 0;

		bool D = Data[0] & DByteMask;
		bool W = Data[0] & WByteMask;

		if (DataStream->DoesStreamHaveSpace(1))
		{
			uint8_t Mod = 0;

			if (HasModRMFields)
			{
				uint8_t Mod = Data[1] & 0b11000000;
				
				if (Mod > 0)
				{
					uint8_t NewMod = Mod >> 6;
					
					if (NewMod << 6 == Mod)
					{
						Mod = NewMod;
					}
					else
					{
						return;
					}

					uint8_t RM = Data[1] & 0b00000111;

					switch (Mod)
					{
					case 0:
						if (RM == 6)
						{
							Source = "Direct Address";
						}
						else
						{
							Source = RegisterPairsArray[RM];
						}
						break;
					case 1:
						StreamStepSize += 1;
						Source = RegisterPairsArray[RM] + " + " std::to_string(Data[2]);
						break;
					case 2:
						StreamStepSize += 2;
						uint16_t* TwoBytePtr = (uint16_t*)Data[2];
						uint16_t TwoByteValue = *TwoBytePtr;
						Source = RegisterPairsArray[RM] + " + " std::to_string(TwoByteValue);
						break;
					case 3:
						Source = RegisterArray[RM + (W * 8)];
						break;
					}
				}
			}
		}
	}

	static std::array<std::string, 16> RegisterArray{ "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	static std::array<std::string, 8> RegisterPairsArray{ "(BX) + (SI)", "(BX) + (DI)", "(BP) + (SI)", "(BP) + (DI)", "(SI)", "(DI)", "(BP)", "(BX)" };

	std::string OperationName;
	uint8_t OpCodeByteMask;
	uint8_t DByteMask;
	uint8_t WByteMask;
	uint8_t SByteMask;
	uint8_t VByteMask;
	uint16_t RegByteMask;
	uint8_t HasModRMFields : 1;
	uint8_t HasWContingentData : 1;
	uint8_t AdditionalInstructionSize : 2;
};

class InstructionSet
{
	InstructionSet()
	{
		Instructions =
		{
			/*Register/Memory to/from register*/	
			Instruction("mov", 0b10001000, 0b00000010, 0b00000001, 0, 0, 0b11000000, 0b0000000000111000, 0b00000111),
			/*Immediate to register*/				
			Instruction("mov", 0b10110000, 0, 0b00001000, 0, 0, 0b11000000, 0b0000011100000000, 0, true);
		}
	}


	const Instruction* FindInstruction(uint8_t* data)
	{
		uint8_t instructionByte = *data;

		for (size_type i = Instructions.size(); i != 0; --i)
		{
			if ((instructionByte & Instructions[i - 1].OpCodeByteMask) == Instructions[i - 1].OpCodeByteMask)
			{
				return &Instructions[i - 1];
			}
		}

		return nullptr;
	}

	std::array<Instruction, 2> Instructions;
};

void DecodeInstructionAnd(uint8_t* data, int offset)
{
	uint8_t w = data[offset] & 1;
	uint8_t d = (data[offset] & (1 << 1)) >> 1;
	uint8_t cmdSelector = ~3;
	uint8_t opCode = data[offset] & cmdSelector;

	uint8_t mod = (data[offset + 1] & (3 << 6)) >> 6;
	uint8_t reg = (data[offset + 1] & (7 << 3)) >> 3;
	uint8_t rm = data[offset + 1] & 7;

	if (w)
	{
		reg = reg + 8;
		rm = rm + 8;
	}

	std::array<std::string, 16> registerArray{ "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	//MOV code is 100010dw
	std::map<uint8_t, std::string> opCodeMap = { {136, "MOV"} };

	std::string opCodeString;

	if (opCodeMap.count(opCode) > 0)
	{
		opCodeString = opCodeMap.at(opCode);
	}
	else
	{
		opCodeString = "Invalid Opcode";
	}

	std::string regString;

	if (reg < 16)
	{
		regString = registerArray[reg];
	}
	else
	{
		regString = "Invalid Reg";
	}

	std::string rmString;

	if (rm < 16)
	{
		rmString = registerArray[rm];
	}
	else
	{
		rmString = "Invalid RM";
	}

	if (d)
		std::cout << opCodeString << " " << regString << ", " << rmString << std::endl;
	else
		std::cout << opCodeString << " " << rmString << ", " << regString << std::endl;
}


int main()
{
	std::cout << "Please enter the full path for the file you want to open, enter '.' to use default:\n";

	std::string FilePath;
	std::cin >> FilePath;

	if (FilePath == ".")
	{
		FilePath = "C:/VSProjects/listing_0037_single_register_mov";
	}

	ByteStream DataStream();

	if (DataStream.LoadFromFilePath(FilePath))
	{

	}


	for (int i = 0; i < fileSize - 1; i += 2)
	{
		InterpretBytePair((uint8_t*)data, i);
	}

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
