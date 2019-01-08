
all: mainmulti.out mainonce.out

mainmulti.out: $^
	g++ -g -c inifile.cpp -o inifile.o
	g++ -g -c DbfReadMmapMulti.cpp -o DbfReadMulti.o
	g++ -g inifile.o DbfReadMulti.o HandleData.cpp -o mainmulti.out

mainonce.out: $^
	g++ -g -c inifile.cpp -o inifile.o
	g++ -g -c DbfReadMmapOnce.cpp -o DbfReadOnce.o
	g++ -g inifile.o DbfReadOnce.o HandleData.cpp -o mainonce.out

clean:
	rm -rf *.o *.out