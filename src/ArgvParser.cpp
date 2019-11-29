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
//-----------------------------------------------------------------------------
struct TCmdLineArguments argv_config = {
	false,  /* --help */
	false,  /* --version */
	true,   /* --over-cfg */
	NULL,   /* --machine */
	false,  /* --rmm */
	0,      /* --scaler */
	-1,     /* --halfpass */
	-1,     /* --profile */
	-1,     /* --volume */
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
	NULL,   /* --snap */
	NULL,   /* --memblock */
	0,      /* --memblock-address */
};
TCmdLineSwitch switches[] = {
	{ "-h",   "--help", VAR_BOOL, (void *) &argv_config.help,
				"print this help", SWPAR(NULL) },
	{ "-v",   "--version", VAR_BOOL, (void *) &argv_config.version,
				"print version number", SWPAR(NULL) },
	{ "-c",   "--over-cfg", VAR_BOOL_NEG, (void *) &argv_config.usercfg,
				"override user's configuration", SWPAR(NULL) },
	{ "-m",   "--machine", VAR_STRING, (void *) &argv_config.machine,
				"select machine", SWPAR("{1, 2, 2A.. C2717}") },
	{ "-r",   "--rmm", VAR_BOOL, (void *) &argv_config.rmm,
				"connect ROM module", SWPAR(NULL) },
	{ "-sc",  "--scaler", VAR_INT, (void *) &argv_config.scaler,
				"screen size multiplier", SWPAR("{1..4}") },
	{ "-hp",  "--halfpass", VAR_INT, (void *) &argv_config.halfpass,
				"scanliner (5=LCD)", SWPAR("{0..5}") },
	{ "-cp",  "--profile", VAR_INT, (void *) &argv_config.color,
				"color profile (0=MONO, 1=STD, 2=RGB, 3=ColorACE)", SWPAR("{0..3}") },
	{ "-vol", "--volume", VAR_INT, (void *) &argv_config.volume,
				"sound volume", SWPAR("{0..127}") },
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
	{ "-s",   "--snap", VAR_STRING, (void *) &argv_config.snap,
				"load snapshot", SWPAR("\"filename.psn\"") },
	{ "-b",   "--memblock", VAR_STRING, (void *) &argv_config.memblock,
				"load memory block", SWPAR("\"filename.bin\"") },
	{ "-ptr", "--memblock-address", VAR_INT, (void *) &argv_config.memstart,
				"load memory block at given address", SWPAR(NULL) },
};
TCmdLineSwitches cmdline = { switches, 23 };
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
							warning("ArgvParser", "Parameter parsing error: %s needs argument %s", args[i],
								(cmdline.switches[q].par_descr == NULL ? "" : cmdline.switches[q].par_descr));

							ret = false;
						}

						*((char **) cmdline.switches[q].variable) = args[i + 1];
						i++;
						break;

					case VAR_INT:
						if (i > *argc - 2) {
							warning("ArgvParser", "Parameter parsing error: %s needs argument %s", args[i],
								(cmdline.switches[q].par_descr == NULL ? "" : cmdline.switches[q].par_descr));

							ret = false;
						}

						errnum = 0;
						if (strncasecmp(args[i + 1], "0x", 2) == 0)
							transcode = strtol(&(args[i + 1][2]), &ptr, 16);
						else
							transcode = strtol(args[i + 1], &ptr, 10);

						if (errnum != 0) {
							warning("ArgvParser", "Value %s is not correct argument for parameter %s", args[i + 1], args[i]);
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
								warning("ArgvParser", "Value %s is not correct argument!", args[i]);
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

	return ret;
}
//-----------------------------------------------------------------------------
