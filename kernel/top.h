#include "param.h"

#ifndef XV6_RISCV_TOP_H
#define XV6_RISCV_TOP_H

struct proc_info {
    char name[16];
    int pid;
    int ppid;
    int time;
    int cpu_usage;
    char state[10];
};

struct top_system_struct {
    long uptime;
    int ticktick;
    int total_process;
    int running_process;
    int sleeping_process;
    struct proc_info p_list[NPROC];
};



//extern volatile int ctrl_c_pressed;

#endif //XV6_RISCV_TOP_H
