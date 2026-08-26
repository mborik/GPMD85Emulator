/*	ArgvParser.cpp: Class with common static methods and properties.
	Copyright (c) 2019-2026 Martin Borik <martin@borik.net>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom
	the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
	OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
	OR OTHER DEALINGS IN THE SOFTWARE.
*/
//-----------------------------------------------------------------------------
#ifndef ARGVPARSER_H_
#define ARGVPARSER_H_
//-----------------------------------------------------------------------------
enum TCmdLineSwitchType { VAR_BOOL, VAR_BOOL_NEG, VAR_STRING, VAR_INT };

typedef struct TCmdLineSwitch {
	const char *short_switch;
	const char *long_switch;
	TCmdLineSwitchType var_type;
	void *variable;
	const char *description;
	const char *par_descr;
	bool mandatory;
	int order;
} TCmdLineSwitch;

typedef struct TCmdLineSwitches {
	TCmdLineSwitch *switches;
	unsigned count;
} TCmdLineSwitches;
//-----------------------------------------------------------------------------
extern struct TCmdLineArguments {
	bool  any_related;

	bool  help;
	bool  version;
	bool  overcfg;
	char *machine;
	int   model;
	bool  rmm;
	char *megarom;
	int   scaler;
	int   border;
	int   halfpass;
	int   color;
	int   volume;
	bool  mif85;
	bool  pmd32;
	char *p32_drvA;
	bool  p32_drvA_wp;
	char *p32_drvB;
	bool  p32_drvB_wp;
	char *p32_drvC;
	bool  p32_drvC_wp;
	char *p32_drvD;
	bool  p32_drvD_wp;
	char *tape;
	bool  flashload;
	char *snap;
	char *memblock;
	int   memstart;
	bool  softrender;
} argv_config;
//-----------------------------------------------------------------------------
void IntroMessage();
bool ParseOptions(int *argc, char *(*argv[]));
//-----------------------------------------------------------------------------
#endif
