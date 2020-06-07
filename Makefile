# NFLAG = -D__DEVICE_EMULATION__  -deviceemu

CC = g++
NVCC = nvcc
RM = /bin/rm
PROG = a.out

C_OBJS = coordinate_level_move.o cubie_level_move.o distance_functions.o facelet_level_move.o main.o
C_OBJS += move_table_creator.o search_node_level_move.o solver.o structure_converter.o

CU_OBJS = cuda_solver.o

OBJS = ${C_OBJS} ${CU_OBJS}

CFLAGS = -Ofast -fopenmp -mavx2
NVCC_FLAGS = -O3
LDFLAGS = ${CFLAGS}

M_ARCH = $(shell uname -m)

# for Device Code
CUDA_PATH = /usr/local/cuda
HELPER_PATH = /usr/local/cuda/samples/common/inc
ifeq ($(M_ARCH), x86_64)
LDFLAGS += -L${CUDA_PATH}/lib64
LDFLAGS += -lcudart
else
LDFLAGS += -L${CUDA_PATH}/lib
endif
#DFLAGS += -L${CUDA_SDK_PATH}/lib
#DFLAGS += -lcudart -lcutil_i386
NFLAG += ${NVCC_FLAGS}
NFLAG += -I${CUDA_PATH}/include
NFLAG += -I${HELPER_PATH}/
NFLAG += -arch sm_75
#NFLAG += -maxrregcount 32

all : ${PROG}

${PROG} : ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}
	${RM} -f ${OBJS} *.o core

%.o : %.c
	${CC} -c ${CFLAGS} $<

%.o : %.cu
	${NVCC} -c ${NFLAG} --ptxas-options=-v  $<

clean :
	${RM} -f ${PROG} ${OBJS} *.o core


#cube_gpu.o : main.cu
