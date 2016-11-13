#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <stdio.h>
#include <Clouseau.h>

static int _focus_checker_dom = -1;

//critical errors are errors which can make the checks produce wrong results
#define CRIT(...) EINA_LOG_DOM_CRIT(_focus_checker_dom, __VA_ARGS__)
//errors are reporting failures of the test
#define ERR(...) EINA_LOG_DOM_ERR(_focus_checker_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_focus_checker_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_focus_checker_dom, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(_focus_checker_dom, __VA_ARGS__)

EAPI const char *clouseau_module_name = "Focus Graph Checker";

typedef struct {
   Eina_List *right;
   Eina_List *left;
   Eina_List *top;
   Eina_List *down;
   void *next;
   void *prev;
   void *manager;
   void *redirect;
} Focus_Relations;

typedef struct _Focus_Manager Focus_Manager;

struct _Focus_Manager {
  void *ptr;
  void *first_child;
  Eina_Hash *objects;
  Eina_List *redirects;
  struct {
    Focus_Manager *parent;
    int depth;
  } manager_tree_check; //only valid in execution of _managers_link_check();
};

static Eina_Hash *managers = NULL;

static Focus_Manager*
_manager_get(void *manager)
{
   Focus_Manager *m;

   m = eina_hash_find(managers, &manager);
   if (!m)
     {
        m = calloc(1, sizeof(Focus_Manager));

        m->ptr = manager;
        m->objects = eina_hash_pointer_new(NULL);
        eina_hash_add(managers, &manager, m);
     }

   return m;
}

static void
_focus_manager_redirect_add(Focus_Manager *m, void *redirect)
{
   m->redirects = eina_list_append(m->redirects, redirect);
}

static void
_pointer_free(void *data)
{
   free(data);
}

static Eina_Bool
_init(void)
{
   eina_init();
   _focus_checker_dom = eina_log_domain_register("Focus Checker", EINA_COLOR_CYAN);
   eina_log_domain_level_set("Focus Checker", EINA_LOG_LEVEL_INFO);
   managers = eina_hash_pointer_new(_pointer_free);
   return EINA_TRUE;
}

static void
_shutdown(void)
{
   eina_log_domain_unregister(_focus_checker_dom);
   eina_shutdown();
}

static Eina_List*
_list_ptr_get(Efl_Dbg_Info *info)
{
   Eina_Value_List list;
   Efl_Dbg_Info *data;
   Eina_List *n, *result = NULL;

   if (eina_value_type_get(&info->value) != EINA_VALUE_TYPE_LIST)
     {
        CRIT("Expected list type");
        return NULL;
     }

   eina_value_pget(&info->value, &list);

   EINA_LIST_FOREACH(list.list, n, data)
     {
        unsigned long long ptr;

        eina_value_get(&data->value, &ptr);

        result = eina_list_append(result, (void*)ptr);
     }

   return result;
}

static Focus_Relations*
_tree_it_convert(Clouseau_Tree_Item *it)
{
   Focus_Relations *ret;
   Efl_Dbg_Info *widget, *focus, *next, *prev,
                *right, *left, *top, *down,
                *manager, *redirect;

   clouseau_tree_item_from_legacy_convert(it);

   widget = clouseau_eo_info_find(it->new_eo_info , "Elm_Widget");
   if (!widget)
     {
        return NULL;
     }

   focus = clouseau_eo_info_find(widget, "Focus");
   if (!focus)
     {
        return NULL;
     }

   next = clouseau_eo_info_find(focus, "next");
   prev = clouseau_eo_info_find(focus, "prev");
   right = clouseau_eo_info_find(focus, "right");
   left = clouseau_eo_info_find(focus, "left");
   top = clouseau_eo_info_find(focus, "top");
   down = clouseau_eo_info_find(focus, "down");
   manager = clouseau_eo_info_find(focus, "manager");
   redirect = clouseau_eo_info_find(focus, "redirect");

   if (!next || !prev || !right || !left || !top || !down || !manager || !redirect)
     {
        CRIT("Widget %p did not present the full set of properties.", (void*)it->ptr);

        return NULL;
     }
   ret = calloc(1, sizeof(Focus_Relations));

   eina_value_get(&next->value, &ret->next);
   eina_value_get(&prev->value, &ret->prev);
   ret->right = _list_ptr_get(right);
   ret->left = _list_ptr_get(left);
   ret->top = _list_ptr_get(top);
   ret->down = _list_ptr_get(down);
   eina_value_get(&manager->value, &ret->manager);
   eina_value_get(&redirect->value, &ret->redirect);
   return ret;
}

static void
_add(Clouseau_Tree_Item *it)
{
   Clouseau_Tree_Item *treeit;
   Focus_Relations *rel;
   Focus_Manager *m;
   Eina_List *n;

   rel = _tree_it_convert(it);

   if (rel)
     {
        m = _manager_get(rel->manager);
        if (!m->first_child)
          m->first_child = (void*)it->ptr;
        if (rel->redirect)
          _focus_manager_redirect_add(m, rel->redirect);
        eina_hash_add(m->objects, &it->ptr, rel);
     }

   EINA_LIST_FOREACH(it->children , n, treeit)
     {
        _add(treeit);
     }
}

