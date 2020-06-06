#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<omp.h>
#include<time.h>
#include"cube_structures.h"
#include"coordinate_level_move.h"
#include"solver.h"

static const char MOV_STR[18][4] = {
  "U \0", "U2\0", "U'\0",
  "R \0", "R2\0", "R'\0",
  "B \0", "B2\0", "B'\0",
  "L \0", "L2\0", "L'\0",
  "F \0", "F2\0", "F'\0",
  "D \0", "D2\0", "D'\0",
};

coord_cube random_cube_coord(int32_t shuffle_len, int8_t *shuffle){
  int32_t i;
  coord_cube coord;
  int32_t prev_mv = N_MOVES;
  static const uint8_t invalid_move[7] = { 0x21, 0x0A, 0x14, 0x08, 0x10, 0x20, 0x00 };
  init_coordinate_level_cube(&coord);
  
  for (i = 0; i < shuffle_len; i++){
    int32_t mv = rand() % N_MOVES;
    if ((invalid_move[prev_mv / 3] >> (mv / 3)) & 1){
      i--;
      continue;
    }
    shuffle[i] = mv;
    coordinate_level_move(&coord, mv);
    prev_mv = mv;
  }
  return coord;
}

static
void print_move(int8_t result[20], int8_t solution_len){
  int32_t i;
  for (i = 0; i < solution_len; i++){
    printf("%s ", MOV_STR[result[i]]);
  }
  printf("\n");
}

coord_cube super_flip(int32_t shuffle_len,int8_t* shuffle_log){
  coord_cube ret;
  int32_t i = 0;
  int32_t tw;
  int32_t sf[]={MOVE_U1,MOVE_R2,MOVE_F1,MOVE_B1,MOVE_R1,MOVE_B2,MOVE_R1,MOVE_U2,MOVE_L1,MOVE_B2,MOVE_R1,MOVE_U3,MOVE_D3,
    MOVE_R2,MOVE_F1,MOVE_R3,MOVE_L1,MOVE_B2,MOVE_U2,MOVE_F2};
    
    init_coordinate_level_cube(&ret);
    
    for(i=0;i<shuffle_len;i++){
      tw = sf[i];
      shuffle_log[i] = tw;
      coordinate_level_move(&ret, tw);
    }
    return ret;
  }
  
  int32_t verify_solution(int8_t *shuffle, int32_t shuffle_len, int8_t *sol, int32_t sol_len){
    coord_cube initial;
    coord_cube cube;
    int32_t i;
    
    init_coordinate_level_cube(&initial);
    init_coordinate_level_cube(&cube);
    
    for(i = 0; i < shuffle_len ; i ++){
      coordinate_level_move(&cube, shuffle[i]);
    }
    for(i = 0; i < sol_len ; i ++){
      coordinate_level_move(&cube, sol[i]);
    }
    if(initial.corner_position != cube.corner_position){
      return 0;
    }
    if(initial.corner_orientation != cube.corner_orientation){
      return 0;
    }
    if(initial.edge_flip != cube.edge_flip){
      return 0;
    }
    if(initial.edge_position_ud != cube.edge_position_ud){
      return 0;
    }
    if(initial.edge_position_lr != cube.edge_position_lr){
      return 0;
    }
    if(initial.edge_position_fb != cube.edge_position_fb){
      return 0;
    }
    return 1;
  }
  
  
  int main(){
    coord_cube coord;
    int32_t shuffle_len;
    int8_t result[21];
    int8_t shuffle[100];
    double time_sum[21] = {0};
    double time;
    int32_t solution_len_distribution[21] = {0};
    double t1, t2;
    int32_t ill = 0;
    int32_t i;
    int32_t count = 0;
    init_solver();/* 最初に呼ぶ．*/
    srand((unsigned int) omp_get_wtime());
    for (shuffle_len = 100; shuffle_len <= 100; shuffle_len++){
      for (i = 0; i < 1000; i++){
        int32_t solution_len,  ii;
        coord = random_cube_coord(shuffle_len, shuffle);
        //coord = super_flip(shuffle_len, shuffle);
        //print_move(shuffle, shuffle_len);/* シャッフル手順を印字 */
        t1 = omp_get_wtime();
        solution_len = solve(&coord, result);
        t2 = omp_get_wtime();
        if (solution_len != -1){
          count++;
          printf("solved(%d %d)\n",count, i);
          print_move(result, solution_len);/* 解を印字 */
          if(!verify_solution(shuffle, shuffle_len,result, solution_len)){
            printf("illegal solution \n");
            ill ++;
            //return 1;
          }
        }
        if (solution_len > shuffle_len){
          printf("illegal solution\n");
          return 1;
        }
        time = t2 - t1;
        time_sum[solution_len] += time;
        solution_len_distribution[solution_len]++;
        printf("solution_len = %d, time = %f\n", solution_len, time);/* 解の長さと探索時間を印字 */
        
        printf("solution_len , count , avg_time   ill = %d\n",ill);
        for (ii = 0; ii < 21;ii++){
          printf("%2d , %5d , %f\n", ii, solution_len_distribution[ii], solution_len_distribution[ii] ? time_sum[ii] / solution_len_distribution[ii] : 0);
        }
        
      }
    }
    /* 解の長さの分布と解の長さ毎の平均処理時間を印字 */
    printf("solution_len , count , avg_time\n");
    for (i = 0; i < 21;i++){
      printf("%2d , %5d , %f\n", i, solution_len_distribution[i], solution_len_distribution[i] ? time_sum[i] / solution_len_distribution[i] : 0);
    }
    return 0;
  }
  
  
  
