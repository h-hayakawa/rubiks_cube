#ifndef __SOLVER_STRUCT_H__
#define __SOLVER_STRUCT_H__
#include<stdint.h>
#include"cube_structures.h"


typedef struct __move_hist__{
  uint32_t move_hist_lo;
  uint32_t move_hist_hi;
} move_hist, *move_hist_ptr;


typedef struct __phase2_queue__{
  coord_cube cube;
  move_hist_ptr move_hist;
  uint32_t head;
  uint32_t tail;
  int32_t entry;
  int32_t max_entry;
}phase2_queue, *phase2_queue_ptr;

typedef struct __phase2_chunk__{
  coord_cube cube;
  search_node_cube node_cube;
  move_hist_ptr move_hist;
  int32_t entry;
}phase2_chunk, *phase2_chunk_ptr;

typedef struct __search_node__{
  search_node_cube cube;
  int32_t mov;
  int32_t remain_depth;
}search_node, *search_node_ptr;

#define P_THREAD

#if defined(P_THREAD)
/* PHASE_2_CUBE_QUEUE_SIZEのサイズがでかすぎるとmallocが死ぬ */
#define PHASE_2_CUBE_QUEUE_SIZE        (0x30000000)
//#define PHASE_2_CUBE_QUEUE_SIZE        (0x06000000)
#define PHASE_1_SEARCH_CHUNK_SIZE      (0x00020000)
#define PHASE_2_SEARCH_CHUNK_SIZE_CPU  (0x00010000)
#define PHASE_2_SEARCH_CHUNK_SIZE_GPU  (0x00080000)
#define PHASE_2_SEARCH_CHUNK_SIZE_HOST (0x00000800)
#define PHASE2_SEARCH_DEPTH 11
#define BLOCK_DIM (128)
#endif

#define TWO_PHASE_SEARCH_SWITCH_DEPTH 13


#define DISTANCE_TABLE_SIZE 3493601280u
#define CORNER_DISTANCE_TABLE_SIZE 44089920u
#define INITIAL_CORNER_POSITION 0


#define N_CORNER_POS 40320
#define N_CORNER_ORI 2187
#define N_EDGE_4_POS (1560*8)
#define N_EDGE_FLIP 2048
#define N_SYM 8


#define STREAM_STATE_READY 0
#define STREAM_STATE_BUSY 1


#endif
