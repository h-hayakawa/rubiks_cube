#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#define HAVE_STRUCT_TIMESPEC
#include<pthread.h>
#include<omp.h>
#include"solver.h"
#include"common_defines.h"
#include"coordinate_level_move.h"
#include"search_node_level_move.h"
#include"cube_structures.h"
#include"move_defines.h"
#include"distance_functions.h"
#include"structure_converter.h"
#include"solver_struct.h"
#if (CPU_ONLY == 0)
#include"cuda_solver.h"
#endif


#if defined(SILENT_MODE)
#define printf(a,...)
#endif

coord_cube initial_coord;
int32_t init_flag = 0;


__inline
static int32_t solved_coord(coord_cube_ptr coord){
  #if defined(DEBUG_PRINT)
  if (init_flag == 0){
    printf("function \"init_solver\" is not called\n");
    exit(1);
  }
  if(initial_coord.corner_position != INITIAL_CORNER_POSITION){
    printf("initial_coord.corner_position != INITIAL_CORNER_POSITION\n");
  }
  #endif
  if(coord->corner_position != initial_coord.corner_position){
    return 0;
  }
  if(coord->corner_orientation != initial_coord.corner_orientation){
    return 0;
  }
  if(coord->edge_flip != initial_coord.edge_flip){
    return 0;
  }
  if(coord->edge_position_ud != initial_coord.edge_position_ud){
    return 0;
  }
  if(coord->edge_position_lr != initial_coord.edge_position_lr){
    return 0;
  }
  if(coord->edge_position_fb != initial_coord.edge_position_fb){
    return 0;
  }
  return 1;
}

/* インライン展開用グローバル */
/* search_tree内で関数を使用するのは非効率なためインラインで書いた方が良い．*/
/* 1ファイルで全てのコードを書くか，展開用にグローバル変数を増やすかの2択 */
uint16_t *SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION = NULL;
uint16_t *SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION = NULL;//76.9KB
uint16_t *SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP = NULL;//72.0KB
uint16_t *SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION = NULL;//17.4KB
uint8_t *SOLVER_MOV_TRS_TAB_LR;
uint8_t *SOLVER_MOV_TRS_TAB_FB;
uint8_t *SOLVER_DISTANCE_TABLE = NULL;
uint8_t *SOLVER_CORNER_DISTANCE_TABLE = NULL;
uint16_t *SOLVER_CORNER_ORIENTATION_SYM_to_SYM0 = NULL;
uint16_t *SOLVER_EDGE_FLIP_SYM_to_SYM0 = NULL;


__inline
static
void init_queue(phase2_queue_ptr queue){
  queue->entry = 0;
  queue->move_hist = (move_hist_ptr)malloc(sizeof(move_hist)*PHASE_2_CUBE_QUEUE_SIZE);
  queue->head = 0;
  queue->tail = 0;
  queue->max_entry = PHASE_2_CUBE_QUEUE_SIZE;
}

__inline
static
void init_chunk(phase2_chunk_ptr chunk){
  int32_t tmp;
  tmp = PHASE_2_SEARCH_CHUNK_SIZE_CPU;
  if(tmp < PHASE_2_SEARCH_CHUNK_SIZE_GPU){
    tmp = PHASE_2_SEARCH_CHUNK_SIZE_GPU;
  }
  chunk->entry = 0;
  chunk->move_hist = (move_hist_ptr)malloc(sizeof(move_hist)*tmp);
}

__inline
static
void free_queue(phase2_queue_ptr queue){
  free(queue->move_hist);
}

__inline
static
void free_chunk(phase2_chunk_ptr chunk){
  free(chunk->move_hist);
}

__inline static
void en_queue(phase2_queue_ptr queue, move_hist elm){
  if (queue->entry < queue->max_entry){
    queue->move_hist[queue->head++] = elm;
    queue->head %= queue->max_entry;
    queue->entry++;
  }
}

__inline static
void de_queue_chunk(phase2_queue_ptr queue, phase2_chunk_ptr chunk, int32_t num){
  int32_t i;
  if (queue->entry < num){
    num = queue->entry;
  }
  if(queue->tail + num < queue->max_entry){
    memcpy(chunk->move_hist, queue->move_hist + queue->tail, sizeof(move_hist) * num);
    queue->tail = queue->tail + num;
  }else{
    memcpy(chunk->move_hist, queue->move_hist + queue->tail, sizeof(move_hist) * (queue->max_entry - queue->tail));
    memcpy(chunk->move_hist + (queue->max_entry - queue->tail), queue->move_hist, sizeof(move_hist) * (num - (queue->max_entry - queue->tail)));
    queue->tail = queue->tail + num - queue->max_entry;
  }
  queue->entry -= num;
  chunk->entry = num;
}

