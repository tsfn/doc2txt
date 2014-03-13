VPATH=.:src
cc=g++
cflags=-g -Wall -fmax-errors=1 
objects=main.o Storage.o parse_doc.o doc_text.o doc_image.o

doc2txt: $(objects)
	$(cc) $(objects) -o doc2txt $(cflags)

$(objects): %.o: %.cpp
	$(cc) $< -o $@ $(cflags) -c

.PHONY: clean
clean:
	rm -f *.o doc2txt *.txt
