CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lsfml-graphics -lsfml-window -lsfml-system -lGLEW -framework OpenGL -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreVideo

all: lab14

lab14: main.o
	$(CXX) main.o -o lab14 $(LDFLAGS)

main.o: main.cpp Utils.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

run: lab14
	./lab14

clean:
	rm -f *.o lab14

.PHONY: all clean run