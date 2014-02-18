doc2txt: main.o ole.o
	g++ -g -o doc2txt -Wall -O2 main.o ole.o
	rm -f main.o ole.o

main.o: src/main.cpp src/ole.h
	g++ -g -o main.o -O2 -c src/main.cpp

ole.o: src/ole.cpp src/ole.h
	g++ -g -o ole.o -O2 -c src/ole.cpp