__inline static
int32_t queue_entry(phase2_queue_ptr queue){
  return queue->entry;
}

__inline static
int32_t queue_remain(phase2_queue_ptr queue){
  return queue->max_entry - queue->entry;
}

#define dist(i) ((SOLVER_DISTANCE_TABLE[(i)>>1] >> (((i)&1)<<2))&0x0F)
#define dist_c(i) ((SOLVER_CORNER_DISTANCE_TABLE[(i)>>1] >> (((i)&1)<<2))&0x0F)

typedef struct __phase1_context__{
  search_node node_array[22 - PHASE2_SEARCH_DEPTH];
  search_node_ptr p_node;
  int64_t count;
}phase1_context, *phase1_context_ptr;


/*小規模除算用テーブル*/
static const char div_tab[25]={
  0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,
};//100byte

/*小規模剰余算用テーブル*/
static const char mod_tab[25]={
  0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0,
};//100byte

/*小規模除算*/
/* a/b  , a < 24*/
#define div3(a) (div_tab[(a)])

/*小規模剰余算*/
/* a%b  , a < 24, b = 2,3,4,5*/
#define mod3(a) (mod_tab[(a)])


__inline
static
int32_t search_tree_phase2_1thread(int8_t *result, phase2_chunk_ptr chunk){
  int32_t solved_flag = 0;
  int32_t *solved_flag_ptr = &solved_flag;
  static const uint8_t invalid_move[7] = { 0x21, 0x0A, 0x14, 0x08, 0x10, 0x20, 0x00 };
  int32_t k;

  for (k = 0; k < chunk->entry && *solved_flag_ptr == 0; k++){
    search_node node_array[PHASE2_SEARCH_DEPTH + 2];
    register search_node_ptr p_node;
    register search_node_ptr node_arr = node_array;
    move_hist hist = chunk->move_hist[k];
    int32_t mv;
    coord_cube cube = chunk->cube;
    node_arr[0].cube = chunk->node_cube;
    mv = N_MOVES;
    while (hist.move_hist_hi){
      mv = (hist.move_hist_hi & 0x1F) - 1;
      coordinate_level_move(&cube,mv);
      search_node_level_move(&node_arr[0].cube, mv);
      hist.move_hist_hi >>= 5;
    }
    while (hist.move_hist_lo){
      mv = (hist.move_hist_lo & 0x1F) - 1;
      coordinate_level_move(&cube, mv);
      search_node_level_move(&node_arr[0].cube, mv);
      hist.move_hist_lo >>= 5;
    }

    node_arr[0].remain_depth = PHASE2_SEARCH_DEPTH;
    node_arr[0].mov = mv;
    node_arr[1].mov = -1;

    p_node = node_arr;
    while (p_node >= node_arr && *solved_flag_ptr == 0){
      if (p_node[0].remain_depth == 0){
        if(*solved_flag_ptr == 0){
          int32_t i,j;
          *solved_flag_ptr = 1;
          hist = chunk->move_hist[k];
          j = 0;
          while (hist.move_hist_hi){
            mv = (hist.move_hist_hi & 0x1F) - 1;
            result[j++] = mv;
            hist.move_hist_hi >>= 5;
          }
          while (hist.move_hist_lo){
            mv = (hist.move_hist_lo & 0x1F) - 1;
            result[j++] = mv;
            hist.move_hist_lo >>= 5;
          }
          for (i = 0; i < node_arr[0].remain_depth; i++){
            result[i+j] = node_arr[i + 1].mov;
          }
        }
        p_node--;
      }
      else{
        int32_t mov;
        for (mov = p_node[1].mov + 1; mov < N_MOVES; mov++){
          int32_t mv_lr, mv_fb;
          int32_t sym;
          int32_t c_ori, c_ori_sym, c_ori_ud, c_pos;
          uint32_t flip, flip_sym, class_;
          int32_t edge_4pos;
          int64_t index;
          p_node[1].remain_depth = p_node[0].remain_depth - 1;
          if (p_node[1].remain_depth < 0){
            continue;
          }
          if ((invalid_move[div3(p_node[0].mov)] >> div3(mov)) & 1){
            continue;
          }
          c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.ud_corner_orientation * N_MOVES + mov];
          c_ori_ud = c_ori;
          p_node[1].cube.ud_corner_orientation = c_ori;
          flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.ud_edge_flip          * N_MOVES + mov];
          p_node[1].cube.ud_edge_flip = flip;
          edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.ud_edge_ud            * N_MOVES + mov];
          p_node[1].cube.ud_edge_ud = edge_4pos;
          sym = edge_4pos & 0x07;
          class_ = edge_4pos >> 3;
          c_ori_sym = SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[c_ori * N_SYM + sym];
          flip_sym = SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
          index = (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip_sym * N_CORNER_ORI + c_ori_sym;
          if (p_node[1].remain_depth < dist(index)){
            continue;
          }
          mv_lr = SOLVER_MOV_TRS_TAB_LR[mov];
          c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.lr_corner_orientation * N_MOVES + mv_lr];
          p_node[1].cube.lr_corner_orientation = c_ori;
          flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.lr_edge_flip          * N_MOVES + mv_lr];
          p_node[1].cube.lr_edge_flip = flip;
          edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.lr_edge_ud            * N_MOVES + mv_lr];
          p_node[1].cube.lr_edge_ud = edge_4pos;
          sym = edge_4pos & 0x07;
          class_ = edge_4pos >> 3;
          c_ori_sym = SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[c_ori * N_SYM + sym];
          flip_sym = SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
          index = (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip_sym * N_CORNER_ORI + c_ori_sym;
          if (p_node[1].remain_depth < dist(index)){
            continue;
          }
          mv_fb = SOLVER_MOV_TRS_TAB_FB[mov];
          c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.fb_corner_orientation * N_MOVES + mv_fb];
          p_node[1].cube.fb_corner_orientation = c_ori;
          flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.fb_edge_flip          * N_MOVES + mv_fb];
          p_node[1].cube.fb_edge_flip = flip;
          edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.fb_edge_ud            * N_MOVES + mv_fb];
          p_node[1].cube.fb_edge_ud = edge_4pos;
          sym = edge_4pos & 0x07;
          class_ = edge_4pos >> 3;
          c_ori_sym = SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[c_ori * N_SYM + sym];
          flip_sym = SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
          index = (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip_sym * N_CORNER_ORI + c_ori_sym;
          if (p_node[1].remain_depth < dist(index)){
            continue;
          }
          c_pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION[p_node[0].cube.corner_position * N_MOVES + mov];
          if (p_node[1].remain_depth < dist_c(c_pos * N_CORNER_ORI + c_ori_ud)){
            continue;
          }
          p_node[1].cube.corner_position = c_pos;

          p_node[1].mov = mov;
          break;
        }
        if (mov == N_MOVES){
          p_node--;
        }
        else{
          p_node++;
          p_node[1].mov = -1;
        }
      }
    }
  }
  if (*solved_flag_ptr == 1){
    return 1;
  }
  return 0;
}

