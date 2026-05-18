CXX ?= clang
CC ?= clang

CXXFLAGS = -Wall --std=c++17 -O3
CFLAGS = -Wall --std=c11 -O0 #В продашене поставить -O3 (!!!), т.к. 
#эта библиотека компилиркется оч долго, или ПРОСТО ПОДКЛЮЧИТЬ .SO

SRCXX = main.cpp database.cpp
SRCC = sqlite3.c

OUT = prj.out
OBJ = $(SRCXX:.cpp=.o) $(SRCC:.c=.o)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $<

.c.o:
	$(CC) -c $(CFLAGS) $<

$(OUT): $(OBJ)
	$(CXX) -o $(OUT) $(OBJ)

all: $(OUT)

clean:
	rm -f $(OBJ) $(OUT)

.PHONY: all clean
