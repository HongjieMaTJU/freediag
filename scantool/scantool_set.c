/*
 *	freediag - Vehicle Diagnostic Utility
 *
 *
 * Copyright (C) 2001 Richard Almeida & Ibex Ltd (rpa@ibex.co.uk)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************
 *
 *
 * Mostly ODBII Compliant Scan Tool (as defined in SAE J1978)
 *
 * CLI routines - "set" commands
 *
 *
 */

#include "diag.h"
#include "diag_l1.h"
#include "diag_l2.h"

#include "scantool.h"
#include "scantool_cli.h"

CVSID("$Id$");

#define PROTO_NONE	"<not_used>"

int 	set_speed;	/* Comms speed */
unsigned char	set_testerid;	/* Our tester ID */
int	set_addrtype;	/* Use virtual addressing */
unsigned char	set_destaddr;	/* Dest ECU address */
int	set_L1protocol;	/* L1 protocol type */
int	set_L2protocol;	/* Protocol type */
int	set_initmode;

int set_display;		/* English (1), or Metric (0) */

const char *	set_vehicle;	/* Vehicle */
const char *	set_ecu;	/* ECU name */

const char  *	set_interface;	/* H/w interface to use */

char set_subinterface[SUBINTERFACE_MAX];		/* and sub-interface ID */

/*
 * XXX All commands should probably have optional "init" hooks.
 */
int set_init(void) {
	/* Reset parameters to defaults. */

	set_speed = 10400;	/* Comms speed */
	set_testerid = 0xf1;	/* Our tester ID */
	set_addrtype = 1;	/* Use virtual addressing */
	set_destaddr = 0x33;	/* Dest ECU address */
	set_L1protocol = DIAG_L1_ISO9141;	/* L1 protocol type */
	set_L2protocol = DIAG_L2_PROT_ISO9141;	/* Protocol type */
	set_initmode = DIAG_L2_TYPE_FASTINIT ;

	set_display = 0;		/* English (1), or Metric (0) */

	set_vehicle = "ODBII";	/* Vehicle */
	set_ecu = "ODBII";	/* ECU name */

	set_interface = "CARSIM";	/* H/w interface to use */

	#if 0
	set_interface = "MET16";	/* H/w interface to use */
	set_interface = "VAGTOOL";	/* H/w interface to use */
	#endif

	memset(set_subinterface, 0, sizeof(set_subinterface));

	return 0;
}


/* Return values from the commands */
#define CMD_OK		0	/* OK */
#define CMD_USAGE	1	/* Bad usage, print usage info */
#define CMD_FAILED	2	/* Cmd failed */
#define CMD_EXIT	3	/* Exit called */
#define CMD_UP		4	/* Go up one level in command tree */

#define FLAG_HIDDEN	1	/* Hidden command */

/* SET sub menu */
static int cmd_set_help(int argc, char **argv);
static int cmd_set_show(int argc, char **argv);
static int cmd_set_speed(int argc, char **argv);
static int cmd_set_testerid(int argc, char **argv);
static int cmd_set_destaddr(int argc, char **argv);
static int cmd_set_addrtype(int argc, char **argv);
static int cmd_set_l1protocol(int argc, char **argv);
static int cmd_set_l2protocol(int argc, char **argv);
static int cmd_set_initmode(int argc, char **argv);
static int cmd_set_display(int argc, char **argv);

static int cmd_set_interface(int argc, char **argv);

const struct cmd_tbl_entry set_cmd_table[] =
{
	{ "help", "help [command]", "Gives help for a command",
		cmd_set_help, 0, NULL},

	{ "interface", "interface NAME [id]", "Shows/Sets the interface to use. Use set interface ? to get a list of names",
		cmd_set_interface, 0, NULL},

	{ "display", "display [english/metric]", "Sets english or metric display",
		cmd_set_display, 0, NULL},

	{ "speed", "speed [speed]", "Shows/Sets the speed to connect",
		cmd_set_speed, 0, NULL},
	{ "testerid", "testerid [testerid]",
		"Shows/Sets the source ID for us to use",
		cmd_set_testerid, 0, NULL},
	{ "destaddr", "destaddr [destaddr]",
		"Shows/Sets the destination ID to connect",
		cmd_set_destaddr, 0, NULL},

	{ "addrtype", "addrtype [func/phys]", "Shows/Sets the address type to use",
		cmd_set_addrtype, 0, NULL},


	{ "l1protocol", "l1protocol [protocolname]", "Shows/Sets the hardware protocol to use. Use set l1protocol ? to get a list of protocols",
		cmd_set_l1protocol, 0, NULL},

	{ "l2protocol", "l2protocol [protocolname]", "Shows/Sets the software protocol to use. Use set l2protocol ? to get a list of protocols",
		cmd_set_l2protocol, 0, NULL},

	{ "initmode", "initmode [modename]", "Shows/Sets the initialisation mode to use. Use set initmode ? to get a list of protocols",
		cmd_set_initmode, 0, NULL},

	{ "show", "show", "Shows all set'able values",
		cmd_set_show, 0, NULL},

	{ "quit", "quit", "Return to previous menu level",
		cmd_quit, 0, NULL},
	{ "exit", "exit", "Return to previous menu level",
		cmd_quit, FLAG_HIDDEN, NULL},

	{ NULL, NULL, NULL, NULL, 0, NULL}
};

