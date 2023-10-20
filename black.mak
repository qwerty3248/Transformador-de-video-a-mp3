# define C compiler
CC = g++

# define flags for the C compiler
CFLAGS = -g -std=c++11

# define header file directory
INC_DIR = /usr/local/include

# define library file directory
LIB_DIR = /usr/local/lib

# define target
TARGET = main

# define source file
SRC = main.cpp

# define object file
OBJ = $(SRC:.cpp=.o)

# define libraries
LIBS = -lavcodec -lavformat -lavutil

# define include path
INC = -I$(INC_DIR)

# define library path
LIB = -L$(LIB_DIR)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LIB) $(INC) -o $(TARGET) $(OBJ) $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $(LIB) $(INC) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
