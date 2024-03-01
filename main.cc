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

bool findSolution(const string& inputString) {
  unsigned char buf[IDIM*JDIM];
  inputBuffer(inputString, buf);

  // initialize the game
  game_state = gameBoard;
  gameBoard.Init(buf);

  // search for a solution
  move solution[IDIM*JDIM];
  int size = 0;
  bool found = depthFirstSearch(gameBoard, size, solution);

  return found;
}

void sendData(const int packetSize, const int gameIndex, const vector<string>& inputString, const int dest) {
  // always send packetSize
  MPI_SEND(&packetSize, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);

  if (packetSize > 0) {
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
    MPI_Send(indexBuf, packetSize, MPI_INT, dest, 1, MPI_COMM_WORLD);
    MPI_Send(&dataSize, 1, MPI_INT, dest, 2, MPI_COMM_WORLD);
    MPI_Send(stringBuf, dataSize, MPI_CHAR, dest, 3, MPI_COMM_WORLD);
  }
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
  int packetSize = 2;
  int increasePacket = 0;
  int killedClients = 0;

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

  // initialize tracking
  int gameIndex = 0;
  int solutions[numGames] = {0};
  vector<string> inputString(numGames);

  // run through the input and save each game into input vector
  for (int i = 0; i < numGames; i++) {
    inputString[i] = readInput(input);
  }

  // now server loop
  while (1) {
    MPI_Request request;
    MPI_Status status;
    int flag = 0;
    int recvPacket;

    // check to see if there's data from a client
    MPI_Irecv(&recvPacket, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
    MPI_Test(&request, &flag, &status);

    if (!flag) {
      // data not recieved, do wait routine
      while (!flag) {
        // check if problem exists
        if (gameIndex < numGames) {
          // if there are less than packetSize amount of games, only do one game
          if (numGames - gameIndex < packetSize) {
            packetSize = 1;
          }

          // do work
          for (int i = 0; i < packetSize; i++) {
            findSolution(inputString[gameIndex+i]);

            // record solution if found
            if (found) {
              solutions[gameIndex] = 1;
            }
          }

          // increment game idnex
          gameIndex += packetSize;
        }

        // check to see if there's data waiting
        MPI_Test(&request, &flag, &status);
      }
    }
    else {
      // if the clients are too fast, then there's not enough for the clients to do
      if(increasePacket && recvPacket > 0) {
        packetSize++;
      }
    }

    // now that we know there's something for us, let's interpet it
    if (flag) {
      int source = status.MPI_SOURCE;

      if (recvPacket == -1) {
        // -1 means the client died
        killedClients++;

        if (killedClients == numProcessors - 1) {
          // if all clients are killed, break out of the loop
          break;
        }
        else {
          // if not all are killed, next loop iteration
          continue;
        }
      }
      if (recvPacket > 0) {
        // this isn't the first run, so we have more data to get and record
        int indexBuf[recvPacket];
        int solutionBuf[recvPacket];

        // recieve the rest of the data sent back to us
        MPI_Recv(indexBuf, recvPacket, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(solutionBuf, recvPacket, MPI_INT, source, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // put record the solution states
        for (int i = 0; i < recvPacket; i++) {
          solutions[indexBuf[i]] = solutionBuf[i];
        }
      }

      // check if problems exist
      if (gameIndex < numGames) {
        // if there are less than packetSize amount of games, only do one game
        if (numGames - gameIndex < packetSize) {
          packetSize = 1;
        }
      }
      else {
        // if there are no problems left, send a 0 packet
        packetSize = 0;
      }

      // send data to the client that responded
      sendData(packetSize, gameIndex, inputString, source);

      // increase the game index
      gameIndex += packetSize;
    }
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
      inputBuffer(inputString[i],buf);

      game_state gameBoard;
      gameBoard.Init(buf);

      // solutions
      move solution[IDIM*JDIM];
      int size = 0;

      // we're only working on boards with found solutions, so we can just run
      // the search function without caring if it's found
      depthFirstSearch(gameBoard, size, solution);

      // now output to file
      outputSolution(buf, output, solution, size);
      count++;
    }
  }

  cout << "Processed " << gameIndex << " games with maximum packet size of " << maxPacket << "." << endl;

  // Report how cases had a solution.
  cout << "found " << count << " solutions" << endl ;
}

// Put the code for the client here
void client(int myID) {
  // initialize packetSize
  int packetSize = 0;
  int *indexBuf = nullptr;
  int *solutionBuf = nullptr;
  int dataSize;

  while (1) {
    // send data
    MPI_Send(&packetSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    if (packetSize > 0) {
      // send the rest of the data if it's not dead or first run
      MPI_Send(&packetSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Send(indexBuf, packetSize, MPI_INT, 0, 1, MPI_COMM_WORLD);
      MPI_Send(solutionBuf, packetSize, MPI_INT, 0, 2, MPI_COMM_WORLD);

      // dealocate memory
      delete[] indexBuf;
      delete[] solutionBuf;

      // reset variable
      indexBuf = nullptr;
      solutionBuf = nullptr;
    }

    if (packetSize == -1) {
      // dealocate memory
      delete[] indexBuf;
      delete[] solutionBuf;

      // break out of the client loop
      break;
    }

    // get message
    MPI_Recv(&packetSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    // interpret message
    if (packetSize == 0) {
      packetSize = -1;
      continue;
    }

    // get data
    indexBuf = new int[packetSize];
    MPI_Recv(indexBuf, packetSize, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&dataSize, 1, MPI_Init, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    char buf[dataSize];
    MPI_Recv(buf, dataSize, MPI_CHAR, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // do work
    // unpackage the data
    string boardStates[packetSize];
    for (int i = 0; i < packetSize; i++) {
      boardStates[i].assign(buf + (IDIM*JDIM*i), IDIM*JDIM);
    }

    // process the data
    solutionBuf = new int[packetSize];

    for (int i = 0; i < packetSize; i++) {
      bool found = findSolution(boardStates[i]);

      // If a solution is found, mark it
      if (found) {
        solutionBuf[i] = 1;
      }
      else {
        solutionBuf[i] = 0;
      }
    }
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
