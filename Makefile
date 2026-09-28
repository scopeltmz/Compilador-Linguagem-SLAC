# COMPLAC - Compilador da linguagem SLAC²
# Autores: Thomaz de Souza Scopel (RA 10417183)
#          Matteo Porcare (RA 10417286)
#
# Makefile - Compila os fontes de src/ (cabeçalhos em include/), gera os
#            objetos em build/ e o binário 'complac' na raiz do projeto.
#   make             compila
#   make clean       remove build/, o binário e os logs gerados em testes/

CC        = gcc
CFLAGS    = -Wall -Wextra -std=c99 -Iinclude
TARGET    = complac
SRC_DIR   = src
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

ifeq ($(OS),Windows_NT)
    MKDIR    = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM_BUILD = if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
    RM_FILES = del /Q /F $(TARGET).exe testes\*.tk testes\*.ts testes\*.trc 2>nul
else
    MKDIR    = mkdir -p $(BUILD_DIR)
    RM_BUILD = rm -rf $(BUILD_DIR)
    RM_FILES = rm -f $(TARGET) testes/*.tk testes/*.ts testes/*.trc
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# -MMD gera as dependências de cabeçalhos (.d) automaticamente
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	$(MKDIR)

clean:
	-$(RM_BUILD)
	-$(RM_FILES)

-include $(OBJS:.o=.d)

.PHONY: all clean
