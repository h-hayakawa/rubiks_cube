#include<stdio.h>
#include<stdlib.h>
#include<helper_cuda.h>
#include<helper_timer.h>
#include"move_defines.h"
#include"solver_struct.h"
#include"cuda_solver.h"

#if (N_GPU > 0)

uint16_t *CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[N_STREAM/2] = {NULL};//76.9KB
uint16_t *CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[N_STREAM/2] = {NULL};//72.0KB
uint16_t *CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[N_STREAM/2] = {NULL};//17.4KB
uint16_t *CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_POSITION[N_STREAM/2] = {NULL};

__constant__ uint8_t CUDA_SOLVER_MOV_TRS_TAB_LR[N_MOVES];
__constant__ uint8_t CUDA_SOLVER_MOV_TRS_TAB_FB[N_MOVES];
uint8_t *CUDA_SOLVER_DISTANCE_TABLE[N_STREAM/2] = {NULL};
uint8_t *CUDA_SOLVER_CORNER_DISTANCE_TABLE[N_STREAM/2] = {NULL};

uint16_t *CUDA_SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[N_STREAM/2] = {NULL};
__constant__ uint16_t CUDA_SOLVER_EDGE_FLIP_SYM_to_SYM0[N_EDGE_FLIP * N_SYM];
__constant__ uint8_t D_INVALID_MOVE[7];


move_hist *D_MOVE_HIST[N_STREAM];
search_node_cube *D_SEARCH_NODE[N_STREAM];
uint32_t *D_PHASE2_ENTRY[N_STREAM];
int32_t *D_RESULT[N_STREAM];
cudaStream_t STREAM[N_STREAM];


__device__
void d_search_node_level_move(search_node_cube_ptr cube, int32_t mov_id,
uint16_t *d_corner_8pos_mov_table,
uint16_t *d_corner_8ori_mov_table,
uint16_t *d_edge_4pos_mov_table,
uint16_t *d_edge_12flip_mov_table
){
  int32_t mv_lr,mv_fb;
  cube->corner_position       = d_corner_8pos_mov_table[cube->corner_position       * N_MOVES + mov_id];
  cube->ud_corner_orientation = d_corner_8ori_mov_table[cube->ud_corner_orientation * N_MOVES + mov_id];
  cube->ud_edge_flip          = d_edge_12flip_mov_table[cube->ud_edge_flip          * N_MOVES + mov_id];
  cube->ud_edge_ud            = d_edge_4pos_mov_table  [cube->ud_edge_ud            * N_MOVES + mov_id];
  mv_lr = CUDA_SOLVER_MOV_TRS_TAB_LR[mov_id];
  cube->lr_corner_orientation = d_corner_8ori_mov_table[cube->lr_corner_orientation * N_MOVES + mv_lr];
  cube->lr_edge_flip          = d_edge_12flip_mov_table[cube->lr_edge_flip          * N_MOVES + mv_lr];
  cube->lr_edge_ud            = d_edge_4pos_mov_table  [cube->lr_edge_ud            * N_MOVES + mv_lr];
  mv_fb = CUDA_SOLVER_MOV_TRS_TAB_FB[mov_id];
  cube->fb_corner_orientation = d_corner_8ori_mov_table[cube->fb_corner_orientation * N_MOVES + mv_fb];
  cube->fb_edge_flip          = d_edge_12flip_mov_table[cube->fb_edge_flip          * N_MOVES + mv_fb];
  cube->fb_edge_ud            = d_edge_4pos_mov_table  [cube->fb_edge_ud            * N_MOVES + mv_fb];

}
#define dist_c(i) ((d_corner_distance_table[(i)>>1] >> (((i)&1)<<2))&0x0F)

