#ifndef __DISTANCE_FUNCTIONS_H__
#define __DISTANCE_FUNCTIONS_H__

#include"cube_structures.h"


typedef struct __distance_tables__{
  uint8_t *dist_tab;
  uint8_t *corner_dist_tab;
  uint16_t *c_ori_sym;
  uint16_t *e_flip_sym;
}distance_tables, *distance_tables_ptr;



void init_distance_table();
int8_t distance(search_node_cube_ptr node);
void get_distance_tables(distance_tables_ptr dst);


#endif
