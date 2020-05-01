#ifndef __SEARCH_NODE_LEVEL_MOVE_H__
#define __SEARCH_NODE_LEVEL_MOVE_H__



typedef struct __search_node_level_tables__{
  uint16_t *mov_corner_pos;
  uint16_t *mov_corner_8_ori;
  uint16_t *mov_edge_12_flip;
  uint16_t *mov_edge_4_pos;
  uint8_t *mov_trs_lr;
  uint8_t *mov_trs_fb;
}search_node_level_tables, *search_node_level_tables_ptr;



void search_node_level_move(search_node_cube_ptr cube, int32_t mov_id);
void init_search_node_level_move_table();
void init_search_node_level_cube(search_node_cube_ptr cube);
void get_search_node_level_tables(search_node_level_tables_ptr dst);


#endif
