#define _GNU_SOURCE
#include <dlfcn.h>
#include <ffi.h>

#include <Eina.h>
#include <Eo.h>
#include <Eolian.h>
#include <Ecore_X.h>
#include <Evas.h>
#include <Elementary.h>
#define ELM_INTERNAL_API_ARGESFSDFEFC
#include <elm_widget.h>

#include "Clouseau_Debug.h"

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef DEBUG_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! DEBUG_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SWAP_64(x) x
#define SWAP_32(x) x
#define SWAP_16(x) x
#define SWAP_DBL(x) x
#else
#define SWAP_64(x) eina_swap64(x)
#define SWAP_32(x) eina_swap32(x)
#define SWAP_16(x) eina_swap16(x)
#define SWAP_DBL(x) SWAP_64(x)
#endif

#define STORE(_buf, pval, sz) \
{ \
   memcpy(_buf, pval, sz); \
   _buf += sz; \
}

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}

#define WRAPPER_TO_XFER_MAIN_LOOP(foo) \
static void \
_intern_main_loop ## foo(void *data) \
{ \
   Main_Loop_Info *info = data; \
   _main_loop ## foo(info->session, info->srcid, info->buffer, info->size); \
   free(info->buffer); \
   free(info); \
} \
static Eina_Bool \
foo(Eina_Debug_Session *session, int srcid, void *buffer, int size) \
{ \
   Main_Loop_Info *info = calloc(1, sizeof(*info)); \
   info->session = session; \
   info->srcid = srcid; \
   info->size = size; \
   if (info->size) \
     { \
        info->buffer = malloc(info->size); \
        memcpy(info->buffer, buffer, info->size); \
     } \
   ecore_main_loop_thread_safe_call_async(_intern_main_loop ## foo, info); \
   return EINA_TRUE; \
}

static int _snapshot_start_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_done_op = EINA_DEBUG_OPCODE_INVALID;
static int _klids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _eoids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_info_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_highlight_op = EINA_DEBUG_OPCODE_INVALID;
static int _win_screenshot_op = EINA_DEBUG_OPCODE_INVALID;

enum {
   HIGHLIGHT_R = 255,
   HIGHLIGHT_G = 128,
   HIGHLIGHT_B = 128,
   HIGHLIGHT_A = 255
};

typedef struct
{
   Eina_Debug_Session *session;
   int srcid;
   void *buffer;
   unsigned int size;
} Main_Loop_Info;

typedef struct
{
   Eolian_Type_Type etype;
   const char *name;
   Eolian_Debug_Basic_Type type;
   const char *print_format;
   void *ffi_type_p;//ffi_type
   const unsigned int size;
} Param_Type_Info;

const Param_Type_Info param_types[] =
{
     {EOLIAN_TYPE_REGULAR, "",  EOLIAN_DEBUG_INVALID_TYPE, "",   &ffi_type_pointer, 0},
     {EOLIAN_TYPE_COMPLEX, "pointer", EOLIAN_DEBUG_POINTER,  "%p", &ffi_type_pointer, 8},
     {EOLIAN_TYPE_COMPLEX, "char", EOLIAN_DEBUG_STRING,  "%s", &ffi_type_pointer, 8},
     {EOLIAN_TYPE_REGULAR, "char", EOLIAN_DEBUG_CHAR,  "%c", &ffi_type_uint, 1},
     {EOLIAN_TYPE_REGULAR, "int",  EOLIAN_DEBUG_INT, "%d", &ffi_type_sint, 4},
     {EOLIAN_TYPE_REGULAR, "short",  EOLIAN_DEBUG_SHORT,    "%d", &ffi_type_sint, 4},
     {EOLIAN_TYPE_REGULAR, "double", EOLIAN_DEBUG_DOUBLE,   "%f", &ffi_type_pointer, 8},
     {EOLIAN_TYPE_REGULAR, "bool", EOLIAN_DEBUG_BOOLEAN,     "%d", &ffi_type_uint, 1},
     {EOLIAN_TYPE_REGULAR, "long", EOLIAN_DEBUG_LONG,     "%f", &ffi_type_pointer, 8},
     {EOLIAN_TYPE_REGULAR, "uint", EOLIAN_DEBUG_UINT,     "%u", &ffi_type_uint, 4},
     {0, NULL, 0, NULL, NULL, 0}
};

static Eolian_Debug_Basic_Type
_eolian_type_resolve(const Eolian_Unit *unit, const Eolian_Type *eo_type)
{
   Eolian_Type_Type type = eolian_type_type_get(eo_type);
   Eolian_Type_Type type_base = type;

   if(type == EOLIAN_TYPE_COMPLEX)
     {
         eo_type = eolian_type_base_type_get(eo_type);
         type_base = eolian_type_type_get(eo_type);
     }

   if (type_base == EOLIAN_TYPE_REGULAR)
     {
        const char *full_name = eolian_type_full_name_get(eo_type);
        const Eolian_Typedecl *alias = eolian_typedecl_alias_get_by_name(unit, full_name);
        if (alias)
          {
            eo_type = eolian_typedecl_base_type_get(alias);
            type_base = eolian_type_type_get(eo_type);
            full_name = eolian_type_full_name_get(eo_type);
          }

        if(full_name)
          {
           int i;
           for (i = 0; param_types[i].name; i++)
              if (!strcmp(full_name, param_types[i].name) &&
                      param_types[i].etype == type)  return i;
          }
     }

   if (type == EOLIAN_TYPE_COMPLEX)
      return EOLIAN_DEBUG_POINTER;

   return EOLIAN_DEBUG_INVALID_TYPE;

}

static const Eolian_Class *
_class_find_by_name(const char *eo_klname, const Eolian_Unit **unit)
{
   const Eolian_Class *kl = NULL;
   if (!eo_klname) return NULL;
   char *klname = strdup(eo_klname);

   Eina_Strbuf *buf = eina_strbuf_new();
   eina_strbuf_append(buf, eo_klname);
   eina_strbuf_replace_all(buf, ".", "_");
   eina_strbuf_append(buf, ".eo");
   char *tmp = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   eina_str_tolower(&tmp);
   *unit = eolian_file_parse(tmp);
   free(tmp);

   tmp = klname;
   do
     {
        kl = eolian_class_get_by_name(*unit, klname);
        if (kl) goto end;
        while (*tmp && *tmp != '_') tmp++;
        if (*tmp) *tmp = '.';
     }
   while (*tmp);
   printf("Class %s not found.\n", klname);

end:
   free(klname);
   return kl;
}

static int
_function_invoke(Eo *ptr, const Eolian_Unit *unit, const Eolian_Function *foo, Eolian_Function_Type foo_type,
      Eolian_Debug_Parameter *params, Eolian_Debug_Return *ret)
{
   /* The params table contains the keys and the values.
    * This function doesn't allocate memory */
   ffi_type *types[EOLIAN_DEBUG_MAXARGS]; /* FFI types */
   void *values[EOLIAN_DEBUG_MAXARGS];    /* FFI Values */
   void *pointers[EOLIAN_DEBUG_MAXARGS];  /* Used as values for out params, as we have to give a pointer to a pointer */
   Eolian_Function_Parameter *eo_param;
   Eina_Iterator *itr;
   Eolian_Debug_Basic_Type ed_type;
   int argc, ffi_argc = 0;

   if (!foo) return -1;
   if (foo_type == EOLIAN_PROPERTY) return -1;

   /* Eo object storage */
   types[ffi_argc] = &ffi_type_pointer;
   values[ffi_argc] = &ptr;
   ffi_argc++;

   itr = eolian_property_keys_get(foo, foo_type);
   argc = 0;
   EINA_ITERATOR_FOREACH(itr, eo_param)
     {
        /* Not verified */
        ed_type = _eolian_type_resolve(unit, eolian_parameter_type_get(eo_param));
        if (!ed_type) goto error;

        types[ffi_argc] = param_types[ed_type].ffi_type_p;
        values[ffi_argc] = &(params[argc].value.value.value);
        ffi_argc++;
        argc++;
     }

   itr = foo_type == EOLIAN_METHOD ? eolian_function_parameters_get(foo) :
      eolian_property_values_get(foo, foo_type);
   EINA_ITERATOR_FOREACH(itr, eo_param)
     {
        ed_type = _eolian_type_resolve(unit, eolian_parameter_type_get(eo_param));
        if (!ed_type) goto error;

        if (foo_type == EOLIAN_PROP_GET ||
              (foo_type == EOLIAN_METHOD && eolian_parameter_direction_get(eo_param) == EOLIAN_OUT_PARAM))
          {
             /* Out parameter */
             params[argc].value.value.value = 0;
             params[argc].value.type = ed_type;
             types[ffi_argc] = &ffi_type_pointer;
             pointers[ffi_argc] = &(params[argc].value.value.value);
             values[ffi_argc] = &pointers[ffi_argc];
          }
        else
          {
             /* In parameter */
             types[ffi_argc] = param_types[ed_type].ffi_type_p;
             values[ffi_argc] = &(params[argc].value.value.value);
          }
        ffi_argc++;
        argc++;
     }

   const Eolian_Type *eo_type = eolian_function_return_type_get(foo, foo_type);
   ffi_type *ffi_ret_type = &ffi_type_void;
   if (eo_type)
     {
        ed_type = _eolian_type_resolve(unit, eo_type);
        if (!ed_type) goto error;

        ffi_ret_type = param_types[ed_type].ffi_type_p;
     }
   else if (argc == 1 && foo_type == EOLIAN_PROP_GET)
     {
        /* If there is no return type but only one value is present, the value will
         * be returned by the function.
         * So we need FFI to not take it into account when invoking the function.
         */
        ffi_ret_type = param_types[params[0].value.type].ffi_type_p;
        ffi_argc--;
     }

   const char *full_func_name = eolian_function_full_c_name_get(foo,  EOLIAN_PROP_GET, EINA_FALSE);
   void *eo_func = dlsym(RTLD_DEFAULT, full_func_name);
   if (!eo_func) goto error;
   ffi_cif cif;
   if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, ffi_argc,
            ffi_ret_type, types) == FFI_OK)
     {
        void *result;
        ffi_call(&cif, eo_func, &result, values);
        if (ret) ret->value.type = EOLIAN_DEBUG_VOID;
        if (eo_type)
          {
             if (ret)
               {
                  ret->value.value.value = (uint64_t) result;
                  ret->value.type = ed_type;
               }
          }
        else if (argc == 1 && foo_type == EOLIAN_PROP_GET)
           params[0].value.value.value = (uint64_t) result;
        goto success;
     }

