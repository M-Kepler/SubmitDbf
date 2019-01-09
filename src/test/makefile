
all: mainmulti.out

mainmulti.out: $^
	g++ --std=c++11 -g -c  inifile.cpp -o inifile.o
	g++ --std=c++11 -g -c DbfReadMmapMulti.cpp -o DbfReadMulti.o
	g++ --std=c++11 -g  inifile.o DbfReadMulti.o HandleData.cpp -o mainmulti.out

clean:
	rm -rf *.o *.out