#include "kuser_locl.h"
RCSID("$Id$");

static void
usage (void)
{
    errx (1, "Usage: %s [-f] [-p] [principal]", __progname);
}

int
main (int argc, char **argv)
{
  krb5_error_code err;
  krb5_context context;
  krb5_ccache  ccache;
  krb5_principal principal;
  krb5_principal server;
  krb5_creds cred;
  krb5_preauthtype pre_auth_types[] = {KRB5_PADATA_ENC_TIMESTAMP};
  int c;
  int forwardable = 0;
  krb5_flags options = 0;
  char *realm;
  int preauth = 1;

  set_progname (argv[0]);

  while ((c = getopt (argc, argv, "fp")) != EOF) {
      switch (c) {
      case 'f':
	  forwardable = 1;
	  options = 1;		/* XXX */
	  break;
      case 'p':
	  preauth = 0;
	  break;
      default:
	  usage ();
      }
  }
  argc -= optind;
  argv += optind;

  err = krb5_init_context (&context);
  if (err)
      errx (1, "krb5_init_context: %s", krb5_get_err_text(context, err));
  
  err = krb5_cc_default (context, &ccache);
  if (err)
      errx (1, "krb5_cc_default: %s", krb5_get_err_text(context, err));
  
  err = krb5_get_default_realm (context, &realm);
  if (err)
      errx (1, "krb5_get_default_realm: %s",
	    krb5_get_err_text(context, err));

  if(argv[0]){
      err = krb5_parse_name (context, argv[0], &principal);
      if (err)
	  errx (1, "krb5_parse_name: %s", krb5_get_err_text(context, err));
      fprintf (stderr, "%s@%s's ", argv[0], realm);
  }else{
      struct passwd *pw;

      pw = getpwuid(getuid());
      err = krb5_build_principal(context, &principal,
				 strlen(realm), realm,
				 pw->pw_name, NULL);
      if (err)
	  errx (1, "krb5_build_principal: %s",
		krb5_get_err_text(context, err));
      fprintf (stderr, "%s@%s's ", pw->pw_name, realm);
  }
  free(realm);

  err = krb5_cc_initialize (context, ccache, principal);
  if (err)
      errx (1, "krb5_cc_initialize: %s",
	    krb5_get_err_text(context, err));

  cred.client = principal;
  cred.times.endtime = 0;

  err = krb5_build_principal_ext (context,
				  &cred.server,
#ifdef USE_ASN1_PRINCIPAL
				  strlen(principal->realm),
				  principal->realm,
#else
				  principal->realm.length,
				  principal->realm.data,
#endif
				  strlen("krbtgt"),
				  "krbtgt",
#ifdef USE_ASN1_PRINCIPAL
				  strlen(principal->realm),
				  principal->realm,
#else
				  principal->realm.length,
				  principal->realm.data,
#endif
				  NULL);
  if (err)
      errx (1, "krb5_build_principal_ext: %s",
	    krb5_get_err_text(context, err));

#ifdef USE_ASN1_PRINCIPAL
  cred.server->name.name_type = KRB5_NT_SRV_INST;
#else
  cred.server->type = KRB5_NT_SRV_INST;
#endif

  err = krb5_get_in_tkt_with_password (context,
				       options,
				       NULL,
				       NULL,
				       preauth ? pre_auth_types : NULL,
				       NULL,
				       ccache,
				       &cred,
				       NULL);
  if (err)
      errx (1, "krb5_get_in_tkt_with_password: %s",
	    krb5_get_err_text(context, err));
  
  krb5_free_context (context);
  return 0;
}