__global__
void search_tree_phase2_cuda_karnel(
  search_node_cube_ptr d_bridge_cube_node,
  move_hist_ptr move_hist,
  uint32_t *d_bridge_entry_count,
  int32_t * result,
  uint16_t *d_corner_8pos_mov_table,
  uint16_t *d_corner_8ori_mov_table,
  uint16_t *d_edge_4pos_mov_table,
  uint16_t *d_edge_12flip_mov_table,
  uint8_t *d_distance_table,
  uint8_t *d_corner_distance_table,
  uint16_t *corner_ori_sym
){
  int32_t i;
  uint32_t mh,ml;
  uint32_t mh_bk, ml_bk;
  int32_t remain_depth;
  int32_t mov;
  int32_t node_idx;
  search_node node_arr[PHASE2_SEARCH_DEPTH + 2];
  search_node curr_node;
  search_node next_node;

  i = blockIdx.x*blockDim.x + threadIdx.x;
  curr_node.cube = d_bridge_cube_node[0];

  mh = mh_bk = move_hist[i].move_hist_hi;
  ml = ml_bk = move_hist[i].move_hist_lo;

  mov = N_MOVES;

  while(mh){
    mov = (mh & 0x1F)-1;
    d_search_node_level_move(&curr_node.cube,mov,
                  d_corner_8pos_mov_table,
                  d_corner_8ori_mov_table,
                  d_edge_4pos_mov_table,
                  d_edge_12flip_mov_table
                  );

    mh >>= 5;
  }
  while(ml){
    mov = (ml & 0x1F)-1;
    d_search_node_level_move(&curr_node.cube,mov,
                  d_corner_8pos_mov_table,
                  d_corner_8ori_mov_table,
                  d_edge_4pos_mov_table,
                  d_edge_12flip_mov_table
                  );

    ml >>= 5;
  }

  curr_node.mov = mov;
  node_arr[0] = curr_node;
  node_arr[1].mov = -1;

  node_idx = 0;
  remain_depth = PHASE2_SEARCH_DEPTH;

  while(node_idx >=0 && result[0] == -1){
    curr_node = node_arr[node_idx];
    next_node = node_arr[node_idx + 1];
    if(remain_depth == 0){
      if(result[0] == -1){
        int32_t ii,jj = 0;
        if(atomicCAS(&result[0],-1,0)==-1){
          mh = mh_bk;
          ml = ml_bk;
          while(mh){
            result[jj++] = (mh & 0x1F)-1;
            mh >>= 5;
          }
          while(ml){
            result[jj++] = (ml & 0x1F)-1;
            ml >>= 5;
          }
          for(ii = 0;ii<PHASE2_SEARCH_DEPTH;ii++){
            result[jj+ii] = node_arr[ii+1].mov;
          }
        }
        break;
      }
      node_idx--;
      remain_depth ++;
    }else{
      for(mov = next_node.mov + 1; mov < N_MOVES; mov ++){
        int32_t mv_lr, mv_fb;
        int32_t sym;
        int32_t c_ori, c_ori_sym, c_ori_ud, c_pos;
        uint32_t flip, flip_sym, class_;
        int32_t edge_4pos;
        uint32_t index1, index2;
        if((D_INVALID_MOVE[(curr_node.mov * 0x16)>>6]>>((mov * 0x16)>>6))&1){
          continue;
        }
//================================================================
        c_ori = d_corner_8ori_mov_table[curr_node.cube.ud_corner_orientation * N_MOVES + mov];
        c_ori_ud = c_ori;
        next_node.cube.ud_corner_orientation = c_ori;
        flip = d_edge_12flip_mov_table[curr_node.cube.ud_edge_flip          * N_MOVES + mov];
        next_node.cube.ud_edge_flip = flip;
        edge_4pos = d_edge_4pos_mov_table[curr_node.cube.ud_edge_ud            * N_MOVES + mov];
        next_node.cube.ud_edge_ud = edge_4pos;
        sym = edge_4pos & 0x07;
        class_ = edge_4pos >> 3;
        c_ori_sym = corner_ori_sym[c_ori * N_SYM + sym];
        flip_sym = CUDA_SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
        index1 = class_* (N_EDGE_FLIP/2) * N_CORNER_ORI + ((flip_sym * N_CORNER_ORI + c_ori_sym)>>1);
        index2 = ((flip_sym + c_ori_sym)&1)<<2;
        if(remain_depth - 1 < ((d_distance_table[index1] >> index2)&0x0F)){
          continue;
        }
        mv_lr = CUDA_SOLVER_MOV_TRS_TAB_LR[mov];
        c_ori = d_corner_8ori_mov_table[curr_node.cube.lr_corner_orientation * N_MOVES + mv_lr];
        next_node.cube.lr_corner_orientation = c_ori;
        flip = d_edge_12flip_mov_table[curr_node.cube.lr_edge_flip          * N_MOVES + mv_lr];
        next_node.cube.lr_edge_flip = flip;
        edge_4pos = d_edge_4pos_mov_table[curr_node.cube.lr_edge_ud            * N_MOVES + mv_lr];
        next_node.cube.lr_edge_ud = edge_4pos;
        sym = edge_4pos & 0x07;
        class_ = edge_4pos >> 3;
        c_ori_sym = corner_ori_sym[c_ori * N_SYM + sym];
        flip_sym = CUDA_SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
        index1 = class_* (N_EDGE_FLIP/2) * N_CORNER_ORI + ((flip_sym * N_CORNER_ORI + c_ori_sym)>>1);
        index2 = ((flip_sym + c_ori_sym)&1)<<2;
        if(remain_depth - 1 < ((d_distance_table[index1] >> index2)&0x0F)){
          continue;
        }
        mv_fb = CUDA_SOLVER_MOV_TRS_TAB_FB[mov];
        c_ori = d_corner_8ori_mov_table[curr_node.cube.fb_corner_orientation * N_MOVES + mv_fb];
        next_node.cube.fb_corner_orientation = c_ori;
        flip = d_edge_12flip_mov_table[curr_node.cube.fb_edge_flip          * N_MOVES + mv_fb];
        next_node.cube.fb_edge_flip = flip;
        edge_4pos = d_edge_4pos_mov_table[curr_node.cube.fb_edge_ud            * N_MOVES + mv_fb];
        next_node.cube.fb_edge_ud = edge_4pos;
        sym = edge_4pos & 0x07;
        class_ = edge_4pos >> 3;
        c_ori_sym = corner_ori_sym[c_ori * N_SYM + sym];
        flip_sym = CUDA_SOLVER_EDGE_FLIP_SYM_to_SYM0[flip * N_SYM + sym];
        index1 = class_* (N_EDGE_FLIP/2) * N_CORNER_ORI + ((flip_sym * N_CORNER_ORI + c_ori_sym)>>1);
        index2 = ((flip_sym + c_ori_sym)&1)<<2;
        if(remain_depth - 1 < ((d_distance_table[index1] >> index2)&0x0F)){
          continue;
        }

        c_pos = d_corner_8pos_mov_table[curr_node.cube.corner_position * N_MOVES + mov];
        if (remain_depth - 1 < dist_c(c_pos * N_CORNER_ORI + c_ori_ud)){
          continue;
        }
        next_node.cube.corner_position = c_pos;
//================================================================
        next_node.mov = mov;
        node_arr[node_idx + 1] = next_node;
        break;
      }
      if(mov == N_MOVES){
        node_idx--;
        remain_depth ++;
      }else{
        node_idx++;
        remain_depth --;
        node_arr[node_idx + 1].mov = -1;
      }
    }
  }
}

