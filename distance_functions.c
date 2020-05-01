#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include"distance_functions.h"
#include"structure_converter.h"
#include"cube_structures.h"
#include"move_defines.h"
#include"search_node_level_move.h"
#include"common_defines.h"
#include"cubie_level_move.h"

#if defined(SILENT_MODE)
#define printf(a,...)
#endif


uint8_t *DISTANCE_TABLE = NULL;
uint8_t *CORNER_DISTANCE_TABLE = NULL;
uint16_t *CORNER_ORIENTATION_SYM_to_SYM0 = NULL;
uint16_t *CORNER_ORIENTATION_SYM0_to_SYM = NULL;
uint16_t *EDGE_FLIP_SYM_to_SYM0 = NULL;
uint16_t *EDGE_FLIP_SYM0_to_SYM = NULL;

__inline
static int32_t cubie_to_corner_ori(cubie_cube_ptr cube){
  search_node_cube node;
  convert_cubie_to_search_node(cube, &node);
  return node.ud_corner_orientation;
}
__inline
static int32_t cubie_to_edge_flip(cubie_cube_ptr cube){
  search_node_cube node;
  convert_cubie_to_search_node(cube, &node);
  return node.ud_edge_flip;
}
#define N_CORNER_ORI 2187
#define N_CORNER_POS 40320
#define N_EDGE_FLIP 2048
#define N_UD_EDGE 11880
#define N_EQ_UD_EDGE 1560
#define N_SYM 8
#define UNDEFINED 0x7FFF

#define ELM_TYPE_EDGE_FLIP 0
#define ELM_TYPE_CORNER_ORIENTATION 1
__inline
static int32_t ori_elm(cubie_cube_ptr cube, int32_t elm_type){
  switch (elm_type){
    case ELM_TYPE_EDGE_FLIP:
    return cubie_to_edge_flip(cube);
    break;
    case ELM_TYPE_CORNER_ORIENTATION:
    return cubie_to_corner_ori(cube);
    break;
  }
  return 0;
}
__inline
static void init_ori_tab(uint16_t** dst_sym_to_sym0_tab, uint16_t** dst_sym0_to_sym_tab, int32_t entry, int32_t elm_type){
  cubie_cube cubie[N_SYM], cubie2[N_SYM];
  int32_t i;
  int32_t ct = 0;
  uint16_t *sym_to_sym0_tab;
  uint16_t *sym0_to_sym_tab;
  sym_to_sym0_tab = (uint16_t*)malloc(sizeof(uint16_t)* entry * N_SYM);
  sym0_to_sym_tab = (uint16_t*)malloc(sizeof(uint16_t)* entry * N_SYM);
  for (i = 0; i < entry * N_SYM; i++){
    sym_to_sym0_tab[i] = UNDEFINED;
    sym0_to_sym_tab[i] = UNDEFINED;
  }
  for (i = 0; i < N_SYM; i++){
    init_cubie_level_cube(&cubie[i]);
  }
  while (ct < entry * N_SYM){
    int32_t mv;
    int32_t sym0_elm;
    copy_cubie8(cubie, cubie2);
    sym0_elm = ori_elm(&cubie2[0], elm_type);
    for (i = 0; i < N_SYM; i++){
      int32_t sym_n_elm = ori_elm(&cubie2[i], elm_type);
      if (sym_to_sym0_tab[sym_n_elm * N_SYM + i] == UNDEFINED){
        sym_to_sym0_tab[sym_n_elm * N_SYM + i] = sym0_elm;
        ct++;
      }
      sym0_to_sym_tab[sym0_elm * N_SYM + i] = sym_n_elm;
    }
    mv = rand() % N_MOVES;
    sym_cubie_move8(cubie, mv);
  }
  *dst_sym_to_sym0_tab = sym_to_sym0_tab;
  *dst_sym0_to_sym_tab = sym0_to_sym_tab;
}
__inline
static void init_sym_edge_flip_table(){
  init_ori_tab(
    &EDGE_FLIP_SYM_to_SYM0,
    &EDGE_FLIP_SYM0_to_SYM,
    N_EDGE_FLIP,
    ELM_TYPE_EDGE_FLIP
  );
}
__inline
static void init_sym_corner_orientation_table(){
  init_ori_tab(
    &CORNER_ORIENTATION_SYM_to_SYM0,
    &CORNER_ORIENTATION_SYM0_to_SYM,
    N_CORNER_ORI,
    ELM_TYPE_CORNER_ORIENTATION
  );
}

