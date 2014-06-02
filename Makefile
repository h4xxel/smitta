DEBUG	=	-g -O0

CFLAGS	+=	-Wall -std=c99 $(DEBUG)
LDFLAGS	+=	

RM	=	rm -f

OBJ	=	smitta.o
BIN	=	smitta

all: $(OBJ)
	$(CC) $(CFLAGS) -o "$(BIN)" $(OBJ) $(LDFLAGS)

clean:
	$(RM) $(OBJ) $(BIN)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o "$@" "$<"
