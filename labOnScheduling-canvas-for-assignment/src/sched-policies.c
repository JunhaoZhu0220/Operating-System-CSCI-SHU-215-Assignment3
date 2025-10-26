#include <os-scheduling.h>

/*******************************
 * QUEUE MANAGEMENT PRIMITIVES *
 *******************************/

void printQueues(task tasks[], sched_data *schedData)
{
    int i, j, taskIndex = 0;
    printf("Nb of queues %d\n", schedData->nbOfQueues);
    for (i = 0; i < schedData->nbOfQueues; i++)
    {
        j = 0;
        printf("Q%d => ", i);
        while (j < MAX_NB_OF_TASKS)
        {
            taskIndex = schedData->queues[i][j];
            if (taskIndex == -1)
            {
                j = MAX_NB_OF_TASKS;
            }
            else
            {
                printf("%s ", tasks[taskIndex].name);
                j++;
            }
        }
        printf("\n");
    }
}

void initQueues(int nbQ, sched_data *schedData)
{
    int i, j;
    printf("Initializing %d job queue(s)\n", nbQ);
    schedData->nbOfQueues = nbQ;
    for (j = 0; j < nbQ; j++)
    {
        for (i = 0; i < MAX_NB_OF_TASKS; i++)
        {
            schedData->queues[j][i] = -1;
        }
    }
}

int enqueue(sched_data *schedData, int queueIndex, int taskIndex)
{
    int end = 0;
    while ((end < MAX_NB_OF_TASKS) && (schedData->queues[queueIndex][end] != -1))
        end++;
    if (end < MAX_NB_OF_TASKS)
    {
        schedData->queues[queueIndex][end] = taskIndex;
        return 0;
    }
    else
    {
        // memory exhausted
        return -1;
    }
}

int dequeue(sched_data *schedData, int queueIndex)
{
    int j;
    int taskIndex = schedData->queues[queueIndex][0];
    if (taskIndex != -1)
    {
        for (j = 0; j < MAX_NB_OF_TASKS - 1; j++)
        {
            schedData->queues[queueIndex][j] = schedData->queues[queueIndex][j + 1];
        }
        schedData->queues[queueIndex][MAX_NB_OF_TASKS - 1] = -1;
    }
    return taskIndex;
}

int dequeue_with_idx(sched_data *schedData, int queueIndex, int taskIndex)
{
    // Find and remove the task with the given taskIndex from the queue
    for (int queuePos = 0; queuePos < MAX_NB_OF_TASKS; queuePos++)
    {
        if (schedData->queues[queueIndex][queuePos] == taskIndex)
        {
            // Remove from current position by shifting all elements after it
            for (int k = queuePos; k < MAX_NB_OF_TASKS - 1; k++)
            {
                schedData->queues[queueIndex][k] = schedData->queues[queueIndex][k + 1];
            }
            schedData->queues[queueIndex][MAX_NB_OF_TASKS - 1] = -1;
            return taskIndex;
        }
    }
    return -1; // Task not found in queue
}

int head(sched_data *schedData, int queueIndex)
{
    return schedData->queues[queueIndex][0];
}

/*******************************
 * TASKS MANAGEMENT PRIMITIVES *
 *******************************/

int admitNewTasks(task tasks[], int nbOfTasks, sched_data *schedData, int currentTime)
{
    int i, j;
    j = 0;
    while (schedData->queues[0][j] != -1)
        j++;
    for (i = 0; i < nbOfTasks; i++)
    {
        if ((tasks[i].state == UPCOMING) && (tasks[i].arrivalDate == currentTime))
        {
            tasks[i].state = READY;
            schedData->queues[0][j] = i;
            j++;
        }
    }
    return 1;
}

/***********************
 * SCHEDULING POLICIES *
 ***********************/

