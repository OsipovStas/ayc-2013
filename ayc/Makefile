CC=icpc
OPTIMIZATION=-O3
LIB=
CFLAGS=-tbb -mkl -Wall -Werror  -std=c++11  -pedantic -Iincludes $(OPTIMIZATION)
CFLAGS_DEBUG= -pg -mkl -tbb -g -Wall -Werror -std=c++11 -pedantic -Iincludes -O0 -debug all 
CFLAGS_MIC=$(CFLAGS) -mmic -mkl -tbb
LDFLAGS=-mkl -tbb $(OPTIMIZATION)
LDFLAGS_DEBUG= -pg -mkl -tbb -g -O0 -debug all 
LDFLAGS_MIC=$(OPTIMIZATION) -mkl -tbb -mmic
EXEC=run
SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:src/%.cpp=obj/%.o)
OBJ_MIC=$(SRC:src/%.cpp=obj/%.omic)
OBJ_DEBUG=$(SRC:src/%.cpp=obj/%.odeb)

TEAM_ID = 2c45ca54c555ad3c6a546db04a159064


all:$(EXEC)

mic:$(OBJ_MIC)
	$(CC) $(LDFLAGS_MIC) -o $@ $^ 

default:all

debug:$(OBJ_DEBUG)
	$(CC) $(LDFLAGS_DEBUG) -o $@ $^ 


run:$(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ 

obj/%.o:src/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $< 

obj/%.omic:src/%.cpp
	$(CC) $(CFLAGS_MIC) -o $@ -c $< 

obj/%.odeb:src/%.cpp
	$(CC) $(CFLAGS_DEBUG) -o $@ -c $< 

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


