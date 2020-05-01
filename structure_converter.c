#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include"structure_converter.h"
#include"cube_structures.h"
#include"common_defines.h"
#include"cubie_level_move.h"

#if defined(SILENT_MODE)
#define printf(a,...)
#endif


__inline
static void corner_pos_unpack(uint16_t index, corner_position_val_type corner_position[8]){
  int16_t i, j;
  for (i = 8 - 1; i >= 0; i--)
  {
    corner_position[i] = index % (uint16_t)(8 - i);
    index /= (uint16_t)(8 - i);
    for (j = i + 1; j < 8; j++)
    if (corner_position[j] >= corner_position[i])
    corner_position[j]++;
  }
}
__inline
static uint16_t corner_pos_pack(corner_position_val_type corner_position[8]){
  uint16_t ret, i, j;
  ret = 0;
  for (i = 0; i < 8; i++){
    ret *= (8 - i);
    for (j = i + 1; j < 8; j++)
    if (corner_position[j] < corner_position[i])
    ret++;
  }
  return ret;
}
__inline
static void corner_ori_unpack(uint16_t index, corner_orientation_val_type corner_orientation[8]){
  int16_t i;
  
  for (i = 0; i < 7; i++)
  {
    corner_orientation[i] = index % 3u;
    index = index / 3u;
  }
  corner_orientation[7] = (2 * (corner_orientation[0] + corner_orientation[1] + corner_orientation[2] + corner_orientation[3]
    + corner_orientation[4] + corner_orientation[5] + corner_orientation[6])) % 3u;
}
__inline
static uint16_t corner_ori_pack(corner_orientation_val_type corner_orientation[8]){
  uint16_t ret;
  int16_t i;
  ret = 0;
  for (i = 6; i >= 0; i--)
  ret = 3 * ret + corner_orientation[i];
  return ret;
}
__inline
static void edge_4pos_unpack(uint16_t index, edge_position_val_type edge_position[12], const edge_position_val_type edge_list[4]){
  int16_t i, k;
  uint16_t tmp[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  uint16_t div = 11 * 10 * 9;
  for (i = 0; i < 4; i++){
    k = (index / div) % (12 - i);
    div /= (11 - i);
    edge_position[tmp[k]] = edge_list[i];
    for (; k < 11; k++)
    tmp[k] = tmp[k + 1];
  }
}
__inline
static uint16_t edge_4pos_pack(edge_position_val_type edge_position[12], const edge_position_val_type edge_list[4]){
  uint16_t ret, i, k;
  uint16_t tmp[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  ret = 0;
  for (i = 0; i < 4; i++){
    for (k = 0; k < 12; k++){
      if (edge_position[k] == edge_list[i])
      break;
    }
    #if defined(DEBUG_PRINT)
    if(k >= 12){
      printf("error in edge_4pos_pack\n");
    }
    #endif
    ret *= (12 - i);
    ret += tmp[k];
    for (; k < 12; k++)
    tmp[k]--;
  }
  return ret;
}
__inline
static void edge_flip_unpack(uint16_t index, edge_flip_val_type edge_flip[12]){
  int16_t i;
  for (i = 0; i < 11; i++){
    edge_flip[i] = index & 1;
    index = index >> 1;
  }
  edge_flip[11] = (edge_flip[0] + edge_flip[1] + edge_flip[2] + edge_flip[3] +
    edge_flip[4] + edge_flip[5] + edge_flip[6] + edge_flip[7] +
    edge_flip[8] + edge_flip[9] + edge_flip[10]) & 1;
}
__inline
static uint16_t edge_flip_pack(edge_flip_val_type edge_flip[12]){
  uint16_t ret;
  int16_t i;
  ret = 0;
  for (i = 10; i >= 0; i--)
  ret = (ret << 1) + edge_flip[i];
  return ret;
}
static const edge_position_val_type UD_EDGE[4] = { 4, 7, 6, 5 };
static const edge_position_val_type LR_EDGE[4] = { 0, 8, 10, 2 };
static const edge_position_val_type FB_EDGE[4] = { 1, 3, 11, 9 };
void convert_cubie_to_coordinate(cubie_cube_ptr src, coord_cube_ptr dst){
  
  dst->corner_orientation = corner_ori_pack(src->corner_orientation);
  dst->corner_position = corner_pos_pack(src->corner_position);
  dst->edge_flip = edge_flip_pack(src->edge_flip);
  dst->edge_position_ud = edge_4pos_pack(src->edge_position, UD_EDGE);
  dst->edge_position_lr = edge_4pos_pack(src->edge_position, LR_EDGE);
  dst->edge_position_fb = edge_4pos_pack(src->edge_position, FB_EDGE);
}
void convert_coordinate_to_cubie(coord_cube_ptr src, cubie_cube_ptr dst){
  int32_t i;
  edge_position_val_type edge_pos_ud[12] = { 0 };
  edge_position_val_type edge_pos_lr[12] = { 0 };
  edge_position_val_type edge_pos_fb[12] = { 0 };
  corner_pos_unpack(src->corner_position, dst->corner_position);
  corner_ori_unpack(src->corner_orientation, dst->corner_orientation);
  edge_flip_unpack(src->edge_flip, dst->edge_flip);
  edge_4pos_unpack(src->edge_position_ud, edge_pos_ud, UD_EDGE);
  edge_4pos_unpack(src->edge_position_lr, edge_pos_lr, LR_EDGE);
  edge_4pos_unpack(src->edge_position_fb, edge_pos_fb, FB_EDGE);
  for (i = 0; i < 12; i++){
    dst->edge_position[i] = (edge_pos_ud[i] + edge_pos_lr[i] + edge_pos_fb[i]);
  }
}

/*====================================================================================================*/

//1 <= n <= 12 , r <= n
__inline
static uint32_t combination(uint32_t n, uint32_t r){
  uint32_t tmp1 = 1, tmp2 = 1;
  if (r > n || n > 12){
    return 0;
  }
  while (r > 0){
    tmp1 *= n--;
    tmp2 *= r--;
  }
  return (tmp1 / tmp2);
}
__inline
static uint16_t edge_pos_comb_pack4(edge_position_val_type edge[12], const edge_position_val_type edge_list[4]){
  int16_t i, k;
  uint16_t ret;
  ret = 0;
  k = 3;
  for (i = 11; i >= 0 && k >= 0; i--){
    if (edge[i] == edge_list[0] || edge[i] == edge_list[1] || edge[i] == edge_list[2] || edge[i] == edge_list[3]){
      k--;
    }
    else{
      ret += combination(i, k);
    }
  }
  #if defined(DEBUG_PRINT)
  if (k != -1)
  printf("error in func edge_pos_comb_pack4\n");
  #endif
  return ret;
}
    
__inline
static int32_t cubie_to_ud(cubie_cube_ptr cube){
  return edge_4pos_pack(cube->edge_position, UD_EDGE);
}
    
uint16_t *NODE_UD_POS_TAB;
uint16_t *VALID_TAB;
#define UNDEFINED 0x7FFF
#define N_UD_EDGE 11880
#define N_EQ_UD_EDGE 1560
#define N_SYM 8

void init_node_flip_ud_tab(){
  cubie_cube cubie[N_SYM];
  int32_t i;
  int32_t eq_class = 0;
  int32_t add_flag = 0;
  int32_t ct = 0;
  uint32_t rd = 0;
  
  NODE_UD_POS_TAB = (uint16_t*)malloc(sizeof(uint16_t)* N_UD_EDGE);
  VALID_TAB = (uint16_t*)calloc(N_EQ_UD_EDGE * N_SYM, sizeof(uint16_t));
  for (i = 0; i < N_UD_EDGE; i++){
    NODE_UD_POS_TAB[i] = UNDEFINED;
  }
  for (i = 0; i < N_SYM; i++){
    init_cubie_level_cube(&cubie[i]);
  }
  while (ct < N_UD_EDGE){
    int32_t mv;
    add_flag = 0;
    for (i = 0; i < N_SYM; i++){
      if (NODE_UD_POS_TAB[cubie_to_ud(&cubie[i])] == UNDEFINED){
        NODE_UD_POS_TAB[cubie_to_ud(&cubie[i])] = eq_class * N_SYM + i;
        VALID_TAB[eq_class * N_SYM + i] = 1;
        add_flag = 1;
        ct++;
      }
    }
    eq_class += add_flag;
    rd = rd * 7 + 7;/* 粗末な乱数生成，rand()使うと結果が環境依存．そもそも乱数使う時点でNGだが他の方法がめんどくさい */
    mv = rd % N_MOVES;
    sym_cubie_move8(cubie, mv);
  }
}

int32_t valid_idx(int32_t i){
  if (i < N_EQ_UD_EDGE * N_SYM)
  return VALID_TAB[i];
  return 0;
}
    
    
__inline
static 
void convert_cubie_to_search_node_ud(cubie_cube_ptr src, search_node_cube_ptr dst){
  dst->corner_position = corner_pos_pack(src->corner_position);
  dst->ud_corner_orientation = corner_ori_pack(src->corner_orientation);
  /* 対称関係を盛り込んだテーブル値を返す(equivalence_class<<3)+sym_idがud_edgeの値として戻る */
  dst->ud_edge_ud = NODE_UD_POS_TAB[edge_4pos_pack(src->edge_position, UD_EDGE)];
  dst->ud_edge_flip = edge_flip_pack(src->edge_flip);
}

    
void convert_cubie_to_search_node(cubie_cube_ptr src, search_node_cube_ptr dst){
  search_node_cube lr, fb;
  cubie_cube cubie_lr, cubie_fb;
  equivalent_cubie_lr(src,&cubie_lr);
  equivalent_cubie_fb(src, &cubie_fb);
  convert_cubie_to_search_node_ud(src, dst);
  convert_cubie_to_search_node_ud(&cubie_lr, &lr);
  convert_cubie_to_search_node_ud(&cubie_fb, &fb);
  
  dst->lr_corner_orientation = lr.ud_corner_orientation;
  dst->lr_edge_flip = lr.ud_edge_flip;
  dst->lr_edge_ud = lr.ud_edge_ud;
  dst->fb_corner_orientation = fb.ud_corner_orientation;
  dst->fb_edge_flip = fb.ud_edge_flip;
  dst->fb_edge_ud = fb.ud_edge_ud;
}


void convert_coord_to_search_node(coord_cube_ptr src, search_node_cube_ptr dst){
  cubie_cube cubie;
  convert_coordinate_to_cubie(src, &cubie);
  convert_cubie_to_search_node(&cubie, dst);
}
