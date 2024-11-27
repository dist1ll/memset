CC = cc
CFLAGS = -O2

TARGET = memset

# Default target
all: $(TARGET)

# Compile and link the C and assembly files directly
$(TARGET): main.c asm.S
	$(CC) $(CFLAGS) -o $(TARGET) main.c asm.S
