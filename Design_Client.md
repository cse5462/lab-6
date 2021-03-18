# TicTacToe Client (Player 2) Design
> This is the design document for the TicTacToe Client ([tictactoeClient.c](https://github.com/CSE-5462-Spring-2021/assignment5-conner-ben/blob/main/tictactoeClient.c)).  
> By: Ben Nagel

## Table of Contents
- TicTacToe Class Protocol - [Protocol Document](https://docs.google.com/document/d/1wq3D-pyuyNu0O_81yzq8HaUqDNXmpJbGD7M6_t26xgg/edit?usp=sharing)
- [Environment Constants](#environment-constants)
- [High-Level Architecture](#high-level-architecture)
- [Low-Level Architecturet](#low-level-architecture)

## Environment Constants
```C#
/* Define the number of rows and columns */
#define ROWS 3 
#define COLUMNS 3
/* The number of command line arguments. */
#define NUM_ARGS 3
```

## High-Level Architecture
At a high level, the client application takes in input from the user and trys to send a datagram to the server to start a game. If everything worked, it waits for the server to send the first move of tictactoe via datagram. Once the move was received, the client marks the move on the client board, if the move was valid, otherwise closes the connection. If the move was valid the client sends the server the next move and this processes continues until there is a winner or a tie.
```C
int main(int argc, char *argv[])
{
    struct buffer Buffer = {0};
    char board[ROWS][COLUMNS];
    int sd;
    struct sockaddr_in server_address;
    struct sockaddr_in troll;
    int portNumber;
    char serverIP[29];

    // check for two arguments
    if (argc != 3)
    {
        printf("Wrong number of command line arguments");
        printf("Input is as follows: tictactoeP2 <port-num> <ip-address>");
        exit(1);
    }
    // create the socket
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        printf("ERROR making the socket");
        exit(1);
    }
    else
    {
        printf("Socket Created\n");
    }

    portNumber = strtol(argv[1], NULL, 10);
    strcpy(serverIP, argv[2]);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    server_address.sin_addr.s_addr = inet_addr(serverIP);

    troll.sin_family = AF_INET;
    troll.sin_port = htons(4444);
    troll.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr *)&troll, sizeof(troll)) < 0)
    {
        printf("ERROR WITH SOCKET\n");
        exit(1);
    }

    // connnect to the sever
    socklen_t fromLength = sizeof(struct sockaddr);
    Buffer.version = 4;
    Buffer.command = 0;
    Buffer.seqNum = 0;
    if (sendto(sd, &Buffer, sizeof(Buffer), 0, (struct sockaddr *)&server_address, fromLength) < 0)
    {
        close(sd);
        perror("error    connecting    stream    socket");
        exit(1);
    }

    printf("Connected to the server!\n");
    initSharedState(board);                                   // Initialize the 'game' board
    tictactoe(board, sd, (struct sockaddr *)&server_address); // call the 'game'
    return 0;
}
```

## Low-Level Architecture
- Gets either player 1 choice or player 2's choice and makes sure the input is valid.
```C
 do
    {
         
        socklen_t fromLength=sizeof(struct sockaddr);
        int choice;
        print_board(board);            // call function to print the board on the screen
        player = (player % 2) ? 1 : 2; // Mod math to figure out who the player is
        if (player == 2)
        {
            printf("Player %d, enter a number:  ", player); // player 2 picks a spot
            scanf("%d", &input);    //using scanf to get the choice
            while(getchar()!='\n');
            while (input < 1 || input > 9)                  //makes sure the input is between 1-9
            {
                printf("Invalid input choose a number between 1-9.\n");
                printf("Player %d, enter a number:  ", player); // player 2 picks a spot
                
                scanf("%d", &input);
                while(getchar()!='\n');
            }
            pick = input + '0';
            player2.version=2;
            player2.move=1;
            player2.place=pick;
        }
        else
        {
            
            printf("Waiting for square selection from player 1..\n"); // gets chosen spot from player 1
            struct timeval time;
            time.tv_sec = 20;
            time.tv_usec = 0;

    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)) < 0) {
        printf("Error with setSocketopt\n");
        printf("Closing connection!\n");
        exit(1);
    }
            rc =recvfrom(sd,&player1,sizeof(player1),0,(struct sockaddr *)serverAdd,&fromLength);
            pick=player1.place;
            
            // checks to see if the connection was cut mid stream
            if (rc <= 0)
            {
                if( (errno == EAGAIN || errno == EWOULDBLOCK) ){
                    printf("Error: Player ran out of time to respond\n");
                    printf("Closing connection!\n");
                    exit(1);
                }
                printf("Connection lost!\n");
                printf("Closing connection!\n");
                exit(1);
            }
            if(player1.move==0|| player1.version!=2){
                printf("Player 1 sent invalid datagram\n");
                printf("Closing connection!\n");
                exit(1);
            }
        }
        choice = pick - '0'; // converts to a int
        if (player == 1)     // prints choices
        {
            printf("Player 1 picked: %d\n", choice);
        }
        else
        {
            printf("Player 2 picked: %d\n", choice);
        }
        mark = (player == 1) ? 'X' : 'O'; //depending on who the player is, either us x or o
        /******************************************************************/
        /** little math here. you know the squares are numbered 1-9, but  */
        /* the program is using 3 rows and 3 columns. We have to do some  */
        /* simple math to conver a 1-9 to the right row/column            */
        /******************************************************************/
        row = (int)((choice - 1) / ROWS);
        column = (choice - 1) % COLUMNS;

        /* first check to see if the row/column chosen is has a digit in it, if it */
        /* square 8 has and '8' then it is a valid choice                          */

        if (board[row][column] == (choice + '0'))
        {
            board[row][column] = mark;
            // sends player 2 chioce if it is valid on the board
            if (player == 2)
            {
                printf("version: %d, move %d, place %c, sd %d\n",player2.version,player2.move,player2.place,sd);

                rc=sendto(sd,&player2,sizeof(player2),0,(struct sockaddr *)serverAdd,fromLength);
                if (rc<0 )
                {
                    printf("%d\n",rc);
                    printf("Connection lost!\n");
                    printf("Closing connection!\n");
                    printf("Bye\n");
                    exit(1);
                }
            }
        }
        else
        {
            printf("Invalid move\n");
            if (player == 1)
            {
                printf("The spot picked is not empty\n");
                printf("Closing the game & connection\n");
                exit(1);
            }
            else if (player == 2)
            {
                printf("The spot picked is not empty\n");
                printf("Pick a new number\n");
                continue;
            }
            player--;
            getchar();
        }
        /* after a move, check to see if someone won! (or if there is a draw */
        i = checkwin(board);

        player++;
        // memset(pick, 0, 1);

    } while (i == -1); // -1 means no one won

```

