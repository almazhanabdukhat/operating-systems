#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>

typedef struct Process
{
    int A;  //process id
    int B;  //CPU time
    int C;  //IO time
    int AT; //arrival time
    int timeReady; //process becomes ready at timeReady
    int blockedTime; //process is blocked for blockedTime
    int timeTerm; //process terminates at timeTerm
    int cpuUsed; //process has used cpuUsed of CPU
    int status; //0 is blocked, 1 is running, 2 is terminated, 3 is ready
} ProcessB; //process object

int sched; //scheduling algorithm: 0,1 or 2
int num_proc; //number of processes
int counter=-1; //time count

void printStatus(ProcessB *processContainer, char *statusStr, int s, int i);
void scheduler(FILE *outputFile, ProcessB *processContainer);
void printResult(ProcessB *processContainer, char *statusStr, int s, int i);
void printOutF(FILE *outputFile, double val, int t, char *finalStr);
void enqueueWait(ProcessB *processContainer, int *waitList, int *readyProcessQueue, int *endQ);
void enqueueBlocked(ProcessB *processContainer, int *blockList, int *readyProcessQueue, int *endQ);
int listEmpty(int list[], int flagCount);
void addReadyQueue(int *readyProcessQueue, int *endQ, int i);
void addReadyQueue(int *readyProcessQueue, int *endQ, int i)
{
    if (sched == 0 || sched == 1)
    {
        readyProcessQueue[(*endQ)] = i;
        *endQ = (*endQ + 1) % num_proc;
    }
    if (sched == 2)
    {
        readyProcessQueue[i] = 1;
    }
}