__inline
static
int32_t search_tree_phase1(search_node_cube_ptr cube_node, int32_t search_depth, int8_t *result, phase2_queue_ptr queue, int32_t num, phase1_context_ptr context, int32_t reset_flag){
  search_node node_array[22 - PHASE2_SEARCH_DEPTH];
  register search_node_ptr p_node;
  register search_node_ptr node_arr = node_array;
  static const uint8_t invalid_move[7] = { 0x21, 0x0A, 0x14, 0x08, 0x10, 0x20, 0x00 };
  int64_t count;
  if (queue_remain(queue) == 0){
    return 0;
  }
  if (reset_flag){
    node_arr[0].cube = *cube_node;
    node_arr[0].remain_depth = search_depth;
    node_arr[0].mov = N_MOVES;
    node_arr[1].mov = -1;
    p_node = node_arr;
    context->count = 0;
    count = 0;
  }
  else{
    int32_t i;
    for (i = 0; i < 22 - PHASE2_SEARCH_DEPTH; i++){
      node_arr[i] = context->node_array[i];
    }
    p_node = context->p_node;
    count = context->count;
  }

  #if defined(DEBUG_PRINT)
  if (search_depth < PHASE2_SEARCH_DEPTH){
    printf("error\n");
    return -1;
  }
  #endif
  while (p_node >= node_arr){
    int32_t mov;
    /* あるノードがphase2探索の条件を満たした(phase2に送るノードを追加する)． */
    if (p_node[0].remain_depth == PHASE2_SEARCH_DEPTH){
      move_hist hist = { 0 };
      int32_t i;
      count++;
      for (i = 0; i < (search_depth - PHASE2_SEARCH_DEPTH); i++){
        if (i < 6){
          hist.move_hist_lo <<= 5;
          hist.move_hist_lo += p_node[-i].mov + 1;
        }
        else{
          hist.move_hist_hi <<= 5;
          hist.move_hist_hi += p_node[-i].mov + 1;
        }
      }
      en_queue(queue, hist);
      if (--num == 0 || queue_remain(queue) == 0){
        /* 探索途中の場合は状態を保存して後で再開できるようにする */
        for (i = 0; i < 22 - PHASE2_SEARCH_DEPTH; i++){
          context->node_array[i] = node_arr[i];
        }
        context->p_node = p_node;
        context->count = count;
        return 0;
      }
      p_node--;
      continue;
    }
    for (mov = p_node[1].mov + 1; mov < N_MOVES; mov++){
      int32_t mv_lr, mv_fb;
      int32_t sym;
      int32_t c_ori, c_ori_sym, c_ori_ud, c_pos;
      uint32_t flip, flip_sym, class_;
      int32_t edge_4pos;
      int64_t index;
      p_node[1].remain_depth = p_node[0].remain_depth - 1;
      if (p_node[1].remain_depth < 0){
        continue;
      }
      if ((invalid_move[div3(p_node[0].mov)] >> div3(mov)) & 1){
        continue;
      }
      c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.ud_corner_orientation * N_MOVES + mov];
      c_ori_ud = c_ori;
      p_node[1].cube.ud_corner_orientation = c_ori;
      flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.ud_edge_flip          * N_MOVES + mov];
      p_node[1].cube.ud_edge_flip = flip;
      edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.ud_edge_ud            * N_MOVES + mov];
      p_node[1].cube.ud_edge_ud = edge_4pos;

      mv_lr = SOLVER_MOV_TRS_TAB_LR[mov];
      c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.lr_corner_orientation * N_MOVES + mv_lr];
      p_node[1].cube.lr_corner_orientation = c_ori;
      flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.lr_edge_flip          * N_MOVES + mv_lr];
      p_node[1].cube.lr_edge_flip = flip;
      edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.lr_edge_ud            * N_MOVES + mv_lr];
      p_node[1].cube.lr_edge_ud = edge_4pos;

      mv_fb = SOLVER_MOV_TRS_TAB_FB[mov];
      c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.fb_corner_orientation * N_MOVES + mv_fb];
      p_node[1].cube.fb_corner_orientation = c_ori;
      flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.fb_edge_flip          * N_MOVES + mv_fb];
      p_node[1].cube.fb_edge_flip = flip;
      edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.fb_edge_ud            * N_MOVES + mv_fb];
      p_node[1].cube.fb_edge_ud = edge_4pos;

      c_pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION[p_node[0].cube.corner_position * N_MOVES + mov];

      p_node[1].cube.corner_position = c_pos;
      p_node[1].mov = mov;
      break;
    }
    if (mov == N_MOVES){
      p_node--;
    }
    else{
      p_node++;
      p_node[1].mov = -1;
    }
  }
  context->count = count;
  return 1;
}