error:
   argc = -1;
success:
   eina_iterator_free(itr);
   return argc;
}

static Eina_Bool
_eolian_function_is_implemented(
      const Eolian_Function *function_id, Eolian_Function_Type func_type,
      const Eolian_Unit *unit, const Eolian_Class *klass)
{
   Eina_Iterator *impl_itr = NULL;
   Eolian_Function_Type found_type = EOLIAN_UNRESOLVED;
   Eina_Bool found = EINA_TRUE;
   if (!function_id || !klass) return EINA_FALSE;
   Eina_List *list = eina_list_append(NULL, klass), *list2, *itr;
   EINA_LIST_FOREACH(list, itr, klass)
     {
        const char *inherit_name;
        const Eolian_Implement *impl;
        if (eolian_class_type_get(klass) == EOLIAN_CLASS_INTERFACE) continue;
        impl_itr = eolian_class_implements_get(klass);
        EINA_ITERATOR_FOREACH(impl_itr, impl)
          {
             if (eolian_implement_is_pure_virtual(impl, EOLIAN_PROP_SET)) continue;
             Eolian_Function_Type impl_type = EOLIAN_UNRESOLVED;
             const Eolian_Function *impl_func = eolian_implement_function_get(impl, &impl_type);
             if (impl_func == function_id)
               {
                  /* The type matches the requested or is not important for the caller */
                  if (func_type == EOLIAN_UNRESOLVED || impl_type == func_type) goto end;
                  if (impl_type == EOLIAN_METHOD) continue;
                  /* In case we search for a property type */
                  if (impl_type == EOLIAN_PROPERTY &&
                        (func_type == EOLIAN_PROP_GET || func_type == EOLIAN_PROP_SET))
                     goto end;
                  /* Property may be splitted on multiple implements */
                  if (func_type == EOLIAN_PROPERTY)
                    {
                       if (found_type == EOLIAN_UNRESOLVED) found_type = impl_type;
                       if ((found_type == EOLIAN_PROP_SET && impl_type == EOLIAN_PROP_GET) ||
                             (found_type == EOLIAN_PROP_GET && impl_type == EOLIAN_PROP_SET))
                          goto end;
                    }
               }
          }
        eina_iterator_free(impl_itr);
        impl_itr = NULL;

        Eina_Iterator *inherits_itr = eolian_class_inherits_get(klass);
        EINA_ITERATOR_FOREACH(inherits_itr, inherit_name)
          {
             const Eolian_Class *inherit = eolian_class_get_by_name(unit, inherit_name);
             /* Avoid duplicates. */
             if (!eina_list_data_find(list, inherit))
               {
                  list2 = eina_list_append(list, inherit);
               }
          }
        eina_iterator_free(inherits_itr);
     }
   (void) list2;
   found = EINA_FALSE;
end:
   if (impl_itr) eina_iterator_free(impl_itr);
   eina_list_free(list);
   return found;
}