int FCFS(task tasks[], int nbOfTasks, sched_data *schedData, int currentTime)
{

    int i;

    // Initialize single queue
    if (currentTime == 0)
    {
        printf("Initializing job queue\n");
        initQueues(1, schedData);
    }

    admitNewTasks(tasks, nbOfTasks, schedData, currentTime);
    printQueues(tasks, schedData);

    // Is the first task in the queue running? Has that task finished its computations?
    //   If so, put it in terminated state and remove it from the queue
    //   If not, continue this task
    i = head(schedData, 0);
    if (i != -1)
    {
        if (tasks[i].state == RUNNING)
        {
            if (tasks[i].executionTime == tasks[i].computationTime)
            {
                tasks[i].state = TERMINATED;
                tasks[i].completionDate = currentTime;
                dequeue(schedData, 0);
            }
            else
            {
                /* Reelect this task */
                tasks[i].executionTime++;
                return i;
            }
        }
    }

    // Otherwise, elect the first task in the queue
    i = head(schedData, 0);
    if (i != -1)
    {
        tasks[i].executionTime++;
        tasks[i].state = RUNNING;
        return i;
    }

    // No task could be elected
    return -1;
}

int SJF(task tasks[], int nbOfTasks, sched_data *schedData, int currentTime)
{
    int i, j, minCompTime;

    // N.B: SJF does not require any queue!

    // Admit new tasks (currentTime >= arrivalTime)
    for (i = 0; i < nbOfTasks; i++)
    {
        if ((tasks[i].state == UPCOMING) && (tasks[i].arrivalDate == currentTime))
        {
            tasks[i].state = READY;
        }
    }

    // Is there a task running? Has that task finished its computations?
    //   If so, put it in terminated state
    //   If not, continue this task
    for (i = 0; i < nbOfTasks; i++)
    {
        if (tasks[i].state == RUNNING)
        {
            if (tasks[i].executionTime == tasks[i].computationTime)
            {
                tasks[i].state = TERMINATED;
                tasks[i].completionDate = currentTime;
                break;
            }
            else
            {
                /* Reelect this task */
                tasks[i].executionTime++;
                return i;
            }
        }
    }

    // Otherwise, find the task in READY state that has the shortest computation time
    j = -1;
    minCompTime = 0;
    for (i = 0; i < nbOfTasks; i++)
    {
        if (tasks[i].state == READY)
        {
            if ((j == -1) || (minCompTime > tasks[i].computationTime))
            {
                j = i;
                minCompTime = tasks[i].computationTime;
            }
        }
    }
    if (j != -1)
    {
        tasks[j].executionTime++;
        tasks[j].state = RUNNING;
        return j;
    }

    return -1;
}

int SRTF(task tasks[], int nbOfTasks, sched_data *schedData, int currentTime)
{
    int i, j, remainingCompTime, minRemainingCompTime;

    // N.B: SRTF does not require any queue!

    // Admit new tasks (currentTime >= arrivalTime)
    for (i = 0; i < nbOfTasks; i++)
    {
        if ((tasks[i].state == UPCOMING) && (tasks[i].arrivalDate == currentTime))
        {
            tasks[i].state = READY;
        }
    }

    // Is there a task running?
    //      => determine its remaining computation time
    //   Has that task finished its computations?
    //      => put it in terminated state
    //   else
    //      => put it back to READY
    for (i = 0; i < nbOfTasks; i++)
    {
        if (tasks[i].state == RUNNING)
        {
            remainingCompTime = tasks[i].computationTime - tasks[i].executionTime;
            if (remainingCompTime == 0)
            {
                tasks[i].state = TERMINATED;
                tasks[i].completionDate = currentTime;
            }
            else
            {
                tasks[i].state = READY;
            }
            break;
        }
    }

    // Now elect the task in READY state that has the shortest remaining time
    j = -1;
    minRemainingCompTime = -1;
    for (i = 0; i < nbOfTasks; i++)
    {
        if (tasks[i].state == READY)
        {
            remainingCompTime = tasks[i].computationTime - tasks[i].executionTime;
            if ((j == -1) ||
                (minRemainingCompTime > remainingCompTime))
            {
                j = i;
                minRemainingCompTime = remainingCompTime;
            }
        }
    }
    if (j != -1)
    {
        tasks[j].executionTime++;
        tasks[j].state = RUNNING;
        return j;
    }

    return -1;
}