#define N_CPU_STREAM ((N_THREADS-1) * 2)

typedef struct __cpu_stream_state__{
  int32_t stream_state_array[N_CPU_STREAM];
}cpu_stream_state, *cpu_stream_state_ptr;

typedef struct __thread_param__{
  int32_t thread_id;
  int32_t call_flag[2];
  int32_t exit_flag;
  int32_t solved_flag[2];
  phase2_chunk_ptr chunk_ptr[2];
  int8_t result_buffer[2][20];
  pthread_t p_thread_id;
  pthread_mutex_t mp[2];
}thread_param, *thread_param_ptr;

thread_param THREAD_PARAM[N_THREADS - 1];

void *phase2_worker(void * thread_prm){
  thread_param_ptr param = (thread_param_ptr)thread_prm;
  int32_t i;
  int32_t thread_id = param->thread_id;
  while(!THREAD_PARAM[thread_id].exit_flag){
    for(i = 0;i < 2; i ++){
      int32_t p;
      pthread_mutex_lock(& THREAD_PARAM[thread_id].mp[i]);
      p = THREAD_PARAM[thread_id].call_flag[i];
      pthread_mutex_unlock(& THREAD_PARAM[thread_id].mp[i]);
      if(p){
        int8_t result[2][20];
        int32_t j;
        int32_t solved_flag[2];
        solved_flag[i] = search_tree_phase2_1thread(result[i], THREAD_PARAM[thread_id].chunk_ptr[i]);
        pthread_mutex_lock(& THREAD_PARAM[thread_id].mp[i]);
        for(j = 0; j < 20;j++){
          THREAD_PARAM[thread_id].result_buffer[i][j] = result[i][j];
        }
        THREAD_PARAM[thread_id].call_flag[i] = 0;
        THREAD_PARAM[thread_id].solved_flag[i] = solved_flag[i];
        pthread_mutex_unlock(& THREAD_PARAM[thread_id].mp[i]);
      }
    }
    usleep(1);
  }
  return 0;
}

void create_phase2_worker(){
  int32_t i;
  for(i = 0; i < N_THREADS - 1 ; i++){
    THREAD_PARAM[i].thread_id = i;
    THREAD_PARAM[i].call_flag[0] = 0;
    THREAD_PARAM[i].call_flag[1] = 0;
    THREAD_PARAM[i].exit_flag = 0;
    pthread_mutex_init( &THREAD_PARAM[i].mp[0], NULL);
    pthread_mutex_init( &THREAD_PARAM[i].mp[1], NULL);
    pthread_create(&THREAD_PARAM[i].p_thread_id, NULL, phase2_worker , &THREAD_PARAM[i]);
  }
}

