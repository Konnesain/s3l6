all: a.out

a.out: main.cpp
	g++ main.cpp -lpqxx

clean:
	rm -f a.out