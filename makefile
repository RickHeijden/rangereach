CXX      = g++
CXXFLAGS = -O3 -mavx -march=native -std=c++17 -w
LDFLAGS  =

SOURCES = containers/graph.cpp containers/labeling.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: creates qpoints qmbrs
creates: create_scc create_dag create_bfl create_int
qpoints: spareach_int spareach_bfl socreach 3dreach 3dreach_rev 2dreach 2dreach_comp 2dreach_pointer  
qmbrs: spareach_int_mbr
	
create_scc: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) creates/create_scc.cpp -o create_scc.exec $(LDADD)

create_dag: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o creates/create_dag.cpp -o create_dag.exec $(LDADD)

create_bfl: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o creates/create_bfl_input.cpp -o create_bfl_input.exec $(LDADD)

create_int: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o containers/labeling.o creates/create_int.cpp -o create_int.exec $(LDADD)


spareach_int: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o containers/labeling.o methods/main_spareach-int.cpp -o run_spareach-int.exec $(LDADD)

spareach_int_mbr: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o containers/labeling.o methods/main_spareach-int_MBR.cpp -o run_spareach-int_MBR.exec $(LDADD)

spareach_bfl: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o containers/labeling.o methods/main_spareach-bfl.cpp -o run_spareach-bfl.exec $(LDADD)

socreach: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o containers/labeling.o methods/main_socreach.cpp -o run_socreach.exec $(LDADD)

3dreach: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o containers/labeling.o methods/main_3dreach.cpp -o run_3dreach.exec $(LDADD)

3dreach_rev: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o containers/labeling.o methods/main_3dreach_rev.cpp -o run_3dreach_rev.exec $(LDADD)

2dreach: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o methods/main_2dreach.cpp -o run_2dreach.exec $(LDADD)

2dreach_comp: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o methods/main_2dreach_comp.cpp -o run_2dreach_comp.exec $(LDADD)

2dreach_pointer: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) containers/graph.o methods/main_2dreach_pointer.cpp -o run_2dreach_pointer.exec $(LDADD)


.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf containers/*.o
	rm -rf create_*.exec
	rm -rf run_*.exec
