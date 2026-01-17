#source file search paths
VPATH = ./src 

	# compiler include search paths
# INCS = -I./include \
# 	-I../windex/include \
# 	-I../raven-set \
# 	-I../boost/boost1_78 

LIBS = \
	-lgdiplus -lgdi32 -lcomdlg32 -lstdc++fs -lComctl32 \


static_link_flags = -static-libstdc++ -static-libgcc -static

# folders
ODIR = ./.vscode/obj

OPT = -O2
CCflags = -std=c++17 $(OPT) -D_mingw_

# sources
OBJ_test = \
	$(ODIR)/test.o  \
	$(ODIR)/Particle.o  \


### COMPILE

$(ODIR)/%.o: %.cpp  
	g++  $(CCflags) \
	-c -o $@ $< $(INCS)

test: $(OBJ_test)
	g++ $(OPT) \
	-o bin/test.exe $^  \
	$(static_link_flags) $(LIBS)

.PHONY: clean
clean:
	del /q .vscode\\obj\\*.o