static unsigned int
_class_buffer_fill(Eo *obj, const Eolian_Unit *unit, const Eolian_Class *ekl, char *buf)
{
   unsigned int size = 0;
   Eina_Iterator *funcs = eolian_class_functions_get(ekl, EOLIAN_PROPERTY);
   const Eolian_Function *func;
   EINA_ITERATOR_FOREACH(funcs, func)
     {
        if (eolian_function_type_get(func) == EOLIAN_PROP_SET ||
              !_eolian_function_is_implemented(func, EOLIAN_PROP_GET, unit, ekl)) continue;
        Eina_Iterator *keys_itr = eolian_property_keys_get(func, EOLIAN_PROP_GET);
        eina_iterator_free(keys_itr);
        /* We dont support functions with key parameters */
        if (keys_itr) continue;

        Eolian_Debug_Parameter params[EOLIAN_DEBUG_MAXARGS];
        Eolian_Debug_Return ret;
        int argnum = _function_invoke(obj, unit, func, EOLIAN_PROP_GET, params, &ret);

        if (argnum == -1) continue;

        int len, i;
        if (!size) // only if its the first func to succeed
          {
             const char *class_name = eolian_class_full_name_get(ekl);
             len = strlen(class_name) + 1;
             memcpy(buf, class_name, len);
             size += len;
          }
        else
          {
             //write zero length string to mark that it's still the same class
             buf[size] = '\0';
             size++;
          }
        const char *func_name = eolian_function_name_get(func);
        len = strlen(func_name) + 1;
        memcpy(buf + size, func_name, len);
        size += len;
        for (i = 0; i < argnum; i++) //print params
          {
             // if its a string we wont copy the pointer but the value
             if (params[i].value.type == EOLIAN_DEBUG_STRING)
               {
                  if((char *)params[i].value.value.value == NULL)
                     params[i].value.value.value = (uint64_t)"";
                  len = strlen((char *)params[i].value.value.value) + 1;
                  memcpy(buf + size, (char *)params[i].value.value.value, len);
                  size += len;
               }
             else
               {
                  if (param_types[params[i].value.type].size == 8)
                    {
                       uint64_t value = SWAP_64(params[i].value.value.value);
                       memcpy(buf + size, &value, 8);
                    }
                  else if (param_types[params[i].value.type].size == 4)
                    {
                       int value = SWAP_32(params[i].value.value.value);
                       memcpy(buf + size, &value, 4);
                    }
                  else
                    {
                       memcpy(buf + size, &(params[i].value.value.value),
                             param_types[params[i].value.type].size);
                    }
                  size += param_types[params[i].value.type].size;
               }
          }
        /*
           For Eolian funcs that dont define "pram type" for their
           return value we dont have part of the info that we have on
           normal Eolian param (such as name). Also the return value wont
           be in the function params iterator - hence argnum = 0. For that
           case we use Eolian_Debug_Return struct
           */
        if (ret.value.type != EOLIAN_DEBUG_VOID)
          {
             //if its a string we wont copy the pointer but the values
             if (ret.value.type == EOLIAN_DEBUG_STRING)
               {
                  if((char *)ret.value.value.value == NULL)
                     ret.value.value.value =  (uint64_t)"";
                  len = strlen((char *)ret.value.value.value) + 1;
                  memcpy(buf + size, (char *)ret.value.value.value, len);
                  size += len;
               }
             else
               {
                  if (param_types[ret.value.type].size == 8)
                    {
                       uint64_t value = SWAP_64(ret.value.value.value);
                       memcpy(buf + size, &value, 8);
                    }
                  else if (param_types[ret.value.type].size == 4)
                    {
                       int value = SWAP_32(ret.value.value.value);
                       memcpy(buf + size, &value, 4);
                    }
                  else
                    {
                       memcpy(buf + size, &(ret.value.value.value),
                             param_types[ret.value.type].size);
                    }
                  size += param_types[ret.value.type].size;
               }
          }

     }
   eina_iterator_free(funcs);
   return size;
}

