CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lsqlite3 -lpthread -lm

SRCS = main.c server.c database.c authentication.c user.c freelancer.c search.c projects.c hiring.c commission.c
OBJS = $(SRCS:.c=.o)
TARGET = cutcraft_server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
