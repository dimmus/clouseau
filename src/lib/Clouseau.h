#ifndef _CLOUSEAU_H
#define _CLOUSEAU_H

/* FIXME: This doesn't include anything yet.
 * Still need to expose stuff for clouseaud and clouseau_client. Those will be
 * added once the whole interface will be better defined.
 * These functions will probably need to be renamed/change as well.
 * We'll also remove the private include once things are done. */

#include <Elementary.h>

typedef struct _Clouseau_Bitmap Clouseau_Bitmap;
typedef struct _Clouseau_Tree_Item Clouseau_Tree_Item;

/* Legacy type. */
typedef struct _Clouseau_Object Clouseau_Object;

typedef enum
{
   CLOUSEAU_OBJ_TYPE_UNKNOWN,
   CLOUSEAU_OBJ_TYPE_OTHER,
   CLOUSEAU_OBJ_TYPE_ELM,
   CLOUSEAU_OBJ_TYPE_TEXT,
   CLOUSEAU_OBJ_TYPE_IMAGE,
   CLOUSEAU_OBJ_TYPE_EDJE,
   CLOUSEAU_OBJ_TYPE_TEXTBLOCK
} Clouseau_Object_Type;

struct _Clouseau_Bitmap
{
   unsigned char  *bmp;
   int bmp_count; /* is (w * h), for EET_DATA_DESCRIPTOR_ADD_BASIC_VAR_ARRAY */
   Evas_Coord w;
   Evas_Coord h;
};

/* FIXME: Should name be a stringshare?
 * FIXME: Should strip this structure to be half the size. Most of the stuff are
 * not really needed. */
struct _Clouseau_Tree_Item
{
   Eina_List *children;
   Eina_List *eo_info;          /* The intermediate type we use for eet. */
   Eo_Dbg_Info *new_eo_info;
   const char *name;
   unsigned long long ptr;      /* Just a ptr, we keep the value but not accessing mem */
   Clouseau_Object *info;       /* Legacy */
   Eina_Bool is_obj;
   Eina_Bool is_clipper;
   Eina_Bool is_visible;
};

EAPI Eina_Bool clouseau_app_connect(void);
EAPI void clouseau_app_data_req_cb_set(Eina_List *(*cb)(void));
EAPI void clouseau_app_canvas_bmp_cb_set(void *(*cb)(Ecore_Evas *ee, Evas_Coord *w_out, Evas_Coord *h_out));
EAPI Clouseau_Object *clouseau_object_information_get(Clouseau_Tree_Item *treeit);

EAPI Eina_Bool clouseau_daemon_connect(void);
EAPI Eina_Bool clouseau_client_connect(void);
EAPI Eina_Bool clouseau_disconnect(void);

EAPI int clouseau_init(const char *appname);
EAPI int clouseau_shutdown(void);

/* FIXME: Remove. */
#include "clouseau_private.h"

#endif
