/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * rpmdb2solv
 * 
 * Reads rpm database (and evtl. more, like product metadata) to build
 * a .solv file of 'installed' solvables.
 * Writes .solv to stdout
 * 
 */

#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pool.h"
#include "repo.h"
#include "repo_rpmdb.h"
#include "repo_products.h"
#include "repo_solv.h"
#include "common_write.h"

static void
usage(int status)
{
  fprintf(stderr, "\nUsage:\n"
	  "rpmdb2solv [-n] [-x] [-b <basefile>] [-p <productsdir>] [-r <root>]\n"
	  " -n : No packages, do not read rpmdb, useful to only parse products\n"
	  " -x : use extrapool\n"
	  " -b <basefile> : Write .solv to <basefile>.solv instead of stdout\n"
	  " -p <productsdir> : Scan <productsdir> for .prod files, representing installed products\n"
	  " -r <root> : Prefix rpmdb path and <productsdir> with <root>\n"
	 );
  exit(status);
}


int
main(int argc, char **argv)
{
  Pool *pool = pool_create();
  Repo *repo, *ref = 0;
  Repodata *repodata;
  FILE *fp;
  Pool *refpool;
  int c;
  int extrapool = 0;
  int nopacks = 0;
  const char *root = 0;
  const char *basefile = 0;
  const char *proddir = 0;

  /*
   * parse arguments
   */
  
  while ((c = getopt (argc, argv, "hnxb:r:p:")) >= 0)
    switch (c)
      {
      case 'h':
	  usage(0);
	break;
      case 'r':
        root = optarg;
        break;
      case 'b':
        basefile = optarg;
        break;
      case 'n':
	nopacks = 1;
	break;
      case 'p':
	proddir = optarg;
	break;
      case 'x':
        extrapool = 1;
        break;
      default:
	usage(1);
      }
  
  /*
   * ???
   */
  
  if (optind < argc)
    {
      if (extrapool)
	refpool = pool_create();
      else
        refpool = pool;
      if ((fp = fopen(argv[optind], "r")) == NULL)
        {
          perror(argv[optind]);
          exit(1);
        }
      ref = repo_create(refpool, "ref");
      repo_add_solv(ref, fp);
      repo_disable_paging(ref);
      fclose(fp);
    }

  /*
   * create 'installed' repository
   * add products
   * add rpmdb
   * write .solv
   */

  repo = repo_create(pool, "installed");
  repodata = repo_add_repodata(repo, 0);

  if (!nopacks)
    repo_add_rpmdb(repo, repodata, ref, root);

  if (proddir && *proddir)
    {
      /* if <root> given, not '/', and proddir does not start with <root> */
      if (root && *root)
	{
	  int rootlen = strlen(root);
	  if (strncmp(root, proddir, rootlen))
	    {
	      char *buf;
	      buf = (char *)sat_malloc(rootlen + strlen(proddir) + 2); /* + '/' + \0 */
	      strcpy(buf, root);
	      if (root[rootlen-1] != '/'
		  && *proddir != '/')
		{
		  strcpy(buf+rootlen, "/");
		  ++rootlen;
		}
	      strcpy(buf+rootlen, proddir);
	      proddir = buf;
	    }
	}
      
      repo_add_products(repo, repodata, proddir, root);
    }
      
  if (repodata)
    repodata_internalize(repodata);

  if (ref)
    {
      if (ref->pool != pool)
	pool_free(ref->pool);
      else
	repo_free(ref, 1);
      ref = 0;
    }

  tool_write(repo, basefile, 0);
  pool_free(pool);

  exit(0);
}