static Eina_Bool
_obj_info_req_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size EINA_UNUSED)
{
   uint64_t ptr64;
   memcpy(&ptr64, buffer, sizeof(ptr64));
   Eo *obj = (Eo *)SWAP_64(ptr64);

   const char *class_name = efl_class_name_get(obj);
   const Eolian_Unit *unit = NULL;
   const Eolian_Class *kl = _class_find_by_name(class_name, &unit);
   char *buf;
   unsigned int size_curr = 0;
   if (!kl)
     {
        printf("Class %s not found.\n", class_name);
        goto end;
     }
   buf = malloc(100000);
   memcpy(buf, &ptr64, sizeof(uint64_t));
   size_curr = sizeof(uint64_t);

   Eina_List *itr, *list2;
   Eina_List *list = eina_list_append(NULL, kl);
   EINA_LIST_FOREACH(list, itr, kl)
     {
        const char *inherit_name;
        Eina_Iterator *inherits_itr = eolian_class_inherits_get(kl);

        size_curr += _class_buffer_fill(obj, unit, kl, buf + size_curr);
        EINA_ITERATOR_FOREACH(inherits_itr, inherit_name)
          {
             const Eolian_Class *inherit = eolian_class_get_by_name(unit, inherit_name);
             if (!inherit) printf("class not found for name: \"%s\"", inherit_name);
             /* Avoid duplicates in MRO list. */
             if (!eina_list_data_find(list, inherit))
                list2 = eina_list_append(list, inherit);
          }
        eina_iterator_free(inherits_itr);
     }
   (void) list2;

   eina_debug_session_send(session, srcid, _obj_info_op, buf, size_curr);
   free(buf);

end:
   return EINA_TRUE;
}

