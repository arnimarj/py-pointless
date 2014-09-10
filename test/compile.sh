INCLUDE="-I../include -I../judy-1.0.5/src/"
#FLAGS="-pedantic -Wall -std=c99 -D_REENTRANT -D_GNU_SOURCE    -O2 -g -pg -fno-omit-frame-pointer -DNDEBUG -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls"
FLAGS="-Wall -D_REENTRANT -D_GNU_SOURCE  -g  -O2 -DNDEBUG"
SOURCE="../src/*.c ./c_api/*.c  ../judy-1.0.5/src/obj/.libs/libJudy.a"
LDFLAGS="-lpthread -ldl"
#FLAGS="-pg -g -fno-omit-frame-pointer -O2 -DNDEBUG -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls -fno-inline"
#-liconv"

g++ -std=c++11 $FLAGS $INCLUDE $SOURCE $LDFLAGS
