CC = gcc
OBJS = nsh.o

shell:$(OBJS)
	$(CC) $^ -o nsh

%.o:%.c
	$(CC) -c $^

run:
	./nsh
	
submit:
	curl -F "token=ESFD_asftB4PfgsXM3qqfw" -F "lab_num=1" -F "file=@nsh.c" http://114.212.81.7:9999/upload_code 
	
score:
	curl "http://114.212.81.7:9999/download?token=ESFD_asftB4PfgsXM3qqfw&lab_num=1"
	
report:
	curl -F "token=ESFD_asftB4PfgsXM3qqfw" -F "lab_num=1" -F "file=@nsh.pdf" http://114.212.81.7:9999/upload_report
