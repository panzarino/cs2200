
/*
 * student.c
 * Multithreaded OS Simulation for CS 2200
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;

static int type;
static int timeslice;
static unsigned int count_cpu;

static pthread_mutex_t ready_mutex;
static pthread_cond_t ready_cond;
static pcb_t *queue_head;

static void queue_add(pcb_t *process) {
    pthread_mutex_lock(&ready_mutex);
    process->next = NULL;
    if (queue_head == NULL) {
        queue_head = process;
    } else {
        pcb_t *current_node = queue_head;
        while (current_node->next != NULL) {
            current_node = current_node->next;
        }
        current_node->next = process;
    }
    pthread_cond_broadcast(&ready_cond);
    pthread_mutex_unlock(&ready_mutex);
}

static pcb_t* queue_get() {
    pthread_mutex_lock(&ready_mutex);
    if (queue_head == NULL) {
        pthread_mutex_unlock(&ready_mutex);
        return NULL;
    }
    pcb_t *process = queue_head;
    queue_head = queue_head->next;
    pthread_mutex_unlock(&ready_mutex);
    return process;
}

static pcb_t* queue_get_priority() {
    pthread_mutex_lock(&ready_mutex);
    if (queue_head == NULL) {
        pthread_mutex_unlock(&ready_mutex);
        return NULL;
    }
    pcb_t *pre_best = NULL;
    unsigned int best_priority = queue_head->priority;
    pcb_t *process = queue_head;
    while (process->next) {
        if (process->next->priority < best_priority) {
            pre_best = process;
            best_priority = process->next->priority;
        }
        process = process->next;
    }
    if (pre_best) {
        process = pre_best->next;
        pre_best->next = pre_best->next->next;
    } else {
        process = queue_head;
        queue_head = queue_head->next;
    }
    pthread_mutex_unlock(&ready_mutex);
    return process;
}

/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Set the currently running process using the current array
 *
 *   4. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t *process;
    if (type == 3) {
        process = queue_get_priority();
    } else {
        process = queue_get();
    }
    if (process != NULL) {
        process->state = PROCESS_RUNNING;
    }
    current[cpu_id] = process;
    pthread_mutex_unlock(&current_mutex);
    context_switch(cpu_id, process, timeslice);
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    pthread_mutex_lock(&ready_mutex);
    while (queue_head == NULL) {
        pthread_cond_wait(&ready_cond, &ready_mutex);
    }
    pthread_mutex_unlock(&ready_mutex);
    schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 *
 * Remember to set the status of the process to the proper value.
 */
extern void preempt(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_READY;
    queue_add(current[cpu_id]);
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is Priority, wake_up() may need
 *      to preempt the CPU with the lowest priority to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for
 * 	its prototype and the parameters it takes in.
 *
 *  NOTE: A high priority corresponds to a low number.
 *  i.e. 0 is the highest possible priority.
 */
extern void wake_up(pcb_t *process)
{
    process->state = PROCESS_READY;
    queue_add(process);
    if (type == 3) {
        pthread_mutex_lock(&current_mutex);
        unsigned int lowest = 0;
        unsigned int index;
        int found;
        for (unsigned int x; x < count_cpu; x++) {
            if (!current[x]) {
                found = 1;
                break;
            }
            if (current[x]->priority > lowest) {
                lowest = current[x]->priority;
                index = x;
            }
        }
        pthread_mutex_unlock(&current_mutex);
        if (!found && lowest > process->priority) {
            force_preempt(index);
        }
    }
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
    unsigned int cpu_count;

    /* Parse the command line arguments */
    cpu_count = strtoul(argv[1], NULL, 0);

    count_cpu = cpu_count;
    type = 1;
    timeslice = -1;

    if (argc > 2) {
        if (strcmp(argv[2], "-r") == 0 && argc > 3) {
            type = 2;
            timeslice = strtol(argv[3], NULL, 0);
        } else if (strcmp(argv[2], "-p") == 0) {
            type = 3;
        } else {
            fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
                            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
                            "    Default : FIFO Scheduler\n"
                            "         -r : Round-Robin Scheduler\n"
                            "         -p : Priority Scheduler\n\n");
            return -1;
        }
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


#pragma GCC diagnostic pop
