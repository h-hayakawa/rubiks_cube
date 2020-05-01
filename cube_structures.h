#ifndef __CUBE_STRUCTURES__
#define __CUBE_STRUCTURES__
#include<stdint.h>


/*
ルービックキューブには移動可能に見えるシールが6x8個存在(各面中央は数えない)．
ここでは，48箇所のシール位置にどの色が格納されているかでキューブを表現する．
ユーザインタフェースとの連携には便利．
*/

typedef struct _facelet_level_structure__{
  /* キューブの表面情報．face[1] == 0で，1番の場所に0番の色があることを意味する */
  int8_t face[48];
}facelet_cube;


/*============================================================================================================================*/

/*
機械的な観点からのルービックキューブ構造
ルービックキューブは8個のコーナーキューブと12個のエッジキューブから成る
1つのコーナーキューブは8箇所のどこかに位置し，適当な基準に対して捻じれの関係が3通りある
1つのエッジキューブは12箇所のどこかに位置し，適当な基準に対して反転の関係が2通りある
*/

typedef int8_t corner_position_val_type;
typedef int8_t corner_orientation_val_type;
typedef int8_t edge_position_val_type;
typedef int8_t edge_flip_val_type;

typedef struct __cubie_structure__{
  /* コーナーキューブの位置．corner_position[1] == 0の場合は1番の位置に0番のコーナーキューブが存在するという意味 */
  corner_position_val_type corner_position[8];
  /* コーナーキューブの捻じれ．corner_position[1] == 2の場合は1番のコーナーキューブが基準から2捻じれているという意味 */
  corner_orientation_val_type corner_orientation[8];
  /* エッジキューブの位置 */
  edge_position_val_type edge_position[12];
  /* エッジキューブの反転 */
  edge_flip_val_type edge_flip[12];
}cubie_cube,*cubie_cube_ptr;

/*============================================================================================================================*/


/*
キューブの状態を最小限のデータ量で実現する構造
cubie_structureの各配列の出現パターンを適当な整数値に置き換えて保持
コーナーキューブの位置は8!=40320通り
コーナーキューブの捻じれは3^7 = 2187通り
エッジキューブの反転は2^11=2048通り
エッジキューブの位置は12!通りと多いのでUDスライス，LRスライス，FBスライスに分けて保持．
各スライスには4つのキューブが属すので，12x11x10x9=11880通り
*/

typedef struct __coordinate_level_structure__{
  uint16_t corner_position;
  uint16_t corner_orientation;
  uint16_t edge_flip;
  uint16_t edge_position_ud;
  uint16_t edge_position_lr;
  uint16_t edge_position_fb;
}coord_cube, *coord_cube_ptr;


/*============================================================================================================================*/

/* 探索ノードで使用されるキューブ
実装レベルと似ているが，探索ノードレベルでは元のキューブを復元できない
(ルービックキューブ全体を表現する情報を持っていない)．
探索に使用する距離テーブルを構成するための部分問題に関連するデータのみ保持する
*/

typedef struct __search_node_level_structure__{
  uint16_t corner_position;
  uint16_t ud_corner_orientation;
  uint16_t ud_edge_ud;
  uint16_t ud_edge_flip;
  uint16_t lr_corner_orientation;
  uint16_t lr_edge_ud;
  uint16_t lr_edge_flip;
  uint16_t fb_corner_orientation;
  uint16_t fb_edge_ud;
  uint16_t fb_edge_flip;
}search_node_cube, *search_node_cube_ptr;


#endif
