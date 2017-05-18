#define _GNU_SOURCE
#include <dlfcn.h>

#include <Eo.h>

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

static int _snapshot_do_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_done_op = EINA_DEBUG_OPCODE_INVALID;
static int _klids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _eoids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_info_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_objs_get_op = EINA_DEBUG_OPCODE_INVALID;

static Eina_Debug_Error
_snapshot_do_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size)
{
   char *buf = alloca(sizeof(Eina_Debug_Packet_Header) + size);
   Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buf;

   hdr->size = sizeof(Eina_Debug_Packet_Header);
   hdr->cid = srcid;
   hdr->thread_id = 0;
   hdr->opcode = _klids_get_op;
   eina_debug_dispatch(session, buf);

   hdr->size = sizeof(Eina_Debug_Packet_Header) + size;
   if (size) memcpy(buf + sizeof(Eina_Debug_Packet_Header), buffer, size);
   hdr->thread_id = 0xFFFFFFFF;
   hdr->opcode = _snapshot_objs_get_op;
   eina_debug_dispatch(session, buf);

   eina_debug_session_send(session, srcid, _snapshot_done_op, NULL, 0);
   return EINA_DEBUG_OK;
}

typedef struct
{
   Eina_Debug_Session *session;
   char *buf;
   uint64_t *kls;
   int nb_kls;
} _EoIds_Walk_Data;

static Eina_Bool
_eoids_walk_cb(void *_data, Eo *obj)
{
   _EoIds_Walk_Data *data = _data;
   int i;
   Eina_Bool klass_ok = EINA_FALSE;

   for (i = 0; i < data->nb_kls && !klass_ok; i++)
     {
        if (efl_isa(obj, (Efl_Class *)data->kls[i])) klass_ok = EINA_TRUE;
     }
   if (!klass_ok && data->nb_kls) return EINA_TRUE;

   uint64_t u64 = (uint64_t)obj;
   memcpy(data->buf + sizeof(Eina_Debug_Packet_Header), &u64, sizeof(u64));
   eina_debug_dispatch(data->session, data->buf);

   return EINA_TRUE;
}

static Eina_Debug_Error
_snapshot_objs_get_req_cb(Eina_Debug_Session *session, int srcid, void *buffer, int size)
{
   static Eina_Bool (*foo)(Eo_Debug_Object_Iterator_Cb, void *) = NULL;
   _EoIds_Walk_Data data;

   char *buf = alloca(sizeof(Eina_Debug_Packet_Header) + size);
   Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buf;

   hdr->size = sizeof(Eina_Debug_Packet_Header) + size;
   if (size) memcpy(buf + sizeof(Eina_Debug_Packet_Header), buffer, size);
   hdr->cid = srcid;
   hdr->thread_id = 0;
   hdr->opcode = _eoids_get_op;
   eina_debug_dispatch(session, buf);

   hdr->size = sizeof(Eina_Debug_Packet_Header) + sizeof(uint64_t);
   hdr->cid = srcid;
   hdr->thread_id = 0;
   hdr->opcode = _obj_info_op;

   data.session = session;
   data.buf = buf;
   data.kls = buffer;
   data.nb_kls = size / sizeof(uint64_t);
   if (!foo) foo = dlsym(RTLD_DEFAULT, "eo_debug_objects_iterate");
   foo(_eoids_walk_cb, &data);

   return EINA_DEBUG_OK;
}

static const Eina_Debug_Opcode _debug_ops[] =
{
     {"Clouseau/snapshot_do", &_snapshot_do_op, &_snapshot_do_cb},
     {"Clouseau/snapshot/objs_get", &_snapshot_objs_get_op, &_snapshot_objs_get_req_cb},
     {"Clouseau/snapshot_done", &_snapshot_done_op, NULL},
     {"Eo/classes_ids_get", &_klids_get_op, NULL},
     {"Eo/objects_ids_get", &_eoids_get_op, NULL},
     {"Eolian/object/info_get", &_obj_info_op, NULL},
     {NULL, NULL, NULL}
};

EAPI Eina_Bool
clouseau_debug_init(void)
{
   eina_init();

   eina_debug_opcodes_register(NULL, _debug_ops, NULL, NULL);

   printf("%s - In\n", __FUNCTION__);
   return EINA_TRUE;
}

EAPI Eina_Bool
clouseau_debug_shutdown(void)
{
   eina_shutdown();

   return EINA_TRUE;
}
