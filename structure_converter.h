#ifndef __STRUCTURE_CONVERTER_H__
#define __STRUCTURE_CONVERTER_H__

#include"cube_structures.h"

/* 可逆変換 */
void convert_cubie_to_coordinate(cubie_cube_ptr src, coord_cube_ptr dst);
void convert_coordinate_to_cubie(coord_cube_ptr src, cubie_cube_ptr dst);


/* 不可逆変換(探索ノードの構造に依るが，可逆変換である必要は無い) */
void convert_cubie_to_search_node(cubie_cube_ptr src, search_node_cube_ptr dst);
void convert_coord_to_search_node(coord_cube_ptr src, search_node_cube_ptr dst);

void init_node_flip_ud_tab();
int32_t valid_idx(int32_t i);




#endif