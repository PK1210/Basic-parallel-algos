# Badminton Academy Problem

*Coded by:* **Prajwal Krishna**


--------------------------

## Problem Statement

An academy recently started allowing a  group of 3 people to play badminton at their courts. The group should consist of  2 players who play against each other and  a referee to manage the match. Whenever a new player/referee  enters the academy , he/she waits until he/she gets a chance to  meet the organizer . Whenever the organizer is free, he waits until a group of 2 players and a referee can be formed among the unassigned people he met and then the newly formed group  enters the court . Then the players  warm up for the game and in the meantime referee  adjusts the equipment necessary to manage the game. Once all the three are  ready , the  game is started by the referee. Organizer waits till the game is started and once it is started he is free and again starts meeting new players/referees entering the academy.

### Expected function invocation order:
 Players invoke the following functions in order:  
 1. enterAcademy
 2. meetOrganizer
 3. enterCourt
 4. warmUp


 Referees invoke the following functions in order:
 1. enterAcademy
 2. meetOrganizer
 3. enterCourt
 4. adjustEquipment
 5. startGame


### Input
An integer  n indicating  2*n players and  n referees will arrive at the academy in total.
Use sleep(1) for warmUp and sleep(0.5) for adjustEquipment actions. The new person arrive with a  random, modulo 3 second delay and probability that he is a player/referee depends on the remaining number of players/referees that will be coming and each person is a thread.

--------------------------

# Implementation:

### Logic

1. Organizer, each player and referee are separate threads.
2. Organizer waits until he meet 2 player and 1 referee to allow them to court.
3. Once organizer gets players and referee he is no longer free.
4. Once 2 player and 1 referee meets organizer then they enter the court.
5. Player stars warmup while referee adjusting equipments.
6. Referee waits for player to finish warm up and then signal organizer for successful start of match.
7. Organizer becomes free on getting this signal.

### Mutexs and condition variables:

1. organizer_free_cond and organizer_free_mutex to control availablilty of organizer.
2. print_mutex for making sure only one statement can print at a time so no mixing of statements occur.
3. sufficient_people_cond this is on organizer_free_mutex to make sure organizer waits for 2 players and 1 referee before signalling them to start of match.
4. match_started_cond to ensure organizer keeps waiting until match started signal from referee.
5. allow_enterCourt_cond to make players and referee wait till sufficient people are there

A total of 5 condition variables and 5 mutexs are used in this solution.



--------------------------
#### Prajwal Krishna Maitin
