
all: main.out

base.out: $^
	g++ --std=c++11 -g -c  inifile.cpp -o inifile.o
	g++ --std=c++11 -g -c HandleData.cpp -o HandleData.o


main.out : base.out DbfReadMmapMulti.cpp
	g++ --std=c++11 -g -c DbfReadMmapMulti.cpp -o DbfReadMmapMulti.o
	g++ --std=c++11 -g  inifile.o DbfReadMmapMulti.o HandleData.o -o main.out

clean:
	rm -rf *.o *.out