static void HandleError(cudaError_t err, const char *file, int line)
{
    if(err!=cudaSuccess){
    printf("%s in %s file at line %d\n", cudaGetErrorString(err), file, line);
    exit(EXIT_FAILURE);
    }
}
#define HANDLE_ERROR(err) (HandleError(err, __FILE__, __LINE__))

void search_tree_phase2_cuda_async(phase2_chunk_ptr chunk, int32_t stream_id){
  int32_t i;
  int32_t result_init = -1;
  int32_t entry = chunk->entry;
  int32_t device = stream_id/2;
  cudaSetDevice(device);

  if(entry%BLOCK_DIM){
    for(i = entry;i<(entry+BLOCK_DIM-1)/BLOCK_DIM*BLOCK_DIM;i++){
      chunk->move_hist[i] = chunk->move_hist[entry - 1];
    }
    entry = (entry+BLOCK_DIM-1)/BLOCK_DIM*BLOCK_DIM;
  }

  HANDLE_ERROR(cudaMemcpyAsync(D_MOVE_HIST[stream_id],chunk->move_hist,sizeof(move_hist)*(entry),cudaMemcpyHostToDevice,STREAM[stream_id]));
  HANDLE_ERROR(cudaMemcpyAsync(D_SEARCH_NODE[stream_id],&(chunk->node_cube),sizeof(search_node_cube),cudaMemcpyHostToDevice,STREAM[stream_id]));
  HANDLE_ERROR(cudaMemcpyAsync(D_PHASE2_ENTRY[stream_id],&entry,sizeof(int),cudaMemcpyHostToDevice,STREAM[stream_id]));
  HANDLE_ERROR(cudaMemcpyAsync(D_RESULT[stream_id],&result_init,sizeof(int),cudaMemcpyHostToDevice,STREAM[stream_id]));

  search_tree_phase2_cuda_karnel<<<entry/BLOCK_DIM,BLOCK_DIM,0,STREAM[stream_id]>>>(
    D_SEARCH_NODE[stream_id],
    D_MOVE_HIST[stream_id],
    D_PHASE2_ENTRY[stream_id],
    D_RESULT[stream_id],
    CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_POSITION[device],
    CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[device],
    CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[device],
    CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[device],
    CUDA_SOLVER_DISTANCE_TABLE[device],
    CUDA_SOLVER_CORNER_DISTANCE_TABLE[device],
    CUDA_SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[device]
  );
}

