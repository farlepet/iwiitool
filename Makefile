MAINDIR	= .
SRC		= $(MAINDIR)/src
INC		= $(MAINDIR)/inc

CSRC	= $(wildcard $(SRC)/*.c)
OBJ		= $(patsubst %.c,%.o,$(CSRC))
DEPS    = $(patsubst %.c,%.d,$(CSRC))
EXEC	= iwiitool

CFLAGS	    = -I$(INC) -Wall -Wextra -Werror -O2 -ggdb2
LDFLAGS		=

ifeq ($(CC), "clang")
  CFLAGS += -Weverything
endif

.PHONY: all, clean

all: $(EXEC) ansi2iwii iwiigfx

$(EXEC): $(OBJ)
	@echo -e "\033[33m  \033[1mLD\033[21m    \033[34m$(EXEC)\033[0m"
	@$(CC) $(OBJ) $(LDFLAGS) -o $(EXEC)

ansi2iwii: $(EXEC)
	@ln -sf $< $@

iwiigfx: $(EXEC)
	@ln -sf $< $@

clean:
	@echo -e "\033[33m  \033[1mCleaning $(EXEC)\033[0m"
	@rm -f $(OBJ) $(EXEC) ansi2iwii iwiigfx

-include $(DEPS)

%.o: %.c
	@echo -e "\033[32m  \033[1mCC\033[21m    \033[34m$<\033[0m"
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

