#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

//This method counts to 1 Billion
void count_to_two_billion() {
    for (int i = 0; i < 2000000000; i++) {
        if(i % 200000000 == 0){
            printf("i is %d from process %d\n", i, getpid());
        }
    }
}

int main() {
    int mainpid = getpid();
    for(int i = 0; i<10; i++){
        int x = fork();
        if(x == 0){
            printf("This is the process with pid %d counting to two billion\n", getpid());
            count_to_two_billion();
            break;
        }
    }
    if(mainpid == getpid()){
        int status;
        for (int i = 0; i < 10; ++i) {
            int childpid = wait(&status);
            printf("Child process %d finished execution!\n", childpid);
        }
    }

    return 0;
}
