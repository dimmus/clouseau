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


static int _snapshot_start_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_done_op = EINA_DEBUG_OPCODE_INVALID;
static int _klids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _eoids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_info_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_highlight_op = EINA_DEBUG_OPCODE_INVALID;
static int _win_screenshot_op = EINA_DEBUG_OPCODE_INVALID;
static int _focus_manager_list_op = EINA_DEBUG_OPCODE_INVALID;
static int _focus_manager_detail_op = EINA_DEBUG_OPCODE_INVALID;

static Eolian_State *eos = NULL;

static Eet_Data_Descriptor *manager_details = NULL, *manager_list = NULL;
#include "clouseau_focus_serialization.x"

enum {
   HIGHLIGHT_R = 255,
   HIGHLIGHT_G = 128,
   HIGHLIGHT_B = 128,
   HIGHLIGHT_A = 255
};

typedef struct
{
   Eolian_Type_Type etype;
   const char *name;
   Eolian_Debug_Basic_Type type;
} Eolian_Param_Info;

typedef struct
{
   Eolian_Debug_Basic_Type type;
   const char *print_format;
   void *ffi_type_p;//ffi_type
   const unsigned int size;
} Debug_Param_Info;

const Eolian_Param_Info eolian_types[] =
{
     {EOLIAN_TYPE_REGULAR, "",         EOLIAN_DEBUG_INVALID_TYPE  },
     {EOLIAN_TYPE_REGULAR, "pointer",  EOLIAN_DEBUG_POINTER       },
     {EOLIAN_TYPE_REGULAR, "string",   EOLIAN_DEBUG_STRING        },
     {EOLIAN_TYPE_REGULAR, "char",     EOLIAN_DEBUG_CHAR          },
     {EOLIAN_TYPE_REGULAR, "int",      EOLIAN_DEBUG_INT           },
     {EOLIAN_TYPE_REGULAR, "short",    EOLIAN_DEBUG_SHORT         },
     {EOLIAN_TYPE_REGULAR, "double",   EOLIAN_DEBUG_DOUBLE        },
     {EOLIAN_TYPE_REGULAR, "bool",     EOLIAN_DEBUG_BOOLEAN       },
     {EOLIAN_TYPE_REGULAR, "long",     EOLIAN_DEBUG_LONG          },
     {EOLIAN_TYPE_REGULAR, "uint",     EOLIAN_DEBUG_UINT          },
     {EOLIAN_TYPE_REGULAR, "list",     EOLIAN_DEBUG_LIST          },
     {EOLIAN_TYPE_REGULAR, "iterator", EOLIAN_DEBUG_LIST          },
     {EOLIAN_TYPE_CLASS,   "",         EOLIAN_DEBUG_OBJECT        },
     {0,                   NULL,       0}
};

const Debug_Param_Info debug_types[] =
{
     {EOLIAN_DEBUG_INVALID_TYPE, "",   &ffi_type_pointer,   0},
     {EOLIAN_DEBUG_POINTER,      "%p", &ffi_type_pointer,   8},
     {EOLIAN_DEBUG_STRING,       "%s", &ffi_type_pointer,   8},
     {EOLIAN_DEBUG_CHAR,         "%c", &ffi_type_uint,      1},
     {EOLIAN_DEBUG_INT,          "%d", &ffi_type_sint,      4},
     {EOLIAN_DEBUG_SHORT,        "%d", &ffi_type_sint,      4},
     {EOLIAN_DEBUG_DOUBLE,       "%f", &ffi_type_pointer,   8},
     {EOLIAN_DEBUG_BOOLEAN,      "%d", &ffi_type_uint,      1},
     {EOLIAN_DEBUG_LONG,         "%f", &ffi_type_pointer,   8},
     {EOLIAN_DEBUG_UINT,         "%u", &ffi_type_uint,      4},
     {EOLIAN_DEBUG_LIST,         "%p", &ffi_type_pointer,   8},
     {EOLIAN_DEBUG_OBJECT,       "%p", &ffi_type_pointer,   8},
     {0, NULL, 0, 0}
};

typedef struct
{
   const Eolian_Unit *unit;
   const Eolian_Class *kl;
} Eolian_Info;

