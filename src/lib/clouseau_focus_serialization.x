
static void
_init_data_descriptors(void)
{
   Eet_Data_Descriptor_Class klass;
   Eet_Data_Descriptor *relations_eed;

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&klass, Clouseau_Focus_Relation);
   relations_eed = eet_data_descriptor_file_new(&klass);
   #define BASIC(field, type)    EET_DATA_DESCRIPTOR_ADD_BASIC(relations_eed, Clouseau_Focus_Relation, #field , field, type)

   BASIC(class_name, EET_T_STRING);
   BASIC(relation.next, EET_T_UINT);
   BASIC(relation.prev, EET_T_UINT);
   BASIC(relation.logical, EET_T_CHAR);
   BASIC(relation.parent, EET_T_UINT);
   BASIC(relation.redirect, EET_T_UINT);
   BASIC(relation.node, EET_T_UINT);
   BASIC(relation.position_in_history, EET_T_INT);

   #undef BASIC
   #define LIST(field, type)    EET_DATA_DESCRIPTOR_ADD_LIST(relations_eed, Clouseau_Focus_Relation, #field , field, type)

   /*LIST(relation.right, EET_T_UINT);
   LIST(relation.left, EET_T_UINT);
   LIST(relation.top, EET_T_UINT);
   LIST(relation.down, EET_T_UINT);*/

   #undef LIST

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&klass, Clouseau_Focus_Manager_Data);
   manager_details = eet_data_descriptor_file_new(&klass);

   EET_DATA_DESCRIPTOR_ADD_BASIC(manager_details, Clouseau_Focus_Manager_Data, "redirect_manager", redirect_manager, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(manager_details, Clouseau_Focus_Manager_Data, "focused", focused, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(manager_details, Clouseau_Focus_Manager_Data, "class_name", class_name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST(manager_details, Clouseau_Focus_Manager_Data, "relations", relations, relations_eed);
}