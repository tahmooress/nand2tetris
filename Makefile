CC = gcc
CFLAGS = -Wall -g
OBJDIR = bin
OBJS = $(OBJDIR)/main.o $(OBJDIR)/parser.o $(OBJDIR)/code.o $(OBJDIR)/table.o
DEPS = parser.h code.h table.h
TARGET = $(OBJDIR)/assembler

$(OBJDIR)/%.o: %.c $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
