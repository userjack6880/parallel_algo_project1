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

void server(int argc, char *argv[], int numProcessors) {

  // Check to make sure the server can run
  if(argc != 3) {
    cerr << "two arguments please!" << endl;
    MPI_Abort(MPI_COMM_WORLD,-1);
  }

  // output number of processors specified
  cout << "running on " << numProcessors << " processors" << endl;

  // Input case filename 
  ifstream input(argv[1],ios::in);

  // Output case filename
  ofstream output(argv[2],ios::out);
  
  int count = 0;
  int numGames = 0;
  int packetSize = 5; // some arbitrary number to start with

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
      for (int j = 0; j < IDIM*JDIM; j++)
        buf[j] = inputString[j];

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
    string inputStrings[numGames];

    // quick sanity check to see if breaking the probem up is worth doing
    if (numProcessors * packetSize > numGames) {
      // output warning and reduce packetSize to 1
      cout << "There are less games than initial problem breakdown. Reducing packet size to 1!" << endl;
      packetSize = 1;
    }

    // run through the input
    int firstRun = 1;
    int iteration = 0;
    while (gameIndex + packetSize < numGames) {
      // check to see if any clients have data for us, if it's not the first round
      if (!firstRun) {
        MPI_Request request;
        MPI_Status status;
        int flag = 0;
        int recvPacket;

        // cout << "waiting for data from clients" << endl;
        MPI_Irecv(&recvPacket, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        MPI_Test(&request, &flag, &status);

        // if there's something, let's get the rest of the data
        if (flag) {
          int* indexBuf = new int[recvPacket];
          int* solutionBuf = new int[recvPacket];
          int source = status.MPI_SOURCE;

          cout << "recieved data from client " << source << endl;
          MPI_Recv(&indexBuf, recvPacket, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Recv(&solutionBuf, recvPacket, MPI_INT, source, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          int die = 0;
          MPI_Send(&die, 1, MPI_INT, source, 0, MPI_COMM_WORLD);

          // increase the game index
          gameIndex += packetSize;

          // cleanup
          delete[] indexBuf;
          delete[] solutionBuf;

          iteration++;
          if (iteration == numProcessors) {
            break;
          }
        }
      }
      else {
        // get the data and send two packets to each client - an array of game indexes
        // and gameboards
        for (int i = 0; i < numProcessors; i++) {
          cout << "allocating initial data for client " << i + 1 << endl;

          // initialize data for MPI
          int indexBuf[packetSize];
          string stringBuf[packetSize];

          // get data for the packet size
          for (int j = 0; j < packetSize; j++) {
            input >> stringBuf[j];

            if (stringBuf[j].size() != IDIM*JDIM) {
              cerr << "something wrong in input file format!" << endl;
              MPI_Abort(MPI_COMM_WORLD, -1);
            }

            indexBuf[j] = gameIndex + j;
            inputStrings[indexBuf[j]] = stringBuf[j];
          }
          // package into character array
          char* buf;
          packageGames(&buf, stringBuf, packetSize);
          int dataSize = strlen(buf);

          // send it
          MPI_Send(&packetSize, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
          MPI_Send(indexBuf, packetSize, MPI_INT, i + 1, 1, MPI_COMM_WORLD);
          MPI_Send(&dataSize, 1, MPI_INT, i + 1, 2, MPI_COMM_WORLD);
          MPI_Send(buf, dataSize, MPI_CHAR, i + 1, 3, MPI_COMM_WORLD);
          cout << "data sent" << endl;

          // increase the game index
          gameIndex += packetSize;

          // cleanup memory
          delete[] buf;
        }
        firstRun = 0;
      }
    }

    cout << "Processed " << gameIndex << " games." << endl;
  }
  // Report how cases had a solution.
  cout << "found " << count << " solutions" << endl ;
}

// Put the code for the client here
void client(int myID) {
  while (1) {
    cout << "hi, I'm client " << myID << " and I'm hungry for data" << endl;

    // get data
    int packetSize;
    MPI_Recv(&packetSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // die if packet size is 0
    if (packetSize == 0) {
      cout << "client " << myID << ": I've been told to quit" << endl;
      break;
    }
    cout << "client " << myID << ": I got data" << endl;
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
