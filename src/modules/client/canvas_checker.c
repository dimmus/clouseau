#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <stdio.h>
#include <Clouseau.h>

EAPI const char *clouseau_module_name = "Canvas Sanity Checker";

static Eina_Bool
_init(void)
{
   return EINA_FALSE;
}

static void
_shutdown(void)
{
}

EAPI void clouseau_client_module_run(Eina_List *tree)
{
   Clouseau_Tree_Item *treeit;
   Eina_List *l;
   EINA_LIST_FOREACH(tree, l, treeit)
     {
        if (treeit)
           return;
     }
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
