CC			:= g++
FLAGS		:= -std=c++11
LIB_FLAGS	:= -fPIC -shared



GeneLibrary: src/GeneLibrary.cpp
	${CC} -o libGeneLibrary.so $^ ${LIB_FLAGS} ${FLAGS}

view: src/Geneview.cpp GeneLibrary
	${CC} -o $@ $< -L. -lGeneLibrary -lcurses ${FLAGS}

test: src/test.cpp GeneLibrary
	${CC} -o $@ $< -L. -lGeneLibrary ${FLAGS}

src/targetfunc.o: src/targetfunc.cpp
	${CC} -o $@ -c $< ${FLAGS}

main: src/main.cpp src/targetfunc.o GeneLibrary
	${CC} -o $@ $< src/targetfunc.o -L. -lGeneLibrary ${FLAGS}