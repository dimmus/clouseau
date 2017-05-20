#ifndef _CLOUSEAU_H
#define _CLOUSEAU_H

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

#include <Eina.h>

typedef struct _Extension_Config Extension_Config;
typedef struct _Clouseau_Extension Clouseau_Extension;

typedef Eo *(*Ui_Get_Cb)(Clouseau_Extension *ext, Eo *parent);
typedef void (*Session_Changed_Cb)(Clouseau_Extension *ext);
typedef void (*App_Changed_Cb)(Clouseau_Extension *ext);
typedef void (*Import_Data_Cb)(Clouseau_Extension *ext, char *buffer, int size);

typedef Eo *(*Inwin_Create_Cb)();
typedef void (*Ui_Freeze_Cb)(Clouseau_Extension *ext, Eina_Bool freeze);

struct _Clouseau_Extension
{
   const char *name;                      /* Name filled by the extension */
   Eina_Debug_Session *session;           /* Current session */
   int app_id;                            /* Current application */
   Eina_Stringshare *path_to_config;      /* Path to configuration directory */
   Eo *ui_object;                         /* Main object of the UI extension */
   Session_Changed_Cb session_changed_cb; /* Function called when the session changed */
   App_Changed_Cb app_changed_cb;         /* Function called when the app changed */
   Import_Data_Cb import_data_cb;         /* Function called when data has to be imported */
   Inwin_Create_Cb inwin_create_cb;       /* Function to call to create a Inwin */
   Ui_Freeze_Cb ui_freeze_cb;             /* Function to call to freeze/thaw the UI */
   void *data;                            /* Data allocated and managed by the extension */
   Extension_Config *ext_cfg;             /* Extention configuration - used by Clouseau */
};

#endif