static Eina_Bool
_snapshot_objs_get_req_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size)
{
   Eina_Iterator *iter;
   Eina_List *itr;
   char *buf, *tmp;
   Eo *obj;
   Eina_List *objs = NULL;
   uint64_t *kls = buffer;
   int nb_kls = size / sizeof(uint64_t), i;

   for (i = 0; i < nb_kls; i++) kls[i] = SWAP_64(kls[i]);
   iter = eo_objects_iterator_new();
   EINA_ITERATOR_FOREACH(iter, obj)
     {
        Eina_Bool klass_ok = EINA_FALSE;
        for (i = 0; i < nb_kls && !klass_ok; i++)
          {
             if (efl_isa(obj, (Efl_Class *)kls[i])) klass_ok = EINA_TRUE;
             if (klass_ok || !nb_kls) objs = eina_list_append(objs, obj);
          }
     }
   eina_iterator_free(iter);

   size = eina_list_count(objs) * 3 * sizeof(uint64_t);
   buf = tmp = malloc(size);
   EINA_LIST_FOREACH(objs, itr, obj)
   {
      Eo *parent;
      uint64_t u64 = SWAP_64((uint64_t)obj);
      STORE(tmp, &u64, sizeof(u64));
      u64 = SWAP_64((uint64_t)efl_class_get(obj));
      STORE(tmp, &u64, sizeof(u64));
      parent = elm_object_parent_widget_get(obj);
      if (!parent && efl_isa(obj, EFL_CANVAS_OBJECT_CLASS))
        {
           parent = evas_object_data_get(obj, "elm-parent");
           if (!parent) parent = evas_object_smart_parent_get(obj);
        }
      if (!parent) parent = efl_parent_get(obj);
      u64 = SWAP_64((uint64_t)parent);
      STORE(tmp, &u64, sizeof(u64));
   }
   eina_debug_session_send(session, srcid, _eoids_get_op, buf, size);
   free(buf);

   EINA_LIST_FREE(objs, obj)
     {
        uint64_t u64 = SWAP_64((uint64_t)obj);
        _obj_info_req_cb(session, srcid, &u64, sizeof(uint64_t));
     }
   return EINA_TRUE;
}

static void
_main_loop_snapshot_start_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size)
{
   char *all_kls_buf;
   char *tmp;
   Eina_Iterator *iter;
   Efl_Class *kl;
   uint64_t *kls;
   Eina_List *kls_strs = NULL;
   int nb_kls = 0;

   tmp = buffer;
   while (size > 0)
     {
        kls_strs = eina_list_append(kls_strs, tmp);
        size -= strlen(tmp) + 1;
        tmp += strlen(tmp) + 1;
        nb_kls++;
     }
   size = nb_kls * sizeof(uint64_t);
   if (size)
     {
        kls = alloca(size);
        memset(kls, 0, size);
     }
   else kls = NULL;

   all_kls_buf = tmp = malloc(10000);
   iter = eo_classes_iterator_new();
   EINA_ITERATOR_FOREACH(iter, kl)
     {
        Eina_List *itr;
        const char *kl_name = efl_class_name_get(kl), *kl_str;
        int len = strlen(kl_name) + 1;
        uint64_t u64 = SWAP_64((uint64_t)kl);
        Eina_Bool found = EINA_FALSE;

        /* Fill the buffer with class id/name */
        STORE(tmp, &u64, sizeof(u64));
        STORE(tmp, kl_name, len);

        /* Check for filtering */
        if (!nb_kls) continue;
        EINA_LIST_FOREACH(kls_strs, itr, kl_str)
          {
             if (found) continue;
             if (!strcmp(kl_name, kl_str))
               {
                  int i;
                  for (i = 0; i < nb_kls && !found; i++)
                    {
                       if (!kls[i])
                         {
                            kls[i] = (uint64_t)kl;
                            found = EINA_TRUE;
                         }
                    }
               }
          }
     }
   eina_iterator_free(iter);

   eina_debug_session_send(session, srcid,
         _klids_get_op, all_kls_buf, tmp - all_kls_buf);
   free(all_kls_buf);

   _snapshot_objs_get_req_cb(session, srcid, kls, size);

   eina_debug_session_send(session, srcid, _snapshot_done_op, NULL, 0);
}

WRAPPER_TO_XFER_MAIN_LOOP(_snapshot_start_cb)

/* Highlight functions. */
static Eina_Bool
_obj_highlight_fade(void *_rect)
{
   Evas_Object *rect = _rect;
   int r, g, b, a;
   double na;

   evas_object_color_get(rect, &r, &g, &b, &a);
   if (a < 20)
     {
        evas_object_del(rect);
        /* The del callback of the object will destroy the animator */
        return EINA_TRUE;
     }

   na = a - 5;
   r = na / a * r;
   g = na / a * g;
   b = na / a * b;
   evas_object_color_set(rect, r, g, b, na);

   return EINA_TRUE;
}

