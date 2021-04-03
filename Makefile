all:
	g++ -std=c++11 -o a.out col216minor.cpp

.SILENT:
	all
clean:
	rm -f ./a.out