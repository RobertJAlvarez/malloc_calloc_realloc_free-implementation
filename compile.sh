make clean

gcc -fPIC -Wall -g -O0 -c memory.c 
gcc -fPIC -Wall -g -O0 -c alloc.c
gcc -fPIC -shared -o memory.so memory.o alloc.o -lpthread

#To try the code out:
export LD_LIBRARY_PATH=`pwd`:"$LD_LIBRARY_PATH" #Tells bash that when you are looking for a library, also look into the current one
export LD_PRELOAD=`pwd`/memory.so #When you load an executable, first preload my memory.so and then only load the rest
export MEMORY_DEBUG=no #Tells our code that we do want to see debug messages
