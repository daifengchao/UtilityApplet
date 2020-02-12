EXEC = serial_tool

OBJS = serial_tool.o\
	   serial.o

HEADERS = serial.h

REALTIME_LIBS = -lpthread -lrt
#STATIC_COMPILER= --static

all:${EXEC}
${EXEC} : ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${REALTIME_LIBS} ${STATIC_COMPILER} 
${OBJS2} : ${HEADERS}

.PHONY : clean
clean :
	-rm -f ${OBJS} ${EXEC}
