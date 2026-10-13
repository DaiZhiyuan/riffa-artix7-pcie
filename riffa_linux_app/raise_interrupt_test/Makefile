TARGET = raise_interrupt_test
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean distclean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ -lriffa $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)

distclean: clean