const char * const l0_names[] =
{
	"MET16", "SE9141", "VAGTOOL", "BR1", "ELM", "CARSIM", NULL
};

const char * const l1_names[] =
{
	"ISO9141", "ISO14230",
	"J1850-VPW", "J1850-PWM", "CAN", "", "", "RAW", NULL
};

const char * const l2_names[] =
{
	"RAW", "ISO9141", PROTO_NONE, "ISO14230",
	"J1850", "CAN", "VAG", "MB1", NULL
};

const char * const l2_initmodes[] =
{
	"5BAUD", "FAST", "CARB", NULL
};

#ifdef WIN32
static int
cmd_set_show(int argc,
char **argv)
#else
static int
cmd_set_show(int argc __attribute__((unused)),
char **argv __attribute__((unused)))
#endif
{
	/* Show stuff */
	int offset;
	for (offset=0; offset < 8; offset++)
	{
		if (set_L1protocol == (1 << offset))
			break;
	}

	printf("interface: %s id %s\n", set_interface, set_subinterface);
	printf("speed:    Connect speed: %d\n", set_speed);
	printf("display:  %s units\n", set_display?"english":"metric");
	printf("testerid: Source ID to use: 0x%x\n", set_testerid);
	printf("addrtype: %s addressing\n",
		set_addrtype ? "functional" : "physical");
	printf("destaddr: Destination address to connect to: 0x%x\n",
		set_destaddr);
	printf("l1protocol: Layer 1 (H/W) protocol to use %s\n",
		l1_names[offset]);
	printf("l2protocol: Layer 2 (S/W) protocol to use %s\n",
		l2_names[set_L2protocol]);
	printf("initmode: Initmode to use with above L2 protocol is %s\n",
		l2_initmodes[set_initmode]);

	return (CMD_OK);
}


static int cmd_set_interface(int argc, char **argv)
{
	if (argc > 1)
	{
		int i, prflag = 0, found = 0;
		if (strcmp(argv[1], "?") == 0)
		{
			prflag = 1;
			printf("interface protocol: valid names are ");
		}
		for (i=0; l0_names[i] != NULL; i++)
		{
			if (prflag)
				printf("%s ", l0_names[i]);
			else
				if (strcasecmp(argv[1], l0_names[i]) == 0)
				{
					found = 1;
					set_interface = l0_names[i];
				}
		}
		if (prflag)
			printf("\n");
		if (! found)
		{
			printf("interface: invalid interface %s\n", argv[1]);
			printf("interface: use \"set interface ?\" to see list of names\n");
		} else {
			if (argc > 2)
				strncpy(set_subinterface, argv[2], sizeof(set_subinterface));
			else
				strncpy(set_subinterface, "0", sizeof(set_subinterface));
		}
	}
	else
	{
		printf("interface: interface to use: %s subinterface %s\n",
			set_interface, set_subinterface);
	}
	return (CMD_OK);
}

static int
cmd_set_display(int argc, char **argv)
{
	if (argc > 1)
	{
		if (strcasecmp(argv[1], "english") == 0)
			set_display = 1;
		else if (strcasecmp(argv[1], "metric") == 0)
			set_display = 0;
		else
			return (CMD_USAGE);
	}
	else
		printf("display: %s units\n", set_display?"english":"metric");

	return (CMD_OK);
}

static int
cmd_set_speed(int argc, char **argv)
{
	if (argc > 1)
	{
		set_speed = htoi(argv[1]);
	}
	else
		printf("speed: Connect speed: %d\n", set_speed);

	return (CMD_OK);
}