/*
 * Check that all widgets are linked to each other
 *
 * This is working by duplicating the hash of objects,.
 * Exploring a item means that all the right left top
 * down next and prev directions are added into the set of "need explotation".
 * At first one random element is added to the set of need exploration.
 * Then remove each time a element from the set, if the element is still
 * in the duplicated hash, the element is explored, if not the element is ignored.
 *
 * If the set is now empty and the hash has still elements there must be a set
 * of not linked elements to the other elements.
 */
Eina_Bool
_duplicate(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   eina_hash_add(fdata, key, data);

   return EINA_TRUE;
}

Eina_Bool
_count(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
  (*((int*)fdata))++;
  return EINA_TRUE;
}

static void
_manager_check(Focus_Manager *manager)
{
   Eina_Hash *dup;
   Eina_Iterator *itr;
   void *ptr;
   Eina_List *runner = NULL, *n, *n_next;

   dup = eina_hash_pointer_new(NULL);
   eina_hash_foreach(manager->objects, _duplicate, dup);

   runner = eina_list_append(runner, manager->first_child);

   do
     {
        void *ptr;
        Focus_Relations *rel;

        ptr = eina_list_data_get(runner);
        runner = eina_list_remove(runner, ptr);

        rel = eina_hash_find(manager->objects, &ptr);
        if (!rel)
          {
              ERR("In manager %p %p is used in a reference, but not part of the manager", manager->ptr , ptr);
              continue;
          }

        rel = eina_hash_find(dup, &ptr);
        if (!rel)
          {
             continue;
          }

        eina_hash_del_by_key(dup, &ptr);


        runner = eina_list_merge(runner, rel->left);
        runner = eina_list_merge(runner, rel->right);
        runner = eina_list_merge(runner, rel->top);
        runner = eina_list_merge(runner, rel->down);
        runner = eina_list_append(runner, rel->next);
        runner = eina_list_append(runner, rel->prev);

   } while(eina_list_count(runner) != 0);

   //if we are done and sonething is left in the hash we have a not connected subset in the manager. BAD
   int count;

   eina_hash_foreach(dup, _count, &count);

   if (count > 0)
     {
        ERR("Manager %p has a disconnected set of widgets", manager->ptr);
        /* FIXME print all the children */
     }
   else
     INF("Manager %p is checked and fine", manager->ptr);
}

/**
 * Functions to check that the with redirect created objects are a tree
 *
 * This works by iterating 3 times throuw the elements, at first, we are setting the parent of each child object
 * In the second stage we are searching for the element with no parent, which is the root.
 * And we ensure that there is just one root.
 * In the last stage we are calculating the depth of each manager, if in some element a other depth is laready
 * calculated we know that there is at least one cycle.
 */

static Eina_Bool
_depth_calc(Focus_Manager *m, int depth)
{
   Eina_List *n;
   Focus_Manager *m2;
   Eina_Bool suc = EINA_TRUE;

   if (m->manager_tree_check.depth != 0)
     {
        ERR("Manager %p is part of a cycle in the tree!", m->ptr);
        return EINA_FALSE;
     }

   m->manager_tree_check.depth = depth;

   EINA_LIST_FOREACH(m->redirects, n, m2)
     {
        if (!_depth_calc(m2, depth + 1))
          suc = EINA_FALSE;
     }
   return suc;
}

static void
_managers_link_check(void)
{
   Focus_Manager *root = NULL, *m;
   Eina_Hash *manager_cache;
   Eina_Iterator *itr;

   //check if this things contains cycles
   manager_cache = eina_hash_pointer_new(NULL);

   //First stage set parents;
   itr = eina_hash_iterator_data_new(managers);
   EINA_ITERATOR_FOREACH(itr, m)
     {
        Focus_Manager *m2;
        Eina_List *n;

        //now we are setting the children
        EINA_LIST_FOREACH(m->redirects, n, m2)
          {
             m2->manager_tree_check.parent = m;
          }
     }
   eina_iterator_free(itr);

   //second stage, search for the root
   itr = eina_hash_iterator_data_new(managers);
   EINA_ITERATOR_FOREACH(itr, m)
     {
        if (!m->manager_tree_check.parent && !root)
          root = m;
        else if (!m->manager_tree_check.parent && root)
          ERR("Error, there is already a root manager (%p), but %p does not have any parents", root->ptr, m->ptr);
     }
   eina_iterator_free(itr);

   //thirds stage, check that this is really a tree
   if (_depth_calc(root, 1))
     INF("All managers are linked in the form of a tree");
}

EAPI void
clouseau_client_module_run(Eina_List *tree)
{
   Clouseau_Tree_Item *treeit;
   Eina_List *n;
   Eina_Iterator *managers_itr;
   Focus_Manager *manager;

   //move everything into a pointer hash
   EINA_LIST_FOREACH(tree, n, treeit)
     _add(treeit);

   //check that there are no islands in the graph of a manager
   managers_itr = eina_hash_iterator_data_new(managers);
   EINA_ITERATOR_FOREACH(managers_itr, manager)
     {
        _manager_check(manager);
     }

   //check that all managers are linked to at least one other manager
   _managers_link_check();

}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