static Eolian_Debug_Basic_Type
_eolian_type_resolve(const Eolian_Type *eo_type)
{
   Eolian_Type_Type type = eolian_type_type_get(eo_type);
   Eolian_Type_Type type_base = type;

   if (type == EOLIAN_TYPE_CLASS) return EOLIAN_DEBUG_OBJECT;

   if (type_base == EOLIAN_TYPE_REGULAR)
     {
        const char *full_name = eolian_type_name_get(eo_type);
        const Eolian_Typedecl *alias = eolian_state_alias_by_name_get(eos, full_name);
        if (alias)
          {
             eo_type = eolian_typedecl_base_type_get(alias);
             type_base = eolian_type_type_get(eo_type);
             full_name = eolian_type_name_get(eo_type);
          }

        if (full_name)
          {
             int i;
             for (i = 0; eolian_types[i].name; i++)
                if (!strcmp(full_name, eolian_types[i].name) &&
                      eolian_types[i].etype == type)  return i;
          }
     }

   return EOLIAN_DEBUG_INVALID_TYPE;
}

static int
_function_invoke(Eo *ptr, const Eolian_Function *foo, Eolian_Function_Type foo_type,
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
        ed_type = _eolian_type_resolve(eolian_parameter_type_get(eo_param));
        if (!ed_type) goto error;

        types[ffi_argc] = debug_types[ed_type].ffi_type_p;
        values[ffi_argc] = &(params[argc].value.value.value);
        params[argc].eparam = eo_param;
        ffi_argc++;
        argc++;
     }

   itr = foo_type == EOLIAN_METHOD ? eolian_function_parameters_get(foo) :
      eolian_property_values_get(foo, foo_type);
   EINA_ITERATOR_FOREACH(itr, eo_param)
     {
        ed_type = _eolian_type_resolve(eolian_parameter_type_get(eo_param));
        if (!ed_type) goto error;
        params[argc].eparam = eo_param;

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
             types[ffi_argc] = debug_types[ed_type].ffi_type_p;
             values[ffi_argc] = &(params[argc].value.value.value);
          }
        ffi_argc++;
        argc++;
     }

   const Eolian_Type *eo_type = eolian_function_return_type_get(foo, foo_type);
   ffi_type *ffi_ret_type = &ffi_type_void;
   if (eo_type)
     {
        ed_type = _eolian_type_resolve(eo_type);
        if (!ed_type) goto error;

        ffi_ret_type = debug_types[ed_type].ffi_type_p;
     }
   else if (argc == 1 && foo_type == EOLIAN_PROP_GET)
     {
        /* If there is no return type but only one value is present, the value will
         * be returned by the function.
         * So we need FFI to not take it into account when invoking the function.
         */
        ffi_ret_type = debug_types[params[0].value.type].ffi_type_p;
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
                  ret->etype = eo_type;
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

#if 0
/* Commented until we are sure it is not needed anymore */
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
#endif

static int
_param_buffer_fill(char *buf, uint64_t v, int size)
{
   if (size == 8)
     {
        uint64_t value = SWAP_64(v);
        memcpy(buf, &value, 8);
     }
   else if (size == 4)
     {
        int value = SWAP_32(v);
        memcpy(buf, &value, 4);
     }
   else
     {
        memcpy(buf, &v, size);
     }
   return size;
}

static int
_complex_buffer_fill(char *buf, const Eolian_Type *eo_type, uint64_t value)
{
   Eina_List *l = NULL;
   const char *eo_tname = eolian_type_name_get(eo_type);
   void *data;
   int size = 0, count = -1;
   Eolian_Debug_Basic_Type type;
   if (!strcmp(eo_tname, "iterator"))
     {
        Eina_Iterator *iter = (Eina_Iterator *)value;
        EINA_ITERATOR_FOREACH(iter, data)
          {
             l = eina_list_append(l, data);
          }
        eina_iterator_free(iter);
     }
   else if (!strcmp(eo_tname, "list"))
     {
        Eina_List *int_l = (Eina_List *)value;
        Eina_Stringshare *free_foo_str = eolian_type_free_func_get(eo_type);
        l = eina_list_clone(int_l);
        if (free_foo_str)
          {
             void (*func)(Eina_List *) = dlsym(RTLD_DEFAULT, free_foo_str);
             if (func) func(int_l);
             else
                printf("Function %s not found", free_foo_str);
          }
     }

   type = _eolian_type_resolve(eolian_type_base_type_get(eo_type));

   if (type != EOLIAN_DEBUG_INVALID_TYPE) count = SWAP_32(eina_list_count(l));

   memcpy(buf + size, &count, 4);
   size += 4;

   EINA_LIST_FREE(l, data)
     {
        if (count == -1) continue;
        size += _param_buffer_fill(buf+size, (uint64_t)data, debug_types[type].size);
     }
   return size;
}

static Eina_Bool
_api_resolvable(Eo *obj, const Eolian_Function *function)
{
   Efl_Object_Op_Call_Data call_data = {};
   Efl_Object_Op op;
   const char *func_c_name;
   void *func_api;

   func_c_name = eolian_function_full_c_name_get(function, EOLIAN_PROP_GET, EINA_FALSE);
   func_api = dlsym(RTLD_DEFAULT, func_c_name);
   op = _efl_object_op_api_id_get(func_api, obj, func_c_name, __FILE__, __LINE__);
   _efl_object_call_resolve(obj, func_c_name, &call_data, op, __FILE__, __LINE__);

   return !!call_data.func;
}

static unsigned int
_class_buffer_fill(Eo *obj, const Eolian_Class *ekl, char *buf)
{
   unsigned int size = 0;
   Eina_Iterator *funcs = eolian_class_functions_get(ekl, EOLIAN_PROPERTY);
   const Eolian_Function *func;
   EINA_ITERATOR_FOREACH(funcs, func)
     {
        if (eolian_function_type_get(func) == EOLIAN_PROP_SET) continue;

        if (!_api_resolvable(obj, func)) continue;

        Eina_Iterator *keys_itr = eolian_property_keys_get(func, EOLIAN_PROP_GET);
        eina_iterator_free(keys_itr);
        /* We dont support functions with key parameters */
        if (keys_itr) continue;

        Eolian_Debug_Parameter params[EOLIAN_DEBUG_MAXARGS];
        Eolian_Debug_Return ret;
        int argnum = _function_invoke(obj, func, EOLIAN_PROP_GET, params, &ret);

        if (argnum == -1) continue;

        int len, i;
        if (!size) // only if its the first func to succeed
          {
             const char *class_name = eolian_class_name_get(ekl);
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
                  const Eolian_Type *eo_type = eolian_parameter_type_get(params[i].eparam);
                  size += _param_buffer_fill(buf+size, params[i].value.value.value,
                        debug_types[params[i].value.type].size);
                  if (params[i].value.type == EOLIAN_DEBUG_LIST)
                    {
                       size += _complex_buffer_fill(buf+size, eo_type,
                             params[i].value.value.value);
                    }
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
                  size += _param_buffer_fill(buf+size, ret.value.value.value,
                        debug_types[ret.value.type].size);
                  if (ret.value.type == EOLIAN_DEBUG_LIST)
                    {
                       size += _complex_buffer_fill(buf+size,
                             ret.etype, ret.value.value.value);
                    }
               }
          }

     }
   eina_iterator_free(funcs);
   return size;
}

