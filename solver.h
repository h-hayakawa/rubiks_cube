#ifndef __SOLVER_H__
#define __SOLVER_H__



#include"cube_structures.h"
#include"coordinate_level_move.h"
#include"search_node_level_move.h"


#define N_THREADS 32


/* もろもろのテーブルを初期化．solve関数を呼ぶ前に1度は呼ぶ */
void init_solver();

/* 解があれば手数を返しresultに手順を格納，解が無ければ-1を返す */
int32_t solve(coord_cube_ptr cube, int8_t *result);



#endif
