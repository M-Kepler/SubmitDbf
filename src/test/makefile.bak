
all: mainmmapmulti.out mainmmaponce.out mainnommap.out

base.out: $^
	g++ --std=c++11 -g -c  inifile.cpp -o inifile.o
	g++ --std=c++11 -g -c HandleData.cpp -o HandleData.o

mainnommap.out : base.out DbfReadNoMmap.cpp
	g++ --std=c++11 -g -c DbfReadNoMmap.cpp -o DbfReadNoMmap.o
	g++ --std=c++11 -g  inifile.o DbfReadNoMmap.o HandleData.o -o mainnommap.out

mainmmapmulti.out : base.out DbfReadMmapMulti.cpp
	g++ --std=c++11 -g -c DbfReadMmapMulti.cpp -o DbfReadMmapMulti.o
	g++ --std=c++11 -g  inifile.o DbfReadMmapMulti.o HandleData.o -o mainmmapmulti.out

mainmmaponce.out : base.out DbfReadMmapOnce.cpp
	g++ --std=c++11 -g -c DbfReadMmapOnce.cpp -o DbfReadMmapOnce.o
	g++ --std=c++11 -g  inifile.o DbfReadMmapOnce.o HandleData.o -o mainmmaponce.out
clean:
	rm -rf *.o *.out