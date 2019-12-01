/*	ArgvParser.cpp: Class with common static methods and properties.
	Copyright (c) 2019 Martin Borik <mborik@users.sourceforge.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
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
} argv_config;
//-----------------------------------------------------------------------------
void IntroMessage();
bool ParseOptions(int *argc, char *(*argv[]));
//-----------------------------------------------------------------------------
#endif
