# TicTacToe Server (Player 1) Design
> This is the design document for the TicTacToe Server ([tictactoeServer.c](https://github.com/CSE-5462-Spring-2021/assignment-6-conner-ben/blob/main/tictactoeServer.c)).  
> By: Conner Graham

## Table of Contents
- TicTacToe Class Protocol - [Protocol Document](https://docs.google.com/document/d/1H9yrRi0or_yTt-0xs5QAp2C6umw2qWW1FDL3EyVwF5g/edit?usp=sharing)
- [Environment Constants](#environment-constants)
- [Environment Structures](#environment-structures)
- [High-Level Architecture](#high-level-architecture)
- [Low-Level Architecturet](#low-level-architecture)

## Environment Constants
```C#
VERSION = 4             // protocol version number

NUM_ARGS = 2            // number of command line arguments
GAME_TIMEOUT = 30       // number of seconds spent waiting before a timeout
SERVER_TIMEOUT = TBD    // number of seconds spent waiting before a timeout
MAX_RESENDS = TBD       // maximum number of resend attempts before resetting a game
ROWS = 3                // number of rows for the TicIacToe board
COLUMNS = 3             // number of columns for the TicIacToe board
MAX_GAMES = TBD         // maximum number of games that can be played simultaneously
P1_MARK = TBD           // baord marker used for Player 1
P2_MARK = TBD           // baord marker used for Player 2

// COMMANDS
NEW_GAME = 0x00         // command to begin a new game
MOVE = 0x01             // command to issue a move
GAME_OVER = 0x02        // command to end a game
```

## Environment Structures
Structure for each TicTacToe game.
```C
struct TTT_Game {
    int gameNum;                    // game number
    int seqNum;                     // sequence number game currently on
    double timeout;                 // amount of time before game timeout
    int resends;                    // number of resends before quitting game
    struct sockaddr_in p2Address;   // address of remote player for game
    int winner;                     // player who won, 0 if draw, -1 if game ongoing
    struct Buffer lastSent;         // previous command sent in game
    char board[ROWS*COLUMNS];       // TicTacToe game board state
};
```
Structure to send and recieve player datagrams.
```C
struct Buffer {
    char version;   // version number
    char seqNum;    // sequence number
    char command;   // player command
    char data;      // data for command if applicable
    char gameNum;   // game number
};
```

## High-Level Architecture
At a high level, the server application attempts to validate and extract the arguments passed
to the application. It then attempts to create and bind the server endpoint. If everything was
successful, the TicTacToe server is started. If an error occurs before the server is started,
the program terminates and prints appropriate error messages, otherwise an error message is
printed and the server continues.
```C
int main(int argc, char *argv[]) {
    /* check that the arg count is correct */
    if (!correct) exit(EXIT_FAILURE);
    extract_args(params...);
    create_endpoint(params...);
    tictactoe(params...);
    return 0;
}
```

## Low-Level Architecture
Extracts the user provided arguments to their respective local variables and performs
validation on their formatting. If any errors are found, the function terminates the process.
```C
void extractArgs(params...) {
    /* extract and validate remote port number */
    if (!valid) exit(EXIT_FAILURE);
}
```
Creates the comminication endpoint with the provided IP address and port number. If any
errors are found, the function terminates the process.
```C
int create_endpoint(params...) {
    /* attempt to create socket */
    if (created) {
        /* initialize socket with params from user */
    } else {
        exit(EXIT_FAILURE);
    }
    /* attempt to bind socket to address */
    if (!bind) {
        exit(EXIT_FAILURE);
    }
    return socket-descriptor;
}
```
Initializes a set of game boards and processes any commands receivedfrom other players. These
commands can include initializing a game of TicTacToe when a player requests one or responding
to other players moves until a winner is found or the game is a draw. If a player takes too
long to respond, the game times out and is reset for another player to play. If no player
responds to the server for a period of time, the server times out and all ongoing games are
reset for other players to play.
```C
void tictactoe(params...) {
    /* initialize all games */
    /* set server timeout time */
    while (TRUE) {
        get_command(params...);
        if (!error) {
            /* retrieve appropriate game */
            /* process command */
            /* update timeout clock for each ongoing game */
            /* reset any game that has timed out */
        } else if (error == timeout) {    // server timeout
            /* check if any games are currently being played */
            if (games open) /* reset all games */;
        }
    }
}
```
- Gets a command from the remote player and attempts to validate the data and syntax based on
  the current protocol.
    ```C
    int get_command(params...) {
        /* receive command from remote player */
        if (error) return ERROR_CODE;
        /* check version number */
        if (!valid) return ERROR_CODE;
        /* check command */
        if (!valid) return ERROR_CODE;
        /* check game number */
        if (!valid) return ERROR_CODE;
        return (number of bytes received for command);
    }
    ```
    - Handles the NEW_GAME command from the remote player. Initializes a new game, if available,
      and sends the first move to the remote player.
        ```C
        void new_game(params...) {
            if (there is an open game) {
                /* register player address to open game */
                /* initialize the game board */
                send_p1_move(params...);
                if (error) {
                    /* reset game */
                    return;
                }
                /* update and print game board */
                /* change turn to other player */
            }
        }
        ```
    - Handles the MOVE command from the remote player. Receives and processes a move from the
      remote player and sends a move back. If the game has ended from a move, an appropriate
      message is printed and the game is reset for a new player.
        ```C
        void move(params...) {
            /* get move from remote player */
            if (player address matches that assigned to game) {
                /* check that move is valid */
                if (valid) {
                    /* update board with Player 2's move */
                    if (game over) return;
                    send_p1_move(params...);
                    if (error) {
                        /* reset game */
                        return;
                    }
                    /* update board with Player 1's move */
                    if (game over) return;
                    /* print board after move exchange */
                } else {
                    /* reset game */
                }
            }
        }
        ```
- Determines whether or not the given move is valid based on the current state of the game.
    ```C
    int validate_move(params...) {
        if (move not numer [1-9]) return FALSE;
        if (move has already been made) return FALSE;
        return TRUE;
    }
    ```
- Sends Player 1's move to the remote player.
    ```C
    int send_p1_move(params...) {
        /* get move to send to remote player */
        /* pack move info into datagram */
        /* send move to remote player */
        if (error) return ERROR_CODE;
        return (move);
    }
    ```
- Checks if the current game has ended, prints the appropriate message if so, and resets the
  game for a new player.
    ```C
    int game_over(params...) {
        /* check if somebody won */
        if (winner) /* print winner info */;
        /* check for draw */
        if (draw) /* print draw info */;
        /* check if game still in progress */
        if (!winner && !draw) return FALSE;
        /* reset game */
        return TRUE;
    }
    ```