void stop_phase2_worker(){
  int32_t i;
  for(i = 0; i < N_THREADS - 1 ; i++){
    THREAD_PARAM[i].exit_flag = 1;
  }
  for(i = 0; i < N_THREADS - 1 ; i++){
    pthread_join(THREAD_PARAM[i].p_thread_id, NULL);
  }
}

void search_tree_phase2_cpu_async(phase2_chunk_ptr chunk, int32_t stream_id){
  int32_t thread_id = stream_id / 2;
  pthread_mutex_lock(& THREAD_PARAM[thread_id].mp[stream_id % 2]);
  THREAD_PARAM[thread_id].chunk_ptr[stream_id % 2] = chunk;
  THREAD_PARAM[thread_id].call_flag[stream_id % 2] = 1;
  pthread_mutex_unlock(& THREAD_PARAM[thread_id].mp[stream_id % 2]);
}

int32_t get_search_result_cpu(int8_t *result, int32_t stream_id){
  int32_t thread_id = stream_id / 2;
  int32_t i;
  int32_t ret;
  pthread_mutex_lock(& THREAD_PARAM[thread_id].mp[stream_id % 2]);
  for(i = 0; i < 20 ; i++){
    result[i] = THREAD_PARAM[thread_id].result_buffer[stream_id % 2][i];
  }
  if(ret = THREAD_PARAM[thread_id].solved_flag[stream_id % 2]){
    THREAD_PARAM[thread_id].solved_flag[stream_id % 2] = 0;
  }
  pthread_mutex_unlock(& THREAD_PARAM[thread_id].mp[stream_id % 2]);
  return ret;
}

void sync_all_stream_cpu(){
  int32_t i;
  int32_t flag = 1;
  while(flag){
    flag = 0;
    for(i = 0 ; i < N_CPU_STREAM ; i++){
      pthread_mutex_lock(& THREAD_PARAM[i/2].mp[i % 2]);
      if(THREAD_PARAM[i/2].call_flag[i%2]){
        flag = 1;
      }
      pthread_mutex_unlock(& THREAD_PARAM[i/2].mp[i % 2]);
    }
    usleep(1);
  }
}
void get_stream_state_cpu(cpu_stream_state_ptr state){
  int32_t i;
  for(i = 0 ; i < N_CPU_STREAM ; i++){
    pthread_mutex_lock(& THREAD_PARAM[i/2].mp[i % 2]);
    if(THREAD_PARAM[i/2].call_flag[i%2]){
      state->stream_state_array[i] = STREAM_STATE_BUSY;
    }else{
      state->stream_state_array[i] = STREAM_STATE_READY;
    }
    pthread_mutex_unlock(& THREAD_PARAM[i/2].mp[i % 2]);
  }
}

