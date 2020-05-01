#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include"cube_structures.h"
#include"move_defines.h"
#include"move_table_creator.h"
#include"structure_converter.h"
#include"coordinate_level_move.h"
#include"cubie_level_move.h"
#include"common_defines.h"

#if defined(SILENT_MODE)
#define printf(a,...)
#endif

__inline
static uint32_t cubie_to_table_index(cubie_cube_ptr cubie, table_index_type idx_type){
  if (idx_type & (COORD_CORNER_ORIENTATION | COORD_CORNER_POSITION | COORD_EDGE_FLIP | COORD_EDGE_4_POSITION)){
    coord_cube coord;
    convert_cubie_to_coordinate(cubie, &coord);
    switch (idx_type){
    case COORD_CORNER_ORIENTATION:
      return coord.corner_orientation;
    case COORD_CORNER_POSITION:
      return coord.corner_position;
    case COORD_EDGE_FLIP:
      return coord.edge_flip;
    case COORD_EDGE_4_POSITION:
      return coord.edge_position_ud;
    default:
#if defined(DEBUG_PRINT)
      printf("error in function cubie_to_table_index\n");
#endif
      return 0;
    }
  }
  else if (idx_type & (SEARCH_NODE_CORNER_ORIENTATION | SEARCH_NODE_EDGE_FLIP | SEARCH_NODE_EDGE_4_POS | SEARCH_NODE_CORNER_POS)){
    search_node_cube search_node;
    convert_cubie_to_search_node(cubie, &search_node);
    switch (idx_type){
    case SEARCH_NODE_CORNER_ORIENTATION:
      return search_node.ud_corner_orientation;
    case SEARCH_NODE_EDGE_FLIP:
      return search_node.ud_edge_flip;
    case SEARCH_NODE_EDGE_4_POS:
      return search_node.ud_edge_ud;
    case SEARCH_NODE_CORNER_POS:
      return search_node.corner_position;
    default:
#if defined(DEBUG_PRINT)
      printf("error in function cubie_to_table_index\n");
#endif
      return 0;
    }
  }
#if defined(DEBUG_PRINT)
  printf("error in function cubie_to_table_index\n");
#endif
  return 0;
}


void create_move_table(void **dst_move_tab, int32_t dst_size, table_index_type idx_type, int32_t tab_elm_size){
  int32_t i;
  void *dst_tab;
  cubie_cube *sorted;
  uint8_t *searched;
  int32_t sorted_entry = 0;
  cubie_cube cubie;
  uint32_t table_index;
  int32_t entry_count = 0;
  dst_tab = malloc(tab_elm_size * dst_size * N_MOVES);
  sorted = (cubie_cube *)malloc(sizeof(cubie_cube)* dst_size);
  searched = (uint8_t*)calloc(dst_size, sizeof(uint8_t));

  for (i = 0; i < dst_size * N_MOVES; i++){
    if (tab_elm_size == 2){
      ((uint16_t*)dst_tab)[i] = 0xFFFFu;
    }
    else if (tab_elm_size == 4){
      ((uint32_t*)dst_tab)[i] = 0xFFFFFFFFu;
    }
  }

  init_cubie_level_cube(&cubie);
  table_index = cubie_to_table_index(&cubie, idx_type);
  sorted[sorted_entry++] = cubie;
  searched[table_index] = 1;
  entry_count++;
  for (i = 0; i<sorted_entry; i++){
    int32_t j;
    for (j = 0; j < N_MOVES; j++){
      cubie_cube cubie_next = sorted[i];
      uint32_t src_index;
      src_index = cubie_to_table_index(&cubie_next, idx_type);
      cubie_level_move(&cubie_next, (move_ids)j);
      table_index = cubie_to_table_index(&cubie_next, idx_type);
      if (tab_elm_size == 2){
        ((uint16_t*)dst_tab)[src_index * N_MOVES + j] = table_index;
      }
      else if (tab_elm_size == 4){
        ((uint32_t*)dst_tab)[src_index * N_MOVES + j] = table_index;
      }
      if (!searched[table_index]){
        sorted[sorted_entry++] = cubie_next;
        searched[table_index] = 1;
        entry_count++;
      }
    }
  }
  *dst_move_tab = dst_tab;
  free(sorted);
  free(searched);
}
