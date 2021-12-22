# Libchessutil, a library for chess utilities in C/C++
# Copyright (C) 2021 Morgan Houppin
#
# Libchessutil is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Libchessutil is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

EXE := libchessutil.a

SOURCES := \
	cu_board.c \
	cu_init.c \
	cu_movegen.c

HEADERS := \
	cu_core.h \
	cu_movegen.h

ifeq ($(prefix),)
prefix = /usr/local
endif

ifeq ($(OS),Windows_NT)
	EXE := libchessutil.lib
	_ := $(warning "Windows support is incomplete for now. Errors during compilation or linkage are")
	_ := $(warning "expected, and installation is not available. It is still possible to only use")
	_ := $(warning "the source files and compile them directly with other source code.")
endif

OBJECTS := $(SOURCES:%.c=%.o)
DEPENDS := $(SOURCES:%.c=%.d)

all: $(EXE)

$(EXE): $(OBJECTS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -Wall -Wextra -Wpedantic -Wshadow -Wvla -Werror -O3 -std=gnu11 -I include -MMD $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

-include $(DEPENDS)

clean:
	rm -f $(OBJECTS)
	rm -f $(DEPENDS)

fclean:
	$(MAKE) clean
	rm -f $(EXE)

re:
	$(MAKE) fclean
	+$(MAKE) all

install: all
	install -m 644 -D -t $(prefix)/include $(HEADERS)
	install -m 755 -D -t $(prefix)/lib $(EXE)

uninstall:
	for header in $(HEADERS); do \
		rm -f $(prefix)/include/$$header; \
	done
	rm -f $(prefix)/lib/$(EXE);

.PHONY: all clean fclean re
