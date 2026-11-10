compile_server: compile_server.cc $(wildcard *.hpp ../comm/*.h ../comm/*.hpp) | temp
	g++ -std=c++11 -pthread $(CPPFLAGS) $(CXXFLAGS) $(shell pkg-config --cflags jsoncpp) $< -o $@ $(LDFLAGS) -ljsoncpp $(LDLIBS)

temp:
	mkdir -p $@

.PHONY: clean
clean:
	rm -f compile_server
