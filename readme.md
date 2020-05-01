ルービックキューブをベンチマーク材料にして、
マルチスレッドやらGPUやらに負荷をかけるためのオモチャ。 

ソルバーとしては出回ってるフリーソフトでもっと優秀なのがある。 
 
GPU用はこのcommitをした段階で実験環境が無かったのでCPU用のみ 
 
前提：pthreadが動く環境 
 
windowsならMinGW-w64でposix threadを指定してインストール 
 
solver.hで使うthread数を指定 
 
 
gcc *.c -fopenmp -Ofast -mavx2