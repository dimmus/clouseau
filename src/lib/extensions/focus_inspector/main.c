#include <Eina.h>
#include <Elementary.h>
#include "../../Clouseau.h"
#include "../../Clouseau_Debug.h"
#include "gui.h"

static Instance inst;

static int _focus_manager_list_op = EINA_DEBUG_OPCODE_INVALID;
static int _focus_manager_detail_op = EINA_DEBUG_OPCODE_INVALID;

static Eet_Data_Descriptor *manager_details = NULL, *manager_list = NULL;
#include "../../clouseau_focus_serialization.x"

static Eina_Bool
_main_loop_focus_manager_list_cb(Eina_Debug_Session *session, int src EINA_UNUSED, void *buffer, int size)
{
   Clouseau_Extension *ext = eina_debug_session_data_get(session);
   Clouseau_Focus_Managers *managers;
   if (!manager_list) _init_manager_list_descriptors();

   managers = eet_data_descriptor_decode(manager_list, buffer, size);

   ui_managers_add(ext->data, managers);

   return EINA_TRUE;
}

WRAPPER_TO_XFER_MAIN_LOOP(_focus_manager_list_cb)

static Eina_Bool
_main_loop_focus_manager_detail_cb(Eina_Debug_Session *session, int src EINA_UNUSED, void *buffer, int size)
{
   Clouseau_Extension *ext = eina_debug_session_data_get(session);
   Clouseau_Focus_Manager_Data *pd;
   if (!manager_details) _init_data_descriptors();

   pd = eet_data_descriptor_decode(manager_details, buffer, size);

   ui_manager_data_arrived(ext->data, pd);
   return EINA_TRUE;
}

WRAPPER_TO_XFER_MAIN_LOOP(_focus_manager_detail_cb)

EINA_DEBUG_OPCODES_ARRAY_DEFINE(_ops,
     {"Clouseau/Elementary_Focus/list", &_focus_manager_list_op, &_focus_manager_list_cb},
     {"Clouseau/Elementary_Focus/detail", &_focus_manager_detail_op, &_focus_manager_detail_cb},
     {NULL, NULL, NULL}
);

static void
_session_changed(Clouseau_Extension *ext)
{
   int i = 0;
   Eina_Debug_Opcode *ops = _ops();

   while (ops[i].opcode_name)
     {
        if (ops[i].opcode_id) *(ops[i].opcode_id) = EINA_DEBUG_OPCODE_INVALID;
        i++;
     }
   if (ext->session)
     {
        eina_debug_session_data_set(ext->session, ext);
        eina_debug_opcodes_register(ext->session, ops, NULL, ext);
     }

   ui_managers_add(ext->data, NULL);
   ui_manager_data_arrived(ext->data, NULL);

}

static void
_app_changed(Clouseau_Extension *ext)
{
   ui_managers_add(ext->data, NULL);
   ui_manager_data_arrived(ext->data, NULL);

   com_refresh_managers(ext->data);
}

EAPI void
com_refresh_managers(Instance *ext_inst)
{
   eina_debug_session_send(ext_inst->ext->session, ext_inst->ext->app_id, _focus_manager_list_op, NULL, 0);
}

EAPI void
com_defailt_manager(Instance *ext_inst, Efl_Ui_Focus_Manager *manager)
{
   void *tmp[1];
   tmp[0] = manager;

   eina_debug_session_send(ext_inst->ext->session, ext_inst->ext->app_id, _focus_manager_detail_op, tmp, sizeof(void*));
}

EAPI const char *
extension_name_get()
{
   return "Focus Inspector";
}

EAPI Eina_Bool
extension_start(Clouseau_Extension *ext, Eo *parent)
{
   eina_init();

   inst.ext = ext;
   ext->data = &inst;
   ext->ui_object = ui_create(ext->data, parent);
   ext->session_changed_cb = _session_changed;
   ext->app_changed_cb = _app_changed;

   return !!ext->ui_object;
}

EAPI Eina_Bool
extension_stop(Clouseau_Extension *ext)
{
   efl_del(ext->ui_object);

   eina_shutdown();

   return EINA_TRUE;
}
