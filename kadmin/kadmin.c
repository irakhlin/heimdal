/*
 * Copyright (c) 1997, 1998, 1999 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. All advertising materials mentioning features or use of this software 
 *    must display the following acknowledgement: 
 *      This product includes software developed by Kungliga Tekniska 
 *      H�gskolan and its contributors. 
 *
 * 4. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#include "kadmin_locl.h"
#include <sl.h>

RCSID("$Id$");

static char *config_file;
static char *keyfile;
static int local_flag;
static int help_flag;
static int version_flag;
static char *realm;
static char *admin_server;
static int server_port = 0;
static char *client_name;

static struct getargs args[] = {
    {	"principal", 	'p',	arg_string,	&client_name,
	"principal to authenticate as" },
    { 
	"config-file",	'c',	arg_string,	&config_file, 
	"location of config file",	"file" 
    },
    {
	"key-file",	'k',	arg_string, &keyfile, 
	"location of master key file", "file"
    },
    {	
	"realm",	'r',	arg_string,   &realm, 
	"realm to use", "realm" 
    },
    {	
	"admin-server",	'a',	arg_string,   &admin_server, 
	"server to contact", "host" 
    },
    {	
	"server-port",	's',	arg_integer,   &server_port, 
	"server to contact", "port number" 
    },
    {	"local", 'l', arg_flag, &local_flag, "local admin mode" },
    {	"help",		'h',	arg_flag,   &help_flag },
    {	"version",	'v',	arg_flag,   &version_flag }
};

static int num_args = sizeof(args) / sizeof(args[0]);

static SL_cmd commands[] = {
    /* commands that are only available with `-l' */
    { 
	"dump",		dump,		"dump [file]",
	"Dumps the database in a human readable format to the\n"
	"specified file, or the standard out." 
    },
    { 
	"load",		load,		"load file",
	"Loads a previously dumped file."
    },
    { 
	"merge",	merge,		"merge file" ,
	"Merges the contents of a dump file into the database."
    },
    { 
	"init",		init,		"init realm...",
	"Initializes the default principals for a realm.\n"
	"Creates the database if necessary."
    },
    /* common commands */
    { 
	"add",	add_new_key, 	"add principal" ,
	"Adds a principal to the database."
    },
    { "add_new_key"},
    { "ank"},
    { 
	"passwd",	cpw_entry, 	"passwd expression..." ,
	"Changes the password of one or more principals\n"
	"matching the expressions."
    },
    { "change_password"},
    { "cpw"},
    { 
	"delete",	del_entry, 	"delete expression...",
	"Deletes all principals matching the expressions."
    },
    { "del_entry" },
    { 
	"ext_keytab",	ext_keytab, 	"ext_keytab expression...",
	"Extracts the keys of all principals matching the expressions,\n"
	"and stores them in a keytab." 
    },
    { 
	"get",		get_entry, 	"get expression...",
	"Shows information about principals matching the expressions."
    },
    { "get_entry" },
    { 
	"rename",	rename_entry, 	"rename source target",
	"Renames `source' to `target'."
    },
    { 
	"modify",	mod_entry, 	"modify principal",
	"Modifies some attributes of the specified principal."
    },
    { 
	"privileges",	get_privs,	"privileges",
	"Shows which kinds of operations you are allowed to perform."
    },
    { 
	"list",		list_princs,	"list expression...", 
	"Lists principals in a terse format. The same as `get -t'." 
    },
    { "help",		help, "help"},
    { "?"},
    { "exit",		exit_kadmin, "exit"},
    { NULL}
};

krb5_context context;
void *kadm_handle;

int
help(int argc, char **argv)
{
    sl_help(commands, argc, argv);
    return 0;
}

int
exit_kadmin (int argc, char **argv)
{
    return 1;
}

static void
usage(int ret)
{
    arg_printusage (args, num_args, NULL, "");
    exit (ret);
}

int
get_privs(int argc, char **argv)
{
    u_int32_t privs;
    char str[128];
    kadm5_ret_t ret;
    
    ret = kadm5_get_privs(kadm_handle, &privs);
    if(ret)
	krb5_warn(context, ret, "kadm5_get_privs");
    else{
	ret =_kadm5_privs_to_string(privs, str, sizeof(str));
	printf("%s\n", str);
    }
    return 0;
}

int
main(int argc, char **argv)
{
    krb5_error_code ret;
    krb5_config_section *cf;
    kadm5_config_params conf;
    int optind = 0;
    int e;
    SL_cmd *cmd;

    set_progname(argv[0]);

    krb5_init_context(&context);

    while((e = getarg(args, num_args, argc, argv, &optind)))
	warnx("error at argument `%s'", argv[optind]);

    if (help_flag)
	usage (0);

    if (version_flag)
	krb5_errx(context, 0, "%s", heimdal_version);

    argc -= optind;
    argv += optind;

    if (config_file == NULL)
	config_file = HDB_DB_DIR "/kdc.conf";

    if(krb5_config_parse_file(config_file, &cf) == 0) {
	const char *p = krb5_config_get_string (context, cf, 
						"kdc", "key-file", NULL);
	if (p)
	    keyfile = strdup(p);
    }

    memset(&conf, 0, sizeof(conf));
    if(realm) {
	krb5_set_default_realm(context, realm); /* XXX should be fixed
						   some other way */
	conf.realm = realm;
	conf.mask |= KADM5_CONFIG_REALM;
    }

    if (admin_server) {
	conf.admin_server = admin_server;
	conf.mask |= KADM5_CONFIG_ADMIN_SERVER;
    }

    if (server_port) {
	conf.kadmind_port = htons(server_port);
	conf.mask |= KADM5_CONFIG_KADMIND_PORT;
    }

    if(local_flag){
	ret = kadm5_s_init_with_password_ctx(context, 
					     KADM5_ADMIN_SERVICE,
					     NULL,
					     KADM5_ADMIN_SERVICE,
					     &conf, 0, 0, 
					     &kadm_handle);
	cmd = commands;
    } else {
	ret = kadm5_c_init_with_password_ctx(context, 
					     client_name,
					     NULL,
					     KADM5_ADMIN_SERVICE,
					     &conf, 0, 0, 
					     &kadm_handle);
	cmd = commands + 4; /* XXX */
    }
    
    if(ret)
	krb5_err(context, 1, ret, "kadm5_init_with_password");
    if (argc != 0)
	exit(sl_command(cmd, argc, argv));

    ret = sl_loop(cmd, "kadmin> ") != 0;
    kadm5_destroy(kadm_handle);
    krb5_free_context(context);
    return ret;
}
