# Target library
objs :=  fs.o disk.o
lib := libfs.a
CC := gcc
CFLAGS := -Wall -Werror -g
deps := $(patsubst %.o,%.d,$(objs)) 
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

libfs.a: $(objs)
	ar rcs libfs.a $(objs)

%.o: %.c         
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	$(Q)rm -rf fs.o disk.o  $(deps) $(targets)
