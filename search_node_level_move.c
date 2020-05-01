#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include"cube_structures.h"
#include"move_defines.h"
#include"common_defines.h"
#include"structure_converter.h"
#include"cubie_level_move.h"
#include"search_node_level_move.h"
#include"move_table_creator.h"

#if defined(SILENT_MODE)
#define printf(a,...)
#endif


uint16_t *SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION = NULL;//76.9KB
uint16_t *SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP = NULL;//72.0KB
uint16_t *SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION = NULL;//17.4KB
uint16_t *SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION = NULL;//17.4KB

/* 全く同じものがcubie_levelに存在 */
uint8_t sn_MOV_TRS_TAB_LR[N_MOVES];
uint8_t sn_MOV_TRS_TAB_FB[N_MOVES];

__inline
static
void init_twist_trs_tab(){
  sn_MOV_TRS_TAB_LR[MOVE_U1] = MOVE_R1;
  sn_MOV_TRS_TAB_LR[MOVE_U2] = MOVE_R2;
  sn_MOV_TRS_TAB_LR[MOVE_U3] = MOVE_R3;
  sn_MOV_TRS_TAB_LR[MOVE_R1] = MOVE_D1;
  sn_MOV_TRS_TAB_LR[MOVE_R2] = MOVE_D2;
  sn_MOV_TRS_TAB_LR[MOVE_R3] = MOVE_D3;
  sn_MOV_TRS_TAB_LR[MOVE_B1] = MOVE_B1;
  sn_MOV_TRS_TAB_LR[MOVE_B2] = MOVE_B2;
  sn_MOV_TRS_TAB_LR[MOVE_B3] = MOVE_B3;
  sn_MOV_TRS_TAB_LR[MOVE_L1] = MOVE_U1;
  sn_MOV_TRS_TAB_LR[MOVE_L2] = MOVE_U2;
  sn_MOV_TRS_TAB_LR[MOVE_L3] = MOVE_U3;
  sn_MOV_TRS_TAB_LR[MOVE_F1] = MOVE_F1;
  sn_MOV_TRS_TAB_LR[MOVE_F2] = MOVE_F2;
  sn_MOV_TRS_TAB_LR[MOVE_F3] = MOVE_F3;
  sn_MOV_TRS_TAB_LR[MOVE_D1] = MOVE_L1;
  sn_MOV_TRS_TAB_LR[MOVE_D2] = MOVE_L2;
  sn_MOV_TRS_TAB_LR[MOVE_D3] = MOVE_L3;

  sn_MOV_TRS_TAB_FB[MOVE_U1] = MOVE_F1;
  sn_MOV_TRS_TAB_FB[MOVE_U2] = MOVE_F2;
  sn_MOV_TRS_TAB_FB[MOVE_U3] = MOVE_F3;
  sn_MOV_TRS_TAB_FB[MOVE_R1] = MOVE_R1;
  sn_MOV_TRS_TAB_FB[MOVE_R2] = MOVE_R2;
  sn_MOV_TRS_TAB_FB[MOVE_R3] = MOVE_R3;
  sn_MOV_TRS_TAB_FB[MOVE_B1] = MOVE_U1;
  sn_MOV_TRS_TAB_FB[MOVE_B2] = MOVE_U2;
  sn_MOV_TRS_TAB_FB[MOVE_B3] = MOVE_U3;
  sn_MOV_TRS_TAB_FB[MOVE_L1] = MOVE_L1;
  sn_MOV_TRS_TAB_FB[MOVE_L2] = MOVE_L2;
  sn_MOV_TRS_TAB_FB[MOVE_L3] = MOVE_L3;
  sn_MOV_TRS_TAB_FB[MOVE_F1] = MOVE_D1;
  sn_MOV_TRS_TAB_FB[MOVE_F2] = MOVE_D2;
  sn_MOV_TRS_TAB_FB[MOVE_F3] = MOVE_D3;
  sn_MOV_TRS_TAB_FB[MOVE_D1] = MOVE_B1;
  sn_MOV_TRS_TAB_FB[MOVE_D2] = MOVE_B2;
  sn_MOV_TRS_TAB_FB[MOVE_D3] = MOVE_B3;
}



