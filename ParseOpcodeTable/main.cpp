#include <iostream>
#include <fstream>
#include <strstream>
#include <sstream>
#include <windows.h>
#include <string>
#include <vector>
#include "string_conv.h"

using namespace std;

enum ADDRMODE { IMP, ACC, IMM, ADR, ABS, IND, REL, ABX, ABY, ZPG, ZPX, ZPY, INX, INY, ERR, NUM_ADDR_MODES };
unsigned char AddrBytes[NUM_ADDR_MODES] = {1, 1, 2, 3, 3, 3, 2, 3, 3, 2, 2, 2, 2, 2, 1};

char *addr_mode_names[]= { "IMP", "ACC", "IMM", "ADR", "ABS", "IND", "REL", "ABX", "ABY", "ZPG", "ZPX", "ZPY", "INX", "INY", "ERR"};

enum ADDRMODE TraceAddrMode[256] =
{
	IMP, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	ADR, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMP, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ADR, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMP, INX, ERR, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, IND, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
	IMM, INX, IMM, INX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS, REL, INY, ERR, INY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX
};


#define	BP_NA (0)
#define	BP_RD (1)
#define	BP_WR (2)
#define	BP_RW (BP_RD | BP_WR)

unsigned char TraceIO[256] =
{
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_NA, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_RD, BP_WR, BP_RD, BP_WR, BP_WR, BP_WR, BP_WR, BP_WR, BP_NA, BP_RD, BP_NA, BP_RD, BP_WR, BP_WR, BP_WR, BP_WR,
	BP_NA, BP_WR, BP_NA, BP_RD, BP_WR, BP_WR, BP_WR, BP_WR, BP_NA, BP_WR, BP_NA, BP_RD, BP_RD, BP_WR, BP_RD, BP_RD,
	BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD,
	BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RD, BP_RD,
	BP_RD, BP_RD, BP_RD, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_RD, BP_RD, BP_RD, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RD, BP_RD, BP_RD, BP_RW, BP_RW,
	BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW, BP_NA, BP_RD, BP_NA, BP_RW, BP_RD, BP_RD, BP_RW, BP_RW
};

// from atari800 emulator
static const int cycles[256] =
{
	7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,		/* 0x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,		/* 1x */
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,		/* 2x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,		/* 3x */

	6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,		/* 4x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,		/* 5x */
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,		/* 6x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,		/* 7x */

	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,		/* 8x */
	2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,		/* 9x */
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,		/* Ax */
	2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,		/* Bx */

	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,		/* Cx */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,		/* Dx */
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,		/* Ex */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7		/* Fx */
};



void ReadFile(vector <string> &lines, char *filename)
{
	fstream f;
	f.open( filename, ios::in);
	if ( f.fail())
		return;

	string line;
	while( getline( f, line)) 
	{
		while (line.size()<62)
			line.push_back(' ');
		lines.push_back(line);
	}
}

std::string ReplaceString(std::string subject, const std::string& search,
	const std::string& replace) {
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return subject;
}

int main(void)
{
	vector <string> lines;
	ReadFile(lines,"6502-transitions.txt");

	bool started=false;

	ofstream out_file;
	out_file.open ("output.txt");
	out_file << "struct OpcodeDef opcode_def[256]={\r\n";

	for (size_t i=0;i<lines.size();++i)
	{
		stringstream out_line;
		string line=lines[i];
		if (line.find("00 BRK")!=string::npos)
			started=true;
		if (started && line[0]==' ' && line[1]==' ' && line.find("bytes:")!=string::npos)
		{
			out_line << "  {";

			string opcode_string;
			opcode_string=line[2];
			opcode_string+=line[3];
			out_line << "0x" << opcode_string << ",";
			byte opcode=String2HexValue<int>(opcode_string);

			bool illegal=false;
			string name;
			if (line[5]=='*')
			{
				name+=line[6];
				name+=line[7];
				name+=line[8];
				illegal=true;
			}
			else
			{
				name+=line[5];
				name+=line[6];
				name+=line[7];
			}
			out_line << "\""<<name<<"\",";

			string len;
			len+=line[30];
			if (len==" ")
				len="1";

			byte l=String2Value<int>(len);
			byte table_bytes=AddrBytes [TraceAddrMode[opcode]];
			if (l!= table_bytes)
			{
				printf("%d (0x%x) inconsistent number of bytes\n",opcode,opcode);
			}
			out_line << Value2String<int>(table_bytes) << ",";
			
			string cycles_s;
			cycles_s+=line[40];
			if (cycles_s==" ")
				cycles_s="2";

			byte c=String2Value<int>(cycles_s);
			byte table_cycles=cycles[opcode];
			if (c!= table_cycles)
			{
				printf("%d (0x%x) inconsistent number of cycles\n",opcode,opcode);
			}
			out_line << Value2String<int>(table_cycles) << ",";

			string input;
			input+=line[42];
			input+=line[43];
			input+=line[44];
			input+=line[45];
			input+=line[46];
			if (input.find("A")!=string::npos)
				out_line<<"D_A|";
			if (input.find("X")!=string::npos)
				out_line<<"D_X|";
			if (input.find("Y")!=string::npos)
				out_line<<"D_Y|";
			if (input.find("S")!=string::npos)
				out_line<<"D_S|";
			if (input.find("P")!=string::npos)
				out_line<<"D_P|";
			out_line << "D_NONE,";
			
			string output;
			output+=line[49];
			output+=line[50];
			output+=line[51];
			output+=line[52];
			output+=line[53];
			if (output.find("A")!=string::npos)
				out_line<<"D_A|";
			if (output.find("X")!=string::npos)
				out_line<<"D_X|";
			if (output.find("Y")!=string::npos)
				out_line<<"D_Y|";
			if (output.find("S")!=string::npos)
				out_line<<"D_S|";
			if (output.find("P")!=string::npos)
				out_line<<"D_P|";
			out_line << "D_NONE,";

			char mem_read=line[55];
			char mem_write=line[56];
			if (mem_read=='R')
			{
				out_line<<"MEM_R|";
			}
			if ( (mem_read=='R') != ((TraceIO[opcode]&BP_RD)==BP_RD))
			{
				printf("%d (0x%x) inconsistent mem read\n",opcode,opcode);
			}
			if (mem_write=='W')
			{
				out_line<<"MEM_W";
			}
			if ( (mem_write=='W') != ((TraceIO[opcode]&BP_WR)==BP_WR))
			{
				printf("%d (0x%x) inconsistent mem write\n",opcode,opcode);
			}
			if (mem_read!='R' && mem_write!='W')
				out_line<<"MEM_NONE";
			out_line<<',';

			string addressing;
			addressing+=line[58];
			addressing+=line[59];
			addressing+=line[60];
			addressing+=line[51];

			byte addr_type=TraceAddrMode[opcode];
			out_line << addr_mode_names[addr_type] << ",";

			if (illegal)
				out_line << "ILLEGAL";
			else
				out_line << "LEGAL";

			out_line << "}," << endl;

			std::string to_format;
			std::ostringstream os;
			out_line>>os.rdbuf();
			to_format=os.str();

			to_format=ReplaceString(to_format,"|D_NONE,",",");
			to_format=ReplaceString(to_format,"|,",",");

			out_file << to_format.c_str();
		}
	}
	out_file << "}" << endl;

} 