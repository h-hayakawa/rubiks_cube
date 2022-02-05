#ifndef __SOLVER_SETTINGS_H__
#define __SOLVER_SETTINGS_H__

// CPU threads
// GPUに十分な負荷がかからない場合はここをCPUの物理コア数を設定する
// CPU単体実行時は論理スレッド数を指定した方が、物理コア数を設定したときより僅かに速い
#define N_THREADS 16

// 使うGPUの数
#define N_GPU 2

// nVidia GPUが無いとき or CPUだけのベンチが欲しいときに使う
// nvccがインストールされていない場合は gcc *.c -fopenmp -Ofast
#define CPU_ONLY 0

//　ベンチマーク測定などで、ランダムシードを統一したい場合はこのdefineを有効にする
// このdefineがない場合は現在時刻をseedとする
#define FIXED_RANDOM_SEED 0

// 処理状況非表示（パフォーマンスチューニングしたいときとかに使う）
#define NO_STATUS_PRINT 1


////////////////////////////////////////////////////////////////////////////////
#if (CPU_ONLY)
#undef N_GPU
#define N_GPU 0
#endif
#if (N_GPU == 0)
#undef CPU_ONLY
#define CPU_ONLY 1
#endif
#endif
