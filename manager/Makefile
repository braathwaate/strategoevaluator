#Makefile for Stratego

#Use this to build with graphics
LIBRARIES = -lSDL -lGL -lpthread
#Use this to build without graphics
#LIBRARIES = -lpthread
#Use this on Mac OSX (Thanks spartan)
#LIBRARIES = -I/opt/local/include/ -L/opt/local/lib/ -lSDL -framework OpenGL -lpthread
CPP = g++ -Wall -pedantic -g
OBJ = main.o controller.o network_controller.o ai_controller.o human_controller.o program.o network.o thread_util.o stratego.o graphics.o game.o

BIN = stratego



$(BIN) : $(OBJ) 
	$(CPP) -o $(BIN) $(OBJ) $(LIBRARIES)




%.o : %.cpp %.h
	$(CPP) -c $<

clean :
	$(RM) $(BIN) $(OBJ) $(LINKOBJ)

clean_full: #cleans up all backup files
	$(RM) $(BIN) $(OBJ) $(LINKOBJ)
	$(RM) *.*~
	$(RM) *~