void scheduler(FILE *outputFile, ProcessB *processContainer)
{
    
    int fcfs = 0;
    int rr = 0;
    int sjf = 0;
    int startQ = 0;
    int endQ = 0;

    int readyProcessQueue[num_proc]; //queue of ready processes
    int waitList[num_proc]; //list that flags waiting processes
    int blockList[num_proc];//list that flags blocking processes
    int terminatedList[num_proc]; //list that flags terminated processes

    char *finalStr = malloc(2200 * sizeof(char));
    char *statusStr = malloc(600 * sizeof(char));
    for (int i = 0; i < num_proc; i++)
    {
        waitList[i] = 1;
        blockList[i]=0;
        terminatedList[i]=0;
        readyProcessQueue[i] = -1;
    }
   
    switch (sched)
    {
    case 0:
        fcfs = 1;
        break;
    case 1:
        rr = 1;
        break;
    case 2:
        sjf = 1;
        break;
    }

    int current_run = -1;
    int timeCPUusageRR = 0; 
    int timeCPUusage = 0;
    int s = -1;

    while (listEmpty(terminatedList, 1) < num_proc)
    {
        counter++;
        s++;
        
        if (!listEmpty(waitList, 0))
            enqueueWait(processContainer, &waitList, &readyProcessQueue, &endQ);

        if (!listEmpty(blockList, 0))
            enqueueBlocked(processContainer, &blockList, &readyProcessQueue, &endQ);
        
        int left = 0;
        strcpy(statusStr, "");
        int cont;
        if (sched == 1 || sched == 0)
        {
            cont = (current_run == -1);
        }
        if (sched == 2)
        {
            cont = (!(current_run >= 0));
        }
        if (cont)
        {
            if (fcfs || rr)
                left = 1;
        }
        else
        {
            
            ProcessB *p = &processContainer[current_run];
            int halfCPU = (p->B + 1) / 2;

            if (p->cpuUsed == halfCPU)
            {
                if (p->blockedTime > 0)
                { //flag process as terminated
                    terminatedList[current_run] = 1; 
                    if (sjf)
                        readyProcessQueue[current_run] = 0;
                    current_run = -1;
                    if (rr)
                        timeCPUusageRR = 0;
                    p->status = 2; //change status
                    p->timeTerm = counter;
                }
                else
                { //flag process as blocked
                    blockList[current_run] = 1; 
                    if (sjf)
                        readyProcessQueue[current_run] = 0;
                    current_run = -1;
                    if (rr)
                        timeCPUusageRR = 0;
                    p->status = 0; //change status
                    p->blockedTime = 1;
                }
                if (fcfs || rr)
                    left = 1;
            }
            //add process to the ready queue
            else if (timeCPUusageRR == 2 && (rr))
            {
                timeCPUusageRR = 0;
                p->status = 3;
                p->timeReady = counter;
                readyProcessQueue[endQ] = current_run;
                endQ = (endQ + 1) % num_proc;
                current_run = -1;
                left = 1;
            }
            else
            {
                if (sjf)
                {
                    readyProcessQueue[current_run] = 1;
                    current_run = -1;
                    p->status = 3; //p terminated
                }
                else
                { //continue running the process
                    p->cpuUsed++;
                    timeCPUusage++;
                    if (rr)
                        timeCPUusageRR++;
                }
            }
        }
        int condition = 0;
        if (fcfs)
            condition = (left > 0 && startQ != endQ);
        else if (rr)
            condition = (left > 0 && readyProcessQueue[startQ] != -1);

        if (!sjf && condition)

        {
            int currentNum = startQ;
            int timeProcessReady = processContainer[readyProcessQueue[startQ]].timeReady;
            int checkNum = processContainer[readyProcessQueue[startQ]].A;
            int i = 1;
            //determine if there are other ready processes that should be given preference
            while (readyProcessQueue[(startQ + i) % num_proc] >= 0 && timeProcessReady == processContainer[readyProcessQueue[(startQ + i) % num_proc]].timeReady)
            {
                if (processContainer[readyProcessQueue[(startQ + i) % num_proc]].A < checkNum)
                {
                    currentNum = (startQ + i) % num_proc;
                    checkNum = processContainer[readyProcessQueue[currentNum]].A;
                }
                i++;
            }
            if (currentNum != startQ)
            {
                int tmp = readyProcessQueue[startQ];
                readyProcessQueue[startQ] = readyProcessQueue[currentNum];
                readyProcessQueue[currentNum] = tmp;
            }

            current_run = readyProcessQueue[startQ];
            processContainer[current_run].cpuUsed++;
            timeCPUusage++;
            if (rr)
                timeCPUusageRR = 1;
            processContainer[current_run].status = 1;
            readyProcessQueue[startQ] = -1;
            startQ = (startQ + 1) % num_proc;
        }

        if (sjf)
        {
            int currentNum = -1;
            int shortestTimeLeft = INT_MAX; 
            int pidShortest = INT_MAX;
            for (int i = 0; i < num_proc; i++)
            {
                if (readyProcessQueue[i] < 1)
                    continue;
                else if (processContainer[i].AT > counter)
                    continue;
                else
                {
                    ProcessB pr = processContainer[i];
                    int halfCPU = (pr.B + 1) / 2;
                    int timeLeft;
                    if (pr.blockedTime > 0)
                    {
                        timeLeft = halfCPU - pr.cpuUsed;
                    }
                    else
                    {
                        timeLeft = halfCPU + halfCPU - pr.cpuUsed;
                    }
                    if (timeLeft <= shortestTimeLeft)
                    {
                        if (timeLeft == shortestTimeLeft)
                        {
                            if (processContainer[i].A < pidShortest)
                            {
                                currentNum = i;
                                shortestTimeLeft = timeLeft;
                                pidShortest = processContainer[i].A;
                            }
                        }
                        else
                        {
                            currentNum = i;
                            shortestTimeLeft = timeLeft;
                            pidShortest = processContainer[i].A;
                        }
                    }
                }
            }
            if (currentNum >= 0)
            {
                current_run = currentNum;
                processContainer[current_run].cpuUsed++;
                timeCPUusage++;
                processContainer[current_run].status = 1;
                readyProcessQueue[currentNum] = -1;
            }
        }
        int count = 0;
        for (int i = 0; i < num_proc; i++)
        {
            if (processContainer[i].status == 0 || processContainer[i].status == 3 || processContainer[i].status == 1 || processContainer[i].timeReady >= 0)
            {
                count++;
            }
        }
        for (int i = 0; i < num_proc; i++)
        {
           
            if (processContainer[i].AT > counter || processContainer[i].timeTerm > 0)
            {
                continue;
            }
            else
            {
                printStatus(processContainer, statusStr, s, i);
            }
        }
        if (listEmpty(terminatedList, 1) != num_proc)
            fprintf(outputFile, "%d %s\n", counter, statusStr);
    } 

    for (int i = 0; i < num_proc; i++)
    {
        printResult(processContainer, finalStr, s, i);
    }

    double val = timeCPUusage / (counter * 1.0);
    printOutF(outputFile, val, (counter - 1), finalStr);

    free(finalStr);
    free(statusStr);
    
}

