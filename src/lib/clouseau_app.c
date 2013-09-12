#include "clouseau_private.h"

#include <Elementary.h>
#include <Ecore_X.h>

static Ecore_Con_Server *econ_server = NULL;
static Ecore_Con_Eet *eet_svr = NULL;
static Eina_Stringshare *_my_appname = NULL;

static Clouseau_Object *_clouseau_object_information_get(Clouseau_Tree_Item *treeit);

static void
libclouseau_item_add(Evas_Object *o, Clouseau_Tree_Item *parent)
{
   Clouseau_Tree_Item *treeit;
   Eina_List *children;
   Evas_Object *child;

   treeit = calloc(1, sizeof(Clouseau_Tree_Item));
   if (!treeit) return ;

   treeit->ptr = (uintptr_t) o;
   treeit->is_obj = EINA_TRUE;

   treeit->name = eina_stringshare_add(evas_object_type_get(o));
   treeit->is_clipper = !!evas_object_clipees_get(o);
   treeit->is_visible = evas_object_visible_get(o);
   treeit->info = _clouseau_object_information_get(treeit);

   parent->children = eina_list_append(parent->children, treeit);

   /* if (!evas_object_smart_data_get(o)) return ; */

   /* Do this only for smart object */
   children = evas_object_smart_members_get(o);
   EINA_LIST_FREE(children, child)
     libclouseau_item_add(child, treeit);
}