#if defined(P_THREAD)
__inline
static
int32_t two_phase_search_tree(coord_cube_ptr cube, search_node_cube_ptr cube_node, int32_t search_depth, int8_t *result){
  int32_t reset_flag;
  int32_t phase1_finished = 0;
  int32_t ret = 0;
#if (CPU_ONLY == 0)
  stream_state state;
#endif
  cpu_stream_state cpu_state;
  int32_t i;
#if (CPU_ONLY == 0)
  int32_t enq_flag[N_STREAM] = {0};
#endif
#if (N_CPU_STREAM)
  int32_t enq_flag_cpu[N_CPU_STREAM] = {0};
#endif
  int32_t cpu_search_flag;
  int64_t cpu_count = 0, gpu_count = 0;
  double rate;
  double t1, t2, t3;

  phase1_context context;
  phase2_queue queue;
#if (CPU_ONLY == 0)
  phase2_chunk gpu_chunk[N_STREAM];
#endif
  phase2_chunk cpu_chunk[N_CPU_STREAM];
  phase2_chunk host_chunk;

#if (CPU_ONLY == 0)
  sync_all_stream();
  get_stream_state(&state);
#endif

  create_phase2_worker();
  sync_all_stream_cpu();
  get_stream_state_cpu(&cpu_state);

  init_queue(&queue);
  queue.cube = *cube;
#if (CPU_ONLY == 0)
  for(i = 0;i < N_STREAM ; i ++){
    init_chunk(&gpu_chunk[i]);
    gpu_chunk[i].cube = queue.cube;
    convert_coord_to_search_node(&queue.cube, &gpu_chunk[i].node_cube);
  }
#endif
  for(i = 0;i < N_CPU_STREAM ; i ++){
    init_chunk(&cpu_chunk[i]);
    cpu_chunk[i].cube = queue.cube;
    convert_coord_to_search_node(&queue.cube, &cpu_chunk[i].node_cube);
  }
  init_chunk(&host_chunk);
  host_chunk.cube = queue.cube;
  convert_coord_to_search_node(&queue.cube, &host_chunk.node_cube);


  t1 = omp_get_wtime();
  reset_flag = 1;
  while ((!phase1_finished || queue_entry(&queue))&& !ret){
    cpu_search_flag = 1;
    if(!phase1_finished){
      if(queue_remain(&queue)){
        cpu_search_flag = 0;
        phase1_finished = search_tree_phase1(cube_node, search_depth, result, &queue, PHASE_1_SEARCH_CHUNK_SIZE, &context, reset_flag);
        reset_flag = 0;
        //printf("phase1 search is called. depth = %d remain entry = %d\n", search_depth, queue_entry(&queue));
      }
      if(phase1_finished){
        t2 = omp_get_wtime();
#if(NO_STATUS_PRINT == 0)
        printf("==================== phase1 finished =====================\n");
#endif
      }
    }
    if(cpu_search_flag){
      int32_t entry;
      de_queue_chunk(&queue, &host_chunk, PHASE_2_SEARCH_CHUNK_SIZE_HOST);
      entry = host_chunk.entry;
      cpu_count += entry;
      ret = search_tree_phase2_1thread(result, &host_chunk);
      if(ret){
#if(NO_STATUS_PRINT == 0)
        printf("solved host\n");
#endif
        goto SYNC;
      }
#if(NO_STATUS_PRINT == 0)
      rate = cpu_count?(double)gpu_count/cpu_count:0;
      printf("  phase2 enqueued on CPU     entry = %6d remain entry = %10d   GPU / CPU rate = %.4f depth = %d ############\n",
                  entry , queue_entry(&queue), rate, search_depth);
#endif
    }
    ///////////////////////////////////////////////////////////////
#if (CPU_ONLY == 0)
    for(i = 0 ; i < N_STREAM; i ++){
      if(state.stream_state_array[i] == STREAM_STATE_READY && queue_entry(&queue)){
        int32_t entry;
        de_queue_chunk(&queue, &gpu_chunk[i], PHASE_2_SEARCH_CHUNK_SIZE_GPU);
        entry = gpu_chunk[i].entry;
        gpu_count += entry;
        search_tree_phase2_cuda_async(&gpu_chunk[i], i);
        enq_flag [i] = 1;
        rate = cpu_count?(double)gpu_count/cpu_count:0;
#if(NO_STATUS_PRINT == 0)
        printf("  phase2 enqueued on GPU(%2d) entry = %6d remain entry = %10d   GPU / CPU rate = %.4f depth = %d\n",
              i, entry,queue_entry(&queue), rate, search_depth);
#endif
      }
    }
    get_stream_state(&state);
    for(i = 0 ; i < N_STREAM; i ++){
      if(state.stream_state_array[i] == STREAM_STATE_READY){
        if(enq_flag [i] && !ret ){
          if (get_search_result(result, i)){
#if(NO_STATUS_PRINT == 0)
            printf("solved GPU %d\n",i);
#endif
            ret = 1;
            goto SYNC;
          }
          enq_flag [i] = 0;
        }
      }
    }
#endif
    ///////////////////////////////////////////////////////////////
#if(N_CPU_STREAM)
    for(i = 0 ; i < N_CPU_STREAM; i ++){
      if(cpu_state.stream_state_array[i] == STREAM_STATE_READY && queue_entry(&queue)){
        int32_t entry;
        de_queue_chunk(&queue, &cpu_chunk[i], PHASE_2_SEARCH_CHUNK_SIZE_CPU);
        entry = cpu_chunk[i].entry;
        cpu_count += entry;
        search_tree_phase2_cpu_async(&cpu_chunk[i], i);
        enq_flag_cpu [i] = 1;
        rate = cpu_count?(double)gpu_count/cpu_count:0;
#if(NO_STATUS_PRINT == 0)
        printf("  phase2 enqueued on CPU(%2d) entry = %6d remain entry = %10d   GPU / CPU rate = %.4f depth = %d\n",
               i,entry,queue_entry(&queue), rate, search_depth);
#endif
      }
    }

    get_stream_state_cpu(&cpu_state);
    for(i = 0 ; i < N_CPU_STREAM; i ++){
      if(cpu_state.stream_state_array[i] == STREAM_STATE_READY){
        if(enq_flag_cpu [i] && !ret ){
          if (get_search_result_cpu(result, i)){
#if(NO_STATUS_PRINT == 0)
            printf("solved CPU %d\n",i);
#endif
            ret = 1;
            goto SYNC;
          }
          enq_flag_cpu [i] = 0;
        }
      }
    }
#endif
  }

SYNC:;
#if (CPU_ONLY == 0)
  sync_all_stream();
  //get_stream_state(&state);
  for(i = 0 ; i < N_STREAM; i ++){
    if(enq_flag [i] && !ret){
      if (get_search_result(result, i)){
#if(NO_STATUS_PRINT == 0)
        printf("solved GPU %d\n",i);
#endif
        ret = 1;
      }
    }
  }
#endif
#if(N_CPU_STREAM)
  sync_all_stream_cpu();
  for(i = 0 ; i < N_CPU_STREAM; i ++){
    if(enq_flag_cpu [i] && !ret){
      if (get_search_result_cpu(result, i)){
#if(NO_STATUS_PRINT == 0)
        printf("solved CPU %d\n",i);
#endif
        ret = 1;
      }
    }
  }
  stop_phase2_worker();
#endif
  t3 = omp_get_wtime();

  free_queue(&queue);
#if (CPU_ONLY == 0)
  for(i = 0;i < N_STREAM ; i ++){
    free_chunk(&gpu_chunk[i]);
  }
#endif
  for(i = 0;i < N_CPU_STREAM ; i ++){
    free_chunk(&cpu_chunk[i]);
  }
  free_chunk(&host_chunk);
#if(NO_STATUS_PRINT == 0)
  printf("depth %d , GPU : CPU rate = %.4f : %.4f cpu=%lld , gpu = %lld , phase2sum = %lld\n",search_depth, (double)gpu_count/(cpu_count + gpu_count),(double)cpu_count/(cpu_count + gpu_count), (long long int)cpu_count, (long long int)gpu_count, (long long int)(cpu_count + gpu_count));

  printf("phase1 : %fsec\n", t2 - t1);
  //printf("phase2 : %fsec\n", t3 - t2);
  printf("phase2 : %fsec\n", t3 - t1);
  printf("===================== search finished. depth = %d =====================\n", search_depth);
#endif
  return ret;
}
#endif



