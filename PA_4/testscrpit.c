#/!/bin/bash

#File: testscript
#Author: Andy Sayler
#Project: CSCI 3753 Programming Assignment 3
#Create Date: 2012/03/09
#Modify Date: 2012/03/21
#Description:
#	A simple bash script to run a signle copy of each test case
#	and gather the relevent data.

ITERATIONS=10000000
PROCESSES = 1000
BYTESTOCOPY=102400
BLOCKSIZE=1024
TIMEFORMAT="wall=%e user=%U system=%S CPU=%P i-switched=%c v-switched=%w"
RESULT_FILE="/home/user/Documents/PA_4/results.csv"
MAKE="make -s"

echo Building code...
$MAKE clean
$MAKE

echo "\"Process Type\",\"Scheduler Type\",Iterations,\"Processes\",Wall,User,System,CPU,I-Switched,V-switched" > "$RESULT_FILE"
echo Starting test runs...


echo Calculating pi over $ITERATIONS iterations using SCHED_OTHER with $PROCESSES simultaneous process...
TIMEFORMAT="CPU BOUND,SCHED_OTHER,$ITERATIONS,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a ./pi-sched $ITERATIONS SCHED_OTHER $PROCESSES > /dev/null

echo Calculating pi over $ITERATIONS iterations using SCHED_FIFO with $PROCESSES simultaneous process...
TIMEFORMAT="CPU BOUND,SCHED_FIFO,$ITERATIONS,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a sudo ./pi-sched $ITERATIONS SCHED_FIFO $PROCESSES > /dev/null

echo Calculating pi over $ITERATIONS iterations using SCHED_RR with $PROCESSES simultaneous process...
TIMEFORMAT="CPU BOUND,SCHED_RR,$ITERATIONS,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a sudo ./pi-sched $ITERATIONS SCHED_RR $PROCESSES > /dev/null

echo Copying $BYTESTOCOPY bytes in blocks of $BLOCKSIZE from rwinput to rwoutput
echo Using SCHED_OTHER with $PROCESSES simultaneous processes...
TIMEFORMAT="\"I/O BOUND\",SCHED_OTHER,1,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a ./rw $BYTESTOCOPY $BLOCKSIZE SCHED_OTHER $PROCESSES > /dev/null

echo Copying $BYTESTOCOPY bytes in blocks of $BLOCKSIZE from rwinput to rwoutput
echo Using SCHED_FIFO with $PROCESSES simultaneous processes...
TIMEFORMAT="\"I/O BOUND\",SCHED_FIFO,1,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a sudo ./rw $BYTESTOCOPY $BLOCKSIZE SCHED_FIFO $PROCESSES > /dev/null

echo Copying $BYTESTOCOPY bytes in blocks of $BLOCKSIZE from rwinput to rwoutput
echo Using SCHED_RR with $PROCESSES simultaneous processes...
TIMEFORMAT="\"I/O BOUND\",SCHED_RR,1,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a sudo ./rw $BYTESTOCOPY $BLOCKSIZE SCHED_RR $PROCESSES > /dev/null

echo Calculating pi and writing over $ITERATIONS iterations using SCHED_OTHER with $PROCESSES simultaneous process...
TIMEFORMAT="CPU BOUND,SCHED_OTHER,$ITERATIONS,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a ./mixed $ITERATIONS SCHED_OTHER $PROCESSES > /dev/null

echo Calculating pi nd writing over $ITERATIONS iterations using SCHED_FIFO with $PROCESSES simultaneous process...
TIMEFORMAT="CPU BOUND,SCHED_FIFO,$ITERATIONS,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a sudo ./mixed $ITERATIONS SCHED_FIFO $PROCESSES > /dev/null

echo Calculating pi and writing over $ITERATIONS iterations using SCHED_RR with $PROCESSES simultaneous process...
TIMEFORMAT="CPU BOUND,SCHED_RR,$ITERATIONS,$PROCESSES,%e,%U,%S,%P,%c,%w"
/usr/bin/time -f "$TIMEFORMAT" -o "$RESULT_FILE" -a sudo ./mixed $ITERATIONS SCHED_RR $PROCESSES > /dev/null
