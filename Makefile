INC = -I /opt/local/include/
LIB = -L /opt/local/lib

all: container_size

container_size: container_size.cpp
	g++ -std=c++11  $(INC) $(LIB) container_size.cpp -ljsoncpp 
	./a.out
	rm a.out	

