# Concurrent Voting At Polling Booths
A concurrent process of voting where the threads representing evms, voters and booths run concurrently using conditional variables and mutex locks.


*Coded by:* **Prajwal Krishna**

--------------------------

## Compile and Run
- Compile using gcc and run the executables
```
gcc -pthread pollBooth.c
./a.out
```

```
number_of_booths
number_of_voters   number_of_evms   max_slots_in_evm    (for booth 0)
number_of_voters   number_of_evms   max_slots_in_evm    (for booth 1)
number_of_voters   number_of_evms   max_slots_in_evm    (for booth 2)
.
.
.
```
--------------------------
## Problem Statement

To simulate working of an election (**the power of Democracy**).
There are multiple booths each booth has a fixed list of voters and got a fixed number of evms. Each evm can have a random number of slots i.e. more than 1 person can vote in an evm. Evm transfers votes only when all its voters are done. Evm can be called multiple times but each time number of slot available on it might be not same.A person only votes once and has no affinity for any evm.

--------------------------

## Implementation

### Logic
Each booth works independently so the process can be restricted to one booth.

1. When a voter arrives he waits to get assigned to an evm.
2. Evm gets free slots and starts assigning voters to itself and signals voters about it.
3. Evm then starts voting phase and wait for voters to cast their votes.
4. Voters cast their votes which takes some finite amount of time.
5. Voters then signal evm about that they have casted their votes.
6. Evm once received signals from all its voters tells booth that it has finished its job and gets ready for another set voters if voters remain.
7. When last voter of a booth gets an evm assigned it broadcasts to all evms to make sure no evm keeps waiting for evms.


### Implementation Details

- Each booth works independently so the details can be restricted to one booth by making each booth a separate thread
- Each booth requires only one mutex
- Each evm and voter is a new thread joined to booth thread.
- Each evm has a condition variable 'votes_casted_cond' which is to make sure that evm returns only when all its voters has voted.
- Each booth has a condition variable 'evms_available_cond' which is to make sure voters wait until they get an evm.
- Similarly each evm has a condition variable 'voters_waiting_cond' which makes sure evm waits until it gets voters.




--------------------------
#### Prajwal Krishna Maitin
