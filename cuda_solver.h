#ifndef __CUDA_SOLVER_H__
#define __CUDA_SOLVER_H__


#define N_GPU 2

#define N_STREAM (N_GPU * 2)


#define STREAM_STATE_READY 0
#define STREAM_STATE_BUSY 1


typedef struct __stream_state__{
  int32_t stream_state_array[N_STREAM];
}stream_state, *stream_state_ptr;


//int32_t search_tree_phase2_cuda(int8_t *result, phase2_chunk_ptr chunk);

void search_tree_phase2_cuda_async(phase2_chunk_ptr chunk, int32_t stream_id);
int32_t get_search_result(int8_t *result, int32_t stream_id);
void sync_all_stream();
void get_stream_state(stream_state_ptr state);


void init_device_table(
  uint16_t * search_node_level_mov_tab_corner_8_ori,
  uint16_t * search_node_level_mov_tab_corner_8_pos,
  uint16_t * search_node_level_mov_tab_edge_12_flip,
  uint16_t * search_node_level_mov_tab_edge_4_pos,
  uint16_t * corner_ori_sym_tab,
  uint16_t * edge_flip_sym_tab,
  uint8_t * distance_table,
  uint8_t * corner_distance_table,
  uint8_t * mov_trs_lr,
  uint8_t * mov_trs_fb
);

#endif
