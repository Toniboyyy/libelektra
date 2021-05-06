/**
 * @file
 *
 * @brief tool for creating and finding the GPG test key to be used in Elektra's unit tests.
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include <gpgme.h>
#include <stdio.h>

#define ELEKTRA_GEN_GPG_TESTKEY_UNUSED __attribute__ ((unused))
#define ELEKTRA_GEN_GPG_TESTKEY_DESCRIPTION "elektra testkey (gen-gpg-testkey)"

gpgme_error_t passphrase_cb (void * hook ELEKTRA_GEN_GPG_TESTKEY_UNUSED, const char * uid_hint ELEKTRA_GEN_GPG_TESTKEY_UNUSED,
			     const char * passphrase_info ELEKTRA_GEN_GPG_TESTKEY_UNUSED, int prev_was_bad ELEKTRA_GEN_GPG_TESTKEY_UNUSED,
			     int fd)
{
	gpgme_io_writen (fd, "\n", 2);
	return 0;
}

int main (void)
{
	gpgme_error_t err;
	gpgme_ctx_t ctx;
	gpgme_key_t key;
	gpgme_genkey_result_t res;

	gpgme_check_version (NULL);
	err = gpgme_engine_check_version (GPGME_PROTOCOL_OpenPGP);
	if (err)
	{
		fprintf (stderr, "gpgme version check failed: %s\n", gpgme_strerror (err));
		return -1;
	}

	// setup gpgme context
	err = gpgme_new (&ctx);
	if (err)
	{
		fprintf (stderr, "gpgme error: %s\n", gpgme_strerror (err));
		return -1;
	}

	// configure gpgme
	gpgme_set_pinentry_mode (ctx, GPGME_PINENTRY_MODE_LOOPBACK);
	gpgme_set_passphrase_cb (ctx, passphrase_cb, NULL);

	// look for the elektra key
	err = gpgme_op_keylist_start (ctx, ELEKTRA_GEN_GPG_TESTKEY_DESCRIPTION, 1 /* secret keys only! */);
	if (err)
	{
		fprintf (stderr, "gpgme error: %s\n", gpgme_strerror (err));
		goto cleanup;
	}

	err = gpgme_op_keylist_next (ctx, &key);
	if (err && gpg_err_code (err) != GPG_ERR_EOF)
	{
		fprintf (stderr, "gpgme error: %s\n", gpgme_strerror (err));
		goto cleanup;
	}

	if (err && gpg_err_code (err) == GPG_ERR_EOF)
	{
		err = gpgme_op_createkey (ctx, ELEKTRA_GEN_GPG_TESTKEY_DESCRIPTION, NULL, 0, 0, NULL,
					  GPGME_CREATE_SIGN | GPGME_CREATE_ENCR);

		if (err)
		{
			fprintf (stderr, "failed to create GPG test key with error message: %s: %s\n", gpgme_strsource (err),
				 gpgme_strerror (err));
			goto cleanup;
		}

		res = gpgme_op_genkey_result (ctx);
		fprintf (stdout, "%s", res->fpr);
	}
	else
	{
		// display the key ID
		fprintf (stdout, "%s", key->subkeys->fpr);
		gpgme_key_release (key);
		gpgme_op_keylist_end (ctx);
	}

	gpgme_release (ctx);
	return 0;

cleanup:
	gpgme_release (ctx);
	return -1;
}