int main(int argc, char *argv[])
{
    
    FILE *inputf;
    if (argc != 3)
    {
        printf("Format: ./scheduling input_file scheduling_algorithm");
        exit(1);
    }
    char size_f[100];
    if ((inputf = fopen(argv[1], "r")))
    {
        fscanf(inputf, "%s", size_f);
        num_proc = atoi(size_f);
        if (num_proc== 0)
        {
            printf("Input file is invalid");
            exit(1);
        }
    }
    else
    {
        printf("Input file is invalid");
        exit(1);
    }

    ProcessB *processContainer = malloc(sizeof(ProcessB) * num_proc);
    if (processContainer == NULL)
    {
        printf("Memory allocation error");
        exit(1);
    }

    for (int i = 0; i < num_proc; i++)
    {
        fscanf(inputf, "%d %d %d %d", &processContainer[i].A, &processContainer[i].B, &processContainer[i].C, &processContainer[i].AT);
        processContainer[i].timeReady = -1;
        processContainer[i].blockedTime = 0;
        processContainer[i].timeTerm = -1;
       processContainer[i].cpuUsed = 0;
        processContainer[i].status = -1;
    }

    FILE *outputf;
    const char div[10] = ".";
    char *filename = strtok(argv[1], div);
    char outf[100];

    if (strcmp(argv[2], "0") == 0)
    {
        sched = 0;
        sprintf(outf, "%s-%d.txt", filename, 0);
        outputf = fopen(outf, "w");
        scheduler(outputf, processContainer);
    }
    else if (strcmp(argv[2], "1") == 0)
    {
        sched = 1;
        sprintf(outf, "%s-%d.txt", filename, 1);
        outputf = fopen(outf, "w");
        scheduler(outputf, processContainer);
    }
    else if (strcmp(argv[2], "2") == 0)
    {
        sched = 2;
        sprintf(outf, "%s-%d.txt", filename, 2);
        outputf = fopen(outf, "w");
        scheduler(outputf, processContainer);
    }
    else
    {
        printf("Invalid scheduling algorithm");
        exit(1);
    }

    free(processContainer);

    return 0;
}

void printResult(ProcessB *processContainer, char *finalStr, int s, int i)
{
    char resultStr[100];
    int count = 0;

    for (int i = 0; i < num_proc; i++)
    {
        if (processContainer[i].status == 0 || processContainer[i].status == 3 || processContainer[i].status == 1 || processContainer[i].timeReady >= 0)
        {
            count++;
        }
    }
    int end = num_proc - 1 == i;
    switch (end)
    {
    case 1:
        if ((count == 3 && sched == 0 && i == 2))
        {
            sprintf(resultStr, "Turnaround process %d: %d  ", i, processContainer[i].timeTerm - processContainer[i].AT);
        }
        else
        {
            if (count == 3 && (sched != 0) && i == 2)
            {
                sprintf(resultStr, "Turnaround process %d: %d ", i, processContainer[i].timeTerm - processContainer[i].AT);
            }
            else
            {
                sprintf(resultStr, "Turnaround process %d: %d", i, processContainer[i].timeTerm - processContainer[i].AT);
            }
        }
        break;
    case 0:
        if ((num_proc == 2 && sched == 1) || (num_proc == 3 && i == 1))
            sprintf(resultStr, "Turnaround process %d: %d\n", i, processContainer[i].timeTerm - processContainer[i].AT);
        else
            sprintf(resultStr, "Turnaround process %d: %d \n", i, processContainer[i].timeTerm - processContainer[i].AT);
        break;
    }
    strcat(finalStr, resultStr);
}

