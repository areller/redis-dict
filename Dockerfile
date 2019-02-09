FROM redis:5

RUN apt-get update -y &&   \
    apt-get install gcc -y --fix-missing

WORKDIR /opt/redis-dict
ADD . .

RUN gcc -fPIC -std=gnu99 -c src/*.c
RUN ld -o rdict.so *.o -shared -Bsymbolic -lc

WORKDIR /data

CMD ["redis-server","--appendonly", "yes","--loadmodule", "/opt/redis-dict/rdict.so"]