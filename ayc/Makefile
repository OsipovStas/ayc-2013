CC=icpc 
OPTIMIZATION=-O0
LIB= -I/opt/intel/composer_xe_2013_sp1.0.080/mkl/include -L/opt/intel/composer_xe_2013_sp1.0.080/mkl/lib/intel64 -Wl,-R/opt/intel/mkl/lib/intel64 -lmkl-blas95-lp64 -libmkl_core  -libmkl_intel_lp64 -libmkl_intel_thread -libmkl_sequential -lm 
CFLAGS=-ansi -std=c++11  -pedantic -Iincludes $(OPTIMIZATION) -vec-report=2
CFLAGS_DEBUG=  -mkl -W -g -Wall -Werror -ansi -std=c++11  -pedantic -Iincludes -O0 -vec-report=2 -debug all 
CFLAGS_MIC=$(CFLAGS) -mmic
LDFLAGS=-openmp -g -vec-report=2 -lX11
LDFLAGS_DEBUG= -mkl -g -O0 -vec-report=2  -debug all -lX11
LDFLAGS_MIC=-openmp -g $(OPTIMIZATION) -vec-report=2 -mmic
EXEC=run
SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:src/%.cpp=obj/%.o)
OBJ_MIC=$(SRC:src/%.cpp=obj/%.omic)
OBJ_DEBUG=$(SRC:src/%.cpp=obj/%.odeb)

TEAM_ID = 0429b0a6d58790b77176754e72b0674f


all:$(EXEC)

mic:$(OBJ_MIC)
	$(CC) -o $@ $^ $(LDFLAGS_MIC)

default:all

debug:$(OBJ_DEBUG)
	$(CC) -o $@ $^ $(LDFLAGS_DEBUG)


run:$(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

obj/%.o:src/%.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

obj/%.omic:src/%.cpp
	$(CC) -o $@ -c $< $(CFLAGS_MIC)

obj/%.odeb:src/%.cpp
	$(CC) -o $@ -c $< $(CFLAGS_DEBUG)

clean:
	rm -f obj/*.o* $(EXEC) *.zip debug

zip: clean
ifdef TEAM_ID
	zip $(strip $(TEAM_ID)).zip -9r Makefile src/ obj/ includes/
else
	@echo "you need to put your TEAM_ID in the Makefile"
endif

submit: zip
ifdef TEAM_ID
	curl -F "file=@$(strip $(TEAM_ID)).zip" -L http://www.intel-software-academic-program.com/contests/ayc/upload/upload.php
else
	@echo "you need to put your TEAM_ID in the Makefile"
endif


