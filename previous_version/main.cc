#include "game.h"
#include "utilities.h"
// Standard Includes for MPI, C and OS calls
#include <mpi.h>

// C++ standard I/O and library includes
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

// C++ stadard library using statements
using std::cout;
using std::cerr;
using std::endl;

using std::vector;
using std::string;
using std::stoi;

using std::ofstream;
using std::ifstream;
using std::ios;

string readInput(ifstream &input) {
  string inputString;
  input >> inputString;

  if (inputString.size() != IDIM*JDIM) {
    cerr << "something wrong in input file format!" << endl;
    MPI_Abort(MPI_COMM_WORLD,-1);
  }

  return inputString;
}

void inputBuffer(const string& inputString, unsigned char* buf) {
  for (int i = 0; i < IDIM*JDIM; i++) {
    buf[i] = inputString[i];
  }
}

void outputSolution(unsigned char* buf, ofstream& output, const move solution[], const int size) {
  output << "found solution = " << endl;
  game_state s;
  s.Init(buf);
  s.Print(output);
  for (int i = 0; i < size; i++) {
    s.makeMove(solution[i]);
    output << "-->" << endl;
    s.Print(output);
  }
  output << "solved" << endl;
}

void sendData(const int packetSize, const int gameIndex, const vector<string>& inputString, const int dest) {
  // initialize data for MPI
  int indexBuf[packetSize];
  unsigned char stringBuf[packetSize*IDIM*JDIM];

  // get data for the packet size
  size_t offset = 0;
  for (int i = 0; i < packetSize; i++) {
    // create string buffer
    for (int j = 0; j < IDIM*JDIM; j++) {
      stringBuf[j+offset] = inputString[gameIndex+i][j];
    }
    offset += IDIM*JDIM;

    // create index buffer
    indexBuf[i] = gameIndex + i;
  }
  int dataSize = sizeof(stringBuf);

  // send it
  MPI_Send(&packetSize, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
  MPI_Send(indexBuf, packetSize, MPI_INT, dest, 1, MPI_COMM_WORLD);
  MPI_Send(&dataSize, 1, MPI_INT, dest, 2, MPI_COMM_WORLD);
  MPI_Send(stringBuf, dataSize, MPI_CHAR, dest, 3, MPI_COMM_WORLD);
}

void server(int argc, char *argv[], int numProcessors) {

  // check to make sure the server can run
  if(argc < 3) {
    cerr << "not enough arguments" << endl;
    MPI_Abort(MPI_COMM_WORLD,-1);
  }

  // output number of processors specified
  cout << "running on " << numProcessors << " processors" << endl;

  // input case filename 
  ifstream input(argv[1],ios::in);

  // output case filename
  ofstream output(argv[2],ios::out);
  
  int count = 0;
  int numGames = 0;
  int packetSize = 1;
  int increasePacket = 0;
  int decreasePacket = 0;

  if (argc > 3) {
    packetSize = stoi(argv[3]);
    cout << "packet size " << packetSize << endl;
  }
  if (argc > 4) {
    increasePacket = stoi(argv[4]);
    cout << "increase packet: " << increasePacket << endl;
  }
  if (argc > 5) {
    decreasePacket = stoi(argv[5]);
    cout << "decrease packet: " << decreasePacket << endl;
  }

  int maxPacket = packetSize;

  // get the number of games from the input file
  input >> numGames;
  cout << numGames << " games" << endl;

  // if the number of processors is 0, the server is now the processor
  if (numProcessors == 1) {
    // run through each game
    for (int i = 0; i < numGames; i++) {
      // read in a game
      string inputString = readInput(input);

      // read in the initial game state from file
      unsigned char buf[IDIM*JDIM];
      inputBuffer(inputString, buf);

      // initialize the game
      game_state gameBoard;
      gameBoard.Init(buf);

      // variables to store solutions and size
      move solution[IDIM*JDIM];
      int size = 0;

      // search for a solution to the puzzle
      bool found = depthFirstSearch(gameBoard, size, solution);

      // if solution is found, output how to solve the puzzle
      if (found) {
        outputSolution(buf, output, solution, size);
        count++;
      }
    }
  }
  // if it's greater than 1, then we can break it apart
  else {
    // initialize tracking
    int gameIndex = 0;
    int solutions[numGames] = {0};
    vector<string> inputString(numGames);

    // run through the input and save each game into input vector
    for (int i = 0; i < numGames; i++) {
      inputString[i] = readInput(input);
    }

    // now run over each input string and pass to clients
    int firstRun = 1;
    while (1) {
      // first run
      if (firstRun) {
        // get the data and send packets to each client
        for (int i = 1; i < numProcessors; i++) {
          sendData(packetSize, gameIndex, inputString, i);
        }

        // increase the game index
        gameIndex += packetSize;

        firstRun = 0;
      }
      else {
        // every other run
        MPI_Request request;
        MPI_Status status;
        int flag = 0;
        int recvPacket;

        // check to see if there's data from a client
        MPI_Irecv(&recvPacket, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        MPI_Test(&request, &flag, &status);

        if (!flag) {
          // if we're waiting on a client, we have too much for them to do
          if ((packetSize > 1) && decreasePacket) {
            // record the max packet size
            if (packetSize > maxPacket) {
              maxPacket = packetSize;
            }
            packetSize--;
          }

          // while we wait, go ahead and process one if there's at least two left
          while (!flag) {
            // if this is not the last game, as that's handled outside of outer while
            if (gameIndex * numProcessors < numGames) {
              // read in the initial game state from the inputString vector
              unsigned char buf[IDIM*JDIM];
              inputBuffer(inputString[gameIndex], buf);

              // initialize game board
              game_state gameBoard;
              gameBoard.Init(buf);

              move solution[IDIM*JDIM];
              int size = 0;
              bool found = depthFirstSearch(gameBoard, size, solution);

              // put the results into the data
              if (found) {
                solutions[gameIndex] = 1;
              }
              else {
                solutions[gameIndex] = 0;
              }
              // increment the index
              gameIndex++;
            }

            // check to see if there's data waiting
            // this should set flag to 1 and break out of the while loop
            MPI_Test(&request, &flag, &status);
          }
        }
        else {
          // if the clients are too fast, then there's not enough for the clients to do
          if(increasePacket) {
            packetSize++;
          }
        }

        // reduce packet size to 1 if gameIndex + packetSize would not be valid
        if (gameIndex + packetSize > numGames) {
          // record the max packet size
          if (packetSize > maxPacket) {
            maxPacket = packetSize;
          }
          packetSize = 1;
        }

        // now that we know there's something for us, let's get the rest of the data
        if (flag) {
          int indexBuf[recvPacket];
          int solutionBuf[recvPacket];
          int source = status.MPI_SOURCE;

          // recieve the rest of the data sent back to us
          MPI_Recv(indexBuf, recvPacket, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Recv(solutionBuf, recvPacket, MPI_INT, source, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          // put record the solution states
          for (int i = 0; i < recvPacket; i++) {
            solutions[indexBuf[i]] = solutionBuf[i];
          }

          // send data back to the client
          sendData(packetSize, gameIndex, inputString, source);

          // increase the game index
          gameIndex += packetSize;
        }

        // breakout of while loop if the game index has moved to the end
        if (gameIndex == numGames) {
          break;
        }
      }
    }

    // kill each child as they finish
    for (int i = 1; i < numProcessors; i++) {
      MPI_Request request;
      int recvPacket;

      // wait for data to be recieved
      MPI_Recv(&recvPacket, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
 
      // if there's something, let's get the rest of the data
      int indexBuf[recvPacket];
      int solutionBuf[recvPacket];

      MPI_Recv(indexBuf, recvPacket, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(solutionBuf, recvPacket, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      // put record the solution states
      for (int j = 0; j < recvPacket; j++) {
        solutions[indexBuf[j]] = solutionBuf[j];
      }

      // kill the child
      int kill = 0;
      MPI_Send(&kill, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    // now we have to go back through every result and generate
    // solutions for problems that have a solution
    for (int i = 0; i < numGames; i++) {
      if (!solutions[i]) {
        continue;
      }
      else {
        // initialize the game board
        unsigned char buf[IDIM*JDIM];
        for (int j = 0; j < IDIM*JDIM; j++) {
          buf[j] = inputString[i][j];
        }

        game_state gameBoard;
        gameBoard.Init(buf);

        // solutions
        move solution[IDIM*JDIM];
        int size = 0;

        // we're only working on boards with found solutions, so we can just run
        // the search function
        depthFirstSearch(gameBoard, size, solution);

        // now output to file
        outputSolution(buf, output, solution, size);
        count++;
      }
    }

    cout << "Processed " << gameIndex << " games with maximum packet size of " << maxPacket << "." << endl;
  }

  // Report how cases had a solution.
  cout << "found " << count << " solutions" << endl ;
}

// Put the code for the client here
void client(int myID) {
  while (1) {
    // get data
    int packetSize;
    MPI_Recv(&packetSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // die if packet size is 0
    if (packetSize == 0) {
      break;
    }
    int indexBuf[packetSize];
    MPI_Recv(indexBuf, packetSize, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int dataSize;
    MPI_Recv(&dataSize, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    char buf[dataSize];
    MPI_Recv(buf, dataSize, MPI_CHAR, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int boardSize = dataSize / packetSize;

    // unpackage the data
    string boardStates[packetSize];
    for (int i = 0; i < packetSize; i++) {
      boardStates[i].assign(buf + (boardSize*i), boardSize);
    }

    // process the data
    int solutionBuf[packetSize] = {0};

    for (int i = 0; i < packetSize; i++) {
      unsigned char boardState[IDIM*JDIM];
      inputBuffer(boardStates[i], boardState);

      // initialize the game
      game_state gameBoard;
      gameBoard.Init(boardState);

      // Search for a solution to the puzzle
      move solution[IDIM*JDIM];
      int size = 0;
      bool found = depthFirstSearch(gameBoard, size, solution);

      // If a solution is found, mark it
      if (found) {
        solutionBuf[i] = 1;
      }
      else {
        solutionBuf[i] = 0;
      }
    }

    // send what we found back to the server
    MPI_Send(&packetSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(indexBuf, packetSize, MPI_INT, 0, 1, MPI_COMM_WORLD);
    MPI_Send(solutionBuf, packetSize, MPI_INT, 0, 2, MPI_COMM_WORLD);
  }
}

int main(int argc, char *argv[]) {
  // This is a utility routine that installs an alarm to kill off this
  // process if it runs to long.  This will prevent jobs from hanging
  // on the queue keeping others from getting their work done.
  chopsigs_();
  
  // All MPI programs must call this function
  MPI_Init(&argc, &argv);

  
  int myID;
  int numProcessors;

  /* Get the number of processors and my processor identification */
  MPI_Comm_size(MPI_COMM_WORLD, &numProcessors);
  MPI_Comm_rank(MPI_COMM_WORLD, &myID);

  if (myID == 0) {
    // Processor 0 runs the server code
    get_timer(); // zero the timer
    server(argc, argv, numProcessors);

    // Measure the running time of the server
    cout << "execution time = " << get_timer() << " seconds." << endl;
  } 
  else {
    // all other processors run the client code.
    client(myID);
  }

  // All MPI programs must call this before exiting
  MPI_Finalize();
}