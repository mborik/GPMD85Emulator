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
#include "ArgvParser.h"
#include "globals.h"
//-----------------------------------------------------------------------------
#define SWPAR(str) str, false, -1
#define NOVAL INT32_MIN
#define TEST_VALUE_RANGE(val, a, b) (val > NOVAL && val < a) || val > b
//-----------------------------------------------------------------------------
bool argv_config_related = false;
struct TCmdLineArguments argv_config = {
	/* any_related - set to true if any related switch was on cmdline input */
	false,

	false,  /* --help */
	false,  /* --version */
	false,  /* --over-cfg */
	NULL,   /* --machine */
	CM_UNKNOWN, /* machine id translated into TComputerModel form */
	false,  /* --rmm */
	NULL,   /* --megarom */
	NOVAL,  /* --scaler */
	NOVAL,  /* --border */
	NOVAL,  /* --halfpass */
	NOVAL,  /* --profile */
	NOVAL,  /* --volume */
	false,  /* --mif85 */
	false,  /* --pmd32 */
	NULL,   /* --drive-a */
	true,   /* --drive-a-write */
	NULL,   /* --drive-b */
	true,   /* --drive-b-write */
	NULL,   /* --drive-c */
	true,   /* --drive-c-write */
	NULL,   /* --drive-d */
	true,   /* --drive-d-write */
	NULL,   /* --tape */
	true,   /* --tape-real */
	NULL,   /* --snap */
	NULL,   /* --memblock */
	NOVAL,  /* --memblock-address */
	false,  /* --soft-render */
};
TCmdLineSwitch switches[] = {
	{ "-h",   "--help", VAR_BOOL, (void *) &argv_config.help,
				"print this help", SWPAR(NULL) },
	{ "-v",   "--version", VAR_BOOL, (void *) &argv_config.version,
				"print version number", SWPAR(NULL) },
	{ "-c",   "--over-cfg", VAR_BOOL, (void *) &argv_config.overcfg,
				"override user's configuration", SWPAR(NULL) },
	{ "-m",   "--machine", VAR_STRING, (void *) &argv_config.machine,
				"select machine", SWPAR("{1, 2, 2A.. C2717}") },
	{ "-r",   "--rmm", VAR_BOOL, (void *) &argv_config.rmm,
				"connect ROM module", SWPAR(NULL) },
	{ "-mrm", "--megarom", VAR_STRING, (void *) &argv_config.megarom,
				"Mega ROM module image", SWPAR("\"filename.mrm\"") },
	{ "-sc",  "--scaler", VAR_INT, (void *) &argv_config.scaler,
				"screen size multiplier", SWPAR("{1..5}") },
	{ "-bd",  "--border", VAR_INT, (void *) &argv_config.border,
				"screen border width", SWPAR("{0..9}") },
	{ "-hp",  "--halfpass", VAR_INT, (void *) &argv_config.halfpass,
				"scanliner (0=NONE, 1-4=HALFPASS, 5=LCD)", SWPAR("{0..5}") },
	{ "-cp",  "--profile", VAR_INT, (void *) &argv_config.color,
				"color profile (0=MONO, 1=STD, 2=RGB, 3=ColorACE)", SWPAR("{0..3}") },
	{ "-vol", "--volume", VAR_INT, (void *) &argv_config.volume,
				"sound volume (0=MUTE)", SWPAR("{0..127}") },
	{ "-mif", "--mif85", VAR_BOOL, (void *) &argv_config.mif85,
				"connect MIF 85 music interface", SWPAR(NULL) },
	{ "-p",   "--pmd32", VAR_BOOL, (void *) &argv_config.pmd32,
				"connect PMD 32 disk interface", SWPAR(NULL) },
	{ "-drA", "--drive-a", VAR_STRING, (void *) &argv_config.p32_drvA,
				"drive A disk image", SWPAR("\"filename.p32\"") },
	{ "-dwA", "--drive-a-write", VAR_BOOL_NEG, (void *) &argv_config.p32_drvA_wp,
				"drive A write enabled", SWPAR(NULL) },
	{ "-drB", "--drive-b", VAR_STRING, (void *) &argv_config.p32_drvA,
				"drive B disk image", SWPAR("\"filename.p32\"") },
	{ "-dwB", "--drive-b-write", VAR_BOOL_NEG, (void *) &argv_config.p32_drvA_wp,
				"drive B write enabled", SWPAR(NULL) },
	{ "-drC", "--drive-c", VAR_STRING, (void *) &argv_config.p32_drvA,
				"drive C disk image", SWPAR("\"filename.p32\"") },
	{ "-dwC", "--drive-c-write", VAR_BOOL_NEG, (void *) &argv_config.p32_drvA_wp,
				"drive C write enabled", SWPAR(NULL) },
	{ "-drD", "--drive-d", VAR_STRING, (void *) &argv_config.p32_drvA,
				"drive D disk image", SWPAR("\"filename.p32\"") },
	{ "-dwD", "--drive-d-write", VAR_BOOL_NEG, (void *) &argv_config.p32_drvA_wp,
				"drive D write enabled", SWPAR(NULL) },
	{ "-t",   "--tape", VAR_STRING, (void *) &argv_config.tape,
				"tape image", SWPAR("\"filename.ptp\"") },
	{ "-trs", "--tape-real", VAR_BOOL_NEG, (void *) &argv_config.flashload,
				"real tape speed", SWPAR(NULL) },
	{ "-s",   "--snap", VAR_STRING, (void *) &argv_config.snap,
				"load snapshot", SWPAR("\"filename.psn\"") },
	{ "-b",   "--memblock", VAR_STRING, (void *) &argv_config.memblock,
				"load memory block", SWPAR("\"filename.bin\"") },
	{ "-ptr", "--memblock-address", VAR_INT, (void *) &argv_config.memstart,
				"load memory block at given address", SWPAR("{WORD}") },
	{ "-soft", "--soft-render", VAR_BOOL, (void *) &argv_config.softrender,
				"use software renderer instead of accelerated", SWPAR(NULL) },
};
TCmdLineSwitches cmdline = { switches, 26 };
//-----------------------------------------------------------------------------
void IntroMessage()
{
	printf("\n= %s v%s ~ Copyright (c) %s ~ %s =", PACKAGE_NAME, VERSION, PACKAGE_YEAR, PACKAGE_URL);
	printf("\n- This program comes with ABSOLUTELY NO WARRANTY. This is free software,");
	printf("\n- and you are welcome to redistribute it under certain conditions.\n\n");
}
//-----------------------------------------------------------------------------
bool ParseOptions(int *argc, char *(*argv[]))
{
	unsigned q;
	int i, unflagged_offset = 0, transcode, errnum;
	char *ptr = NULL, **args = *argv;
	bool processed = false, ret = true, cmp1, cmp2;

	for (i = 1; i < *argc; i++) {
		processed = false;
		for (q = 0; !processed && q < cmdline.count; q++) {
			cmp1 = (cmdline.switches[q].short_switch != NULL) &&
				(strcmp(args[i], cmdline.switches[q].short_switch) == 0);
			cmp2 = (cmdline.switches[q].long_switch != NULL) &&
				(strcmp(args[i], cmdline.switches[q].long_switch) == 0);

			if (cmp1 || cmp2) {
				if (cmp1)
					args[i] = (char *) cmdline.switches[q].long_switch;

				processed = true;
				switch (cmdline.switches[q].var_type) {
					case VAR_BOOL:
						*((bool *) cmdline.switches[q].variable) = true;
						break;

					case VAR_BOOL_NEG:
						*((bool *) cmdline.switches[q].variable) = false;
						break;

					case VAR_STRING:
						if (i > *argc - 2) {
							warning("Arguments", "Parameter parsing error: %s needs argument %s", args[i],
								(cmdline.switches[q].par_descr == NULL ? "" : cmdline.switches[q].par_descr));

							ret = false;
						}

						*((char **) cmdline.switches[q].variable) = args[i + 1];
						i++;
						break;

					case VAR_INT:
						if (i > *argc - 2) {
							warning("Arguments", "Parameter parsing error: %s needs argument %s", args[i],
								(cmdline.switches[q].par_descr == NULL ? "" : cmdline.switches[q].par_descr));

							ret = false;
						}

						errnum = 0;
						if (strncasecmp(args[i + 1], "0x", 2) == 0)
							transcode = strtol(&(args[i + 1][2]), &ptr, 16);
						else
							transcode = strtol(args[i + 1], &ptr, 10);

						if (errnum != 0) {
							warning("Arguments", "Value %s is not correct argument for parameter %s", args[i + 1], args[i]);
							ret = false;
						}

						*((int *) cmdline.switches[q].variable) = transcode;
						i++;
						break;
				}
			}
			else {
				if (unflagged_offset == cmdline.switches[q].order) {
					switch (cmdline.switches[q].var_type) {
						case VAR_STRING:
							*((char **) cmdline.switches[q].variable) = args[i];
							break;

						case VAR_INT:
							errnum = 0;
							if (strncasecmp(args[i + 1], "0x", 2) == 0)
								transcode = strtol(&(args[i + 1][2]), &ptr, 16);
							else
								transcode = strtol(args[i + 1], &ptr, 10);

							if (errnum != 0) {
								warning("Arguments", "Value %s is not correct argument!", args[i]);
								ret = false;
							}

							*((int *) cmdline.switches[q].variable) = transcode;
							break;

						default:
							break;
					}
				}
			}

			if (!ret)
				break;
		}

		if (!ret)
			break;

		if (!processed)
			unflagged_offset++;
	}

	if (argv_config.help) {
		IntroMessage();
		printf("Usage: %s [...arguments]\n", args[0]);

		for (q = 0; q < cmdline.count; q++) {
			printf("\t");
			if (cmdline.switches[q].short_switch != NULL && cmdline.switches[q].long_switch != NULL)
				printf("%s  %s", cmdline.switches[q].short_switch, cmdline.switches[q].long_switch);
			if (cmdline.switches[q].par_descr != NULL)
				printf(" %s", cmdline.switches[q].par_descr);
			else
				printf("\t");

			if (cmdline.switches[q].short_switch == NULL || cmdline.switches[q].long_switch == NULL)
				printf("\t\t");

			printf("\t%s\n", cmdline.switches[q].description);
		}

		printf("\n");
		ret = false;
	}
	else {
		if (argv_config.machine != NULL) {
			if (strcmp(argv_config.machine, "1") == 0)
				argv_config.model = (int) CM_V1;
			else if (strcmp(argv_config.machine, "2") == 0)
				argv_config.model = (int) CM_V2;
			else if (strcmp(argv_config.machine, "2A") == 0)
				argv_config.model = (int) CM_V2A;
			else if (strcmp(argv_config.machine, "3") == 0)
				argv_config.model = (int) CM_V3;
			else if (strcmp(argv_config.machine, "Mato") == 0)
				argv_config.model = (int) CM_MATO;
			else if (strcmp(argv_config.machine, "Alfa") == 0)
				argv_config.model = (int) CM_ALFA;
			else if (strcmp(argv_config.machine, "Alfa2") == 0)
				argv_config.model = (int) CM_ALFA2;
			else if (strcmp(argv_config.machine, "C2717") == 0)
				argv_config.model = (int) CM_C2717;
			else {
				warning("Arguments", "Unknown machine '%s'", argv_config.machine);
				argv_config.machine = NULL;
			}
		}
		if (TEST_VALUE_RANGE(argv_config.scaler, 1, 5)) {
			warning("Arguments", "Invalid screen size '%d'", argv_config.scaler);
			argv_config.scaler = NOVAL;
		}
		if (TEST_VALUE_RANGE(argv_config.border, 0, 9)) {
			warning("Arguments", "Invalid border size '%d'", argv_config.border);
			argv_config.border = NOVAL;
		}
		if (TEST_VALUE_RANGE(argv_config.halfpass, 0, 5)) {
			warning("Arguments", "Invalid scanliner '%d'", argv_config.halfpass);
			argv_config.halfpass = NOVAL;
		}
		if (TEST_VALUE_RANGE(argv_config.color, 0, 3)) {
			warning("Arguments", "Invalid color profile '%d'", argv_config.color);
			argv_config.color = NOVAL;
		}
		if (TEST_VALUE_RANGE(argv_config.volume, 0, 127)) {
			warning("Arguments", "Invalid volume level '%d'", argv_config.volume);
			argv_config.volume = NOVAL;
		}

		if (argv_config.megarom != NULL && !argv_config.rmm) {
			// Mega ROM module requires ROM module to be connected, so auto-enable it
			argv_config.rmm = true;
		}
		if (argv_config.memblock == NULL && argv_config.memstart != NOVAL) {
			warning("Arguments", "Memory block file missing");
			argv_config.memstart = NOVAL;
		}
		if (TEST_VALUE_RANGE(argv_config.memstart, 0, 65535)) {
			warning("Arguments", "Invalid memory block start address '%d'", argv_config.memstart);
			argv_config.memstart = 0;
		}
		if (argv_config.memstart == NOVAL)
			argv_config.memstart = 0;
	}

	if (ret) {
		argv_config.any_related =
			argv_config.machine != NULL ||
			argv_config.rmm ||
			argv_config.megarom != NULL ||
			argv_config.scaler > 0 ||
			argv_config.halfpass >= 0 ||
			argv_config.color >= 0 ||
			argv_config.volume >= 0 ||
			argv_config.mif85 ||
			argv_config.pmd32 ||
			argv_config.p32_drvA != NULL ||
			argv_config.p32_drvB != NULL ||
			argv_config.p32_drvC != NULL ||
			argv_config.p32_drvD != NULL ||
			argv_config.tape != NULL ||
			argv_config.flashload == false ||
			argv_config.snap != NULL ||
			argv_config.memblock != NULL;
	}

	return ret;
}
//-----------------------------------------------------------------------------