int32_t get_search_result(int8_t *result, int32_t stream_id){
  int32_t result__[20];
  int32_t i;

  //printf("get_search_result stream = %d\n",stream_id);

  cudaSetDevice(stream_id/2);

  HANDLE_ERROR(cudaMemcpyAsync(result__,D_RESULT[stream_id],sizeof(int32_t)*20,cudaMemcpyDeviceToHost, STREAM[stream_id]));
  cudaStreamSynchronize(STREAM[stream_id]);
  if(result__[0] >= 0){
    for(i = 0;i < 20; i++){
      result[i] = result__[i];
    }
    return 1;
  }
  return 0;
}


int32_t get_n_exec_stream(){
  return N_STREAM;
}

void get_stream_state(stream_state_ptr state){
  int32_t i;
  for(i = 0; i <N_STREAM ; i++){
    if (i % 2 == 0){
      cudaSetDevice(i/2);
    }
    if(cudaStreamQuery(STREAM[i]) == cudaErrorNotReady){
      state->stream_state_array[i] = STREAM_STATE_BUSY;
    }else{
      state->stream_state_array[i] = STREAM_STATE_READY;
      //printf("ready %d  %d\n", i, state->stream_state_array[i]);
    }
    //printf("aa %d  %d\n", i, state->stream_state_array[i]);
  }
}

void sync_all_stream(){
  int32_t i;
  //printf("sync_all\n");
  for(i = 0; i <N_STREAM ; i++){
    if (i % 2 == 0){
      cudaSetDevice(i/2);
    }
    cudaStreamSynchronize(STREAM[i]);
  }
}

