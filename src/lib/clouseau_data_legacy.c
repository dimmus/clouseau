#include "clouseau_data_legacy.h"

Eet_Data_Descriptor *clouseau_object_edd = NULL;
Eet_Data_Descriptor *clouseau_union_edd = NULL;
static Eet_Data_Descriptor *clouseau_textblock_edd = NULL;
static Eet_Data_Descriptor *clouseau_edje_edd = NULL;
static Eet_Data_Descriptor *clouseau_image_edd = NULL;
static Eet_Data_Descriptor *clouseau_text_edd = NULL;
static Eet_Data_Descriptor *clouseau_elm_edd = NULL;
static Eet_Data_Descriptor *clouseau_map_point_props_edd = NULL;

static struct {
   Clouseau_Object_Type u;
   const char *name;
} eet_props_mapping[] = {
   /* As eet_mapping */
   { CLOUSEAU_OBJ_TYPE_OTHER, "CLOUSEAU_OBJ_TYPE_OTHER" },
   { CLOUSEAU_OBJ_TYPE_ELM, "CLOUSEAU_OBJ_TYPE_ELM" },
   { CLOUSEAU_OBJ_TYPE_TEXT, "CLOUSEAU_OBJ_TYPE_TEXT" },
   { CLOUSEAU_OBJ_TYPE_IMAGE, "CLOUSEAU_OBJ_TYPE_IMAGE" },
   { CLOUSEAU_OBJ_TYPE_EDJE, "CLOUSEAU_OBJ_TYPE_EDJE" },
   { CLOUSEAU_OBJ_TYPE_TEXTBLOCK, "CLOUSEAU_OBJ_TYPE_TEXTBLOCK" },
   { CLOUSEAU_OBJ_TYPE_UNKNOWN, NULL }
};

static const char *
_clouseau_props_union_type_get(const void *data, Eina_Bool  *unknow)
{  /* _union_type_get */
   const Clouseau_Object_Type *u = data;
   int i;

   if (unknow)
     *unknow = EINA_FALSE;

   for (i = 0; eet_props_mapping[i].name != NULL; ++i)
     if (*u == eet_props_mapping[i].u)
       return eet_props_mapping[i].name;

   if (unknow)
     *unknow = EINA_TRUE;

   return NULL;
}

static Eina_Bool
_clouseau_props_union_type_set(const char *type, void *data, Eina_Bool unknow)
{  /* same as _union_type_set */
   Clouseau_Object_Type *u = data;
   int i;

   if (unknow)
     return EINA_FALSE;

   for (i = 0; eet_props_mapping[i].name != NULL; ++i)
     if (strcmp(eet_props_mapping[i].name, type) == 0)
       {
          *u = eet_props_mapping[i].u;
          return EINA_TRUE;
       }

   return EINA_FALSE;
}

static void
_clouseau_elm_desc_make(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Elm_Props);
   clouseau_elm_edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_elm_edd, Clouseau_Elm_Props,
                                 "type", type, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_elm_edd, Clouseau_Elm_Props,
                                 "style", style, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_elm_edd, Clouseau_Elm_Props,
                                 "scale", scale, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_elm_edd, Clouseau_Elm_Props,
                                 "has_focus", has_focus, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_elm_edd, Clouseau_Elm_Props,
                                 "is_disabled", is_disabled, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_elm_edd, Clouseau_Elm_Props,
                                 "is_mirrored", is_mirrored, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_elm_edd, Clouseau_Elm_Props,
                                 "is_mirrored_automatic", is_mirrored_automatic, EET_T_UCHAR);
}

static void
_clouseau_text_desc_make(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Evas_Text_Props);
   clouseau_text_edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_text_edd, Clouseau_Evas_Text_Props,
                                 "font", font, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_text_edd, Clouseau_Evas_Text_Props,
                                 "size", size, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_text_edd, Clouseau_Evas_Text_Props,
                                 "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_text_edd, Clouseau_Evas_Text_Props,
                                 "text", text, EET_T_STRING);
}

static void
_clouseau_image_desc_make(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Evas_Image_Props);
   clouseau_image_edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_image_edd, Clouseau_Evas_Image_Props,
                                 "file", file, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_image_edd, Clouseau_Evas_Image_Props,
                                 "key", key, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_image_edd, Clouseau_Evas_Image_Props,
                                 "source", source, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_image_edd, Clouseau_Evas_Image_Props,
                                 "load_err", load_err, EET_T_STRING);
}

