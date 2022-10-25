CC			:= g++
FLAGS		:= -std=c++11
LIB_FLAGS	:= -fPIC -shared



GeneLibrary: src/GeneLibrary.cpp
	${CC} -o libGeneLibrary.so $^ ${LIB_FLAGS} ${FLAGS}

view: src/Geneview.cpp GeneLibrary
	${CC} -o $@ $< -L./ -lGeneLibrary -lcurses ${FLAGS}

targetfunc.o: src/targetfunc.cpp
	${CC} -o $@ -c $< ${FLAGS}

test: src/test.cpp GeneLibrary
	${CC} -o $@ $< targetfunc.o -L./ -lGeneLibrary ${FLAGS}

main: src/main.cpp targetfunc.o GeneLibrary
	${CC} -o $@ $< targetfunc.o -L./ -lGeneLibrary ${FLAGS}

run:
	rm -rf ./save
	mkdir ./save
	./main ./targetfunc.o

clean:
	rm -rf main test view libGeneLibrary.so