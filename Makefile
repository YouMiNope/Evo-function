CC			:= g++
FLAGS		:= -std=c++11 -O3
LIB_FLAGS	:= -fPIC -shared



main: src/main.cpp targetfunc.o GeneLibrary
	${CC} -o $@ $< targetfunc.o -L. -lGeneLibrary ${FLAGS}

GeneLibrary: src/GeneLibrary.cpp
	${CC} -o libGeneLibrary.so $^ ${LIB_FLAGS} ${FLAGS}

view: src/Geneview.cpp GeneLibrary
	${CC} -o $@ $< -L. -lGeneLibrary -lcurses ${FLAGS}

test: src/test.cpp
	${CC} -o $@ $< targetfunc.o -L./ -lGeneLibrary ${FLAGS}

tool: src/seqtool.cpp analysis.o
	${CC} -o $@ $^ ${FLAGS}

targetfunc.o: src/targetfunc.cpp
	${CC} -o $@ -c $^ ${FLAGS}

analysis.o: src/analysis.cpp
	${CC} -o $@ -c $^ ${FLAGS}

run: main
	rm -rf ./save/*
	./main ./targetfunc.o

clean:
	rm -rf main test view libGeneLibrary.so

clr:
	rm -rf ./save/*