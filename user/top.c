#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "../kernel/top.h"

    void clear_previous_output(int lines) {
        for (int i = 0; i < lines; i++) {
            printf("\033[F\033[K"); // Move cursor up one line and clear the line
        }
    }

    int main(int argc, char *argv[]) {
        struct top_system_struct *top_info = malloc(sizeof(struct top_system_struct));

        int lines_to_clear = 0;

        while (1) {
            int err = top(top_info);
            if (err != 0) {
                free(top_info);
                return -1;
            }

            if (lines_to_clear > 0) {
                clear_previous_output(lines_to_clear);
            }

            printf("Uptime: %d seconds\n", top_info->uptime);
            printf("running_process: %d \n", top_info->running_process);
            printf("sleeping_process: %d \n", top_info->sleeping_process);
            printf("total_process: %d \n", top_info->total_process);
            printf("process data:\n");
            printf("name \t PID \t PPID \t state \t\ttime \t cpu %%\n");
            for (int i = 0; i < top_info->total_process; ++i) {
                printf("%s \t %d \t %d \t %s \t %d \t %d \n",
                       top_info->p_list[i].name, top_info->p_list[i].pid,
                       top_info->p_list[i].ppid, top_info->p_list[i].state,
                       top_info->p_list[i].time, top_info->p_list[i].cpu_usage);
            }

            // Calculate the number of lines printed
            lines_to_clear = 6 + top_info->total_process; // 6 lines for headers and additional info
            if(top_info->ticktick==1)
                exit(0);
            sleep(10);
        }

    }