void init_device_table(
  uint16_t * search_node_level_mov_tab_corner_8_ori,
  uint16_t * coordinate_level_mov_tab_corner_8_pos,
  uint16_t * search_node_level_mov_tab_edge_12_flip,
  uint16_t * search_node_level_mov_tab_edge_4_pos,
  uint16_t * corner_ori_sym_tab,
  uint16_t * edge_flip_sym_tab,
  uint8_t * distance_table,
  uint8_t * corner_distance_table,
  uint8_t * mov_trs_lr,
  uint8_t * mov_trs_fb
){
  static const uint8_t invalid_move[7] = { 0x21, 0x0A, 0x14, 0x08, 0x10, 0x20, 0x00 };
  int32_t i;


  for(i = 0;i < N_STREAM ; i++){
    if (i % 2 == 0){
      cudaSetDevice(i/2);
    }
    HANDLE_ERROR(cudaMalloc((void**)&D_MOVE_HIST[i],sizeof(move_hist)*((PHASE_2_SEARCH_CHUNK_SIZE_GPU+BLOCK_DIM-1)/BLOCK_DIM*BLOCK_DIM)));
    HANDLE_ERROR(cudaMalloc((void**)&D_SEARCH_NODE[i],sizeof(search_node_cube)));
    HANDLE_ERROR(cudaMalloc((void**)&D_PHASE2_ENTRY[i],sizeof(int)));
    HANDLE_ERROR(cudaMalloc((void**)&D_RESULT[i],sizeof(int32_t)*(20)));
  }

  for(i = 0;i < N_STREAM/2 ; i++){
    cudaSetDevice(i);

    HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[i],sizeof(uint16_t)*(N_CORNER_ORI*N_MOVES)));
    HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_POSITION[i]   ,sizeof(uint16_t)*(N_CORNER_POS*N_MOVES)));
    HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[i]        ,sizeof(uint16_t)*(N_EDGE_FLIP*N_MOVES)));
    HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[i]     ,sizeof(uint16_t)*(N_EDGE_4_POS*N_MOVES)));
    HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[i],sizeof(uint16_t)*(N_CORNER_ORI*N_SYM)));
    //HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_EDGE_FLIP_SYM_to_SYM0         ,sizeof(uint16_t)*(N_EDGE_FLIP*N_SYM)));
    HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_DISTANCE_TABLE[i]              ,sizeof(uint8_t)*(DISTANCE_TABLE_SIZE)));
    HANDLE_ERROR(cudaMalloc((void**)&CUDA_SOLVER_CORNER_DISTANCE_TABLE[i]              ,sizeof(uint8_t)*(CORNER_DISTANCE_TABLE_SIZE)));

    HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_ORIENTATION[i], search_node_level_mov_tab_corner_8_ori,sizeof(uint16_t)*N_CORNER_ORI*N_MOVES,cudaMemcpyHostToDevice));
    HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_CORNER_8_POSITION[i]   , coordinate_level_mov_tab_corner_8_pos,sizeof(uint16_t)*N_CORNER_POS*N_MOVES,cudaMemcpyHostToDevice));
    HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_12_FLIP[i]        , search_node_level_mov_tab_edge_12_flip,sizeof(uint16_t)*N_EDGE_FLIP *N_MOVES,cudaMemcpyHostToDevice));
    HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_SEARCH_NODE_LEVEL_MOV_TAB_EDGE_4_POSITION[i]     , search_node_level_mov_tab_edge_4_pos  ,sizeof(uint16_t)*N_EDGE_4_POS*N_MOVES,cudaMemcpyHostToDevice));

    HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_CORNER_ORIENTATION_SYM_to_SYM0[i], corner_ori_sym_tab  ,sizeof(uint16_t)*N_CORNER_ORI*N_SYM,cudaMemcpyHostToDevice));
    //HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_EDGE_FLIP_SYM_to_SYM0         , edge_flip_sym_tab   ,sizeof(uint16_t)*N_EDGE_FLIP *N_SYM,cudaMemcpyHostToDevice));


    HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_DISTANCE_TABLE[i], distance_table   ,sizeof(uint8_t)*DISTANCE_TABLE_SIZE,cudaMemcpyHostToDevice));
    HANDLE_ERROR(cudaMemcpy(CUDA_SOLVER_CORNER_DISTANCE_TABLE[i], corner_distance_table   ,sizeof(uint8_t)*CORNER_DISTANCE_TABLE_SIZE,cudaMemcpyHostToDevice));

    HANDLE_ERROR(cudaMemcpyToSymbol(CUDA_SOLVER_MOV_TRS_TAB_LR,mov_trs_lr, sizeof(uint8_t) *N_MOVES));
    HANDLE_ERROR(cudaMemcpyToSymbol(CUDA_SOLVER_MOV_TRS_TAB_FB,mov_trs_fb, sizeof(uint8_t) *N_MOVES));
    HANDLE_ERROR(cudaMemcpyToSymbol(D_INVALID_MOVE ,invalid_move, sizeof(uint8_t) *7));
    HANDLE_ERROR(cudaMemcpyToSymbol(CUDA_SOLVER_EDGE_FLIP_SYM_to_SYM0 , edge_flip_sym_tab,sizeof(uint16_t)*N_EDGE_FLIP *N_SYM));

  //cudaFuncSetCacheConfig(search_tree_phase2_cuda_karnel,cudaFuncCachePreferL1);

  }

  for(i = 0;i < N_STREAM ; i++){
    if (i % 2 == 0){
      cudaSetDevice(i/2);
    }
    HANDLE_ERROR(cudaStreamCreate(&STREAM[i]));
  }
}

#endif
