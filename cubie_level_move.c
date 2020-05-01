#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include"cube_structures.h"
#include"move_defines.h"
#include"cubie_level_move.h"
#include"common_defines.h"

#if defined(SILENT_MODE)
#define printf(a,...)
#endif


/* コーナーキューブにおける各面操作 */
/* move_idに指定された4つのコーナーキューブを置換する
substitute_type0 : (0,1,2,3)->(3,0,1,2)
substitute_type1 : (0,1,2,3)->(2,3,0,1) 
substitute_type2 : (0,1,2,3)->(1,2,3,0)
orientation_switchはコーナーキューブの捻じれ関係が変化する場合に1を指定(L1,L3,F1,F3,R1,R3,B1,B3)
*/
__inline
static void corner_move(corner_position_val_type corner_pos[8], corner_orientation_val_type corner_orientation[8],
  const int8_t move_id[4], int8_t substitute_type, int8_t orientation_switch){
    corner_position_val_type pos_tmp;
    corner_orientation_val_type flip_tmp;
    corner_orientation_val_type ori1, ori2;
    ori1 = orientation_switch;
    ori2 = orientation_switch + orientation_switch;
    switch (substitute_type){
      case 0:
      pos_tmp = corner_pos[move_id[0]];
      corner_pos[move_id[0]] = corner_pos[move_id[3]];
      corner_pos[move_id[3]] = corner_pos[move_id[2]];
      corner_pos[move_id[2]] = corner_pos[move_id[1]];
      corner_pos[move_id[1]] = pos_tmp;
      flip_tmp = (corner_orientation[move_id[0]] + ori1) % 3;
      corner_orientation[move_id[0]] = (corner_orientation[move_id[3]] + ori2) % 3;
      corner_orientation[move_id[3]] = (corner_orientation[move_id[2]] + ori1) % 3;
      corner_orientation[move_id[2]] = (corner_orientation[move_id[1]] + ori2) % 3;
      corner_orientation[move_id[1]] = flip_tmp;
      break;
      case 1:
      pos_tmp = corner_pos[move_id[0]];
      corner_pos[move_id[0]] = corner_pos[move_id[2]];
      corner_pos[move_id[2]] = pos_tmp;
      pos_tmp = corner_pos[move_id[1]];
      corner_pos[move_id[1]] = corner_pos[move_id[3]];
      corner_pos[move_id[3]] = pos_tmp;
      flip_tmp = corner_orientation[move_id[0]];
      corner_orientation[move_id[0]] = corner_orientation[move_id[2]];
      corner_orientation[move_id[2]] = flip_tmp;
      flip_tmp = corner_orientation[move_id[1]];
      corner_orientation[move_id[1]] = corner_orientation[move_id[3]];
      corner_orientation[move_id[3]] = flip_tmp;
      break;
      case 2:
      pos_tmp = corner_pos[move_id[0]];
      corner_pos[move_id[0]] = corner_pos[move_id[1]];
      corner_pos[move_id[1]] = corner_pos[move_id[2]];
      corner_pos[move_id[2]] = corner_pos[move_id[3]];
      corner_pos[move_id[3]] = pos_tmp;
      flip_tmp = (corner_orientation[move_id[0]] + ori1) % 3;
      corner_orientation[move_id[0]] = (corner_orientation[move_id[1]] + ori2) % 3;
      corner_orientation[move_id[1]] = (corner_orientation[move_id[2]] + ori1) % 3;
      corner_orientation[move_id[2]] = (corner_orientation[move_id[3]] + ori2) % 3;
      corner_orientation[move_id[3]] = flip_tmp;
      break;
    }
  }
  
  /* エッジキューブにおける各面操作 */
  /* move_idに指定された4つのエッジキューブを置換する
  substitute_type0 : (0,1,2,3)->(3,0,1,2)
  substitute_type1 : (0,1,2,3)->(2,3,0,1)
  substitute_type2 : (0,1,2,3)->(1,2,3,0)
  flip_switchはエッジキューブの反転関係に影響を与える操作の時に1にする．F1, F3, B1, B3
  */
  __inline
  static void edge_move(edge_position_val_type edge_pos[12], edge_flip_val_type edge_flip[12], const int8_t move_id[4], int8_t substitute_type, edge_flip_val_type flip_switch){
    edge_position_val_type pos_tmp;
    edge_flip_val_type flip_tmp;
    switch (substitute_type){
      case 0:
      pos_tmp = edge_pos[move_id[0]];
      edge_pos[move_id[0]] = edge_pos[move_id[3]];
      edge_pos[move_id[3]] = edge_pos[move_id[2]];
      edge_pos[move_id[2]] = edge_pos[move_id[1]];
      edge_pos[move_id[1]] = pos_tmp;
      flip_tmp = edge_flip[move_id[0]] ^ flip_switch;
      edge_flip[move_id[0]] = edge_flip[move_id[3]] ^ flip_switch;
      edge_flip[move_id[3]] = edge_flip[move_id[2]] ^ flip_switch;
      edge_flip[move_id[2]] = edge_flip[move_id[1]] ^ flip_switch;
      edge_flip[move_id[1]] = flip_tmp;
      break;
      case 1:
      pos_tmp = edge_pos[move_id[0]];
      edge_pos[move_id[0]] = edge_pos[move_id[2]];
      edge_pos[move_id[2]] = pos_tmp;
      pos_tmp = edge_pos[move_id[1]];
      edge_pos[move_id[1]] = edge_pos[move_id[3]];
      edge_pos[move_id[3]] = pos_tmp;
      flip_tmp = edge_flip[move_id[0]];
      edge_flip[move_id[0]] = edge_flip[move_id[2]];
      edge_flip[move_id[2]] = flip_tmp;
      flip_tmp = edge_flip[move_id[1]];
      edge_flip[move_id[1]] = edge_flip[move_id[3]];
      edge_flip[move_id[3]] = flip_tmp;
      break;
      case 2:
      pos_tmp = edge_pos[move_id[0]];
      edge_pos[move_id[0]] = edge_pos[move_id[1]];
      edge_pos[move_id[1]] = edge_pos[move_id[2]];
      edge_pos[move_id[2]] = edge_pos[move_id[3]];
      edge_pos[move_id[3]] = pos_tmp;
      flip_tmp = edge_flip[move_id[0]] ^ flip_switch;
      edge_flip[move_id[0]] = edge_flip[move_id[1]] ^ flip_switch;
      edge_flip[move_id[1]] = edge_flip[move_id[2]] ^ flip_switch;
      edge_flip[move_id[2]] = edge_flip[move_id[3]] ^ flip_switch;
      edge_flip[move_id[3]] = flip_tmp;
      break;
    }
  }
  
  static const int8_t EDGE_U[] = { 0, 1, 2, 3 };
  static const int8_t EDGE_L[] = { 1, 4, 9, 5 };
  static const int8_t EDGE_F[] = { 0, 7, 8, 4 };
  static const int8_t EDGE_R[] = { 3, 6, 11, 7 };
  static const int8_t EDGE_B[] = { 2, 5, 10, 6 };
  static const int8_t EDGE_D[] = { 8, 11, 10, 9 };
  static const int8_t CORNER_U[] = { 0, 1, 2, 3 };
  static const int8_t CORNER_L[] = { 2, 1, 5, 6 };
  static const int8_t CORNER_F[] = { 1, 0, 4, 5 };
  static const int8_t CORNER_R[] = { 0, 3, 7, 4 };
  static const int8_t CORNER_B[] = { 3, 2, 6, 7 };
  static const int8_t CORNER_D[] = { 7, 6, 5, 4 };
  
  
  /* キューブ全体をU面中心-D面中心の軸で90 x (count) °回転 */
  void cubie_level_move_S_U4(cubie_cube_ptr cube, int32_t count){
    static const int8_t EDGE_UD_SLICE[] = { 4, 5, 6, 7 };
    switch (count){
      case 0:
      return;
      break;
      case 1:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_U, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_U, 1, 0);
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_D, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_D, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_UD_SLICE, 1, 0);
      break;
    }
  }
  
  /* キューブ全体をF面中心-B面中心の軸で180°回転 */
  void cubie_level_move_S_F2(cubie_cube_ptr cube){
    static const int8_t EDGE_FB_SLICE[] = { 3, 11, 9, 1 };
    corner_move(cube->corner_position, cube->corner_orientation, CORNER_F, 1, 0);
    edge_move(cube->edge_position, cube->edge_flip, EDGE_F, 1, 0);
    corner_move(cube->corner_position, cube->corner_orientation, CORNER_B, 1, 0);
    edge_move(cube->edge_position, cube->edge_flip, EDGE_B, 1, 0);
    edge_move(cube->edge_position, cube->edge_flip, EDGE_FB_SLICE, 1, 0);
  }
  
  
  __inline
  static corner_orientation_val_type S_LR2(corner_orientation_val_type i){
    switch (i){
      case 0:
      return 0;
      case 1:
      return 2;
      case 2:
      return 1;
    }
    return 0;
  }
  
  /* (0,1) →(1,0) */
  __inline
  static void cubie_level_move_S_LR2_corner_swap(corner_position_val_type corner_pos[8], corner_orientation_val_type corner_orientation[8], int8_t id1, int8_t id2){
    edge_position_val_type pos_tmp;
    edge_flip_val_type flip_tmp;
    pos_tmp = corner_pos[id1];
    corner_pos[id1] = corner_pos[id2];
    corner_pos[id2] = pos_tmp;
    flip_tmp = corner_orientation[id1];
    corner_orientation[id1] = S_LR2(corner_orientation[id2]);
    corner_orientation[id2] = S_LR2(flip_tmp);
  }
  /* (0,1) →(1,0) */
  __inline
  static void cubie_level_move_S_LR2_edge_swap(edge_position_val_type edge_pos[12], edge_flip_val_type edge_flip[12], int8_t id1, int8_t id2){
    edge_position_val_type pos_tmp;
    edge_flip_val_type flip_tmp;
    pos_tmp = edge_pos[id1];
    edge_pos[id1] = edge_pos[id2];
    edge_pos[id2] = pos_tmp;
    flip_tmp = edge_flip[id1];
    edge_flip[id1] = edge_flip[id2];
    edge_flip[id2] = flip_tmp;
  }
  
  /* キューブ全体をLRスライスを軸に反転，物理的にあり得ない配置になるので奇数回の適用は不可 */
  void cubie_level_move_S_LR2(cubie_cube_ptr cube){
    cubie_level_move_S_LR2_corner_swap(cube->corner_position, cube->corner_orientation, 0, 1);
    cubie_level_move_S_LR2_corner_swap(cube->corner_position, cube->corner_orientation, 2, 3);
    cubie_level_move_S_LR2_corner_swap(cube->corner_position, cube->corner_orientation, 4, 5);
    cubie_level_move_S_LR2_corner_swap(cube->corner_position, cube->corner_orientation, 6, 7);
    cubie_level_move_S_LR2_edge_swap(cube->edge_position, cube->edge_flip, 1, 3);
    cubie_level_move_S_LR2_edge_swap(cube->edge_position, cube->edge_flip, 5, 6);
    cubie_level_move_S_LR2_edge_swap(cube->edge_position, cube->edge_flip, 9, 11);
    cubie_level_move_S_LR2_edge_swap(cube->edge_position, cube->edge_flip, 4, 7);
  }
  
  
  void cubie_level_move_S(cubie_cube_ptr cube, int32_t sym_type){
    cubie_level_move_S_U4(cube, sym_type & 1);
    sym_type >>= 1;
    if (sym_type & 1){
      cubie_level_move_S_F2(cube);
    }
    sym_type >>= 1;
    if (sym_type & 1){
      cubie_level_move_S_LR2(cube);
    }
  }
  
  void cubie_level_move_S_inv(cubie_cube_ptr cube, int32_t sym_type){
    if ((sym_type >> 2) & 1){
      cubie_level_move_S_LR2(cube);
    }
    if ((sym_type >> 1) & 1){
      cubie_level_move_S_F2(cube);
    }
    switch (sym_type & 1){
      case 0:
      return;
      case 1:
      cubie_level_move_S_U4(cube, 1);
      break;
    }
  }
  void copy_cubie8(cubie_cube src[8], cubie_cube dst[8]){
    int32_t i;
    for (i = 0; i < 8; i++){
      dst[i] = src[i];
    }
  }
  
  
  void sym_cubie_move8(cubie_cube cubie[8], int32_t mov){
    int32_t i;
    for (i = 0; i < 8; i++){
      cubie_level_move_S(&cubie[i], i);
      cubie_level_move(&cubie[i], mov);
      cubie_level_move_S_inv(&cubie[i], i);
    }
  }
  
  
  void cubie_level_move(cubie_cube_ptr cube, int32_t mov_id){
    switch (mov_id){
      #if defined(MOVE_U1)
      case __MOVE_U1__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_U, 0, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_U, 0, 0);
      break;
      #endif
      #if defined(MOVE_U2)
      case __MOVE_U2__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_U, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_U, 1, 0);
      break;
      #endif
      #if defined(MOVE_U3)
      case __MOVE_U3__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_U, 2, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_U, 2, 0);
      break;
      #endif
      #if defined(MOVE_L1)
      case __MOVE_L1__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_L, 0, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_L, 0, 0);
      break;
      #endif
      #if defined(MOVE_L2)
      case __MOVE_L2__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_L, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_L, 1, 0);
      break;
      #endif
      #if defined(MOVE_L3)
      case __MOVE_L3__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_L, 2, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_L, 2, 0);
      break;
      #endif
      #if defined(MOVE_F1)
      case __MOVE_F1__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_F, 0, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_F, 0, 1);
      break;
      #endif
      #if defined(MOVE_F2)
      case __MOVE_F2__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_F, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_F, 1, 0);
      break;
      #endif
      #if defined(MOVE_F3)
      case __MOVE_F3__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_F, 2, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_F, 2, 1);
      break;
      #endif
      #if defined(MOVE_R1)
      case __MOVE_R1__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_R, 0, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_R, 0, 0);
      break;
      #endif
      #if defined(MOVE_R2)
      case __MOVE_R2__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_R, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_R, 1, 0);
      break;
      #endif
      #if defined(MOVE_R3)
      case __MOVE_R3__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_R, 2, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_R, 2, 0);
      break;
      #endif
      #if defined(MOVE_B1)
      case __MOVE_B1__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_B, 0, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_B, 0, 1);
      break;
      #endif
      #if defined(MOVE_B2)
      case __MOVE_B2__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_B, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_B, 1, 0);
      break;
      #endif
      #if defined(MOVE_B3)
      case __MOVE_B3__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_B, 2, 1);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_B, 2, 1);
      break;
      #endif
      #if defined(MOVE_D1)
      case __MOVE_D1__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_D, 0, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_D, 0, 0);
      break;
      #endif
      #if defined(MOVE_D2)
      case __MOVE_D2__: 
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_D, 1, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_D, 1, 0);
      break;
      #endif
      #if defined(MOVE_D3)
      case __MOVE_D3__:
      corner_move(cube->corner_position, cube->corner_orientation, CORNER_D, 2, 0);
      edge_move(cube->edge_position, cube->edge_flip, EDGE_D, 2, 0);
      break;
      #endif
      #if defined(DEBUG_PRINT)
      default:printf("error in function cubie_level_move\n");
      #endif
    }
  }
  
  void init_cubie_level_cube(cubie_cube_ptr cube){
    int32_t i;
    for (i = 0; i < 8; i++){
      cube->corner_orientation[i] = 0;
      cube->corner_position[i] = i;
    }
    for (i = 0; i < 12; i++){
      cube->edge_flip[i] = 0;
      cube->edge_position[i] = i;
    }
  }
  
  /* 初期キューブをFB軸で90°左回転させたものを初期キューブとした場合の配置を得る（色違いだが全く同じ状況を作る） */
  /* 元キューブと解手数が全く同じになるが，距離関数の戻り値は異なる場合が多い */
  /* 最も距離関数の値が大きいものを距離関数の結果として扱う */
  void equivalent_cubie_lr(cubie_cube_ptr src, cubie_cube_ptr dst){ 
    int32_t i, j, k;
    static const uint8_t C_POS_UD_to_LR[] = { 4, 0, 3, 7, 5, 1, 2, 6 };
    static const uint8_t E_POS_UD_to_LR[] = { 7, 3, 6, 11, 0, 2, 10, 8, 4, 1, 5, 9 };
    static uint8_t C_ORI_UD_to_LR[8][8][3];
    static uint8_t E_ORI_UD_to_LR[12][12][2];
    static int32_t init_flag = 0;
    if (init_flag == 0){
      #define UNDEFINED 100
      int32_t ct = 0;
      cubie_cube ud, lr;
      for (i = 0; i < 8; i++){
        for (j = 0; j < 8; j++){
          for (k = 0; k < 3; k++){
            C_ORI_UD_to_LR[i][j][k] = UNDEFINED;
          }
        }
      }
      for (i = 0; i < 12; i++){
        for (j = 0; j < 12; j++){
          for (k = 0; k < 2; k++){
            E_ORI_UD_to_LR[i][j][k] = UNDEFINED;
          }
        }
      }
      init_cubie_level_cube(&ud);
      init_cubie_level_cube(&lr);
      while (ct < 192 + 288){
        int32_t mv;
        mv = rand()%N_MOVES;
        cubie_level_move(&ud,mv);
        cubie_level_move_lr(&lr,mv);
        for (i = 0; i < 8; i++){
          if (C_ORI_UD_to_LR[i][ud.corner_position[i]][ud.corner_orientation[i]] == UNDEFINED){
            C_ORI_UD_to_LR[i][ud.corner_position[i]][ud.corner_orientation[i]] = lr.corner_orientation[C_POS_UD_to_LR[i]];
            ct++;
          }
        }
        for (i = 0; i < 12; i++){
          if (E_ORI_UD_to_LR[i][ud.edge_position[i]][ud.edge_flip[i]] == UNDEFINED){
            E_ORI_UD_to_LR[i][ud.edge_position[i]][ud.edge_flip[i]] = lr.edge_flip[E_POS_UD_to_LR[i]];
            ct++;
          }
        }
      }
      init_flag = 1;
      #undef UNDEFINED
    }
    for (i = 0; i < 8; i++){
      dst->corner_position[C_POS_UD_to_LR[i]] = C_POS_UD_to_LR[src->corner_position[i]];
      dst->corner_orientation[C_POS_UD_to_LR[i]] = C_ORI_UD_to_LR[i][src->corner_position[i]][src->corner_orientation[i]];
    }
    for (i = 0; i < 12; i++){
      dst->edge_position[E_POS_UD_to_LR[i]] = E_POS_UD_to_LR[src->edge_position[i]];
      dst->edge_flip[E_POS_UD_to_LR[i]] = E_ORI_UD_to_LR[i][src->edge_position[i]][src->edge_flip[i]];
    }
  }
  
  
  /* 初期キューブをLR軸で90°左回転させたものを初期キューブとした場合の配置を得る（色違いだが全く同じ状況を作る） */
  /* 元キューブと解手数が全く同じになるが，距離関数の戻り値は異なる場合が多い */
  /* 最も距離関数の値が大きいものを距離関数の結果として扱う */
  void equivalent_cubie_fb(cubie_cube_ptr src, cubie_cube_ptr dst){
    int32_t i, j, k;
    static const uint8_t C_POS_UD_to_FB[] = { 4, 5, 1, 0, 7, 6, 2, 3 };
    static const uint8_t E_POS_UD_to_FB[] = { 8, 4, 0, 7, 9, 1, 3, 11, 10, 5, 2, 6 };
    static uint8_t C_ORI_UD_to_FB[8][8][3];
    static uint8_t E_ORI_UD_to_FB[12][12][2];
    static int32_t init_flag = 0;
    
    if (init_flag == 0){
      #define UNDEFINED 100
      int32_t ct = 0;
      cubie_cube ud, fb;
      for (i = 0; i < 8; i++){
        for (j = 0; j < 8; j++){
          for (k = 0; k < 3; k++){
            C_ORI_UD_to_FB[i][j][k] = UNDEFINED;
          }
        }
      }
      for (i = 0; i < 12; i++){
        for (j = 0; j < 12; j++){
          for (k = 0; k < 2; k++){
            E_ORI_UD_to_FB[i][j][k] = UNDEFINED;
          }
        }
      }
      init_cubie_level_cube(&ud);
      init_cubie_level_cube(&fb);
      while (ct < 192 + 288){
        int32_t mv;
        mv = rand() % N_MOVES;
        cubie_level_move(&ud, mv);
        cubie_level_move_fb(&fb, mv);
        for (i = 0; i < 8; i++){
          if (C_ORI_UD_to_FB[i][ud.corner_position[i]][ud.corner_orientation[i]] == UNDEFINED){
            C_ORI_UD_to_FB[i][ud.corner_position[i]][ud.corner_orientation[i]] = fb.corner_orientation[C_POS_UD_to_FB[i]];
            ct++;
          }
        }
        for (i = 0; i < 12; i++){
          if (E_ORI_UD_to_FB[i][ud.edge_position[i]][ud.edge_flip[i]] == UNDEFINED){
            E_ORI_UD_to_FB[i][ud.edge_position[i]][ud.edge_flip[i]] = fb.edge_flip[E_POS_UD_to_FB[i]];
            ct++;
          }
        }
      }
      init_flag = 1;
      #undef UNDEFINED
    }
    
    for (i = 0; i < 8; i++){
      dst->corner_position[C_POS_UD_to_FB[i]] = C_POS_UD_to_FB[src->corner_position[i]];
      dst->corner_orientation[C_POS_UD_to_FB[i]] = C_ORI_UD_to_FB[i][src->corner_position[i]][src->corner_orientation[i]];
    }
    for (i = 0; i < 12; i++){
      dst->edge_position[E_POS_UD_to_FB[i]] = E_POS_UD_to_FB[src->edge_position[i]];
      dst->edge_flip[E_POS_UD_to_FB[i]] = E_ORI_UD_to_FB[i][src->edge_position[i]][src->edge_flip[i]];
    }
  }
  
  /* 全く同じものがsearch_node_levelに存在 */
  uint8_t MOV_TRS_TAB_LR[N_MOVES];
  uint8_t MOV_TRS_TAB_FB[N_MOVES];
  
  __inline
  static
  void init_twist_trs_tab(){
    MOV_TRS_TAB_LR[MOVE_U1] = MOVE_R1;
    MOV_TRS_TAB_LR[MOVE_U2] = MOVE_R2;
    MOV_TRS_TAB_LR[MOVE_U3] = MOVE_R3;
    MOV_TRS_TAB_LR[MOVE_R1] = MOVE_D1;
    MOV_TRS_TAB_LR[MOVE_R2] = MOVE_D2;
    MOV_TRS_TAB_LR[MOVE_R3] = MOVE_D3;
    MOV_TRS_TAB_LR[MOVE_B1] = MOVE_B1;
    MOV_TRS_TAB_LR[MOVE_B2] = MOVE_B2;
    MOV_TRS_TAB_LR[MOVE_B3] = MOVE_B3;
    MOV_TRS_TAB_LR[MOVE_L1] = MOVE_U1;
    MOV_TRS_TAB_LR[MOVE_L2] = MOVE_U2;
    MOV_TRS_TAB_LR[MOVE_L3] = MOVE_U3;
    MOV_TRS_TAB_LR[MOVE_F1] = MOVE_F1;
    MOV_TRS_TAB_LR[MOVE_F2] = MOVE_F2;
    MOV_TRS_TAB_LR[MOVE_F3] = MOVE_F3;
    MOV_TRS_TAB_LR[MOVE_D1] = MOVE_L1;
    MOV_TRS_TAB_LR[MOVE_D2] = MOVE_L2;
    MOV_TRS_TAB_LR[MOVE_D3] = MOVE_L3;
    
    MOV_TRS_TAB_FB[MOVE_U1] = MOVE_F1;
    MOV_TRS_TAB_FB[MOVE_U2] = MOVE_F2;
    MOV_TRS_TAB_FB[MOVE_U3] = MOVE_F3;
    MOV_TRS_TAB_FB[MOVE_R1] = MOVE_R1;
    MOV_TRS_TAB_FB[MOVE_R2] = MOVE_R2;
    MOV_TRS_TAB_FB[MOVE_R3] = MOVE_R3;
    MOV_TRS_TAB_FB[MOVE_B1] = MOVE_U1;
    MOV_TRS_TAB_FB[MOVE_B2] = MOVE_U2;
    MOV_TRS_TAB_FB[MOVE_B3] = MOVE_U3;
    MOV_TRS_TAB_FB[MOVE_L1] = MOVE_L1;
    MOV_TRS_TAB_FB[MOVE_L2] = MOVE_L2;
    MOV_TRS_TAB_FB[MOVE_L3] = MOVE_L3;
    MOV_TRS_TAB_FB[MOVE_F1] = MOVE_D1;
    MOV_TRS_TAB_FB[MOVE_F2] = MOVE_D2;
    MOV_TRS_TAB_FB[MOVE_F3] = MOVE_D3;
    MOV_TRS_TAB_FB[MOVE_D1] = MOVE_B1;
    MOV_TRS_TAB_FB[MOVE_D2] = MOVE_B2;
    MOV_TRS_TAB_FB[MOVE_D3] = MOVE_B3;
  }
  
  
  void cubie_level_move_lr(cubie_cube_ptr cube, int32_t mov_id){
    static int init_flag = 0;
    if (init_flag == 0){
      init_twist_trs_tab();
      init_flag = 1;
    }
    cubie_level_move(cube, MOV_TRS_TAB_LR[mov_id]);
  }
  void cubie_level_move_fb(cubie_cube_ptr cube, int32_t mov_id){
    static int init_flag = 0;
    if (init_flag == 0){
      init_twist_trs_tab();
      init_flag = 1;
    }
    cubie_level_move(cube, MOV_TRS_TAB_FB[mov_id]);
  }
