
all: main.out

base.out: $^
	g++ --std=c++11 -g -c  inifile.cpp -o inifile.o
	g++ --std=c++11 -g -c HandleData.cpp -o HandleData.o


main.out : base.out DbfRead.cpp
	g++ --std=c++11 -g -c DbfRead.cpp -o DbfRead.o
	g++ --std=c++11 -g  inifile.o DbfRead.o HandleData.o -o main.out

clean:
	rm -rf *.o *.out