static int
cmd_set_testerid(int argc, char **argv)
{
	if (argc > 1)
	{
		int tmp;
		tmp = htoi(argv[1]);
		if ( (tmp < 0) || (tmp > 0xff))
			printf("testerid: must be between 0 and 0xff\n");
		else
			set_testerid = tmp;
	}
	else
		printf("testerid: Source ID to use: 0x%x\n", set_testerid);

	return (CMD_OK);
}
static int
cmd_set_destaddr(int argc, char **argv)
{
	if (argc > 1)
	{
		int tmp;
		tmp = htoi(argv[1]);
		if ( (tmp < 0) || (tmp > 0xff))
			printf("destaddr: must be between 0 and 0xff\n");
		else
			set_destaddr = tmp;
	}
	else
	{
		printf("destaddr: Destination address to connect to: 0x%x\n",
			set_destaddr);
	}

	return (CMD_OK);
}
static int
cmd_set_addrtype(int argc, char **argv)
{
	if (argc > 1)
	{
		if (strncmp(argv[1], "func", 4) == 0)
			set_addrtype = 1;
		else if (strncmp(argv[1], "phys", 4) == 0)
			set_addrtype = 0;
		else
			return(CMD_USAGE);
	}
	else
	{
		printf("addrtype: %s addressing\n",
			set_addrtype ? "functional" : "physical");
	}

	return (CMD_OK);
}

static int cmd_set_l2protocol(int argc, char **argv)
{
	if (argc > 1)
	{
		int i, prflag = 0, found = 0;
		if (strcmp(argv[1], "?") == 0)
		{
			prflag = 1;
			printf("L2 protocol: valid names are ");
		}
		for (i=0; l2_names[i] != NULL; i++)
		{
			if (prflag)
			{
				if (strcasecmp(l2_names[i], PROTO_NONE)) 
				{
					printf("%s ", l2_names[i]);
				}
			}
			else
				if (strcasecmp(argv[1], l2_names[i]) == 0)
				{
					found = 1;
					set_L2protocol = i;
				}
		}
		if (prflag)
			printf("\n");
		else if (! found)
		{
			printf("l2protocol: invalid protocol %s\n", argv[1]);
			printf("l2protocol: use \"set l2protocol ?\" to see list of protocols\n");
		}
	}
	else
	{
		printf("l2protocol: Layer 2 protocol to use %s\n",
			l2_names[set_L2protocol]);
	}
	return (CMD_OK);
}

static int cmd_set_l1protocol(int argc, char **argv)
{
	if (argc > 1)
	{
		int i, prflag = 0, found = 0;
		if (strcmp(argv[1], "?") == 0)
		{
			prflag = 1;
			printf("L1 protocol: valid names are ");
		}
		for (i=0; l1_names[i] != NULL; i++)
		{
			if (prflag && *l1_names[i])
				printf("%s ", l1_names[i]);
			else
				if (strcasecmp(argv[1], l1_names[i]) == 0)
				{
					set_L1protocol = 1 << i;
					found = 1;
				}
		}
		if (prflag)
			printf("\n");
		else if (! found)
		{
			printf("L1protocol: invalid protocol %s\n", argv[1]);
			printf("l1protocol: use \"set l1protocol ?\" to see list of protocols\n");
		}
	}
	else
	{
		int offset;

		for (offset=0; offset < 8; offset++)
		{
			if (set_L1protocol == (1 << offset))
				break;
		}
		printf("l1protocol: Layer 1 (H/W) protocol to use %s\n",
			l1_names[offset]);

	}
	return (CMD_OK);
}

static int cmd_set_initmode(int argc, char **argv)
{
	if (argc > 1)
	{
		int i, prflag = 0, found = 0;
		if (strcmp(argv[1], "?") == 0)
			prflag = 1;
		for (i=0; l2_initmodes[i] != NULL; i++)
		{
			if (prflag)
				printf("%s ", l2_initmodes[i]);
			else
			{
				if (strcasecmp(argv[1], l2_initmodes[i]) == 0)
				{
					found = 1;
					set_initmode = i;
				}
			}
		}
		if (prflag)
			printf("\n");
		else if (! found)
		{
			printf("initmode: invalid mode %s\n", argv[1]);
			printf("initmode: use \"set initmode ?\" to see list of initmodes\n");
		}
	}
	else
	{
		printf("initmode: Initmode to use with above protocol is %s\n",
			l2_initmodes[set_initmode]);
	}
	return(CMD_OK);
}

static int
cmd_set_help(int argc, char **argv)
{
	return help_common(argc, argv, set_cmd_table);
}
