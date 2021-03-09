/***********************************************************/
/* This program is a 'net-enabled' version of tictactoe.   */
/* Two users, Player 1 and Player 2, send moves back and   */
/* forth, between two computers.                           */
/***********************************************************/

/* #include files go here */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* The protocol version number used. */
#define VERSION 3

/* The number of command line arguments. */
#define NUM_ARGS 2
/* The maximum size of a buffer for the program. */
#define BUFFER_SIZE 100
/* The error code used to signal an invalid move. */
#define ERROR_CODE -1
/* The number of seconds spend waiting before a timeout. */
#define TIMEOUT 30

/* The number of rows for the TicIacToe board. */
#define ROWS 3
/* The number of columns for the TicIacToe board. */
#define COLUMNS 3
/* The maximum number of games the server can play simultaneously. */
#define MAX_GAMES 10
/* The baord marker used for Player 1 */
#define P1_MARK 'X'
/* The baord marker used for Player 2 */
#define P2_MARK 'O'

/* Structure for each game of TicTacToe. */
struct TTT_Game {
    int gameNum;                    // game number
    double timeout;                 // amount of time before game timeout
    struct sockaddr_in p2Address;   // address of remote player for game
    int player;                     // current player's turn
    char board[ROWS*COLUMNS];       // TicTacToe game board state
};

/* Structure to send and recieve player datagrams. */
struct Buffer {
    char version;   // version number
    char command;   // player command
    char data;      // data for command if applicable
    char gameNum;   // game number
};

/*******************/
/* PLAYER COMMANDS */
/*******************/

/* Function pointer type for function to handle player commands. */
typedef void (*command_handler)(int sd, const struct sockaddr_in *playerAddr, const struct Buffer *datagram, struct TTT_Game *game);
/* The command to begin a new game. */
#define NEW_GAME 0x00
/* The command to issue a move. */
#define MOVE 0x01

void new_game(int sd, const struct sockaddr_in *playerAddr, const struct Buffer *datagram, struct TTT_Game *game);
void move(int sd, const struct sockaddr_in *playerAddr, const struct Buffer *datagram, struct TTT_Game *game);

/********************************/
/* SOCKET AND NETWORK FUNCTIONS */
/********************************/

void print_error(const char *msg, int errnum, int terminate);
void handle_init_error(const char *msg, int errnum);
void extract_args(char *argv[], int *port);
void print_server_info(struct sockaddr_in serverAddr);
int create_endpoint(struct sockaddr_in *socketAddr, unsigned long address, int port);
void set_timeout(int sd, int seconds);
void check_timeout(struct TTT_Game roster[MAX_GAMES]);
int same_address(const struct sockaddr_in *addr1, const struct sockaddr_in *addr2);

/******************************/
/* TIC-TAC-TOE GAME FUNCTIONS */
/******************************/

void init_shared_state(struct TTT_Game *game);
void init_game_roster(struct TTT_Game roster[MAX_GAMES]);
int games_in_progress(struct TTT_Game roster[MAX_GAMES]);
int find_open_game(struct TTT_Game roster[MAX_GAMES]);
int get_command(int sd, struct sockaddr_in *playerAddr, struct Buffer *datagram);
int minimax(struct TTT_Game *game, int depth, int isMax);
int find_best_move(struct TTT_Game *game);
int check_win(const struct TTT_Game *game);
int check_draw(const struct TTT_Game *game);
void print_board(const struct TTT_Game *game);
int validate_move(int choice, const struct TTT_Game *game);
int send_p1_move(int sd, struct TTT_Game *game);
void free_game(struct TTT_Game *game);
int game_over(struct TTT_Game *game);
void tictactoe(int sd);

