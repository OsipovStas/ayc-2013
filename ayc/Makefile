CC=icpc -pg
OPTIMIZATION=-O0
LIB=
CFLAGS=-std=c++11  -pedantic -Iincludes $(OPTIMIZATION) -vec-report=2
CFLAGS_DEBUG=-mkl -tbb -W -g -Wall -Werror -std=c++11 -pedantic -Iincludes -O0 -vec-report=2 -debug all 
CFLAGS_MIC=$(CFLAGS) -mmic
LDFLAGS=-g -vec-report=2 
LDFLAGS_DEBUG= -mkl -ltbb -g -O0 -vec-report=2  -debug all 
LDFLAGS_MIC=-g $(OPTIMIZATION) -vec-report=2 -mmic
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


