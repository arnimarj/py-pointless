INCLUDE="-I../include"
#FLAGS="-pedantic -Wall -std=c99 -D_REENTRANT -D_GNU_SOURCE    -O2 -g -pg -fno-omit-frame-pointer -DNDEBUG -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls"
FLAGS="-pedantic -Wall -std=c99 -D_REENTRANT -D_GNU_SOURCE    -O2 -DNDEBUG"
SOURCE="../src/*.c ./c_api/*.c"
LDFLAGS="-lpthread -ldl -lJudy"
#-liconv"

gcc $FLAGS $INCLUDE $SOURCE $LDFLAGS