/**
 * @brief This program creates and sets up a TicTacToe server which acts as Player 1 in a
 * 2-player game of TicTacToe. This server creates a server socket for the clients to communicate
 * with, listens for remote client UDP DAGAGRAM packets, and then initiates a simple
 * game of TicTacToe in which Player 1 and Player 2 take turns making moves which they send to
 * the other player. If an error occurs before the "New Game" command is received, the program
 * terminates and prints appropriate error messages, otherwise an error message is printed and
 * the program searches for a new player waiting to play.
 * 
 * @param argc Non-negative value representing the number of arguments passed to the program
 * from the environment in which the program is run.
 * @param argv Pointer to the first element of an array of argc + 1 pointers, of which the
 * last one is NULL and the previous ones, if any, point to strings that represent the
 * arguments passed to the program from the host environment. If argv[0] is not a NULL
 * pointer (or, equivalently, if argc > 0), it points to a string that represents the program
 * name, which is empty if the program name is not available from the host environment.
 * @return If the return statement is used, the return value is used as the argument to the
 * implicit call to exit(). The values zero and EXIT_SUCCESS indicate successful termination,
 * the value EXIT_FAILURE indicates unsuccessful termination.
 */
int main(int argc, char *argv[]) {
    int sd, portNumber;
    struct sockaddr_in serverAddress;

    /* If arg count correct, extract arguments to their respective variables */
    if (argc != NUM_ARGS) handle_init_error("argc: Invalid number of command line arguments", 0);
    extract_args(argv, &portNumber);

    /* Create server socket and print server information */
    sd = create_endpoint(&serverAddress, INADDR_ANY, portNumber);
    print_server_info(serverAddress);

    /* Start the TicTacToe server */
    tictactoe(sd);

    return 0;
}

/**
 * @brief Prints the provided error message and corresponding errno message (if present) and
 * terminates the process if asked to do so.
 * 
 * @param msg The error description message to display.
 * @param errnum This is the error number, usually errno.
 * @param terminate Whether or not the process should be terminated.
 */
void print_error(const char *msg, int errnum, int terminate) {
    /* Check for valid error code and generate error message */
    if (errnum) {
        printf("ERROR: %s: %s\n", msg, strerror(errnum));
    } else {
        printf("ERROR: %s\n", msg);
    }
    /* Exits process if it should be terminated */
    if (terminate) exit(EXIT_FAILURE);
}

/**
 * @brief Prints a string describing the initialization error and provided error number (if
 * nonzero), the correct command usage, and exits the process signaling unsuccessful termination. 
 * 
 * @param msg The error description message to display.
 * @param errnum This is the error number, usually errno.
 */
void handle_init_error(const char *msg, int errnum) {
    print_error(msg, errnum, 0);
    printf("Usage is: tictactoeServer <remote-port>\n");
    /* Exits the process signaling unsuccessful termination */
    exit(EXIT_FAILURE);
}

/**
 * @brief Extracts the user provided arguments to their respective local variables and performs
 * validation on their formatting. If any errors are found, the function terminates the process.
 * 
 * @param argv Pointer to the first element of an array of argc + 1 pointers, of which the
 * last one is NULL and the previous ones, if any, point to strings that represent the
 * arguments passed to the program from the host environment. If argv[0] is not a NULL
 * pointer (or, equivalently, if argc > 0), it points to a string that represents the program
 * name, which is empty if the program name is not available from the host environment.
 * @param port The remote port number that the server should listen on
 */
void extract_args(char *argv[], int *port) {
    /* Extract and validate remote port number */
    *port = strtol(argv[1], NULL, 10);
    if (*port < 1 || *port != (u_int16_t)(*port)) handle_init_error("remote-port: Invalid port number", 0);
}

/**
 * @brief Prints the server information needed for the client to comminicate with the server.
 * 
 * @param serverAddr The socket address structure for the server comminication endpoint.
 */
void print_server_info(const struct sockaddr_in serverAddr) {
    int hostname;
    char hostbuffer[BUFFER_SIZE], *IP_addr;
    struct hostent *host_entry;

    /* Retrieve the hostname */
    if ((hostname = gethostname(hostbuffer, sizeof(hostbuffer))) == -1) {
        print_error("print_server_info: gethostname", errno, 1);
    }
    /* Retrieve the host information */
    if ((host_entry = gethostbyname(hostbuffer)) == NULL) {
        print_error("print_server_info: gethostbyname", errno, 1);
    }
    /* Convert the host internet network address to an ASCII string */
    IP_addr = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
    /* Print the IP address and port number for the server */
    printf("Server listening at %s on port %hu\n", IP_addr, serverAddr.sin_port);
}