static void
_clouseau_edje_desc_make(void)
{
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Edje_Props);
   clouseau_edje_edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_edje_edd, Clouseau_Edje_Props,
                                 "file", file, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_edje_edd, Clouseau_Edje_Props,
                                 "group", group, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_edje_edd, Clouseau_Edje_Props,
                                 "load_err", load_err, EET_T_STRING);
}

static void
_clouseau_textblock_desc_make(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Evas_Textblock_Props);
   clouseau_textblock_edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_textblock_edd, Clouseau_Evas_Textblock_Props,
                                 "style", style, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_textblock_edd, Clouseau_Evas_Textblock_Props,
                                 "text", text, EET_T_STRING);
}

static void
_clouseau_map_point_props_desc_make(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc,
         Clouseau_Evas_Map_Point_Props);

   clouseau_map_point_props_edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_map_point_props_edd,
         Clouseau_Evas_Map_Point_Props, "x", x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_map_point_props_edd,
         Clouseau_Evas_Map_Point_Props, "y", y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_map_point_props_edd,
         Clouseau_Evas_Map_Point_Props, "z", z, EET_T_INT);
}


void
clouseau_data_descriptors_legacy_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Object);
   clouseau_object_edd = eet_data_descriptor_stream_new(&eddc);

   /* START - evas_props Struct desc */
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.name", evas_props.name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.bt", evas_props.bt, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.layer", evas_props.layer, EET_T_SHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.x", evas_props.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.y", evas_props.y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.w", evas_props.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.h", evas_props.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.scale", evas_props.scale, EET_T_DOUBLE);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.min_w", evas_props.min_w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.min_h", evas_props.min_h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.max_w", evas_props.max_w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.max_h", evas_props.max_h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.req_w", evas_props.req_w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.req_h", evas_props.req_h, EET_T_INT);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.align_x", evas_props.align_x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.align_y", evas_props.align_y, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.weight_x", evas_props.weight_x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.weight_y", evas_props.weight_y, EET_T_DOUBLE);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.r", evas_props.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.g", evas_props.g, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.b", evas_props.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.a", evas_props.a, EET_T_INT);

   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.pass_events", evas_props.pass_events, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.repeat_events", evas_props.repeat_events, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.propagate_events", evas_props.propagate_events, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.has_focus", evas_props.has_focus, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.is_clipper", evas_props.is_clipper, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.is_visible", evas_props.is_visible, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.clipper", evas_props.clipper, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(clouseau_object_edd, Clouseau_Object,
                                 "evas_props.mode", evas_props.mode, EET_T_INT);

   _clouseau_elm_desc_make();
   _clouseau_text_desc_make();
   _clouseau_image_desc_make();
   _clouseau_edje_desc_make();
   _clouseau_textblock_desc_make();

   _clouseau_map_point_props_desc_make();
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(clouseau_object_edd, Clouseau_Object,
         "evas_props.points", evas_props.points,
         clouseau_map_point_props_edd);

   /* for union */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Extra_Props);
   eddc.func.type_get = _clouseau_props_union_type_get;
   eddc.func.type_set = _clouseau_props_union_type_set;
   clouseau_union_edd = eet_data_descriptor_file_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_MAPPING(clouseau_union_edd,
                                   "CLOUSEAU_OBJ_TYPE_ELM", clouseau_elm_edd);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(clouseau_union_edd,
                                   "CLOUSEAU_OBJ_TYPE_TEXT",
                                   clouseau_text_edd);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(clouseau_union_edd,
                                   "CLOUSEAU_OBJ_TYPE_IMAGE",
                                   clouseau_image_edd);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(clouseau_union_edd,
                                   "CLOUSEAU_OBJ_TYPE_EDJE",
                                   clouseau_edje_edd);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(clouseau_union_edd,
                                   "CLOUSEAU_OBJ_TYPE_TEXTBLOCK",
                                   clouseau_textblock_edd);

   EET_DATA_DESCRIPTOR_ADD_UNION(clouseau_object_edd, Clouseau_Object,
                                 "u", extra_props.u,
                                 extra_props.type, clouseau_union_edd);
}

void
clouseau_data_descriptors_legacy_shutdown(void)
{
   eet_data_descriptor_free(clouseau_object_edd);
   eet_data_descriptor_free(clouseau_elm_edd);
   eet_data_descriptor_free(clouseau_text_edd);
   eet_data_descriptor_free(clouseau_image_edd);
   eet_data_descriptor_free(clouseau_edje_edd);
   eet_data_descriptor_free(clouseau_textblock_edd);
   eet_data_descriptor_free(clouseau_union_edd);
   eet_data_descriptor_free(clouseau_map_point_props_edd);
}