int RR(task tasks[], int nbOfTasks, sched_data *schedData, int currentTime)
{
    int i;

    // Initialize single queue
    if (currentTime == 0)
    {
        printf("RR> Initializing job queue\n");
        printf("RR> Quantum duration is %d cycles\n", schedData->quantum);
        initQueues(1, schedData);
    }

    admitNewTasks(tasks, nbOfTasks, schedData, currentTime);

    i = head(schedData, 0);
    // If a task is running, check its status
    if (i != -1 && tasks[i].state == RUNNING)
    {
        // Case 1: The task has finished its computation, terminate it
        if (tasks[i].executionTime == tasks[i].computationTime)
        {
            tasks[i].state = TERMINATED;
            tasks[i].completionDate = currentTime;
            dequeue(schedData, 0);
        }
        // Case 2: The task has used up its time quantum, put it back to READY and enqueue it at the end of the queue
        else if (tasks[i].executionTime > 0 && (tasks[i].executionTime % schedData->quantum == 0))
        {
            tasks[i].state = READY;
            dequeue(schedData, 0);
            enqueue(schedData, 0, i);
        }
    }

    printQueues(tasks, schedData);

    // Elect the task at the head of the queue to run
    i = head(schedData, 0);
    if (i != -1)
    {
        tasks[i].executionTime++;
        tasks[i].state = RUNNING;
        return i;
    }

    // No task could be elected
    return -1;
}

int MFQ(task tasks[], int nbOfTasks, sched_data *schedData, int currentTime)
{
    int i;
    int quantum = schedData->quantum;

    // Initialize 3 queues for 3 priority levels
    if (currentTime == 0)
    {
        printf("MFQ> Initializing job queue\n");
        printf("MFQ> Quantum duration is %d cycles\n", schedData->quantum);
        initQueues(3, schedData);
    }

    admitNewTasks(tasks, nbOfTasks, schedData, currentTime);

    // Check if there's a running task at level 1 and handle it
    i = head(schedData, 0);
    if (i != -1 && tasks[i].state == RUNNING)
    {
        // Case 1: The task has finished its computation, terminate it
        if (tasks[i].executionTime == tasks[i].computationTime)
        {
            tasks[i].state = TERMINATED;
            tasks[i].completionDate = currentTime;
            dequeue(schedData, 0);
            tasks[i].cyclesInQuantum = 0;
        }
        // Case 2: The task has used up its quantum (1 * quantum), move to level 2
        else if (tasks[i].cyclesInQuantum == 1 * quantum)
        {
            tasks[i].state = READY;
            dequeue(schedData, 0);
            enqueue(schedData, 1, i);
            tasks[i].cyclesInQuantum = 0;
        }
        // Case 3: continue running the task
        else
        {
            tasks[i].cyclesInQuantum++;
            tasks[i].executionTime++;
            printQueues(tasks, schedData);
            return i;
        }
    }

    // Check level 2
    i = head(schedData, 1);
    if (i != -1 && tasks[i].state == RUNNING)
    {
        // Case 1: The task has finished its computation, terminate it
        if (tasks[i].executionTime == tasks[i].computationTime)
        {
            tasks[i].state = TERMINATED;
            tasks[i].completionDate = currentTime;
            dequeue(schedData, 1);
            tasks[i].cyclesInQuantum = 0;
        }
        // Case 2: The task has used up its quantum (2 * quantum), move to level 3
        else if (tasks[i].cyclesInQuantum >= 2 * quantum)
        {
            tasks[i].state = READY;
            dequeue(schedData, 1);
            enqueue(schedData, 2, i);
            tasks[i].cyclesInQuantum = 0;
        }
        // Case 3: continue running the task
        else
        {
            tasks[i].cyclesInQuantum++;
            tasks[i].executionTime++;
            printQueues(tasks, schedData);
            return i;
        }
    }

    // Check level 3
    i = head(schedData, 2);
    if (i != -1 && tasks[i].state == RUNNING)
    {
        // Case 1: The task has finished its computation, terminate it
        if (tasks[i].executionTime == tasks[i].computationTime)
        {
            tasks[i].state = TERMINATED;
            tasks[i].completionDate = currentTime;
            dequeue(schedData, 2);
            tasks[i].cyclesInQuantum = 0;
        }
        // Case 2: The task has used up its quantum (3 * quantum), move back to level 1
        else if (tasks[i].cyclesInQuantum >= 3 * quantum)
        {
            tasks[i].state = READY;
            dequeue(schedData, 2);
            enqueue(schedData, 0, i);
            tasks[i].cyclesInQuantum = 0;
        }
        // Case 3: continue running the task
        else
        {
            tasks[i].cyclesInQuantum++;
            tasks[i].executionTime++;
            printQueues(tasks, schedData);
            return i;
        }
    }

    printQueues(tasks, schedData);

    // Elect the first task in the highest priority non-empty queue
    i = head(schedData, 0);
    if (i != -1)
    {
        tasks[i].state = RUNNING;
        tasks[i].cyclesInQuantum = 1;
        tasks[i].executionTime++;
        return i;
    }

    i = head(schedData, 1);
    if (i != -1)
    {
        tasks[i].state = RUNNING;
        tasks[i].cyclesInQuantum = 1;
        tasks[i].executionTime++;
        return i;
    }

    i = head(schedData, 2);
    if (i != -1)
    {
        tasks[i].state = RUNNING;
        tasks[i].cyclesInQuantum = 1;
        tasks[i].executionTime++;
        return i;
    }

    // No task could be elected
    return -1;
}