/**
 * @brief Creates the comminication endpoint with the provided IP address and port number. If any
 * errors are found, the function terminates the process.
 * 
 * @param socketAddr The socket address structure created for the comminication endpoint.
 * @param address The IP address for the socket address structure.
 * @param port The port number for the socket address structure.
 * @return The socket descriptor of the created comminication endpoint.
 */
int create_endpoint(struct sockaddr_in *socketAddr, unsigned long address, int port) {
    int sd;
    /* Create socket */
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) != -1) {
        socketAddr->sin_family = AF_INET;
        /* Assign IP address to socket */
        socketAddr->sin_addr.s_addr = address;
        /* Assign port number to socket */
        socketAddr->sin_port = htons(port);
    } else {
        print_error("create_endpoint: socket", errno, 1);
    }
    /* Bind socket to communication endpoint */
    if (bind(sd, (struct sockaddr *)socketAddr, sizeof(struct sockaddr_in)) == 0) {
        printf("[+]Server socket created successfully.\n");
    } else {
        print_error("create_endpoint: bind", errno, 1);
    }

    return sd;
}

/**
 * @brief Sets the time to wait before a timeout on recvfrom calls to the specified number
 * of seconds, or turns it off if zero seconds was entered.
 * 
 * @param sd The socket descriptor of the server comminication endpoint.
 * @param seconds The number of seconds to wait before a timeout.
 */
void set_timeout(int sd, int seconds) {
    struct timeval time = {0};
    time.tv_sec = seconds;

    /* Sets the recvfrom timeout option */
    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)) < 0) {
        print_error("set_timeout", errno, 0);
    }
}

/**
 * @brief Checks each TicTacToe game to see if it has timed out or not. If one has, that game
 * is reset.
 * 
 * @param roster The array of playable TicTacToe games.
 */
void check_timeout(struct TTT_Game roster[MAX_GAMES]) {
    int i;
    /* Searches over all games */
    for (i = 0; i < MAX_GAMES; i++) {
        /* Check if current game timeout has expired */
        if (roster[i].timeout <= 0) {
            printf("[+]Game #%d has timed out.\n", roster[i].gameNum);
            printf("Player at %s (port %d) ran out of time to respond.\n", inet_ntoa(roster[i].p2Address.sin_addr), roster[i].p2Address.sin_port);
            /* Reset the current game */
            free_game(&roster[i]);
        }
    }
}

/**
 * @brief Checks to see if two communication endpoints have the same address (IP and port) or not.
 * 
 * @param addr1 The address of communication endpoint 1.
 * @param addr2 The address of communication endpoint 2.
 * @return True if the endpoint addresses are the same, false otherwise. 
 */
int same_address(const struct sockaddr_in *addr1, const struct sockaddr_in *addr2) {
    /* Check endpoint IP addresses */
    if (addr1->sin_addr.s_addr != addr2->sin_addr.s_addr) return 0;
    /* Check endpoint port numbers */
    if (addr1->sin_port != addr2->sin_port) return 0;
    return 1;
}

/**
 * @brief Initializes the starting state of the game board that both players start with.
 * 
 * @param game The current game of TicTacToe being played.
 */
void init_shared_state(struct TTT_Game *game) {    
    int i;
    /* Initializes the shared state (aka the board)  */
    for (i = 1; i <= sizeof(game->board); i++) {
        game->board[i-1] = i + '0';
    }
}

/**
 * @brief Initializes the starting state of each game of TicTacToe in the current game roster.
 * 
 * @param roster The array of playable TicTacToe games.
 */
void init_game_roster(struct TTT_Game roster[MAX_GAMES]) {
    int i;
    printf("[+]Initializing shared game states.\n");
    /* Iterates over all games */
    for (i = 0;  i < MAX_GAMES; i++) {
        struct sockaddr_in blankAddr = {0};
        /* Initialize current game attributes to default values */
        roster[i].timeout = TIMEOUT;
        roster[i].p2Address = blankAddr;
        roster[i].gameNum = i+1;
        roster[i].player = 0;
        /* Initialize current game board */
        init_shared_state(&roster[i]);
    }
}

/**
 * @brief Determines how many games are currently being played.
 * 
 * @param roster The array of playable TicTacToe games.
 * @return The number of games currently being played. 
 */
