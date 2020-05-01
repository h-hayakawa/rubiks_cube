#ifndef __MOVE_TABLE_CREATOR_H__
#define __MOVE_TABLE_CREATOR_H__


#include<stdint.h>
#include"cube_structures.h"

typedef enum{
  COORD_CORNER_ORIENTATION       =  1,
  COORD_CORNER_POSITION          =  2,
  COORD_EDGE_FLIP                =  4,
  COORD_EDGE_4_POSITION          =  8,
  SEARCH_NODE_CORNER_ORIENTATION = 16,
  SEARCH_NODE_EDGE_FLIP          = 32,
  SEARCH_NODE_EDGE_4_POS         = 64,
  SEARCH_NODE_CORNER_POS         = 128
}table_index_type;

void create_move_table(void **dst_move_tab, int32_t dst_size, table_index_type idx_type, int32_t tab_elm_size);

#endif
