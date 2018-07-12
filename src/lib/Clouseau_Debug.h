#ifndef _CLOUSEAU_DEBUG_H
#define _CLOUSEAU_DEBUG_H

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

#define EOLIAN_DEBUG_MAXARGS 15

#include <Eo.h>
#include <Eolian.h>
#include <Elementary.h>

typedef void (*Eo_Debug_Class_Extract_Cb)(void *data, uint64_t kl, char *kl_name);

typedef void (*Eo_Debug_Object_Extract_Cb)(void *data, uint64_t obj, uint64_t kl, uint64_t parent);

typedef enum
{
   EOLIAN_DEBUG_INVALID_TYPE = 0,
   EOLIAN_DEBUG_POINTER,
   EOLIAN_DEBUG_STRING,
   EOLIAN_DEBUG_CHAR,
   EOLIAN_DEBUG_INT,
   EOLIAN_DEBUG_SHORT,
   EOLIAN_DEBUG_DOUBLE,
   EOLIAN_DEBUG_BOOLEAN,
   EOLIAN_DEBUG_LONG,
   EOLIAN_DEBUG_UINT,
   EOLIAN_DEBUG_LIST,
   EOLIAN_DEBUG_OBJECT,
   EOLIAN_DEBUG_VOID,
   EOLIAN_DEBUG_STRUCT
} Eolian_Debug_Basic_Type;

typedef struct
{
   Eolian_Debug_Basic_Type type;
   union
     {
        uint64_t value;
     } value;
   Eina_List *complex_type_values;
} Eolian_Debug_Value;

typedef struct
{
   Eolian_Debug_Value value;
   const Eolian_Function_Parameter *eparam;
} Eolian_Debug_Parameter;

typedef struct
{
   Eolian_Debug_Value value;
   const Eolian_Type *etype;
} Eolian_Debug_Return;

typedef struct
{
   const Eolian_Function *efunc;
   Eolian_Debug_Return ret;
   Eina_List *params; /* List of Eolian_Debug_Parameter * */
} Eolian_Debug_Function;

typedef struct
{
   const Eolian_Unit *unit;
   const Eolian_Class *ekl;
   Eina_List *functions; /* List of Eolian_Debug_Function * */
} Eolian_Debug_Class;

typedef struct
{
   uint64_t obj; /* Eo * */
   Eina_List *classes; /* List of Eolian_Debug_Class * */
} Eolian_Debug_Object_Information;

typedef struct
{
   uint64_t obj;
   int w;
   int h;
   char *img;
   int img_size;
   int tm_sec;
   int tm_min;
   int tm_hour;
} Evas_Debug_Screenshot;

typedef struct {
   Eo *redirect_manager;
   Eo *focused;
   const char *class_name;
   Eina_List *relations;
} Clouseau_Focus_Manager_Data;

typedef struct {
   Efl_Ui_Focus_Relations relation;
   const char *class_name;
   Evas_Object *vis;
} Clouseau_Focus_Relation;

typedef struct {
   uintptr_t ptr;
   const char *helper_name;
} Clouseau_Focus_List_Item;

typedef struct {
   Eina_List *managers;
} Clouseau_Focus_Managers;

EAPI void *eo_debug_eoids_request_prepare(int *size, ...);

EAPI void eo_debug_klids_extract(void *buffer, int size, Eo_Debug_Class_Extract_Cb cb, void *data);

EAPI void eo_debug_eoids_extract(void *buffer, int size, Eo_Debug_Object_Extract_Cb cb, void *data);

EAPI void eolian_debug_object_information_free(Eolian_Debug_Object_Information *main);

EAPI Eolian_Debug_Object_Information *
eolian_debug_object_information_decode(char *buffer, unsigned int size);

EAPI Evas_Debug_Screenshot *
evas_debug_screenshot_decode(char *buffer, unsigned int size);

typedef struct
{
   Eina_Debug_Session *session;
   int srcid;
   void *buffer;
   unsigned int size;
} Main_Loop_Info;

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

#endif