int games_in_progress(struct TTT_Game roster[MAX_GAMES]) {
    int i, count = 0;
    /* Searches over all games */
    for (i = 0; i < MAX_GAMES; i++) {
        /* Check if current game still in default state or has been started */
        if (roster[i].player != 0) count++;
    }
    return count;
}

/**
 * @brief Finds an open game of TicTacToe to play if one is available.
 * 
 * @param roster The array of playable TicTacToe games.
 * @return The index of an open game if one is available, otherwise an error code is returned.
 */
int find_open_game(struct TTT_Game roster[MAX_GAMES]) {
    int i, gameIndex = ERROR_CODE;
    /* Searches over all games */
    for (i = 0; i < MAX_GAMES; i++) {
        /* Check if current game is being played */
        if (roster[i].player == 0) {
            gameIndex = i;
            break;
        }
    }
    return gameIndex;
}

/**
 * @brief Gets a command from the remote player and attempts to validate the data and syntax
 * based on the current protocol.
 * 
 * @param sd The socket descriptor of the server comminication endpoint.
 * @param playerAddr The address of the remote player.
 * @param datagram The datagram to store the command that the remote player sends.
 * @return The number of bytes received for the command, or an error code if an error occured. 
 */
int get_command(int sd, struct sockaddr_in *playerAddr, struct Buffer *datagram) {
    int rv;
    socklen_t fromLength = sizeof(struct sockaddr_in);
    /* Receive and validate command from remote player */
    if ((rv = recvfrom(sd, datagram, sizeof(struct Buffer), 0, (struct sockaddr *)playerAddr, &fromLength)) <= 0) {
        /* Check for error receiving command */
        if (rv == 0) {
            print_error("get_command: Received empty datagram. Datagram discarded", 0, 0);
        } else {
            /* Check for server timeout */
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                return 0;
            } else {
                print_error("get_command", errno, 0);
            }
        }
        return ERROR_CODE;
    } else if (datagram->version != VERSION) {  // check for correct version
        print_error("get_command: Protocol version not supported. Datagram discarded", 0, 0);
        return ERROR_CODE;
    } else if (datagram->command < NEW_GAME || datagram->command > MOVE) {  // check for valid command
        print_error("get_command: Invalid command. Datagram discarded", 0, 0);
        return ERROR_CODE;
    } else if (datagram->command != NEW_GAME && (datagram->gameNum < 1 || datagram->gameNum > MAX_GAMES)) { // check for valid game number
        print_error("get_command: Invalid game number. Datagram discarded", 0, 0);
        return ERROR_CODE;
    }
    return rv;
}

/**
 * @brief Handles the NEW_GAME command from the remote player. Initializes a new game, if
 * available, and sends the first move to the remote player.
 * 
 * @param sd The socket descriptor of the server comminication endpoint.
 * @param playerAddr The address of the remote player.
 * @param datagram The datagram containing the command that the remote player sends.
 * @param game The current game of TicTacToe being played.
 */
void new_game(int sd, const struct sockaddr_in *playerAddr, const struct Buffer *datagram, struct TTT_Game *game) {
    int move;
    printf("Player at %s (port %d) issued a NEW_GAME command.\n", inet_ntoa(playerAddr->sin_addr), playerAddr->sin_port);
    /* Check that there was an game open to play */
    if (game != NULL) {
        /* Register player address to game and initialize the board */
        game->p2Address = *playerAddr;
        init_shared_state(game);
        printf("Player assigned to Game #%d. Beginning game.\n", game->gameNum);
        /* Get first move to send to remote player */
        if ((move = send_p1_move(sd, game)) == ERROR_CODE) {
            /* Reset game if there was an error sending the move */
            free_game(game);
            return;
        }
        /* Update and print game board, and change turns */
        game->board[move-1] = P1_MARK;
        game->player = 2;
        print_board(game);
    } else {
        print_error("new_game: Unable to find an open game", 0, 0);
    }
}

/**
 * @brief Handles the MOVE command from the remote player. Receives and processes a move
 * from the remote player and sends a move back. If the game has ended from a move, an
 * appropriate message is printed and the game is reset for a new player.
 * 
 * @param sd The socket descriptor of the server comminication endpoint.
 * @param playerAddr The address of the remote player.
 * @param datagram The datagram containing the command that the remote player sends.
 * @param game The current game of TicTacToe being played.
 */
