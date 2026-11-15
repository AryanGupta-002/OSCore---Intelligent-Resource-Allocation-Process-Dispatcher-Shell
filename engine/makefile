CC = gcc
.DEFAULT_GOAL := all
CFLAGS = -Wall -Wextra -std=c11 -MMD -MP -Iinclude -I$(shell pg_config --includedir)
LDFLAGS = -L$(shell pg_config --libdir)
LDLIBS = -lpq -lm

TARGET = host_engine

SOURCES = main.c \
	common/process.c \
	common/input.c \
	common/metrics.c \
	common/events.c \
	common/execution_trace.c \
	common/db.c \
	host/host.c \
	host/dispatcher.c \
	host/memory.c \
	host/resource_manager.c \
	algorithms/fcfs.c \
	algorithms/sjf.c \
	algorithms/priority.c \
	algorithms/round_robin.c \
	algorithms/srtf.c \
	algorithms/mlfq.c

OBJECTS = $(SOURCES:.c=.o)
DEPS = $(OBJECTS:.o=.d)

-include $(DEPS)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

.PHONY: all clean

DB_NAME = host_db
DB_USER = postgres

db-setup:
	sudo -u $(DB_USER) psql -d $(DB_NAME) -f ../database/schema.sql