__inline
static int32_t index_to_node(int64_t index, search_node_cube_ptr node, int32_t sym){
  int32_t eq_class;
  int32_t flip;
  eq_class = (int32_t)(index / (N_CORNER_ORI * N_EDGE_FLIP));
  if (!valid_idx((eq_class << 3) + sym)){
    return 0;
  }
  node->ud_corner_orientation = CORNER_ORIENTATION_SYM0_to_SYM[(index % N_CORNER_ORI) * N_SYM + sym];
  index /= N_CORNER_ORI;
  flip = index % N_EDGE_FLIP;
  node->ud_edge_flip = EDGE_FLIP_SYM0_to_SYM[flip * N_SYM + sym];
  node->ud_edge_ud = (uint32_t)(eq_class << 3) + sym;
  return 1;
}
__inline
static int32_t index_to_node_corner(int64_t index, search_node_cube_ptr node){
  node->ud_corner_orientation = index % N_CORNER_ORI;
  node->corner_position = index / N_CORNER_ORI;
  return 1;
}
__inline
static int64_t node_to_index_corner(search_node_cube_ptr node){
  return node->corner_position * N_CORNER_ORI + node->ud_corner_orientation;
}


__inline
static int64_t node_to_index_ud(search_node_cube_ptr node){
  int32_t sym = node->ud_edge_ud & 0x07;
  int32_t c_ori;
  uint16_t flip0, class_;
  c_ori = CORNER_ORIENTATION_SYM_to_SYM0[(int32_t)node->ud_corner_orientation * N_SYM + sym];
  class_ = node->ud_edge_ud >> 3;
  flip0 = EDGE_FLIP_SYM_to_SYM0[(int32_t)node->ud_edge_flip * N_SYM + sym];
  return (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip0 * N_CORNER_ORI + c_ori;
}
__inline
static int64_t node_to_index_lr(search_node_cube_ptr node){
  int32_t sym = node->lr_edge_ud & 0x07;
  int32_t c_ori;
  uint16_t flip0, class_;
  c_ori = CORNER_ORIENTATION_SYM_to_SYM0[(int32_t)node->lr_corner_orientation * N_SYM + sym];
  class_ = node->lr_edge_ud >> 3;
  flip0 = EDGE_FLIP_SYM_to_SYM0[(int32_t)node->lr_edge_flip * N_SYM + sym];
  return (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip0 * N_CORNER_ORI + c_ori;
}
__inline
static int64_t node_to_index_fb(search_node_cube_ptr node){
  int32_t sym = node->fb_edge_ud & 0x07;
  int32_t c_ori;
  uint16_t flip0, class_;
  c_ori = CORNER_ORIENTATION_SYM_to_SYM0[(int32_t)node->fb_corner_orientation * N_SYM + sym];
  class_ = node->fb_edge_ud >> 3;
  flip0 = EDGE_FLIP_SYM_to_SYM0[(int32_t)node->fb_edge_flip * N_SYM + sym];
  return (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip0 * N_CORNER_ORI + c_ori;
}

static void create_corner_distance_table(uint8_t **dst, int64_t entry){
  uint8_t *tab;
  int64_t i;
  int32_t depth;
  int64_t index;
  int64_t count;
  int64_t prev_ct = 0;
  search_node_cube node;
  int32_t loop_flag;
  #define tab(i) ((tab[(i)>>1] >> (((i)&1)<<2))&0x0F)
  #define set_tab(i,j) (tab[(i)>>1] = ((j) << ( ((i)&1)<<2 )) | tab((i)^1) << ( (~(i)&1)<<2 )  )
  tab = (uint8_t *)malloc(sizeof(uint8_t)*(entry>>1));
  if (tab == NULL){
    printf("failed to allocate memory\n");
    return;
  }
  /* 全ての値を15で初期化，1byteに2つずつ値を格納(16以上の距離は格納できない) */
  for (i = 0; i< (entry >> 1); i++){
    tab[i] = 0xFF;
  }
  init_search_node_level_cube(&node);
  index = node_to_index_corner(&node);
  set_tab(index, 0);
  count = 1;
  loop_flag = 1;
  depth = 0;
  while (loop_flag){
    printf("depth = %2d , count = %10lld   %10lld\n", depth, count, count - prev_ct);
    prev_ct = count;
    loop_flag = 0;
    for (i = 0; i < entry; i++){
      if (tab(i) == depth){
        int32_t k;
        int32_t mov;
        int64_t src_index;
        src_index = i;
        
        if (!index_to_node_corner(src_index, &node)){
          continue;
        }
        for (mov = 0; mov < N_MOVES; mov++){
          search_node_cube node_tmp = node;
          search_node_level_move(&node_tmp, mov);
          index = node_to_index_corner(&node_tmp);
          if (tab(index) > depth + 1){
            loop_flag = 1;
            set_tab(index, depth + 1);
            count++;
          }
        }
        
      }
    }
    depth++;
  }
  *dst = tab;
}

static void create_distance_table(uint8_t **dst, int64_t entry){
  uint8_t *tab;
  int64_t i;
  int32_t depth;
  int64_t index;
  int64_t count;
  int64_t prev_ct = 0;
  search_node_cube node;
  int32_t loop_flag;
  #define tab(i) ((tab[(i)>>1] >> (((i)&1)<<2))&0x0F)
  #define set_tab(i,j) (tab[(i)>>1] = ((j) << ( ((i)&1)<<2 )) | tab((i)^1) << ( (~(i)&1)<<2 )  )
  tab = (uint8_t *)malloc(sizeof(uint8_t)*(entry>>1));
  if (tab == NULL){
    printf("failed to allocate memory\n");
    return;
  }
  /* 全ての値を15で初期化，1byteに2つずつ値を格納(16以上の距離は格納できない) */
  for (i = 0; i< (entry >> 1); i++){
    tab[i] = 0xFF;
  }
  init_search_node_level_cube(&node);
  index = node_to_index_ud(&node);
  set_tab(index, 0);
  count = 1;
  loop_flag = 1;
  depth = 0;
  while (loop_flag){
    printf("depth = %2d , count = %10lld   %10lld\n", depth, count, count - prev_ct);
    prev_ct = count;
    loop_flag = 0;
    for (i = 0; i < entry; i++){
      if (tab(i) == depth){
        int32_t k;
        int32_t mov;
        int64_t src_index;
        src_index = i;
        for (k = 0; k < N_SYM; k++){
          if (!index_to_node(src_index, &node, k)){
            continue;
          }
          for (mov = 0; mov < N_MOVES; mov++){
            search_node_cube node_tmp = node;
            search_node_level_move(&node_tmp, mov);
            index = node_to_index_ud(&node_tmp);
            if (tab(index) > depth + 1){
              loop_flag = 1;
              set_tab(index, depth + 1);
              count++;
            }
          }
        }
      }
    }
    depth++;
  }
  *dst = tab;
}

static void write_tab(FILE *fp, uint8_t *tab, int64_t entry){
  fwrite(tab, entry >> 1, 1 , fp);
}

static void load_tab(FILE *fp, uint8_t **tab, int64_t entry){
  #define BUFFER_SIZE 65536
  uint8_t buffer[BUFFER_SIZE];
  uint8_t *dst;
  int64_t i;
  size_t read_count;
  dst = (uint8_t *)malloc(sizeof(uint8_t)*(entry >> 1));
  if (dst == NULL){
    printf("failed to allocate memory\n");
    return;
  }
  i = 0;
  #if !defined(__GCC__)
  while (read_count = fread_s(buffer, BUFFER_SIZE, 1, BUFFER_SIZE, fp)){
    #else
    while (read_count = fread(buffer, 1, BUFFER_SIZE, fp)){
      #endif
      int32_t j;
      for (j = 0; j < read_count; i++, j++){
        dst[i] = buffer[j];
      }
    }    
    *tab = dst;
  }
  

void init_distance_table(){
  static const char *filename = DISTANCE_TABLE_FILE_PATH;
  static const char *corner_filename = CORNER_DISTANCE_TABLE_FILE_PATH;
  FILE *fp;
  init_sym_corner_orientation_table();
  init_sym_edge_flip_table();
  #define DISTANCE_TABLE_ENTRY ((int64_t)N_EQ_UD_EDGE*N_EDGE_FLIP*N_CORNER_ORI)
  #if !defined(__GCC__)
  fopen_s(&fp, filename, "rb");
  #else
  fp = fopen(filename,"rb");
  #endif
  if (fp == NULL){
    printf("\ndistance table file \"%s\" is not found\n",filename);
    printf("criating distance table...\n");
    create_distance_table(&DISTANCE_TABLE, DISTANCE_TABLE_ENTRY);
    #if !defined(__GCC__)
    fopen_s(&fp, filename, "wb");
    #else
    fp = fopen(filename, "wb");
    #endif
    if (fp == NULL){
      printf("failed to write table\n");
      return;
    }
    printf("writing distance table file \"%s\"...",filename);
    write_tab(fp, DISTANCE_TABLE, DISTANCE_TABLE_ENTRY);
    fclose(fp);
  }else{
    printf("\nloading distance table file \"%s\" ...",filename);
    load_tab(fp, &DISTANCE_TABLE, DISTANCE_TABLE_ENTRY);
    fclose(fp);
  }
  ////
  #define CORNER_DISTANCE_TABLE_ENTRY (N_CORNER_ORI * N_CORNER_POS)
  #if !defined(__GCC__)
  fopen_s(&fp, corner_filename, "rb");
  #else
  fp = fopen(corner_filename,"rb");
  #endif
  if (fp == NULL){
    printf("\ndistance table file \"%s\" is not found\n",corner_filename);
    printf("criating distance table...\n");
    create_corner_distance_table(&CORNER_DISTANCE_TABLE, CORNER_DISTANCE_TABLE_ENTRY);
    #if !defined(__GCC__)
    fopen_s(&fp, corner_filename, "wb");
    #else
    fp = fopen(corner_filename, "wb");
    #endif
    if (fp == NULL){
      printf("failed to write table\n");
      return;
    }
    printf("writing distance table file \"%s\"...",corner_filename);
    write_tab(fp, CORNER_DISTANCE_TABLE, CORNER_DISTANCE_TABLE_ENTRY);
    fclose(fp);
  }else{
    printf("\nloading distance table file \"%s\" ...",corner_filename);
    load_tab(fp, &CORNER_DISTANCE_TABLE, CORNER_DISTANCE_TABLE_ENTRY);
    fclose(fp);
  }
}




#define dist(i) ((DISTANCE_TABLE[(i)>>1] >> (((i)&1)<<2))&0x0F)
int8_t distance(search_node_cube_ptr node){
  int64_t index;
  int8_t ret,tmp;
  #if defined(DEBUG_PRINT)
  if (DISTANCE_TABLE == NULL){
    printf("DISTANCE_TABLE is not initialized\n");
  }
  #endif
  index = node_to_index_ud(node);
  ret = dist(index);
  index = node_to_index_lr(node);
  if (ret < (tmp = dist(index))){
    ret = tmp;
  }
  index = node_to_index_fb(node);
  if (ret < (tmp = dist(index))){
    ret = tmp;
  }
  return ret;
}



void get_distance_tables(distance_tables_ptr dst){
  if (CORNER_DISTANCE_TABLE == NULL){
    init_distance_table();
  }
  if (DISTANCE_TABLE == NULL){
    init_distance_table();
  }
  if (CORNER_ORIENTATION_SYM_to_SYM0 == NULL){
    init_sym_corner_orientation_table();
  }
  if (EDGE_FLIP_SYM_to_SYM0 == NULL){
    init_sym_edge_flip_table();
  }
  dst->c_ori_sym = CORNER_ORIENTATION_SYM_to_SYM0;
  dst->e_flip_sym = EDGE_FLIP_SYM_to_SYM0;
  dst->dist_tab = DISTANCE_TABLE;
  dst->corner_dist_tab = CORNER_DISTANCE_TABLE;
}