/* Mapping from legacy classes to installed class files*/
static const char *legacy_installed_map[][2] =
{
     { "Efl.Ui.Bg_Widget_Legacy", "Efl.Ui.Bg_Widget" },
     { "Efl.Ui.Button_Legacy", "Efl.Ui.Button" },
     { "Efl.Ui.Check_Legacy", "Efl.Ui.Check" },
     { "Efl.Ui.Clock_Legacy", "Efl.Ui.Clock" },
     { "Efl.Ui.Flip_Legacy", "Efl.Ui.Flip" },
     { "Efl.Ui.Frame_Legacy", "Efl.Ui.Flip" },
     { "Efl.Ui.Image_Legacy", "Efl.Ui.Image" },
     { "Efl.Ui.Image_Zoomable_Legacy", "Efl.Ui.Image_Zoomable" },
     { "Efl.Ui.Layout_Legacy", "Efl.Ui.Layout" },
     { "Efl.Ui.Multibuttonentry_Legacy", "Efl.Ui.Multibuttonentry" },
     { "Efl.Ui.Panes_Legacy", "Efl.Ui.Panes" },
     { "Efl.Ui.Progressbar_Legacy", "Efl.Ui.Progressbar" },
     { "Efl.Ui.Radio_Legacy", "Efl.Ui.Radio" },
     { "Efl.Ui.Slider_Legacy", "Efl.Ui.Slider" },
     { "Efl.Ui.Video_Legacy", "Efl.Ui.Video" },
     { "Efl.Ui.Win_Legacy", "Efl.Ui.Win" },
     { "Elm.Code_Widget_Legacy", "Elm.Code_Widget" },
     { "Elm.Ctxpopup", "Efl.Ui.Layout" },
     { "Elm.Entry", "Efl.Ui.Layout" },
     { "Elm.Colorselector", "Efl.Ui.Layout" },
     { "Elm.List", "Efl.Ui.Layout" },
     { "Elm.Photo", "Efl.Ui.Widget" },
     { "Elm.Actionslider", "Efl.Ui.Layout" },
     { "Elm.Box", "Efl.Ui.Widget" },
     { "Elm.Table", "Efl.Ui.Widget" },
     { "Elm.Thumb", "Efl.Ui.Layout" },
     { "Elm.Menu", "Efl.Ui.Widget" },
     { "Elm.Icon", "Efl.Ui.Image" },
     { "Elm.Prefs", "Efl.Ui.Widget" },
     { "Elm.Map", "Efl.Ui.Widget" },
     { "Elm.Glview", "Efl.Ui.Widget" },
     { "Elm.Web", "Efl.Ui.Widget" },
     { "Elm.Toolbar", "Efl.Ui.Widget" },
     { "Elm.Grid", "Efl.Ui.Widget" },
     { "Elm.Diskselector", "Efl.Ui.Widget" },
     { "Elm.Notify", "Efl.Ui.Widget" },
     { "Elm.Mapbuf", "Efl.Ui.Widget" },
     { "Elm.Separator", "Efl.Ui.Layout" },
     { "Elm.Calendar", "Efl.Ui.Layout" },
     { "Elm.Inwin", "Efl.Ui.Layout" },
     { "Elm.Gengrid", "Efl.Ui.Layout" },
     { "Elm.Scroller", "Efl.Ui.Layout" },
     { "Elm.Player", "Efl.Ui.Layout" },
     { "Elm.Segment_Control", "Efl.Ui.Layout" },
     { "Elm.Fileselector", "Efl.Ui.Layout" },
     { "Elm.Fileselector_Button", "Efl.Ui.Button" },
     { "Elm.Fileselector_Entry", "Efl.Ui.Layout" },
     { "Elm.Flipselector", "Efl.Ui.Layout" },
     { "Elm.Hoversel", "Efl.Ui.Button" },
     { "Elm.Naviframe", "Efl.Ui.Layout" },
     { "Elm.Popup", "Efl.Ui.Layout" },
     { "Elm.Bubble", "Efl.Ui.Layout" },
     { "Elm.Clock", "Efl.Ui.Layout" },
     { "Elm.Conformant", "Efl.Ui.Layout" },
     { "Elm.Dayselector", "Efl.Ui.Layout" },
     { "Elm.Genlist", "Efl.Ui.Layout" },
     { "Elm.Hover", "Efl.Ui.Layout" },
     { "Elm.Index", "Efl.Ui.Layout" },
     { "Elm.Label", "Efl.Ui.Layout" },
     { "Elm.Panel", "Efl.Ui.Layout" },
     { "Elm.Slideshow", "Efl.Ui.Layout" },
     { "Elm.Spinner", "Efl.Ui.Layout" },
     { "Elm.Plug", "Efl.Ui.Widget" },
     { "Elm.Web.None", "Efl.Ui.Widget" },
     { NULL, NULL }
};

