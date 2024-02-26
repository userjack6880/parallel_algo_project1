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

using std::ofstream;
using std::ifstream;
using std::ios;

void packageGames(char** buf, string input[], int packetSize) {
  // calculate length of the full packet
  size_t inputLength = 0;
  for (int i = 0; i < packetSize; i++) {
    inputLength += input[i].length() + 1;
  }

  // allocate memory
  *buf = new char[inputLength];
  if (*buf == nullptr) {
    cerr << "failed to allocate memory!" << endl;
    MPI_Abort(MPI_COMM_WORLD,-1);
  }

  // copy strings to buffer
  size_t offset = 0;
  for (int i = 0; i < packetSize; i++) {
    strncpy(*buf + offset, input[i].c_str(), input[i].length());
    offset += input[i].length();
  }
}

void sendData(int packetSize, ifstream &input, int &gameIndex, vector<string>& inputStrings, int dest) {
  // initialize data for MPI
  int indexBuf[packetSize];
  string stringBuf[packetSize];

  // get data for the packet size
  for (int i = 0; i < packetSize; i++) {
    input >> stringBuf[i];

    if (stringBuf[i].size() != IDIM*JDIM) {
      cerr << "something wrong in input file format!" << endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    indexBuf[i] = gameIndex + i;
    inputStrings[indexBuf[i]] = stringBuf[i];
  }
  // package into character array
  char* buf;
  packageGames(&buf, stringBuf, packetSize);
  int dataSize = strlen(buf);

  // send it
  MPI_Send(&packetSize, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
  MPI_Send(indexBuf, packetSize, MPI_INT, dest, 1, MPI_COMM_WORLD);
  MPI_Send(&dataSize, 1, MPI_INT, dest, 2, MPI_COMM_WORLD);
  MPI_Send(buf, dataSize, MPI_CHAR, dest, 3, MPI_COMM_WORLD);

  // increase the game index
  gameIndex += packetSize;

  // cleanup
  delete[] buf;
}

void server(int argc, char *argv[], int numProcessors) {

  // Check to make sure the server can run
  if(argc != 3) {
    cerr << "two arguments please!" << endl;
    MPI_Abort(MPI_COMM_WORLD,-1);
  }

  // output number of processors specified
  cout << "running on " << numProcessors << " child processors" << endl;

  // Input case filename 
  ifstream input(argv[1],ios::in);

  // Output case filename
  ofstream output(argv[2],ios::out);
  
  int count = 0;
  int numGames = 0;
  int packetSize = 1;
  int maxPacket = 0;

  // get the number of games from the input file
  input >> numGames;


  // if the number of processors is 0, the server is now the processor
  if (numProcessors + 1 == 1) {
    for (int i = 0; i < numGames; i++) {  // for each game in file...
      string inputString;
      input >> inputString;

      if (inputString.size() != IDIM*JDIM) {
        cerr << "something wrong in input file format!" << endl;
        MPI_Abort(MPI_COMM_WORLD,-1);
      }

      // read in the initial game state from file
      unsigned char buf[IDIM*JDIM];
      for (int j = 0; j < IDIM*JDIM; j++) {
        buf[j] = inputString[j];
      }

      // Here we search for the solution to the game.   This is where most of
      // the work is performed.  We will want to farm these tasks out to other
      // processors in the parallel version.  To do this, send buf[] to the
      // client processor and use game_board.Init() on the Client to initialize
      // game board.  The result that will need to be sent back will either be
      // the moves required to solve the game or an indication that the game was
      // not solvable.

      // initialize the game
      game_state gameBoard;
      gameBoard.Init(buf);

      // If we find a solution to the game, put the results in
      // solution
      move solution[IDIM*JDIM];
      int size = 0;

      // Search for a solution to the puzzle
      bool found = depthFirstSearch(gameBoard, size, solution);

      // If the solution is found we want to output how to solve the puzzle
      // in the results file.
      if (found) {
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
        count++;
      }
    }
  }
  // if it's greater than 1, then we can break it apart
  else {
    // get data and package into a packet
    int gameIndex = 0;
    int solutions[numGames] = {0};
    vector<string> inputStrings(numGames);

    // run through the input
    int firstRun = 1;
    while (1) {
      // first run
      if (firstRun) {
        // get the data and send two packets to each client - an array of game indexes
        // and gameboards
        for (int i = 0; i < numProcessors; i++) {
          sendData(packetSize, input, gameIndex, inputStrings, i + 1);
        }
        firstRun = 0;
      }
      else {
        MPI_Request request;
        MPI_Status status;
        int flag = 0;
        int recvPacket;

        // cout << "waiting for data from clients" << endl;
        MPI_Irecv(&recvPacket, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        MPI_Test(&request, &flag, &status);

        if (!flag) {
          // while we wait, go ahead and process one if there's at least two left
          if (gameIndex + 1 < numGames) {
            string inputString;
            input >> inputString;

            if (inputString.size() != IDIM*JDIM) {
              cerr << "something wrong in input file format!" << endl;
              MPI_Abort(MPI_COMM_WORLD,-1);
            }

            unsigned char buf[IDIM*JDIM];
            for (int i = 0; i < IDIM*JDIM; i++) {
              buf[i] = inputString[i];
            }

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
            inputStrings[gameIndex] = inputString;

            // increment the index
            gameIndex++;
          }

          // now wait
          MPI_Wait(&request, &status);
          flag = 1;
        }
        else {
          // if the clients are too fast, then there's not enough for the clients to do
          packetSize++;
        }

        // reduce packet size to 1 if gameIndex + packetSize would not be valid
        if (gameIndex + packetSize > numGames) {
          if (packetSize > maxPacket) {
            maxPacket = packetSize;
          }
          packetSize = 1;
        }

        // if there's something, let's get the rest of the data
        if (flag) {
          int bufIndex[recvPacket];
          int solutionBuf[recvPacket];
          int source = status.MPI_SOURCE;

          MPI_Recv(&bufIndex, recvPacket, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Recv(&solutionBuf, recvPacket, MPI_INT, source, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          // put record the solution states
          for (int i = 0; i < recvPacket; i++) {
            solutions[bufIndex[i]] = solutionBuf[i];
          }

          // send data back to the client
          sendData(packetSize, input, gameIndex, inputStrings, source);
        }

        // breakout of while loop if the game index has moved to the end
        if (gameIndex == numGames) {
          break;
        }
      }
    }

    // kill each child as they finish
    for (int i = 0; i < numProcessors; i++) {
      MPI_Request request;
      int flag = 0;
      int recvPacket;

      // cout << "waiting for data from clients" << endl;
      MPI_Recv(&recvPacket, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
 
      // if there's something, let's get the rest of the data
      int bufIndex[recvPacket];
      int solutionBuf[recvPacket];

      MPI_Recv(&bufIndex, recvPacket, MPI_INT, i + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&solutionBuf, recvPacket, MPI_INT, i + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      // put record the solution states
      for (int i = 0; i < recvPacket; i++) {
        solutions[bufIndex[i]] = solutionBuf[i];
      }

      // kill the child
      int kill = 0;
      MPI_Send(&kill, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
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
          buf[j] = inputStrings[i][j];
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
        output << "found solution = " << endl;
        gameBoard.Init(buf);
        gameBoard.Print(output);
        for (int j = 0; j < size; j++) {
          gameBoard.makeMove(solution[j]);
          output << "-->" << endl;
          gameBoard.Print(output);
        }
        output << "solved" << endl;

        count++;

        delete[] buf;
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
    int bufIndex[packetSize];
    MPI_Recv(&bufIndex, packetSize, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int dataSize;
    MPI_Recv(&dataSize, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    char* buf = new char[dataSize];
    MPI_Recv(buf, dataSize, MPI_CHAR, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int boardSize = dataSize / packetSize;

    // unpackage the data
    string boardStates[packetSize];
    for (int i = 0; i < packetSize; i++) {
      boardStates[i].assign(buf + (boardSize*i), boardSize);
    }

    // process the data
    int solutions[packetSize] = {0};

    for (int i = 0; i < packetSize; i++) {
      unsigned char boardState[IDIM*JDIM];
      for (int j = 0; j < IDIM*JDIM; j++) {
        boardState[j] = boardStates[i][j];
      }

      // initialize the game
      game_state gameBoard;
      gameBoard.Init(boardState);

      // Search for a solution to the puzzle
      move solution[IDIM*JDIM];
      int size = 0;
      bool found = depthFirstSearch(gameBoard, size, solution);

      // If a solution is found, mark it
      if (found) {
        solutions[i] = 1;
      }
      else {
        solutions[i] = 0;
      }
    }

    // send what we found back to the server
    MPI_Send(&packetSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&bufIndex, packetSize, MPI_INT, 0, 1, MPI_COMM_WORLD);
    MPI_Send(&solutions, packetSize, MPI_INT, 0, 2, MPI_COMM_WORLD);

    // cleanup memory
    delete[] buf;
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
    server(argc, argv, numProcessors-1); // the number of processors
                                         // excludes the server

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