__inline
static
int32_t one_phase_search_tree(coord_cube_ptr cube, search_node_cube_ptr cube_node, int32_t search_depth, int8_t *result){
  search_node node_array[22];
  register search_node_ptr p_node;
  register search_node_ptr node_arr = node_array;
  int32_t ret = 0;
  static const uint8_t invalid_move[7] = { 0x21, 0x0A, 0x14, 0x08, 0x10, 0x20, 0x00 };
  uint64_t n_nodes = 0;
  uint64_t test = 0;
  node_arr[0].cube = *cube_node;
  node_arr[0].remain_depth = search_depth;
  node_arr[0].mov = N_MOVES;
  node_arr[1].mov = -1;
  p_node = node_arr;
  while (p_node >= node_arr){
    if (p_node[0].remain_depth == 0){
      int32_t i;
      test++;
      for (i = 0; i < node_arr[0].remain_depth; i++){
        result[i] = node_arr[i + 1].mov;
      }
      ret = 1;
      break;
      p_node--;
    }
    else{
      int32_t mov;
      for (mov = p_node[1].mov + 1; mov < N_MOVES; mov++){
        int32_t mv_lr, mv_fb;
        int32_t sym;
        int32_t c_ori, c_ori_sym, c_ori_ud;
        uint32_t flip, flip_sym, class_;
        int32_t edge_4pos;
        int64_t index;
        int32_t c_pos;
        p_node[1].remain_depth = p_node[0].remain_depth - 1;
        if (p_node[1].remain_depth < 0){
          continue;
        }
        if ((invalid_move[div3(p_node[0].mov)] >> div3(mov)) & 1){
          continue;
        }

        c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.ud_corner_orientation * N_MOVES + mov];
        c_ori_ud = c_ori;
        p_node[1].cube.ud_corner_orientation =  c_ori;
        flip =  SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.ud_edge_flip          * N_MOVES + mov];
        p_node[1].cube.ud_edge_flip = flip;
        edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.ud_edge_ud            * N_MOVES + mov];
        p_node[1].cube.ud_edge_ud = edge_4pos;
        sym = edge_4pos & 0x07;
        class_ = edge_4pos >> 3;
        c_ori_sym = SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[c_ori * N_SYM + sym];
        flip_sym = SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
        index = (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip_sym * N_CORNER_ORI + c_ori_sym;
        if (p_node[1].remain_depth < dist(index)){
          continue;
        }
        mv_lr = SOLVER_MOV_TRS_TAB_LR[mov];
        c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.lr_corner_orientation * N_MOVES + mv_lr];
        p_node[1].cube.lr_corner_orientation = c_ori;
        flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.lr_edge_flip          * N_MOVES + mv_lr];
        p_node[1].cube.lr_edge_flip = flip;
        edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.lr_edge_ud            * N_MOVES + mv_lr];
        p_node[1].cube.lr_edge_ud = edge_4pos;
        sym = edge_4pos & 0x07;
        class_ = edge_4pos >> 3;
        c_ori_sym = SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[c_ori * N_SYM + sym];
        flip_sym = SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
        index = (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip_sym * N_CORNER_ORI + c_ori_sym;
        if (p_node[1].remain_depth < dist(index)){
          continue;
        }
        mv_fb = SOLVER_MOV_TRS_TAB_FB[mov];
        c_ori = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[p_node[0].cube.fb_corner_orientation * N_MOVES + mv_fb];
        p_node[1].cube.fb_corner_orientation = c_ori;
        flip = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[p_node[0].cube.fb_edge_flip          * N_MOVES + mv_fb];
        p_node[1].cube.fb_edge_flip = flip;
        edge_4pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[p_node[0].cube.fb_edge_ud            * N_MOVES + mv_fb];
        p_node[1].cube.fb_edge_ud = edge_4pos;
        sym = edge_4pos & 0x07;
        class_ = edge_4pos >> 3;
        c_ori_sym = SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[c_ori * N_SYM + sym];
        flip_sym = SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
        index = (int64_t)(class_)* N_EDGE_FLIP * N_CORNER_ORI + flip_sym * N_CORNER_ORI + c_ori_sym;
        if (p_node[1].remain_depth < dist(index)){
          continue;
        }

        c_pos = SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION[p_node[0].cube.corner_position * N_MOVES + mov];
        if (p_node[1].remain_depth < dist_c(c_pos * N_CORNER_ORI + c_ori_ud)){
          continue;
        }
        p_node[1].cube.corner_position = c_pos;

        n_nodes++;
        p_node[1].mov = mov;
        break;
      }
      if (mov == N_MOVES){
        p_node--;
      }
      else{
        p_node++;
        p_node[1].mov = -1;
      }
    }
  }
  #if !defined(SILENT_MODE)
  //printf("depth = %2d  n_nodes %10llu  n_test %10llu\n", search_depth, n_nodes, test);
  #endif
  return ret;
}




