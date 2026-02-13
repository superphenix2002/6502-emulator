vm: 6502.o
	gcc 6502.o -o 6502

knn_new.o: 6502.c
	gcc -c 6502.c -o 6502.o -fpermissive -std C99

clean:
	rm *.o output