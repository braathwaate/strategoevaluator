
OBJ = main.o ai.o db.o
BIN = ramen

CFLAGS = -Wall -Wextra -g -std=gnu99
LINKFLAGS = -g -lm

DEPFILES = $(OBJ:%=%.dep)

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LINKFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)
	$(CPP) $(CPPFLAGS) $< -MM -o $@.dep

-include $(DEPFILES)