int32_t solve(coord_cube_ptr cube, int8_t *result){
  int32_t i;
  int32_t re;
  search_node_cube cube_node;
  if (solved_coord(cube)){
    return 0;
  }
  convert_coord_to_search_node(cube, &cube_node);
  for (i = 1; i <= 20; i++){
#if(NO_STATUS_PRINT)
    printf("search_depth = %d\n", i);
#endif
    if (i > TWO_PHASE_SEARCH_SWITCH_DEPTH)
    re = two_phase_search_tree(cube, &cube_node, i, result);
    else
    re = one_phase_search_tree(cube, &cube_node, i, result);
    if (re)
    break;
  }
  if (i == 21){
    return -1;
  }
  return i;
}


void init_solver(){
  search_node_level_tables tab;
  distance_tables dist_tab;
  if (init_flag){/*初期化済ならなにもしない*/
    return;
  }
  init_coordinate_level_cube(&initial_coord);
  init_flag = 1;
  printf("initializing UD_SLICE symmetry table...");
  init_node_flip_ud_tab();
  printf("finished.\n");
  printf("initializing coordinate level move tables...");
  init_coordinate_level_move_table();
  printf("finished.\n");
  printf("initializing search node level move tables...");
  init_search_node_level_move_table();
  printf("finished.\n");
  printf("initializing distance table...");
  init_distance_table();
  printf("finished.\n");
  get_search_node_level_tables(&tab);
  get_distance_tables(&dist_tab);
  SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION = tab.mov_corner_pos;
  SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION = tab.mov_corner_8_ori;
  SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP = tab.mov_edge_12_flip;
  SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION = tab.mov_edge_4_pos;
  SOLVER_MOV_TRS_TAB_LR = tab.mov_trs_lr;
  SOLVER_MOV_TRS_TAB_FB = tab.mov_trs_fb;
  SOLVER_DISTANCE_TABLE = dist_tab.dist_tab;
  SOLVER_CORNER_ORIENTATION_SYM_to_SYM0 = dist_tab.c_ori_sym;
  SOLVER_EDGE_FLIP_SYM_to_SYM0 = dist_tab.e_flip_sym;
  SOLVER_CORNER_DISTANCE_TABLE = dist_tab.corner_dist_tab;
#if (CPU_ONLY == 0)
  init_device_table(
    SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION,
    SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_POSITION,
    SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP,
    SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION,
    SOLVER_CORNER_ORIENTATION_SYM_to_SYM0,
    SOLVER_EDGE_FLIP_SYM_to_SYM0,
    SOLVER_DISTANCE_TABLE,
    SOLVER_CORNER_DISTANCE_TABLE,
    SOLVER_MOV_TRS_TAB_LR,
    SOLVER_MOV_TRS_TAB_FB
  );
#endif
}