static Eina_Bool
_obj_info_req_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size EINA_UNUSED)
{
   static Eina_Hash *_parsed_kls = NULL;
   uint64_t ptr64;
   memcpy(&ptr64, buffer, sizeof(ptr64));
   Eo *obj = (Eo *)SWAP_64(ptr64);
   const char *class_name = NULL;
   const Eolian_Class *kl, *okl;
   Eolian_State *s;
   unsigned int size_curr = 0;
   char *buf;

   if (!obj) return EINA_FALSE;

   class_name = efl_class_name_get(obj);

   if (efl_isa(obj, EFL_UI_LEGACY_INTERFACE))
     {
        for (int i = 0; legacy_installed_map[i][0]; ++i)
          {
             if (!strcmp(legacy_installed_map[i][0], class_name))
               {
                  class_name = legacy_installed_map[i][1];
                  break;
               }
          }
     }

   if (!_parsed_kls) _parsed_kls = eina_hash_string_superfast_new(NULL);
   s = eina_hash_find(_parsed_kls, class_name);
   if (!s)
     {
        const char *fname;
        Eina_Strbuf *sb = eina_strbuf_new();
        s = eos = eolian_state_new();
        eina_strbuf_append(sb, class_name);
        eina_strbuf_replace_all(sb, ".", "_");
        eina_strbuf_tolower(sb);
        eina_strbuf_append(sb, ".eo");
        fname = eina_strbuf_string_get(sb);

        eolian_state_system_directory_add(s);
        if (!eolian_state_file_parse(s, fname))
          {
             printf("File %s cannot be parsed.\n", fname);
             goto end;
          }
        eina_strbuf_free(sb);
        eina_hash_add(_parsed_kls, class_name, s);
     }
   okl = eolian_state_class_by_name_get(s, class_name);
   if (!okl)
     {
        printf("Class %s not found.\n", class_name);
        goto end;
     }
   buf = malloc(100000);
   memcpy(buf, &ptr64, sizeof(uint64_t));
   size_curr = sizeof(uint64_t);

   Eina_List *itr, *list2;
   Eina_List *list = eina_list_append(NULL, okl);
   EINA_LIST_FOREACH(list, itr, kl)
     {
        const Eolian_Class *inherit;
        Eina_Iterator *inherits_itr;

        inherits_itr = eolian_class_inherits_get(kl);
        size_curr += _class_buffer_fill(obj, kl, buf + size_curr);

        EINA_ITERATOR_FOREACH(inherits_itr, inherit)
          {
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

   eina_debug_session_send(session, srcid, _snapshot_start_op, NULL, 0);

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
   if (!efl_isa(obj, EFL_GFX_ENTITY_INTERFACE) && !efl_isa(obj, EFL_CANVAS_SCENE_INTERFACE)) return;
   Evas *e = evas_object_evas_get(obj);
   Eo *rect = evas_object_polygon_add(e);
   evas_object_move(rect, 0, 0);
   if (efl_isa(obj, EFL_GFX_ENTITY_INTERFACE))
     {
        Eina_Rect obj_geom = {.x = 0, .y = 0, .w = 0, .h = 0};
        obj_geom = efl_gfx_entity_geometry_get(obj);
        if (efl_isa(obj, EFL_UI_WIN_CLASS)) obj_geom.x = obj_geom.y = 0;

        evas_object_polygon_point_add(rect, obj_geom.x, obj_geom.y);
        evas_object_polygon_point_add(rect, obj_geom.x + obj_geom.w, obj_geom.y);
        evas_object_polygon_point_add(rect, obj_geom.x + obj_geom.w, obj_geom.y + obj_geom.h);
        evas_object_polygon_point_add(rect, obj_geom.x, obj_geom.y + obj_geom.h);
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

typedef struct
{
   uint64_t ptr64;
   Eina_Debug_Session *session;
   Evas_Object *snapshot;
   int cid;
} Screenshot_Async_Info;

static void
_screenshot_pixels_cb(void *data, Evas *e, void *event_info)
{
   unsigned int hdr_size = sizeof(uint64_t) + 5 * sizeof(int);
   Evas_Event_Render_Post *post = event_info;
   Screenshot_Async_Info *info = data;
   Evas_Object *snapshot = info->snapshot;
   struct tm *t = NULL;
   time_t now = time(NULL);
   unsigned char *resp = NULL, *tmp;
   const void *pixels;
   int w, h, val;

   // Nothing was updated, so let's not bother sending nothingness
   if (!post->updated_area) return;
   pixels = evas_object_image_data_get(snapshot, EINA_FALSE);
   if (!pixels) return;
   evas_object_geometry_get(snapshot, NULL, NULL, &w, &h);

   t = localtime(&now);
   t->tm_zone = NULL;

   resp = tmp = malloc(hdr_size + (w * h * sizeof(int)));
   t->tm_sec = SWAP_32(t->tm_sec);
   t->tm_min = SWAP_32(t->tm_min);
   t->tm_hour = SWAP_32(t->tm_hour);
   STORE(tmp, &info->ptr64, sizeof(uint64_t));
   STORE(tmp, &t->tm_sec, sizeof(int));
   STORE(tmp, &t->tm_min, sizeof(int));
   STORE(tmp, &t->tm_hour, sizeof(int));
   val = SWAP_32(w);
   STORE(tmp, &val, sizeof(int));
   val = SWAP_32(h);
   STORE(tmp, &val, sizeof(int));
   memcpy(tmp, pixels, (w * h * sizeof(int)));

   eina_debug_session_send(info->session, info->cid, _win_screenshot_op, resp,
         hdr_size + (w * h * sizeof(int)));
   if (resp) free(resp);
   evas_object_del(info->snapshot);
   evas_event_callback_del_full(e, EVAS_CALLBACK_RENDER_POST, _screenshot_pixels_cb, info);
}

static void
_main_loop_win_screenshot_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size)
{
   uint64_t ptr64;
   Screenshot_Async_Info *info;
   Evas_Object *snapshot;
   int w, h;

   if (size != sizeof(uint64_t)) return;
   memcpy(&ptr64, buffer, sizeof(ptr64));
   Eo *obj = (Eo *)SWAP_64(ptr64);
   Eo *e = evas_object_evas_get(obj);
   if (!e) return;

   snapshot = evas_object_image_filled_add(e);
   if (!snapshot) return;
   evas_object_image_snapshot_set(snapshot, EINA_TRUE);

   info = calloc(1, sizeof(*info));
   info->ptr64 = ptr64;
   info->snapshot = snapshot;
   info->session = session;
   info->cid = srcid;

   evas_output_size_get(e, &w, &h);
   evas_object_geometry_set(snapshot, 0, 0, w, h);
   efl_gfx_entity_visible_set(snapshot, EINA_TRUE);
   evas_event_callback_add(e, EVAS_CALLBACK_RENDER_POST, _screenshot_pixels_cb, info);
}

WRAPPER_TO_XFER_MAIN_LOOP(_win_screenshot_cb)

EAPI Efl_Dbg_Info*
clouseau_eo_info_find(Efl_Dbg_Info *root, const char *name)
{
   Eina_Value_List eo_list;
   Eina_List *n;
   Efl_Dbg_Info *info;

   if (!root) return NULL;

   eina_value_pget(&(root->value), &eo_list);

   EINA_LIST_FOREACH(eo_list.list, n, info)
     {
        if (!strcmp(info->name, name))
          {
             return info;
          }
     }
   return NULL;
}

static Eina_Bool
_only_manager(const void *container EINA_UNUSED, void *data, void *fdata EINA_UNUSED)
{
   Efl_Dbg_Info *manager_data, *root;

   if (!efl_isa(data, EFL_UI_FOCUS_MANAGER_INTERFACE)) return EINA_FALSE;

   //check for debug information
   root = EFL_DBG_INFO_LIST_APPEND(NULL, "Root");
   efl_dbg_info_get(data, root);
   manager_data = clouseau_eo_info_find(root, "Efl.Ui.Focus.Manager");
   if (!manager_data) return EINA_FALSE;

   return EINA_TRUE;
}

static void
_main_loop_focus_manager_list_cb(Eina_Debug_Session *session, int srcid, void *buffer EINA_UNUSED, int size EINA_UNUSED)
{
   Eina_Iterator *obj_iterator, *manager_iterator;
   Clouseau_Focus_Managers *managers;
   Eo *obj;

   if (!manager_list) _init_data_descriptors();

   managers = alloca(sizeof(Clouseau_Focus_Managers));
   managers->managers = NULL;
   obj_iterator = eo_objects_iterator_new();
   manager_iterator = eina_iterator_filter_new(obj_iterator, _only_manager, NULL, NULL);

   EINA_ITERATOR_FOREACH(manager_iterator, obj)
     {
        Clouseau_Focus_List_Item *item = alloca(sizeof(Clouseau_Focus_List_Item));

        item->ptr = (uintptr_t)(void*)obj;
        item->helper_name = efl_class_name_get(efl_ui_focus_manager_root_get(obj));

        managers->managers = eina_list_append(managers->managers, item);
     }

   int blob_size;
   void *blob = eet_data_descriptor_encode(manager_list, managers, &blob_size);
   eina_debug_session_send(session, srcid, _focus_manager_list_op, blob, blob_size);

   managers->managers = eina_list_free(managers->managers);
}

WRAPPER_TO_XFER_MAIN_LOOP(_focus_manager_list_cb)

static Eina_List*
_fetch_children(Efl_Ui_Focus_Manager *m)
{
   Efl_Dbg_Info *manager_data, *children_data, *root;
   Eina_List *lst = NULL, *n;
   Eina_Value_List result;
   Efl_Dbg_Info *elem;

   root = EFL_DBG_INFO_LIST_APPEND(NULL, "Root");

   efl_dbg_info_get(m, root);

   manager_data = clouseau_eo_info_find(root, "Efl.Ui.Focus.Manager");

   EINA_SAFETY_ON_NULL_RETURN_VAL(manager_data, NULL);

   children_data = clouseau_eo_info_find(manager_data, "children");

   EINA_SAFETY_ON_NULL_RETURN_VAL(children_data, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_type_get(&children_data->value) == EINA_VALUE_TYPE_LIST, NULL);

   eina_value_pget(&children_data->value, &result);

   EINA_LIST_FOREACH(result.list, n, elem)
     {
        void *ptr;

        EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_type_get(&elem->value) == EINA_VALUE_TYPE_UINT64, NULL);
        eina_value_get(&elem->value, &ptr);

        lst = eina_list_append(lst, ptr);
     }

   efl_dbg_info_free(root);

   return lst;
}

static Eina_Bool
_main_loop_focus_manager_detail_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size EINA_UNUSED)
{
   Clouseau_Focus_Manager_Data *res;
   uint64_t ptr64;
   Eo *elem, *manager;

   if (!manager_details) _init_data_descriptors();

   memcpy(&ptr64, buffer, sizeof(ptr64));
   manager = (Eo *)SWAP_64(ptr64);
   if (!efl_isa(manager, EFL_UI_FOCUS_MANAGER_INTERFACE)) return EINA_TRUE;

   Eina_List *children = _fetch_children(manager);

   res = alloca(sizeof(Clouseau_Focus_Manager_Data));
   res->class_name = efl_class_name_get(manager);
   res->relations = NULL;
   res->focused = efl_ui_focus_manager_focus_get(manager);
   res->redirect_manager = efl_ui_focus_manager_redirect_get(manager);

   EINA_LIST_FREE(children, elem)
     {
        Clouseau_Focus_Relation *crel = calloc(1, sizeof(Clouseau_Focus_Relation));
        Efl_Ui_Focus_Relations *rel;

        rel = efl_ui_focus_manager_fetch(manager, elem);
        if (rel)
          {
             memcpy(&crel->relation, rel, sizeof(Efl_Ui_Focus_Relations));
            crel->class_name = efl_class_name_get(elem);

            res->relations = eina_list_append(res->relations, crel);

            free(rel);
          }
     }

   int blob_size;
   void *blob = eet_data_descriptor_encode(manager_details, res, &blob_size);

   eina_debug_session_send(session, srcid, _focus_manager_detail_op, blob, blob_size);

   return EINA_TRUE;
}

WRAPPER_TO_XFER_MAIN_LOOP(_focus_manager_detail_cb)

EINA_DEBUG_OPCODES_ARRAY_DEFINE(_debug_ops,
     {"Clouseau/Object_Introspection/snapshot_start", &_snapshot_start_op, &_snapshot_start_cb},
     {"Clouseau/Object_Introspection/snapshot_done", &_snapshot_done_op, NULL},
     {"Clouseau/Eo/classes_ids_get", &_klids_get_op, NULL},
     {"Clouseau/Eo/objects_ids_get", &_eoids_get_op, NULL},
     {"Clouseau/Eolian/object/info_get", &_obj_info_op, &_obj_info_req_cb},
     {"Clouseau/Evas/object/highlight", &_obj_highlight_op, &_obj_highlight_cb},
     {"Clouseau/Evas/window/screenshot", &_win_screenshot_op, &_win_screenshot_cb},
     {"Clouseau/Elementary_Focus/list", &_focus_manager_list_op, &_focus_manager_list_cb},
     {"Clouseau/Elementary_Focus/detail", &_focus_manager_detail_op, &_focus_manager_detail_cb},
     {NULL, NULL, NULL}
);

EAPI Eina_Bool
clouseau_debug_init(void)
{
   eina_init();
   eolian_init();
   evas_init();

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

static int
_complex_buffer_decode(char *buffer, const Eolian_Type *eo_type,
      Eolian_Debug_Value *v)
{
   Eina_List *l = NULL;
   int size = 0, count;
   Eolian_Debug_Basic_Type type;

   v->type = EOLIAN_DEBUG_LIST;
   memcpy(&count, buffer, 4);
   count = SWAP_32(count);
   buffer += 4;
   size += 4;

   if (count > 0) type = _eolian_type_resolve(eolian_type_base_type_get(eo_type));

   while (count > 0)
     {
        Eolian_Debug_Value *v2 = calloc(1, sizeof(*v2));
        v2->type = type;
        EXTRACT(buffer, &(v2->value), debug_types[type].size);
        v2->value = SWAP_64(v2->value);
        size += debug_types[type].size;
        l = eina_list_append(l, v2);
        count--;
     }
   v->complex_type_values = l;
   return size;
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
   static Eina_Hash *_parsed_kls = NULL;
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
        const Eolian_Type *eo_type;
        int len = strlen(buffer) + 1;
        if (len > 1) // if class_name is not NULL, we begin a new class
          {
             Eolian_State *s;
             if (!_parsed_kls) _parsed_kls = eina_hash_string_superfast_new(NULL);
             s = eina_hash_find(_parsed_kls, buffer);
             if (!s)
               {
                  const char *fname;
                  Eina_Strbuf *sb = eina_strbuf_new();
                  s = eos = eolian_state_new();
                  eina_strbuf_append(sb, buffer);
                  eina_strbuf_replace_all(sb, ".", "_");
                  eina_strbuf_tolower(sb);
                  eina_strbuf_append(sb, ".eo");
                  fname = eina_strbuf_string_get(sb);

                  eolian_state_system_directory_add(s);
                  if (!eolian_state_file_parse(s, fname))
                    {
                       printf("File %s cannot be parsed.\n", fname);
                       goto error;
                    }
                  eina_strbuf_free(sb);
                  eina_hash_add(_parsed_kls, buffer, s);
               }
             kl = calloc(1, sizeof(*kl));
             kl->ekl = eolian_state_class_by_name_get(s, buffer);
             ret->classes = eina_list_append(ret->classes, kl);
          }
        if (!kl)
          {
             printf("Class %s not found!\n", buffer);
             goto error;
          }
        buffer += len;
        size -= len;

        func = calloc(1, sizeof(*func));
//        printf("Class name = %s function = %s\n", eolian_class_name_get(kl->ekl), buffer);
        kl->functions = eina_list_append(kl->functions, func);
        func->efunc = eolian_class_function_by_name_get(kl->ekl, buffer, EOLIAN_PROP_GET);
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
             eo_type = eolian_parameter_type_get(eo_param);
             Eolian_Debug_Basic_Type type = _eolian_type_resolve(eo_type);

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
                       uint64_t value = 0;;
                       EXTRACT(buffer, &value, debug_types[type].size);
                       p->value.value.value = SWAP_64(value);
                       size -= debug_types[type].size;
                       if (type == EOLIAN_DEBUG_LIST)
                         {
                            len = _complex_buffer_decode(buffer, eo_type, &(p->value));
                            buffer += len;
                            size -= len;
                         }
                    }
                  func->params = eina_list_append(func->params, p);
               }
             else
               {
                  printf("Unknown parameter type %s\n", eolian_type_name_get(eo_type));
                  goto error;
               }
          }
        func->ret.etype = eo_type = eolian_function_return_type_get(
              func->efunc, EOLIAN_PROP_GET);
        func->ret.value.type = EOLIAN_DEBUG_VOID;
        if(eo_type)
          {
             Eolian_Debug_Basic_Type type = _eolian_type_resolve(eo_type);
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
                       EXTRACT(buffer, &value, debug_types[type].size);
                       func->ret.value.value.value = SWAP_64(value);
                       size -= debug_types[type].size;
                       if (type == EOLIAN_DEBUG_LIST)
                         {
                            len = _complex_buffer_decode(buffer, eo_type, &(func->ret.value));
                            buffer += len;
                            size -= len;
                         }
                    }
               }
             else
               {
                  printf("Unknown parameter type %s\n", eolian_type_name_get(eo_type));
                  goto error;
               }
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
