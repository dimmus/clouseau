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
   return EINA_TRUE;
}

static void
_shutdown(void)
{
}

typedef struct {
     Clouseau_Tree_Item *ptr;
     Clouseau_Tree_Item *parent;
} Clipper_Info;

typedef struct {
     Clouseau_Tree_Item *ptr;
     Clouseau_Tree_Item *parent;
     uint64_t clipper;
} Clippee_Info;

typedef struct {
     Eina_Hash *clippers;
     Eina_List *clippees;
} Context;

static void
_hash_clipper_free_cb(void *data)
{
   free(data);
}

static uint64_t
_treeit_clipee_is(Clouseau_Tree_Item *treeit)
{
   Eina_List *l;
   Efl_Dbg_Info *eo_root, *eo;
   Eina_Value_List eo_list;
   clouseau_tree_item_from_legacy_convert(treeit);
   eo_root = treeit->new_eo_info;

   eina_value_pget(&(eo_root->value), &eo_list);

   EINA_LIST_FOREACH(eo_list.list, l, eo)
     {
        if (!strcmp(eo->name, "Evas_Object"))
          {
             Eina_Value_List eo_list2;
             Eina_List *l2;
             eina_value_pget(&(eo->value), &eo_list2);
             EINA_LIST_FOREACH(eo_list2.list, l2, eo)
               {
                  if (!strcmp(eo->name, "Clipper"))
                    {
                       uint64_t ptr = 0;
                       eina_value_get(&(eo->value), &ptr);

                       return ptr;
                    }
               }

             return 0;
          }
     }

   return 0;
}


static void
_treeit_clipper_clipee_recursively_add(Context *ctx, Clouseau_Tree_Item *parent)
{
   Clouseau_Tree_Item *treeit;
   Eina_List *l;

   EINA_LIST_FOREACH(parent->children, l, treeit)
     {
        uint64_t ptr;
        if (treeit->is_clipper)
          {
             Clipper_Info *info = calloc(1, sizeof(*info));
             info->ptr = treeit;
             info->parent = parent;

             eina_hash_add(ctx->clippers, &info->ptr->ptr, info);
          }
        else if ((ptr = _treeit_clipee_is(treeit)))
          {
             Clippee_Info *info = calloc(1, sizeof(*info));
             info->ptr = treeit;
             info->parent = parent;
             info->clipper = ptr;

             ctx->clippees = eina_list_append(ctx->clippees, info);
          }

        _treeit_clipper_clipee_recursively_add(ctx, treeit);
     }
}

EAPI void
clouseau_client_module_run(Eina_List *tree)
{
   Context ctx;
   ctx.clippers = eina_hash_int64_new(_hash_clipper_free_cb);
   ctx.clippees = NULL;

   /* Gather the information from the tree. */
   Clouseau_Tree_Item *treeit;
   Eina_List *l;
   EINA_LIST_FOREACH(tree, l, treeit)
     {
        _treeit_clipper_clipee_recursively_add(&ctx, treeit);
     }

   /* Analyze the information. */
   Clippee_Info *info;
   EINA_LIST_FOREACH(ctx.clippees, l, info)
     {
        Clipper_Info *clipper = eina_hash_find(ctx.clippers, &info->clipper);

        if (!clipper)
          {
             printf("Error! Object: %llx. Can't find clipper '%llx'\n", (unsigned long long) info->ptr->ptr, (unsigned long long) info->clipper);
          }
        else if (clipper->parent != info->parent)
          {
             printf("Error! Object's (%llx) parent (%llx) != Clipper's (%llx) parent (%llx)\n", info->ptr->ptr, info->parent->ptr, clipper->ptr->ptr, clipper->parent->ptr);
          }
     }

   printf("Finished running '%s'\n", clouseau_module_name);

   /* Clear the data. */
   {
      void *data;

      eina_hash_free(ctx.clippers);
      EINA_LIST_FREE(ctx.clippees, data)
         free(data);
   }
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