void search_node_level_move(search_node_cube_ptr cube, int32_t mov_id){
  int32_t mv_lr,mv_fb;
#if defined(DEBUG_PRINT)
  if (mov_id >= N_MOVES){
    printf("error in function search_node_level_move\n");
    exit(1);
  }
  if (SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION == NULL){
    printf("search node level move tables are not initialized\n");
    exit(1);
  }
  if (SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP == NULL){
    printf("search node level move tables are not initialized\n");
    exit(1);
  }
  if (SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION == NULL){
    printf("search node level move tables are not initialized\n");
    exit(1);
  }
#endif
  cube->corner_position       = SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION     [cube->corner_position       * N_MOVES + mov_id];
  cube->ud_corner_orientation = SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[cube->ud_corner_orientation * N_MOVES + mov_id];
  cube->ud_edge_flip          = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP        [cube->ud_edge_flip          * N_MOVES + mov_id];
  cube->ud_edge_ud            = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION     [cube->ud_edge_ud            * N_MOVES + mov_id];
  mv_lr = sn_MOV_TRS_TAB_LR[mov_id];
  cube->lr_corner_orientation = SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[cube->lr_corner_orientation * N_MOVES + mv_lr];
  cube->lr_edge_flip          = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP        [cube->lr_edge_flip          * N_MOVES + mv_lr];
  cube->lr_edge_ud            = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION     [cube->lr_edge_ud            * N_MOVES + mv_lr];
  mv_fb = sn_MOV_TRS_TAB_FB[mov_id];
  cube->fb_corner_orientation = SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[cube->fb_corner_orientation * N_MOVES + mv_fb];
  cube->fb_edge_flip          = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP        [cube->fb_edge_flip          * N_MOVES + mv_fb];
  cube->fb_edge_ud            = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION     [cube->fb_edge_ud            * N_MOVES + mv_fb];
}

void init_search_node_level_move_table(){
#define N_CORNER_ORIENTATION 2187 /* 3^7 */
#define N_EDGE_FLIP 2048 /* 2^11 */
#define N_EDGE_4_POS (1560*8)
#define N_CORNER_POSITION 40320
  init_twist_trs_tab();
  create_move_table((void**)&SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION, N_CORNER_ORIENTATION, SEARCH_NODE_CORNER_ORIENTATION, 2);
  create_move_table((void**)&SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP, N_EDGE_FLIP, SEARCH_NODE_EDGE_FLIP, 2);
  create_move_table((void**)&SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION, N_EDGE_4_POS, SEARCH_NODE_EDGE_4_POS, 2);
  create_move_table((void**)&SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION, N_CORNER_POSITION , SEARCH_NODE_CORNER_POS, 2);
}

void init_search_node_level_cube(search_node_cube_ptr cube){
  cubie_cube cubie;
  init_cubie_level_cube(&cubie);
  convert_cubie_to_search_node(&cubie, cube);
}

void get_search_node_level_tables(search_node_level_tables_ptr dst){
  if (SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION == NULL){
    init_search_node_level_move_table();
  }
  if (SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION == NULL){
    init_search_node_level_move_table();
  }
  if (SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP == NULL){
    init_search_node_level_move_table();
  }
  if (SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION == NULL){
    init_search_node_level_move_table();
  }
  dst->mov_corner_pos = SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION;
  dst->mov_corner_8_ori = SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION;
  dst->mov_edge_12_flip = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP;
  dst->mov_edge_4_pos = SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION;
  dst->mov_trs_lr = sn_MOV_TRS_TAB_LR;
  dst->mov_trs_fb = sn_MOV_TRS_TAB_FB;
}
