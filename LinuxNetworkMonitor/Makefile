EXEC = NetworkMonitor
OBJS = NetworkMonitor.o

HEADERS = ""

REALTIME_LIBS = ""
STATIC_COMPILER= --static

all:${EXEC}
${EXEC} : ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${STATIC_COMPILER} 
${OBJS2} : ${HEADERS}

.PHONY : clean
clean :
	-rm -f ${OBJS} ${EXEC}