void printOutF(FILE *outputFile, double val, int t, char *finalStr)
{
    int a = (sched == 2 && num_proc == 2);
    int b = ((sched == 1 || sched == 2) && num_proc == 3);
    int c = (num_proc == 2 && sched == 0);
    int d = (num_proc == 3 && sched == 0);
    int e = (num_proc == 2 && sched == 1);

    if (a)
    {
        fprintf(outputFile, "\n\nFinishing time:%d \nCPU utilization: %.2f\n%s", t, val, finalStr);
    }
    else if (b)
    {
        fprintf(outputFile, "\n\nFinishing time: %d\nCPU utilization: %.2f\n%s", t, val, finalStr);
    }
    else if (c)
    {
        fprintf(outputFile, "\nFinishing time:%d \nCPU utilization: %.2f \n%s", t, val, finalStr);
    }
    else if (d)
    {
        if (val == 1.0)
        {
            fprintf(outputFile, "\nFinishing time: %d\nCPU utilization: %.1f\n%s", t, val, finalStr);
        }
        else
        {
            fprintf(outputFile, "\nFinishing time: %d\nCPU utilization: %.2f\n%s", t, val, finalStr);
        }
    }
    else if (e)
    {
        fprintf(outputFile, "\nFinishing time: %d\nCPU utilization: %.2f \n%s", t, val, finalStr);
    }
    else
    {
        fprintf(outputFile, "\nFinishing time: %d \nCPU utilization: %.2f\n%s", t, val, finalStr);
    }
}

void printStatus(ProcessB *processContainer, char *statusStr, int s, int i)
{
    int count = 0;
    for (int i = 0; i < num_proc; i++)
    {
        if (processContainer[i].status == 0 || processContainer[i].status == 3 || processContainer[i].status == 1 || processContainer[i].timeReady >= 0)
        {
            count++;
        }
    }
    char statstr[25];
    sprintf(statstr, "%d:", i);
    int status = processContainer[i].status;
    int set = 0;
    switch (status)
    {
    case 1:
        set = 1;
        if ((count == 3 && sched == 0 && (s == 6 || s == 7 || s == 8)) || (count == 2 && sched == 2 && s == 4) || (count == 3 && sched == 2 && (s == 12 || s == 13)))
            strcat(statstr, "running ");
        else
            strcat(statstr, "running");
        break;
    case 0:
        set = 1;
        strcat(statstr, "blocked");
        break;
    }

    if (set != 1 && (processContainer[i].timeReady >= 0 || processContainer[i].status == 3))
    {
        strcat(statstr, "ready");
        if (count == 3 && sched == 0 && (i != num_proc - 1))
        {
            if (s == 9 || s == 6 || s == 4 || s == 5 || s == 7 || s == 8)
                strcat(statstr, "  ");
            else if (s == 10)
                strcat(statstr, " ");
        }
    }

    if ((count > 1 && i != count - 1))
    {
        strcat(statstr, " ");
    }
    strcat(statusStr, statstr);
}

int listEmpty(int list[], int flagCount)
{
    int count = 0;
    for (int i = 0; i < num_proc; i++)
    {
        if (list[i])
        {
            if (flagCount)
                count++;
            else
                return 0;
        }
    }
    if (flagCount)
        return count;
    else
        return 1;
}

void enqueueWait(ProcessB *processContainer, int *waitList, int *readyProcessQueue, int *endQ)
{
    for (int i = 0; i < num_proc; i++)
    {
        if (processContainer[i].AT == counter)
        {

            //addReadyQueue(readyProcessQueue,endQ,i);

            switch (sched)
            {
            case 0:
            case 1:
                readyProcessQueue[(*endQ)] = i;
                *endQ = (*endQ + 1) % num_proc;
                break;
            case 2:
                readyProcessQueue[i] = 1;
                break;
            }

            processContainer[i].timeReady = counter;
            waitList[i] = 0;
        }
    }
}
void enqueueBlocked(ProcessB *processContainer, int *blockList, int *readyProcessQueue, int *endQ)
{
    for (int i = 0; i < num_proc; i++)
    {
        if (processContainer[i].status == 0)
        {

            if (processContainer[i].blockedTime == processContainer[i].C)
            {
                processContainer[i].timeReady = counter;
                processContainer[i].status = 3;
                processContainer[i].cpuUsed = 0;
                blockList[i] = 0;
                //addReadyQueue(readyProcessQueue,endQ,i);

                switch (sched)
                {
                case 0:
                case 1:
                    readyProcessQueue[(*endQ)] = i;
                    *endQ = (*endQ + 1) % num_proc;
                    break;
                case 2:
                    readyProcessQueue[i] = 1;
                    break;
                }
            }
            else
            {
                processContainer[i].blockedTime++;
            }
        }
    }
}