void move(int sd, const struct sockaddr_in *playerAddr, const struct Buffer *datagram, struct TTT_Game *game) {
    /* Get move from remote player */
    int move = datagram->data - '0';
    printf("Player at %s (port %d) issued a MOVE command.\n", inet_ntoa(playerAddr->sin_addr), playerAddr->sin_port);
    printf("********  Game #%d  ********\n", game->gameNum);
    /* Check that the move came from the player registered to the game */
    if (same_address(playerAddr, &game->p2Address)) {
        printf("Player 2 chose the move:  %c\n", datagram->data);
        /* Check that the received move is valid */
        if (validate_move(move, game)) {
            /* Update the board (for Player 2) and check if someone won */
            game->board[move-1] = P2_MARK;
            if (game_over(game)) return;
            /* If nobody won, change turns and make a move to send to the remote player */
            game->player = 1;
            if ((move = send_p1_move(sd, game)) == ERROR_CODE) {
                free_game(game);    
                return;
            }
            /* Update the board (for Player 1) and check if someone won */
            game->board[move-1] = P1_MARK;
            if (game_over(game)) return;
            /* If nobody won, change turns and print the board after the exchange */
            game->player = 2;
            print_board(game);
        } else {
            free_game(game);
        }
    } else {
        print_error("move: Player address does not match that registered to game", 0, 0);
        printf("Game address: %s (port %d)\n", inet_ntoa(game->p2Address.sin_addr), game->p2Address.sin_port);
    }
}

/**
 * @brief Provides an optimal move for the maximizing player assuming that minimizing player
 * is also playing optimally.
 * 
 * @param game The current game of TicTacToe being played.
 * @param depth The current depth in game tree.
 * @param isMax Whether it is the maximizers turn or not.
 * @return The best score achievable for the maximizer based on the current state of the game.
 */
int minimax(struct TTT_Game *game, int depth, int isMax) {
    /* Get score for current turn */
    int score = check_win(game);
    /* Check for base case */
    if (score > 0) {    // maximizer won
        return score - depth;
    } else if (score < 0) {    // minimizer won
        return score + depth;
    } else if (check_draw(game)) {  // nobody won
        return 0;
    } else {
        /* Initialize best score for maximizer/minimizer */
        int i, best = (isMax) ? INT32_MIN : INT16_MAX;
        if (isMax) {    // maximizers turn
            /* Searches over all possible moves */
            for (i = 0; i < sizeof(game->board); i++) {
                /* Checks that current move is valid based on the current board */
                if (game->board[i] == (i+1)+'0') {
                    int value;
                    /* Make the move */
                    game->board[i] = P1_MARK;
                    /* Get best score for move and update best move if the score was better */
                    if ((value = minimax(game, depth+1, !isMax)) > best) best = value;
                    /* Undo previous move */
                    game->board[i] = (i+1)+'0';
                }
            }
            return best;
        } else {    // minimizers turn
            /* Searches over all possible moves */
            for (i = 0; i < sizeof(game->board); i++) {
                /* Checks that current move is valid based on the current board */
                if (game->board[i] == (i+1)+'0') {
                    int value;
                    /* Make the move */
                    game->board[i] = P2_MARK;
                    /* Get best score for move and update best move if the score was better */
                    if ((value = minimax(game, depth+1, !isMax)) < best) best = value;
                    /* Undo previous move */
                    game->board[i] = (i+1)+'0';
                }
            }
            return best;
        }
    }
}

/**
 * @brief Finds the optimal move to make to win the game based on the current state of
 * the game board.
 * 
 * @param game The current game of TicTacToe being played.
 * @return The optimal move to make in order to win. 
 */
int find_best_move(struct TTT_Game *game) {
    int i, bestMove = -1, bestValue = INT32_MIN;
    /* Searches over all possible moves */
    for (i = 0; i < sizeof(game->board); i++) {
        /* Checks that current move is valid based on the current board */
        if (game->board[i] == (i+1)+'0') {
            int moveValue;
            /* Make the move */
            game->board[i] = P1_MARK;
            /* Get the move score */
            moveValue = minimax(game, 0, 0);
            /* Undo previous move */
            game->board[i] = (i+1)+'0';
            /* Update the best move if the current score was better */
            if (moveValue > bestValue) {
                bestValue = moveValue;
                bestMove = i+1;
            }
        }
    }
    return bestMove;
}

