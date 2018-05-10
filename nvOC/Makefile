
all : freq_benchmark nvapiOC calc_inner_mean

freq_benchmark : freq_benchmark.c
	gcc freq_benchmark.c -o freq_benchmark.exe -lnvml

nvapiOC : nvapiOC.c
	gcc nvapiOC.c -o nvapiOC.exe

calc_inner_mean : calc_inner_mean
	gcc calc_inner_mean.c -o calc_inner_mean.exe

clean :
	rm -f freq_benchmark.exe nvapiOC.exe calc_inner_mean.exe
