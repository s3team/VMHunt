all: mgse vmextract slicer

mgse: parser.o mg-symengine.o
	g++ -std=c++11 -Wall -g main.cpp parser.o mg-symengine.o -o mgse

vmextract: parser.o
	g++ -std=c++11 -Wall -g vmextract.cpp parser.o -o vmextract

slicer: core.o parser.o
	g++ -std=c++11 -Wall -g slicer.cpp core.o parser.o -o slicer

core.o:
	g++ -c -std=c++11 -Wall -g core.cpp

parser.o:
	g++ -c -std=c++11 -Wall -g parser.cpp

mg-symengine.o:
	g++ -c -std=c++11 -Wall -g mg-symengine.cpp

clean:
	rm -f core.o parser.o mg-symengine.o mgse slicer vmextract