static void
_obj_highlight_del(void *data,
      EINA_UNUSED Evas *e,
      EINA_UNUSED Evas_Object *obj,
      EINA_UNUSED void *event_info)
{  /* Delete timer for this rect */
   ecore_animator_del(data);
}

static void
_main_loop_obj_highlight_cb(Eina_Debug_Session *session EINA_UNUSED, int srcid EINA_UNUSED, void *buffer, int size)
{
   uint64_t ptr64;
   if (size != sizeof(uint64_t)) return;
   memcpy(&ptr64, buffer, sizeof(ptr64));
   Eo *obj = (Eo *)SWAP_64(ptr64);
   if (!efl_isa(obj, EFL_CANVAS_OBJECT_CLASS) && !efl_isa(obj, EVAS_CANVAS_CLASS)) return;
   Evas *e = evas_object_evas_get(obj);
   Eo *rect = evas_object_polygon_add(e);
   evas_object_move(rect, 0, 0);
   if (efl_isa(obj, EFL_CANVAS_OBJECT_CLASS))
     {
        Evas_Coord x = 0, y = 0, w = 0, h = 0;
        evas_object_geometry_get(obj, &x, &y, &w, &h);
        if (efl_isa(obj, EFL_UI_WIN_CLASS)) x = y = 0;

        evas_object_polygon_point_add(rect, x, y);
        evas_object_polygon_point_add(rect, x + w, y);
        evas_object_polygon_point_add(rect, x + w, y + h);
        evas_object_polygon_point_add(rect, x, y + h);
     }
   else
     {
        Evas_Coord w = 0, h = 0;
        evas_output_size_get(obj, &w, &h);

        evas_object_polygon_point_add(rect, 0, 0);
        evas_object_polygon_point_add(rect, w, 0);
        evas_object_polygon_point_add(rect, w, h);
        evas_object_polygon_point_add(rect, 0, h);
     }

   /* Put the object as high as possible. */
   evas_object_layer_set(rect, EVAS_LAYER_MAX);
   evas_object_color_set(rect, HIGHLIGHT_R, HIGHLIGHT_G, HIGHLIGHT_B, HIGHLIGHT_A);
   evas_object_show(rect);

   /* Add Timer for fade and a callback to delete timer on obj del */
   Ecore_Animator *t = ecore_animator_add(_obj_highlight_fade, rect);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_DEL, _obj_highlight_del, t);
}

WRAPPER_TO_XFER_MAIN_LOOP(_obj_highlight_cb)

