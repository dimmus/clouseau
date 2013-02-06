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
        _my_app_name = "clouseau";
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
typedef void (*Eina_Lock_Bt_Func) ();

EAPI Evas_Object *
evas_object_new(Evas *e)
{
   Eina_Lock_Bt_Func lock_bt[EINA_LOCK_DEBUG_BT_NUM];
   int lock_bt_num;
   Evas_Object *(*_evas_object_new)(Evas *e) = dlsym(RTLD_NEXT, __func__);
   Eina_Strbuf *str;
   Evas_Object *r;
   char **strings;
   int i;

   r = _evas_object_new(e);
   if (!r) return NULL;

   lock_bt_num = backtrace((void **)lock_bt, EINA_LOCK_DEBUG_BT_NUM);
   strings = backtrace_symbols((void **)lock_bt, lock_bt_num);

   str = eina_strbuf_new();

   for (i = 1; i < lock_bt_num; ++i)
     eina_strbuf_append_printf(str, "%s\n", strings[i]);

   evas_object_data_set(r, ".clouseau.bt", eina_stringshare_add(eina_strbuf_string_get(str)));

   free(strings);
   eina_strbuf_free(str);

   return r;
}

EAPI void
evas_object_free(Evas_Object *obj, int clean_layer)
{
   void (*_evas_object_free)(Evas_Object *obj, int clean_layer) = dlsym(RTLD_NEXT, __func__);
   const char *tmp;

   tmp = evas_object_data_get(obj, ".clouseau.bt");
   eina_stringshare_del(tmp);

   _evas_object_free(obj, clean_layer);
}
