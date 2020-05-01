#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include"cube_structures.h"
#include"move_defines.h"
#include"common_defines.h"
#include"structure_converter.h"
#include"cubie_level_move.h"
#include"coordinate_level_move.h"
#include"move_table_creator.h"

#if defined(SILENT_MODE)
#define printf(a,...)
#endif


uint16_t *COORDINATE_LEVEL_MOVE_TAB_CORNER_8_POSITION=NULL;//1.38MB
uint16_t *COORDINATE_LEVEL_MOVE_TAB_CORNER_8_ORIENTATION=NULL;//76.9KB
uint16_t *COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION=NULL;//418KB
uint16_t *COORDINATE_LEVEL_MOVE_TAB_EDGE_12_FLIP=NULL;//72.0KB




void coordinate_level_move(coord_cube_ptr cube, int32_t mov_id){
  #if defined(DEBUG_PRINT)
  if(mov_id >= N_MOVES){
    printf("error in function coordinate_level_move\n");
    exit(1);
  }
  if (COORDINATE_LEVEL_MOVE_TAB_CORNER_8_ORIENTATION == NULL){
    printf("coordinate level move tables are not initialized\n");
  }
  if (COORDINATE_LEVEL_MOVE_TAB_CORNER_8_POSITION == NULL){
    printf("coordinate level move tables are not initialized\n");
  }
  if (COORDINATE_LEVEL_MOVE_TAB_EDGE_12_FLIP == NULL){
    printf("coordinate level move tables are not initialized\n");
  }
  if (COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION == NULL){
    printf("coordinate level move tables are not initialized\n");
  }
  #endif
  cube->corner_orientation = COORDINATE_LEVEL_MOVE_TAB_CORNER_8_ORIENTATION[cube->corner_orientation * N_MOVES + mov_id];
  cube->corner_position    = COORDINATE_LEVEL_MOVE_TAB_CORNER_8_POSITION   [cube->corner_position    * N_MOVES + mov_id];
  cube->edge_flip          = COORDINATE_LEVEL_MOVE_TAB_EDGE_12_FLIP        [cube->edge_flip          * N_MOVES + mov_id];
  cube->edge_position_ud   = COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION     [cube->edge_position_ud   * N_MOVES + mov_id];
  cube->edge_position_lr   = COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION     [cube->edge_position_lr   * N_MOVES + mov_id];
  cube->edge_position_fb   = COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION     [cube->edge_position_fb   * N_MOVES + mov_id];
}

void init_coordinate_level_move_table(){
  #define N_CORNER_ORIENTATION 2187 /* 3^7 */
  #define N_CORNER_POSITION 40320  /* 8! */
  #define N_EDGE_FLIP 2048 /* 2^11 */
  #define N_EDGE_4_POS 11880 /* 12*11*10*9 */
  create_move_table((void**)&COORDINATE_LEVEL_MOVE_TAB_CORNER_8_ORIENTATION, N_CORNER_ORIENTATION, COORD_CORNER_ORIENTATION, 2);
  create_move_table((void**)&COORDINATE_LEVEL_MOVE_TAB_CORNER_8_POSITION, N_CORNER_POSITION, COORD_CORNER_POSITION, 2);
  create_move_table((void**)&COORDINATE_LEVEL_MOVE_TAB_EDGE_12_FLIP, N_EDGE_FLIP, COORD_EDGE_FLIP, 2);
  create_move_table((void**)&COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION, N_EDGE_4_POS, COORD_EDGE_4_POSITION, 2);
}

void init_coordinate_level_cube(coord_cube_ptr cube){
  cubie_cube cubie;
  init_cubie_level_cube(&cubie);
  convert_cubie_to_coordinate(&cubie, cube);
}


/* テーブル化すれば速くなる */
void coordinate_level_move_S(coord_cube_ptr cube, int32_t sym_type){
  cubie_cube cubie;
  convert_coordinate_to_cubie(cube, &cubie);
  cubie_level_move_S(&cubie, sym_type);
  convert_cubie_to_coordinate(&cubie, cube);
}

void coordinate_level_move_S_inv(coord_cube_ptr cube, int32_t sym_type){
  cubie_cube cubie;
  convert_coordinate_to_cubie(cube, &cubie);
  cubie_level_move_S_inv(&cubie, sym_type);
  convert_cubie_to_coordinate(&cubie, cube);
}

void equivalent_coord_lr(coord_cube_ptr src, coord_cube_ptr dst){
  cubie_cube cubie, tmp;
  convert_coordinate_to_cubie(src, &cubie);
  equivalent_cubie_lr(&cubie,&tmp);
  convert_cubie_to_coordinate(&tmp, dst);
}
void equivalent_coord_fb(coord_cube_ptr src, coord_cube_ptr dst){
  cubie_cube cubie, tmp;
  convert_coordinate_to_cubie(src, &cubie);
  equivalent_cubie_fb(&cubie, &tmp);
  convert_cubie_to_coordinate(&tmp, dst);
}


void get_coordinate_level_tables(coordinate_level_tables_ptr dst){
  if (COORDINATE_LEVEL_MOVE_TAB_CORNER_8_POSITION == NULL){
    init_coordinate_level_move_table();
  }
  if (COORDINATE_LEVEL_MOVE_TAB_CORNER_8_ORIENTATION == NULL){
    init_coordinate_level_move_table();
  }
  if (COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION == NULL){
    init_coordinate_level_move_table();
  }
  if (COORDINATE_LEVEL_MOVE_TAB_EDGE_12_FLIP == NULL){
    init_coordinate_level_move_table();
  }
  dst->mov_corner_8_pos = COORDINATE_LEVEL_MOVE_TAB_CORNER_8_POSITION;
  dst->mov_corner_8_ori = COORDINATE_LEVEL_MOVE_TAB_CORNER_8_ORIENTATION;
  dst->mov_edge_12_flip = COORDINATE_LEVEL_MOVE_TAB_EDGE_12_FLIP;
  dst->mov_edge_4_pos = COORDINATE_LEVEL_MOVE_TAB_EDGE_4_POSITION;
}




