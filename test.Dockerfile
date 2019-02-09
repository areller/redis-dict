FROM redis:5

RUN apt-get update -y &&   \
    apt-get install gcc -y --fix-missing && \
    apt-get install python -y && \
    apt-get install python-pip -y && \
    apt-get install git -y

RUN pip install -Iv redis==2.10.6
RUN pip install git+https://github.com/RedisLabsModules/RLTest.git@master

WORKDIR /opt/redis-dict
ADD ./src ./src
ADD ./lib ./lib

RUN gcc -fPIC -std=gnu99 -c src/*.c
RUN ld -o rdict.so *.o -shared -Bsymbolic -lc

ADD . .

WORKDIR /opt/redis-dict/tests

CMD ["RLTest", "@test_config.txt"]