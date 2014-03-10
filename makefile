doc2txt: main.o ole.o img.o
	g++ -std=c++11 -g -o bin/doc2txt -Wall main.o ole.o img.o

main.o: src/main.cpp src/ole.hpp
	g++ -std=c++11 -g -Wall -o main.o -c src/main.cpp

ole.o: src/ole.cpp src/ole.hpp
	g++ -std=c++11 -g -Wall -o ole.o -c src/ole.cpp

img.o: src/img.cpp src/ole.hpp
	g++ -std=c++11 -g -Wall -o img.o -c src/img.cpp

clean:
	rm -f *.o