int IORR(task tasks[], int nbOfTasks, sched_data *schedData, int currentTime)
{
    int i, j;

    // Initialize single queue
    if (currentTime == 0)
    {
        printf("IORR> Initializing job queue\n");
        printf("IORR> Quantum duration is %d cycles\n", schedData->quantum);
        initQueues(1, schedData);
    }

    admitNewTasks(tasks, nbOfTasks, schedData, currentTime);

    // Check all suspended tasks and wake them up if IO is complete
    for (j = 0; j < nbOfTasks; j++)
    {
        if (tasks[j].state == SLEEPING)
        {
            tasks[j].cyclesInIO++;
            // Check if IO completed
            if (tasks[j].cyclesInIO == tasks[j].ioDuration)
            {
                tasks[j].cyclesInIO = 0;
                // Check if task has completed all computation, terminate it
                if (tasks[j].executionTime == tasks[j].computationTime)
                {
                    tasks[j].state = TERMINATED;
                    tasks[j].completionDate = currentTime;
                    // Remove terminated task from queue
                    dequeue_with_idx(schedData, 0, j);
                }
                else
                {
                    // Task still has work to do, move back to READY
                    tasks[j].state = READY;
                }
            }
        }
    }

    // Traverse through the queue to check if there's a running task and handle it
    i = -1;
    for (int queuePos = 0; queuePos < MAX_NB_OF_TASKS; queuePos++)
    {
        int taskIndex = schedData->queues[0][queuePos];
        if (taskIndex == -1)
        {
            break;
        }
        if (tasks[taskIndex].state == RUNNING)
        {
            i = taskIndex;
            break;
        }
    }

    if (i != -1)
    {
        tasks[i].cyclesInQuantum++;

        // Case 1: The task needs to perform IO, put it to SLEEPING and move to queue end
        if (tasks[i].ioInterval > 0 && (tasks[i].executionTime % tasks[i].ioInterval == 0))
        {
            tasks[i].state = SLEEPING;
            tasks[i].cyclesInIO = 0;
            tasks[i].cyclesInQuantum = 0;
            // Remove task from current position and add to end
            dequeue_with_idx(schedData, 0, i);
            enqueue(schedData, 0, i);
        }

        // Case 2: The task has finished its computation, terminate it
        else if (tasks[i].executionTime == tasks[i].computationTime)
        {
            tasks[i].state = TERMINATED;
            tasks[i].completionDate = currentTime;
            // Remove terminated task from queue
            dequeue_with_idx(schedData, 0, i);
        }

        // Case 3: The task has used up its time quantum, put it back to READY and move it to end
        else if (tasks[i].cyclesInQuantum == schedData->quantum)
        {
            tasks[i].state = READY;
            // Remove task from current position and add to end
            dequeue_with_idx(schedData, 0, i);
            enqueue(schedData, 0, i);
        }

        // Case 4: Continue running
        else
        {
            tasks[i].executionTime++;
            printQueues(tasks, schedData);
            return i;
        }
    }

    printQueues(tasks, schedData);

    // Elect the first READY task in the queue to run
    for (int queuePos = 0; queuePos < MAX_NB_OF_TASKS; queuePos++)
    {
        int taskIndex = schedData->queues[0][queuePos];
        if (taskIndex == -1)
        {
            break;
        }
        if (tasks[taskIndex].state == READY)
        {
            tasks[taskIndex].state = RUNNING;
            tasks[taskIndex].cyclesInQuantum = 0;
            tasks[taskIndex].executionTime++;
            return taskIndex;
        }
    }

    // No task could be elected
    return -1;
}
