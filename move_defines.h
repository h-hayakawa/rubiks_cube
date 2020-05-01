#ifndef __MOVE_DEFINES_H__
#define __MOVE_DEFINES_H__

/* キューブ操作に適当に名前付け */

/* ここだけ見るとdefineをコメントアウトすればそれに応じたソルバができるように見えるが，実際はダメ． */


#define MOVE_U1 __MOVE_U1__
#define MOVE_U2 __MOVE_U2__
#define MOVE_U3 __MOVE_U3__
#define MOVE_R1 __MOVE_R1__
#define MOVE_R2 __MOVE_R2__
#define MOVE_R3 __MOVE_R3__
#define MOVE_B1 __MOVE_B1__
#define MOVE_B2 __MOVE_B2__
#define MOVE_B3 __MOVE_B3__
#define MOVE_L1 __MOVE_L1__
#define MOVE_L2 __MOVE_L2__
#define MOVE_L3 __MOVE_L3__
#define MOVE_F1 __MOVE_F1__
#define MOVE_F2 __MOVE_F2__
#define MOVE_F3 __MOVE_F3__
#define MOVE_D1 __MOVE_D1__
#define MOVE_D2 __MOVE_D2__
#define MOVE_D3 __MOVE_D3__


/* 上で定義した操作に対して番号付け */

typedef enum{
#if defined(MOVE_U1)
  __MOVE_U1__,
#endif
#if defined(MOVE_U2)
  __MOVE_U2__,
#endif
#if defined(MOVE_U3)
  __MOVE_U3__,
#endif
#if defined(MOVE_R1)
  __MOVE_R1__,
#endif
#if defined(MOVE_R2)
  __MOVE_R2__,
#endif
#if defined(MOVE_R3)
  __MOVE_R3__,
#endif
#if defined(MOVE_B1)
  __MOVE_B1__,
#endif
#if defined(MOVE_B2)
  __MOVE_B2__,
#endif
#if defined(MOVE_B3)
  __MOVE_B3__,
#endif
#if defined(MOVE_L1)
  __MOVE_L1__,
#endif
#if defined(MOVE_L2)
  __MOVE_L2__,
#endif
#if defined(MOVE_L3)
  __MOVE_L3__,
#endif
#if defined(MOVE_F1)
  __MOVE_F1__,
#endif
#if defined(MOVE_F2)
  __MOVE_F2__,
#endif
#if defined(MOVE_F3)
  __MOVE_F3__,
#endif
#if defined(MOVE_D1)
  __MOVE_D1__,
#endif
#if defined(MOVE_D2)
  __MOVE_D2__,
#endif
#if defined(MOVE_D3)
  __MOVE_D3__,
#endif
  N_MOVES
}move_ids;


#endif