/**
 * @brief Determines if someone has won the game yet or not.
 * 
 * @param game The current game of TicTacToe being played.
 * @return True if a player has won the game and false if the game is still going on. 
 */
int check_win(const struct TTT_Game *game) {
    const int score = sizeof(game->board) + 1;
    /***********************************************************************/
    /* Brute force check to see if someone won. Return a +/- score if the  */
    /* game is 'over' or return 0 if game should go on.                    */
    /***********************************************************************/
    if (game->board[0] == game->board[1] && game->board[1] == game->board[2]) { // row matches
        return (game->board[0] == P1_MARK) ? score : -score;
    } else if (game->board[3] == game->board[4] && game->board[4] == game->board[5]) { // row matches
        return (game->board[3] == P1_MARK) ? score : -score;
    } else if (game->board[6] == game->board[7] && game->board[7] == game->board[8]) { // row matches
        return (game->board[6] == P1_MARK) ? score : -score;
    } else if (game->board[0] == game->board[3] && game->board[3] == game->board[6]) { // column matches
        return (game->board[0] == P1_MARK) ? score : -score;
    } else if (game->board[1] == game->board[4] && game->board[4] == game->board[7]) { // column matches
        return (game->board[1] == P1_MARK) ? score : -score;
    } else if (game->board[2] == game->board[5] && game->board[5] == game->board[8]) { // column matches
        return (game->board[2] == P1_MARK) ? score : -score;
    } else if (game->board[0] == game->board[4] && game->board[4] == game->board[8]) { // diagonal matches
        return (game->board[0] == P1_MARK) ? score : -score;
    } else if (game->board[2] == game->board[4] && game->board[4] == game->board[6]) { // diagonal matches
        return (game->board[2] == P1_MARK) ? score : -score;
    } else {
        return 0;  // return of 0 means keep playing
    }
}

/**
 * @brief Determines if there are moves left in the game to be made or not.
 * 
 * @param game The current game of TicTacToe being played.
 * @return True if there are no moves left to be made, false otherwise. 
 */
int check_draw(const struct TTT_Game *game) {
    int i;
    /* Check each board square */
    for (i = 0; i < sizeof(game->board); i++) {
        /* Check if current square has been played */
        if (game->board[i] == (i+1)+'0') return 0;
    }
    return 1;
}

/**
 * @brief Prints out the current state of the game board nicely formatted.
 * 
 * @param game The current game of TicTacToe being played.
 */
void print_board(const struct TTT_Game *game) {
    /*****************************************************************/
    /* Brute force print out the board and all the squares/values    */
    /*****************************************************************/
    /* Print header info */
    printf("\n\n\tTicTacToe Game #%d\n\n", game->gameNum);
    printf("Player 1 (%c)  -  Player 2 (%c)\n\n\n", P1_MARK, P2_MARK);
    /* Print current state of board */
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", game->board[0], game->board[1], game->board[2]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", game->board[3], game->board[4], game->board[5]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", game->board[6], game->board[7], game->board[8]);
    printf("     |     |     \n\n");
}

/**
 * @brief Determines whether a given move is legal (i.e. number 1-9) and valid (i.e. hasn't
 * already been played) for the current game.
 * 
 * @param choice The player move to be validated.
 * @param game The current game of TicTacToe being played.
 * @return True if the given move if valid based on the current board, false otherwise. 
 */
int validate_move(int choice, const struct TTT_Game *game) {
    /* Check to see if the choice is a move on the board */
    if (choice < 1 || choice > 9) {
        print_error("Invalid move: Must be a number [1-9]", 0, 0);
        return 0;
    }
    /* Check to see if the square chosen has a digit in it, if */
    /* square 8 has an '8' then it is a valid choice */
    if (game->board[choice-1] != (choice + '0')) {
        print_error("Invalid move: Square already taken", 0, 0);
        return 0;
    }
    return 1;
}

/**
 * @brief Sends Player 1's move to the remote player.
 * 
 * @param sd The socket descriptor of the server comminication endpoint.
 * @param game The current game of TicTacToe being played.
 * @return The move that was sent, or an error code if there was an issue. 
 */
