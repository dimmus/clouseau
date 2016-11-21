#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <dlfcn.h>
#include <execinfo.h>

#include "Clouseau.h"

static Eina_Bool _elm_is_init = EINA_FALSE;
static const char *_my_app_name = NULL;

/** PRELOAD functions. */

/* Hook on the elm_init
 * We only do something here if we didn't already go into elm_init,
 * which probably means we are not using elm. */
EAPI int
elm_init(int argc, char **argv)
{
   int (*_elm_init)(int, char **) = dlsym(RTLD_NEXT, __func__);

   setenv("ELM_CLOUSEAU", "0", 1);

   if (!_elm_is_init)
     {
        _my_app_name = argv[0];
        _elm_is_init = EINA_TRUE;
     }

   return _elm_init(argc, argv);
}

/* Hook on the main loop
 * We only do something here if we didn't already go into elm_init,
 * which probably means we are not using elm. */
EAPI void
ecore_main_loop_begin(void)
{
   void (*_ecore_main_loop_begin)(void) = dlsym(RTLD_NEXT, __func__);

   if (!_elm_is_init)
     {
        _my_app_name = "unknown";
     }

   clouseau_init();

   if(!clouseau_app_connect(_my_app_name))
     {
        printf("Failed to connect to server.\n");
        return;
     }

   _ecore_main_loop_begin();

   clouseau_shutdown();

   return;
}

#define EINA_LOCK_DEBUG_BT_NUM 64

static void
_clouseau_efl_object_dbg_info_get(Eo *obj, void *pd EINA_UNUSED, Efl_Dbg_Info *root)
{
   const char *bt;
   Efl_Dbg_Info *creation;

   bt = efl_key_data_get(obj, ".clouseau.bt");
   creation = EFL_DBG_INFO_LIST_APPEND(root, "Creation");
   EFL_DBG_INFO_APPEND(creation, "Backtrace", EINA_VALUE_TYPE_STRING, bt);

   efl_dbg_info_get(efl_super(obj, EFL_OBJECT_OVERRIDE_CLASS), root);
}
static void
_clouseau_efl_destructor(Eo *obj, void *pd EINA_UNUSED)
{
   char *bt;

   bt = efl_key_data_get(obj, ".clouseau.bt");
   free(bt);

   efl_destructor(efl_super(obj, EFL_OBJECT_OVERRIDE_CLASS));
}

static char*
_clouseau_backtrace_create(void)
{
   void *bt[EINA_LOCK_DEBUG_BT_NUM];
   Eina_Strbuf *buf;
   int lock_bt_num;

   lock_bt_num = backtrace((void **)bt, EINA_LOCK_DEBUG_BT_NUM);
   buf = eina_strbuf_new();
   for (int i = 0; i < lock_bt_num; ++i)
     {
        Dl_info info;
        const char *file;
        unsigned long long offset, base;

        if ((dladdr(bt[i], &info)) && (info.dli_fname[0]))
          {
             offset = (unsigned long long)bt[i];
             base = (unsigned long long)info.dli_fbase;
             file = info.dli_fname;
          }
        if (file)
          eina_strbuf_append_printf(buf, "%s\t 0x%llx 0x%llx\n", file, offset, base);
        else
          eina_strbuf_append_printf(buf, "??\t -\n");
     }

   {
      char *str;

      str = eina_strbuf_string_steal(buf);
      eina_strbuf_free(buf);

      return str;
   }
}

EAPI Eo*
_efl_add_internal_start(const char *file, int line, const Efl_Class *klass_id, Eo *parent_id, Eina_Bool ref, Eina_Bool is_fallback)
{
   Eo* (*__efl_add_internal_start)(const char *file, int line, const Efl_Class *klass_id, Eo *parent_id, Eina_Bool ref EINA_UNUSED, Eina_Bool is_fallback);
   const char *str;
   Eo *obj;

   //create bt
   str = _clouseau_backtrace_create();

   //containue construction
   __efl_add_internal_start = dlsym(RTLD_NEXT, __func__);
   obj = __efl_add_internal_start(file, line, klass_id, parent_id, ref, is_fallback);

   //override needed functions
   EFL_OPS_DEFINE(ops,
    EFL_OBJECT_OP_FUNC(efl_dbg_info_get, _clouseau_efl_object_dbg_info_get),
    EFL_OBJECT_OP_FUNC(efl_destructor, _clouseau_efl_destructor)
   );
   efl_object_override(obj, &ops);

   //add backtrace and free buf
   efl_key_data_set(obj, ".clouseau.bt", str);

   return obj;
}