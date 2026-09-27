CXX = g++
CXXFLAGS = -Wall -Werror -pedantic -std=c++17

wish: wish.cpp
	$(CXX) $(CXXFLAGS) wish.cpp -o wish

clean:
	rm -f wish *.o