static void
_main_loop_win_screenshot_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size)
{
   struct tm *t = NULL;
   time_t now = time(NULL);
   uint64_t ptr64 = 0;
   Ecore_Evas *ee = NULL;
   Ecore_X_Image *img;
   Ecore_X_Window_Attributes att;
   unsigned char *img_src;
   unsigned char *resp = NULL, *tmp;
   int bpl = 0, rows = 0, bpp = 0;
   int w, h, val;
   unsigned int hdr_size = sizeof(uint64_t) + 5 * sizeof(int);

   if (size != sizeof(uint64_t)) return;
   memcpy(&ptr64, buffer, sizeof(ptr64));
   Eo *e = (Eo *)SWAP_64(ptr64);
   if (!efl_isa(e, EVAS_CANVAS_CLASS)) goto end;

   ee = ecore_evas_ecore_evas_get(e);
   if (!ee) goto end;

   Ecore_X_Window win = (Ecore_X_Window) ecore_evas_window_get(ee);

   if (!win)
     {
        printf("Can't grab window.\n");
        goto end;
     }

   t = localtime(&now);
   t->tm_zone = NULL;

   memset(&att, 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(win, &att);
   w = att.w;
   h = att.h;
   img = ecore_x_image_new(w, h, att.visual, att.depth);
   ecore_x_image_get(img, win, 0, 0, 0, 0, w, h);
   img_src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
   resp = tmp = malloc(hdr_size + (w * h * sizeof(int)));
   t->tm_sec = SWAP_32(t->tm_sec);
   t->tm_min = SWAP_32(t->tm_min);
   t->tm_hour = SWAP_32(t->tm_hour);
   STORE(tmp, &ptr64, sizeof(ptr64));
   STORE(tmp, &t->tm_sec, sizeof(int));
   STORE(tmp, &t->tm_min, sizeof(int));
   STORE(tmp, &t->tm_hour, sizeof(int));
   val = SWAP_32(w);
   STORE(tmp, &val, sizeof(int));
   val = SWAP_32(h);
   STORE(tmp, &val, sizeof(int));
   if (!ecore_x_image_is_argb32_get(img))
     {  /* Fill resp buffer with image convert */
        ecore_x_image_to_argb_convert(img_src, bpp, bpl, att.colormap, att.visual,
              0, 0, w, h, (unsigned int *)tmp,
              (w * sizeof(int)), 0, 0);
     }
   else
     {  /* Fill resp buffer by copy */
        memcpy(tmp, img_src, (w * h * sizeof(int)));
     }

   /* resp now holds window bitmap */
   ecore_x_image_free(img);

   eina_debug_session_send(session, srcid, _win_screenshot_op, resp,
         hdr_size + (w * h * sizeof(int)));

end:
   if (resp) free(resp);
}

WRAPPER_TO_XFER_MAIN_LOOP(_win_screenshot_cb)

EINA_DEBUG_OPCODES_ARRAY_DEFINE(_debug_ops,
     {"Clouseau/Object_Introspection/snapshot_start", &_snapshot_start_op, &_snapshot_start_cb},
     {"Clouseau/Object_Introspection/snapshot_done", &_snapshot_done_op, NULL},
     {"Clouseau/Eo/classes_ids_get", &_klids_get_op, NULL},
     {"Clouseau/Eo/objects_ids_get", &_eoids_get_op, NULL},
     {"Clouseau/Eolian/object/info_get", &_obj_info_op, &_obj_info_req_cb},
     {"Clouseau/Evas/object/highlight", &_obj_highlight_op, &_obj_highlight_cb},
     {"Clouseau/Evas/window/screenshot", &_win_screenshot_op, &_win_screenshot_cb},
     {NULL, NULL, NULL}
);

EAPI Eina_Bool
clouseau_debug_init(void)
{
   eina_init();
   eolian_init();
   evas_init();

   eolian_system_directory_scan();

   eina_debug_opcodes_register(NULL, _debug_ops(), NULL, NULL);

   printf("%s - In\n", __FUNCTION__);
   return EINA_TRUE;
}

EAPI Eina_Bool
clouseau_debug_shutdown(void)
{
   evas_shutdown();
   eolian_shutdown();
   eina_shutdown();

   return EINA_TRUE;
}

EAPI void *
eo_debug_eoids_request_prepare(int *size, ...)
{
   va_list list;
   uint64_t kl;
   int nb_kls = 0, max_kls = 0;
   char *buf = NULL;
   va_start(list, size);
   kl = va_arg(list, uint64_t);
   while (kl)
     {
        nb_kls++;
        if (max_kls < nb_kls)
          {
             max_kls += 10;
             buf = realloc(buf, max_kls * sizeof(uint64_t));
          }
        char *tmp = buf + (nb_kls-1) * sizeof(uint64_t);
        kl = SWAP_64(kl);
        STORE(tmp, &kl, sizeof(uint64_t));
        kl = va_arg(list, uint64_t);
     }
   va_end(list);
   *size = nb_kls * sizeof(uint64_t);
   return buf;
}

EAPI void
eo_debug_eoids_extract(void *buffer, int size, Eo_Debug_Object_Extract_Cb cb, void *data)
{
   if (!buffer || !size || !cb) return;
   char *buf = buffer;

   while (size > 0)
     {
        uint64_t obj, kl, parent;
        EXTRACT(buf, &obj, sizeof(uint64_t));
        EXTRACT(buf, &kl, sizeof(uint64_t));
        EXTRACT(buf, &parent, sizeof(uint64_t));
        cb(data, SWAP_64(obj), SWAP_64(kl), SWAP_64(parent));
        size -= (3 * sizeof(uint64_t));
     }
}

EAPI void
eo_debug_klids_extract(void *buffer, int size, Eo_Debug_Class_Extract_Cb cb, void *data)
{
   if (!buffer || !size || !cb) return;
   char *buf = buffer;
   while (size > 0)
     {
        uint64_t kl;
        char *name;
        EXTRACT(buf, &kl, sizeof(uint64_t));
        name = buf;
        cb(data, SWAP_64(kl), buf);
        buf += (strlen(name) + 1);
        size -= (strlen(name) + 1 + sizeof(uint64_t));
     }
}

EAPI void
eolian_debug_object_information_free(Eolian_Debug_Object_Information *main)
{
   Eolian_Debug_Class *kl;
   if (!main) return;
   EINA_LIST_FREE(main->classes, kl)
     {
        Eolian_Debug_Function *func;
        EINA_LIST_FREE(kl->functions, func)
          {
             Eolian_Debug_Parameter *param;
             EINA_LIST_FREE(func->params, param)
               {
                  if (param->value.type == EOLIAN_DEBUG_STRING)
                     eina_stringshare_del((char *)param->value.value.value);
                  free(param);
               }
             free(func);
          }
        free(kl);
     }
   free(main);
}

/*
 * receive buffer of the following format:
 * Eo *pointer (uint64_t)
 * followed by list of
 * class_name, function. if class_name='\0' this function belongs to last correct class_name
 */
EAPI Eolian_Debug_Object_Information *
eolian_debug_object_information_decode(char *buffer, unsigned int size)
{
   if (size < sizeof(uint64_t)) return NULL;
   Eolian_Debug_Object_Information *ret = calloc(1, sizeof(*ret));
   Eolian_Debug_Class *kl = NULL;

   //get the Eo* pointer
   EXTRACT(buffer, &(ret->obj), sizeof(uint64_t));
   ret->obj = SWAP_64(ret->obj);
   size -= sizeof(uint64_t);

   while (size > 0)
     {
        Eolian_Debug_Function *func;
        Eolian_Function_Parameter *eo_param;
        Eina_Iterator *itr;
        int len = strlen(buffer) + 1;
        if (len > 1) // if class_name is not NULL, we begin a new class
          {
             kl = calloc(1, sizeof(*kl));
             kl->ekl = _class_find_by_name(buffer, &(kl->unit));
             ret->classes = eina_list_append(ret->classes, kl);
          }
        buffer += len;
        size -= len;
        if (!kl) goto error;

        func = calloc(1, sizeof(*func));
        printf("Class name = %s function = %s\n", eolian_class_name_get(kl->ekl), buffer);
        kl->functions = eina_list_append(kl->functions, func);
        func->efunc = eolian_class_function_get_by_name(kl->ekl, buffer, EOLIAN_PROP_GET);
        if(!func->efunc)
          {
             printf("Function %s not found!\n", buffer);
             goto error;
          }
        len = strlen(buffer) + 1;
        buffer += len;
        size -= len;

        //go over function params using eolian
        itr = eolian_property_values_get(func->efunc, EOLIAN_PROP_GET);
        EINA_ITERATOR_FOREACH(itr, eo_param)
          {
             Eolian_Debug_Basic_Type type = _eolian_type_resolve(kl->unit,
                   eolian_parameter_type_get(eo_param));

             if (type)
               {
                  Eolian_Debug_Parameter *p = calloc(1, sizeof(*p));
                  p->eparam = eo_param;
                  p->value.type = type;
                  if (type == EOLIAN_DEBUG_STRING)
                    {
                       len = strlen(buffer) + 1;
                       p->value.value.value = (uint64_t) eina_stringshare_add(buffer);
                       buffer += len;
                       size -= len;
                    }
                  else
                    {
                       uint64_t value;
                       EXTRACT(buffer, &value, param_types[type].size);
                       p->value.value.value = SWAP_64(value);
                       size -= param_types[type].size;
                    }
                  func->params = eina_list_append(func->params, p);
               }
             else goto error;
          }
        func->ret.etype = eolian_function_return_type_get(
              func->efunc, EOLIAN_PROP_GET);
        if(func->ret.etype)
          {
             Eolian_Debug_Basic_Type type = _eolian_type_resolve(kl->unit, func->ret.etype);
             if (type)
               {
                  func->ret.value.type = type;
                  if (type == EOLIAN_DEBUG_STRING)
                    {
                       len = strlen(buffer) + 1;
                       func->ret.value.value.value = (uint64_t) eina_stringshare_add(buffer);
                       buffer += len;
                       size -= len;
                    }
                  else
                    {
                       uint64_t value;
                       EXTRACT(buffer, &value, param_types[type].size);
                       func->ret.value.value.value = SWAP_64(value);
                       size -= param_types[type].size;
                    }
               }
             else goto error;
          }
        eina_iterator_free(itr);
     }
   return ret;

error:
   eolian_debug_object_information_free(ret);
   return NULL;
}

EAPI Evas_Debug_Screenshot *
evas_debug_screenshot_decode(char *buffer, unsigned int size)
{
   Evas_Debug_Screenshot *s = NULL;
   uint64_t obj;
   int tm_sec, tm_min, tm_hour, w, h;
   unsigned int hdr_size = sizeof(uint64_t) + 5 * sizeof(int);
   if (size < hdr_size) return NULL;
   EXTRACT(buffer, &obj, sizeof(uint64_t));
   EXTRACT(buffer, &tm_sec, sizeof(int));
   EXTRACT(buffer, &tm_min, sizeof(int));
   EXTRACT(buffer, &tm_hour, sizeof(int));
   EXTRACT(buffer, &w, sizeof(int));
   EXTRACT(buffer, &h, sizeof(int));
   size -= hdr_size;
   if (size != (w * h * sizeof(int))) return NULL;

   s = calloc(1, sizeof(*s));
   s->obj = SWAP_64(obj);
   s->w = SWAP_32(w);
   s->h = SWAP_32(h);
   s->tm_sec = SWAP_32(tm_sec);
   s->tm_min = SWAP_32(tm_min);
   s->tm_hour = SWAP_32(tm_hour);
   s->img = malloc(size);
   s->img_size = size;
   memcpy(s->img, buffer, size);
   return s;
}
