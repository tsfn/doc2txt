doc2txt: main.o ole.o
	g++ -std=c++11 -g -o bin/doc2txt -Wall main.o ole.o

main.o: src/main.cpp src/ole.h
	g++ -std=c++11 -g -Wall -o main.o -c src/main.cpp

ole.o: src/ole.cpp src/ole.h
	g++ -std=c++11 -g -Wall -o ole.o -c src/ole.cpp

clean:
	rm -f main.o ole.o