static void *
_canvas_bmp_get(Ecore_Evas *ee, Evas_Coord *w_out, Evas_Coord *h_out)
{
   Ecore_X_Image *img;
   Ecore_X_Window_Attributes att;
   unsigned char *src;
   unsigned int *dst;
   int bpl = 0, rows = 0, bpp = 0;
   Evas_Coord w, h;

   /* Check that this window still exists */
   Eina_List *eeitr, *ees = ecore_evas_ecore_evas_list_get();
   Ecore_Evas *eel;
   Eina_Bool found_evas = EINA_FALSE;

   EINA_LIST_FOREACH(ees, eeitr, eel)
      if (eel == ee)
        {
           found_evas = EINA_TRUE;
           break;
        }

   Ecore_X_Window xwin = (found_evas) ?
      (Ecore_X_Window) ecore_evas_window_get(ee) : 0;

   if (!xwin)
     {
        printf("Can't grab X window.\n");
        *w_out = *h_out = 0;
        return NULL;
     }

   Evas *e = ecore_evas_get(ee);
   evas_output_size_get(e, &w, &h);
   memset(&att, 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(xwin, &att);
   img = ecore_x_image_new(w, h, att.visual, att.depth);
   ecore_x_image_get(img, xwin, 0, 0, 0, 0, w, h);
   src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
   dst = malloc(w * h * sizeof(int));  /* Will be freed by the user */
   if (!ecore_x_image_is_argb32_get(img))
     {  /* Fill dst buffer with image convert */
        ecore_x_image_to_argb_convert(src, bpp, bpl,
              att.colormap, att.visual,
              0, 0, w, h,
              dst, (w * sizeof(int)), 0, 0);
     }
   else
     {  /* Fill dst buffer by copy */
        memcpy(dst, src, (w * h * sizeof(int)));
     }

   /* dst now holds window bitmap */
   ecore_x_image_free(img);
   *w_out = w;
   *h_out = h;
   return (void *) dst;
}

static Eina_List *
_load_list(void)
{
   Eina_List *tree = NULL;
   Eina_List *ees;
   Ecore_Evas *ee;

   ees = ecore_evas_ecore_evas_list_get();

   EINA_LIST_FREE(ees, ee)
     {
        Eina_List *objs;
        Evas_Object *obj;
        Clouseau_Tree_Item *treeit;

        Evas *e;
        int w, h;

        e = ecore_evas_get(ee);
        evas_output_size_get(e, &w, &h);

        treeit = calloc(1, sizeof(Clouseau_Tree_Item));
        if (!treeit) continue ;

        treeit->name = eina_stringshare_add(ecore_evas_title_get(ee));
        treeit->ptr = (uintptr_t) ee;

        tree = eina_list_append(tree, treeit);

        objs = evas_objects_in_rectangle_get(e, SHRT_MIN, SHRT_MIN,
              USHRT_MAX, USHRT_MAX, EINA_TRUE, EINA_TRUE);

        EINA_LIST_FREE(objs, obj)
          libclouseau_item_add(obj, treeit);
    }

   return tree;  /* User has to call clouseau_tree_free() */
}

/* Highlight functions. */
static Eina_Bool
_clouseau_highlight_fade(void *_rect)
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

static Evas_Object *
_clouseau_verify_e_children(Evas_Object *obj, Evas_Object *ptr)
{
   /* Verify that obj still exists (can be found on evas canvas) */
   Evas_Object *child;
   Evas_Object *p = NULL;
   Eina_List *children;

   if (ptr == obj)
     return ptr;

   children = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(children, child)
     {
        p = _clouseau_verify_e_children(child, ptr);
        if (p) break;
     }
   eina_list_free(children);

   return p;
}

static Evas_Object *
_clouseau_verify_e_obj(Evas_Object *obj)
{
   /* Verify that obj still exists (can be found on evas canvas) */
   Evas_Object *o;
   Eina_List *ees;
   Ecore_Evas *ee;
   Evas_Object *rt = NULL;

   ees = ecore_evas_ecore_evas_list_get();
   EINA_LIST_FREE(ees, ee)
     {
        Evas *e;
        Eina_List *objs;

        e = ecore_evas_get(ee);
        objs = evas_objects_in_rectangle_get(e, SHRT_MIN, SHRT_MIN,
                                             USHRT_MAX, USHRT_MAX,
                                             EINA_TRUE, EINA_TRUE);

        EINA_LIST_FREE(objs, o)
          {
             rt = _clouseau_verify_e_children(o, obj);
             if (rt) break;
          }
        eina_list_free(objs);

        if (rt) break;
     }
   eina_list_free(ees);

   return rt;
}

static void
_clouseau_highlight_del(void *data,
                        EINA_UNUSED Evas *e,
                        EINA_UNUSED Evas_Object *obj,
                        EINA_UNUSED void *event_info)
{  /* Delete timer for this rect */
   ecore_animator_del(data);
}

EAPI void
clouseau_data_object_highlight(Evas_Object *obj, Clouseau_Evas_Props *props, bmp_info_st *view)
{
   Ecore_Animator *t;
   Evas_Object *r;
   int x, y, wd, ht;
   Evas *e = NULL;

   if (props)
     {
        e = evas_object_evas_get(view->win);
     }
   else
     {
        /* Check validity of object when working online */
        if (!_clouseau_verify_e_obj(obj))
          {
             printf("<%s> Evas Object not found <%p> (probably deleted)\n",
                   __func__, obj);
             return;
          }

        /* Take evas from object if working online */
        e = evas_object_evas_get(obj);
        if (!e) return;
     }

   /* Continue and do the Highlight */
   r = evas_object_polygon_add(e);
   evas_object_move(r, 0, 0);

   if (props)
     {
        /* When working offline grab info from struct */
        Evas_Coord x_bmp, y_bmp;

        /* If there's a map, highlight the map coords, not the object's */
        if (props->points_count > 0)
          {
             int i = props->points_count;
             for ( ; i > 0 ; i--)
               {
                  evas_object_polygon_point_add(r,
                        props->points[i].x, props->points[i].y);
               }
          }
        else
          {
             evas_object_geometry_get(view->o, &x_bmp, &y_bmp, NULL, NULL);
             x =  (view->zoom_val * props->x) + x_bmp;
             y =  (view->zoom_val * props->y) + y_bmp;
             wd = (view->zoom_val * props->w);
             ht = (view->zoom_val * props->h);
             evas_object_polygon_point_add(r, x, y);
             evas_object_polygon_point_add(r, x + wd, y);
             evas_object_polygon_point_add(r, x + wd, y + ht);
             evas_object_polygon_point_add(r, x, y + ht);
          }
     }
   else
     {
        const Evas_Map *map;
        if ((map = evas_object_map_get(obj)))
          {
             int i = evas_map_count_get(map);
             for ( ; i > 0 ; i--)
               {
                  Evas_Coord mx, my;
                  evas_map_point_coord_get(map, i - 1, &mx, &my, NULL);
                  evas_object_polygon_point_add(r, mx, my);
               }
          }
        else
          {
             evas_object_geometry_get(obj, &x, &y, &wd, &ht);
             evas_object_polygon_point_add(r, x, y);
             evas_object_polygon_point_add(r, x + wd, y);
             evas_object_polygon_point_add(r, x + wd, y + ht);
             evas_object_polygon_point_add(r, x, y + ht);
          }
     }

   /* Put the object as high as possible. */
   evas_object_layer_set(r, EVAS_LAYER_MAX);
   evas_object_color_set(r,
                         HIGHLIGHT_R, HIGHLIGHT_G, HIGHLIGHT_B, HIGHLIGHT_A);
   evas_object_show(r);

   /* Add Timer for fade and a callback to delete timer on obj del */
   t = ecore_animator_add(_clouseau_highlight_fade, r);
   evas_object_event_callback_add(r, EVAS_CALLBACK_DEL,
                                  _clouseau_highlight_del, t);
/* Print backtrace info, saved for future ref
   tmp = evas_object_data_get(obj, ".clouseau.bt");
   fprintf(stderr, "Creation backtrace :\n%s*******\n", tmp); */
}

static Clouseau_Object *
_clouseau_object_information_get(Clouseau_Tree_Item *treeit)
{
   Evas_Object *obj = (void*) (uintptr_t) treeit->ptr;
   Eo_Dbg_Info *eo_dbg_info;

   if (!treeit->is_obj)
     return NULL;

   eo_dbg_info = EO_DBG_INFO_LIST_APPEND(NULL, "");
   eo_do(obj, eo_dbg_info_get(eo_dbg_info));
   treeit->eo_info = clouseau_eo_to_legacy_convert(eo_dbg_info);

   eo_dbg_info_free(eo_dbg_info); /* Free original list */

   return NULL;
}


Eina_Bool
_add(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED Ecore_Con_Server *conn)
{
/*   ecore_con_server_data_size_max_set(conn, -1); */
   connect_st t = { getpid(), _my_appname };
   ecore_con_eet_send(reply, CLOUSEAU_APP_CLIENT_CONNECT_STR, &t);

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
_del(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      Ecore_Con_Server *conn)
{
   if (!conn)
     {
        printf("Failed to establish connection to the server.\n");
     }

   printf("Lost server with ip <%s>\n", ecore_con_server_ip_get(conn));

   ecore_con_server_del(conn);

   return ECORE_CALLBACK_RENEW;
}

void
_data_req_cb(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* data req includes ptr to GUI, to tell which client asking */
   data_req_st *req = value;
   tree_data_st t;
   t.gui = req->gui;  /* GUI client requesting data from daemon */
   t.app = req->app;  /* APP client sending data to daemon */
   t.tree = _load_list();

   if (t.tree)
     {  /* Reply with tree data to data request */
        ecore_con_eet_send(reply, CLOUSEAU_TREE_DATA_STR, &t);
        clouseau_data_tree_free(t.tree);
     }
}

void
_highlight_cb(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* Highlight msg contains PTR of object to highlight */
   highlight_st *ht = value;
   Evas_Object *obj = (Evas_Object *) (uintptr_t) ht->object;
   clouseau_data_object_highlight(obj, NULL, NULL);
}

void
_bmp_req_cb(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* Bitmap req msg contains PTR of Ecore Evas */
   bmp_req_st *req = value;
   Evas_Coord w, h;
   unsigned int size = 0;
   void *bmp = _canvas_bmp_get((Ecore_Evas *) (uintptr_t)
         req->object, &w, &h);

   bmp_info_st t = { req->gui,
        req->app, req->object , req->ctr, w, h,
        NULL,NULL, NULL, 1.0,
        NULL, NULL, NULL, NULL, NULL, NULL };

   void *p = clouseau_data_packet_compose(CLOUSEAU_BMP_DATA_STR,
         &t, &size, bmp, (w * h * sizeof(int)));


   if (p)
     {
        ecore_con_eet_raw_send(reply, CLOUSEAU_BMP_DATA_STR, "BMP", p, size);
        free(p);
     }

   if (bmp)
     free(bmp);
}

typedef struct
{
   Ecore_Event_Handler *ee_handle;
   Ecore_Exe *daemon_exe;
} Msg_From_Daemon_Data;

static void
_msg_from_daemon_data_free(Msg_From_Daemon_Data *msg_data)
{
   ecore_event_handler_del(msg_data->ee_handle);
   ecore_exe_free(msg_data->daemon_exe);
   free(msg_data);
}

static Eina_Bool
_msg_from_daemon(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Data *msg = (Ecore_Exe_Event_Data *)event;

   if (!strncmp(msg->data, CLOUSEAUD_READY_STR, sizeof(CLOUSEAUD_READY_STR)))
     {
        const char *address = LOCALHOST;

        if (eet_svr)
          {
             fprintf(stderr, "Clouseau: Trying to connect to daemon although already supposedly connected. Error.\n");
             return ECORE_CALLBACK_DONE;
          }

        econ_server = ecore_con_server_connect(ECORE_CON_REMOTE_TCP,
              LOCALHOST, PORT, NULL);

        if (!econ_server)
          {
             printf("could not connect to the server: %s, port %d.\n",
                   address, PORT);
             return ECORE_CALLBACK_DONE;
          }

        eet_svr = ecore_con_eet_client_new(econ_server);
        if (!eet_svr)
          {
             printf("could not create con_eet client.\n");
             return ECORE_CALLBACK_DONE;
          }

        clouseau_register_descs(eet_svr);

        /* Register callbacks for ecore_con_eet */
        ecore_con_eet_server_connect_callback_add(eet_svr, _add, NULL);
        ecore_con_eet_server_disconnect_callback_add(eet_svr, _del, NULL);
        ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_DATA_REQ_STR,
              _data_req_cb, NULL);
        ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_HIGHLIGHT_STR,
              _highlight_cb, NULL);
        ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_BMP_REQ_STR,
              _bmp_req_cb, NULL);

        _msg_from_daemon_data_free(data);
     }

   return ECORE_CALLBACK_DONE;
}

void
clouseau_app_disconnect(void)
{
   ecore_con_eet_server_free(eet_svr);
   eet_svr = NULL;
   ecore_con_server_del(econ_server);
   econ_server = NULL;
}

EAPI Eina_Bool
clouseau_app_connect(const char *appname)
{
   Msg_From_Daemon_Data *msg_data = calloc(1, sizeof(*msg_data));

   eina_stringshare_replace(&_my_appname, appname);

   msg_data->ee_handle = ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _msg_from_daemon, msg_data);
   /* FIXME: Possibly have a better way to get rid of preload. */
   msg_data->daemon_exe = ecore_exe_pipe_run("LD_PRELOAD='' " CLOUSEAUD_PATH,
         ECORE_EXE_PIPE_READ_LINE_BUFFERED |
         ECORE_EXE_PIPE_READ, NULL);

   if (!msg_data->daemon_exe)
     {
        _msg_from_daemon_data_free(msg_data);
        fprintf(stderr, "Could not start the daemon.!\n");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}
