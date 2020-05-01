#ifndef __COMMON_DEFINES_H__
#define __COMMON_DEFINES_H__

#define __GCC__



/* いくつかの関数で順序関係のあるものが正しく実行されてるか確認するためのもの．実行速度に影響するので実験時はオフに． */
//#define DEBUG_PRINT

/* GUIとかとの連携時に文字出力が邪魔な場合に有効にする． */
//#define SILENT_MODE


/* configファイルか何か作った方が便利? */
/* 距離テーブルのパスorファイル名．生成に3時間ほど要するのでファイルにしておく */
#define DISTANCE_TABLE_FILE_PATH "distance_table.tab"
#define CORNER_DISTANCE_TABLE_FILE_PATH "corner_distance_table.tab"


#endif