int send_p1_move(int sd, struct TTT_Game *game) {
    struct Buffer datagram = {0};
    /* Get move to send to remote player */
    int move = find_best_move(game);
    while (!validate_move(move, game)) move = find_best_move(game);
    /* Pack move information into datagram */
    datagram.version = VERSION;
    datagram.command = MOVE;
    datagram.data = move + '0';
    datagram.gameNum = game->gameNum;
    /* Send the move to the remote player */
    printf("Server sent the move:  %c\n", datagram.data);
    if (sendto(sd, &datagram, sizeof(struct Buffer), 0, (struct sockaddr *)&game->p2Address, sizeof(struct sockaddr_in)) < 0) {
        print_error("send_p1_move", errno, 0);
        return ERROR_CODE;
    }
    return (datagram.data - '0');
}

/**
 * @brief Resets the current game for a new player.
 * 
 * @param game The current game of TicTacToe being played.
 */
void free_game(struct TTT_Game *game) {
    struct sockaddr_in blankAddr = {0};
    printf("Game #%d has ended. Resetting game for new player.\n", game->gameNum);
    /* Reset game attributes */
    game->timeout = TIMEOUT;
    game->p2Address = blankAddr;
    game->player = 0;
    /* Reset game board */
    init_shared_state(game);
}

/**
 * @brief Checks if the current game has ended, prints the appropriate message if so,
 * and resets the game for a new player.
 * 
 * @param game The current game of TicTacToe being played.
 * @return True if the game has ended, false otherwise. 
 */
int game_over(struct TTT_Game *game) {
    /* Check if somebody won the game */
    if (check_win(game) != 0) {
        /* Print final game board and winning player */
        print_board(game);
        printf("==>\a Player %d wins\n", game->player);
    } else if (check_draw(game)) {
        /* Print final game board and that the game was a draw */
        print_board(game);
        printf("==>\a It's a draw\n");
    } else {
        return 0;
    }
    /* Reset the game for a new player */
    free_game(game);
    return 1;
}

/**
 * @brief Plays a simple game of TicTacToe with a remoye player that ends when either
 * someone wins, there is a draw, or the remote player leaves the game.
 * 
 * @param sd The socket descriptor of the server comminication endpoint.
 */
void tictactoe(int sd) {
    int waitPrompt = 1;
    struct TTT_Game gameRoster[MAX_GAMES] = {{0}};
    command_handler commands[] = {new_game, move};

    /* Initialize all games and server timeout time */
    init_game_roster(gameRoster);
    set_timeout(sd, TIMEOUT);
    /* Play all the games */
    while (1) {
        int rv;
        time_t start, stop;
        struct sockaddr_in playerAddr = {0};
        struct Buffer datagram = {0};
        /* Start clock for elapsed time from last command */
        if (waitPrompt) printf("[+]Waiting for another player to issue a command...\n");
        start = time(NULL);
        /* Wait for a command to be received */
        if ((rv = get_command(sd, &playerAddr, &datagram)) > 0) {
            int i, gameIndx = (datagram.command == NEW_GAME) ? find_open_game(gameRoster) : datagram.gameNum-1;
            commands[(int)datagram.command](sd, &playerAddr, &datagram, (gameIndx < 0) ? NULL : &gameRoster[gameIndx]);
            /* Stop clock for elapsed time from last command and update timeout clock for each ongoing game */
            stop = time(NULL);
            for (i = 0; i < MAX_GAMES; i++) {
                /* Check if game is being played and update timeout clock if so */
                if (gameRoster[i].player != 0) gameRoster[i].timeout -= difftime(stop, start);
                /* Reset timout clock for game that just received the command */
                if (i == gameIndx) gameRoster[gameIndx].timeout = TIMEOUT;
            }
            /* Reset any game that has timed out */
            check_timeout(gameRoster);
            waitPrompt = 1;
        } else if (rv == 0) {   // server has timed out
            /* Check if any games are currently being played */
            if (games_in_progress(gameRoster) > 0) {
                print_error("tictactoe: Nobody has responded in a while. Resetting game states", 0, 0);
                /* Reset all games */
                init_game_roster(gameRoster);
                waitPrompt = 1;
            } else {
                waitPrompt = 0;
            }
        }
    }
}
