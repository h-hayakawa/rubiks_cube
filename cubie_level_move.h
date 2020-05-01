#ifndef __CUBIE_LEVEL_MOVE_H__
#define __CUBIE_LEVEL_MOVE_H__

#include"move_defines.h"
#include"cube_structures.h"



void cubie_level_move(cubie_cube_ptr cube, int32_t mov_id);
void init_cubie_level_cube(cubie_cube_ptr cube);

void cubie_level_move_S(cubie_cube_ptr cube, int32_t sym_type);
void cubie_level_move_S_inv(cubie_cube_ptr cube, int32_t sym_type);

void sym_cubie_move8(cubie_cube cubie[8], int32_t mov);

void copy_cubie8(cubie_cube src[8], cubie_cube dst[8]);


void equivalent_cubie_lr(cubie_cube_ptr src, cubie_cube_ptr dst);
void equivalent_cubie_fb(cubie_cube_ptr src, cubie_cube_ptr dst);

void cubie_level_move_lr(cubie_cube_ptr cube, int32_t mov_id);
void cubie_level_move_fb(cubie_cube_ptr cube, int32_t mov_id);


#endif
