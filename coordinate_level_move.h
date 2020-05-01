#ifndef __COORDINATE_LEVEL_MOVE_H__
#define __COORDINATE_LEVEL_MOVE_H__

#include"move_defines.h"
#include"cube_structures.h"

typedef struct __coordinate_level_tables__{
  uint16_t *mov_corner_8_pos;
  uint16_t *mov_corner_8_ori;
  uint16_t *mov_edge_12_flip;
  uint16_t *mov_edge_4_pos;
}coordinate_level_tables, *coordinate_level_tables_ptr;


void coordinate_level_move(coord_cube_ptr cube, int32_t mov_id);
void init_coordinate_level_move_table();
void init_coordinate_level_cube(coord_cube_ptr cube);

void coordinate_level_move_S(coord_cube_ptr cube, int32_t sym_type);
void coordinate_level_move_S_inv(coord_cube_ptr cube, int32_t sym_type);

void equivalent_coord_lr(coord_cube_ptr src, coord_cube_ptr dst);
void equivalent_coord_fb(coord_cube_ptr src, coord_cube_ptr dst);

void get_coordinate_level_tables(coordinate_level_tables_ptr dst);

#endif
