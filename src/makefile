.PHONY: gensql

PLATFORM = OS_IS_LINUX
DATABASE = DATABASE_IS_ORACLE
COMPAT_STRING= "DATABASE_IS_ORACLE"
# ORACLE_HOME = /pasc/cts/kcbp/oracle/product/11.2.0
# DBENGINE_LIBS = -ldbengine -ldbengine_oraoci

ORACLE_FLAGS = def_sqlcode=yes release_cursor=yes sqlcheck=syntax ireclen=256 parse=none code=cpp cpp_suffix=C DBMS=V8 unsafe_null=yes char_map=string
ORACLE_LIBS = -L$(ORACLE_HOME)/lib -lclntsh
CLS_INC = -I$(ORACLE_HOME)/precomp/public -I$(ORACLE_HOME)/rdbms/public -I$(ORACLE_HOME)/plsql/public -DPRECOMP -DKGDB_ORACLE
DB_LIBS = $(ORACLE_LIBS)

I_INCLUDE = $(CLS_INC) -I/usr/local/include  -I../common
LDFLAGS = -L./
LIBS = -lCommondao


L_INCLUDE = $(DB_LIBS) $(DBENGINE_LIBS) -L/usr/lib -L/usr/local/lib -L.

BITSFLAGS = -q64 -qstaticinline
DEBUG_FLAG = _DEBUG
DEBUG_OPS = -g

CC = g++
LFLAGS = -shared $(L_INCLUDE)
LINK = $(CC)

BITSFLAGS = -m64
CFLAGS = $(BITSFLAGS) -w -O2 -fPIC -D$(PLATFORM) -D$(DATABASE) -D$(DEBUG_FLAG) $(I_INCLUDE) $(DEBUG_OPS)

TARGET = ./main.out



# ==============================================================================================================================

gensql: $^
	@echo "===---===--===\nto general sql\n===---===---===\n"
	g++ --std=c++11 -g -c inifile.cpp -o inifile.o
	g++ --std=c++11 -g -c DbfRead.cpp -o DbfRead.o
	g++ --std=c++11 -g  inifile.o DbfRead.o GeneSql.cpp -o gene_sql.out


# ==============================================================================================================================

SQX_ := $(wildcard *.sqx)
SQX_C := $(patsubst %.sqx, %.C, $(SQX_))
SQX_O := $(patsubst %.C, %.o, $(SQX_C))
SQX_SO := $(patsubst %.o, %.so, $(SQX_O))
SQX_BND := $(patsubst %.sqx, %.bnd, $(SQX_))

%.C : FILE_NO ?= $(word 1, $(shell sed -n '/$*.sqx/p' pkg_name.list))
      CUR_DIR ?= $(subst /, ,$(shell pwd))
      SUB_DIR ?= $(word $(shell expr $(words $(CUR_DIR)) - 1),$(CUR_DIR))
      BND_NAME ?= $(subst dao_,,$*)
      PKG_NAME ?= $(SUB_DIR)$(FILE_NO)

%.C : %.sqx
	./DBSwitch.lnx $*.sqx to $(DATABASE)
	proc $(ORACLE_FLAGS) iname=$*.sqx

.PRECIOUS %.o : %.C
	$(CC) -c $(CFLAGS) $< -o $@

%.so : %.o
	$(LINK) $(LFLAGS) $(CLS_LIBS) $^ -o $@ 
	chmod 775 $@

all: $(TARGET)

libCommondao.so : $(SQX_O)
	$(LINK) $(LFLAGS) $(CLS_LIBS) $^ -o $@
	chmod 775 $@
	cp $@ lib/

%.o:%.cpp
	$(LINK) $(LFLAGS) $(CFLAGS) $^ -o $@

SORUCES= $(wildcard  *.c  *.cpp)
OBJS=$(patsubst %.c,%.o, $(patsubst %.cpp,%.o,$(SORUCES)))


$(TARGET):$(OBJS)
	$(CC) $(OBJS) $(I_INCLUDE) $(LDFLAGS) $(LIBS) $(DEBUG_OPS) -o $(TARGET)

clean:
	rm -f *.bnd *.lst *.C *.o *.lis *.out

