#PROGRAMS
CXX = g++
CC = gcc
BIN = backuper
RM = rm -f

#DIRECTORIES
OBJDIR = obj
SRCDIR = src
INCDIR = src
BINDIR = bin

#PARSE SOURCES FILES
#use notdir to remove directory (substitution : deux methodes)
SRC  = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*.cpp)
OBJ  = $(filter %.o,$(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o))
OBJ += $(filter %.o,$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC)))

#FLAGS
LDFLAGS =
#CFLAGS = -Wall -O0 -Wextra $(patsubst %,-I%, $(INCDIR)) -pipe -Wshadow -g
#flto : link-time optimization
CFLAGS = -Wall -Wextra $(patsubst %,-I%, $(INCDIR)) -pipe -O3 -flto
#-msse2 -mfpmath=see : active les extention float pour AMD et PIII et superieur (ne compile pas sur android)
#CFLAGS+= -march=native -mtune=native -mfpmath=sse -msse -msse2 -msse3 -mmmx -m3dnow
CFLAGS+= -fdiagnostics-color -fno-diagnostics-show-caret

#VPATH
#use VPATH variable to tell to makefile to search in different path
#VPATH = here somewhere ...

#.PHONY: des regles et non des noms de fichiers
.PHONY: clean all mrproper
.SILENT: $(q)

#COMPILATIONS RULES
all: .depend $(BINDIR)/$(BIN)

$(BINDIR)/$(BIN): $(OBJ)
		@echo LK $@
		$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
		@echo CXX $@
		$(CXX) -o $@ -c $< $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
		@echo CC $@
		$(CC) -o $@ -c $< $(CFLAGS)

clean:
		$(RM) $(OBJ)

mrproper: clean
		$(RM) $(BINDIR)/$(BIN)
		$(RM) .depend

#install: $(BIN)
#		cp $(BINDIR)/$(BIN) /usr/bin/

#construction du fichier des dependences (automatique)
.depend:
	$(CC) -MM -I$(INCDIR) $(SRC) $(CFLAGS) > $@
	sed -i 's/\(.*\.o:\)/$(OBJDIR)\/\1/g' $@

#inclusion des dependances
-include .depend
