#include <iostream>
#include <cassert>
#include <fstream>
#include <string.h>
#include <string>
#include <cstring>
#include <sstream>
#include <list>


#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wdigraph.h"
#include "dijkstra.h"
#define _MSG_MAX_LENGTH 45
struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the id of the point that is closest to the given point "pt"
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// read the graph from the file that has the same format as the "Edmonton graph" file
void readGraph(const string& filename, WDigraph& g, unordered_map<int, Point>& points) {
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // start new string
        ++at;
      }
      else {
        // append character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // new Point
      int id = stoi(p[1]);
      assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    }
    else {
      // new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}

int create_and_open_fifo(const char * pname, int mode) {
  // creating a fifo special file in the current working directory
  // with read-write permissions for communication with the plotter
  // both proecsses must open the fifo before they can perform
  // read and write operations on it
  if (mkfifo(pname, 0666) == -1) {
    cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
    exit(-1);
  }

  // opening the fifo for read-only or write-only access
  // a file descriptor that refers to the open file description is
  // returned
  int fd = open(pname, mode);

  if (fd == -1) {
    cout << "Error: failed on opening named pipe." << endl;
    exit(-1);
  }

  return fd;
}

// keep in mind that in part 1, the program should only handle 1 request
// in part 2, you need to listen for a new request the moment you are done
// handling one request

/*
  remove the dot from a given string

  Arguement: string coords
*/

void clean(string &coords){   
  int noise = coords.find("."); // find the index of the "."
  if(noise != -1) coords.erase(noise,1);  // remove it.

}

int main() {
  WDigraph graph;
  unordered_map<int, Point> points;

  const char *inpipe = "inpipe";
  const char *outpipe = "outpipe";

  // Open the two pipes
  int in = create_and_open_fifo(inpipe, O_RDONLY);
  cout << "inpipe opened..." << endl;
  int out = create_and_open_fifo(outpipe, O_WRONLY);
  cout << "outpipe opened..." << endl;  

  // build the graph
  readGraph("server/edmonton-roads-2.0.1.txt", graph, points);

  // read requests
  while(true){
    char msg[_MSG_MAX_LENGTH] = {0};     // initialize msg with NULL
    read(in, msg, _MSG_MAX_LENGTH);      // get the msg from inpipe

    if (strcmp("Q\n",msg) == 0) break;  // if Q is inputed, quit
    string output = "";      

    string sPoint_lat_str, sPoint_lon_str, ePoint_lat_str, ePoint_lon_str;

    string str_msg;
    str_msg = msg;          // convert char message to string

    string slat, slon, elat, elon;
    stringstream ss(str_msg);
    ss >> slat >> slon >> elat >> elon; // from the string, input each of the coords

    clean(slat); clean(slon); clean(elat); clean(elon); // remove the dots 
  
    Point sPoint, ePoint;
    sPoint.lat = static_cast<long long>(stod(slat))/10; // convert the coords to float, then long long. Remove the last decimal
    sPoint.lon = static_cast<long long>(stod(slon))/10;
    ePoint.lat = static_cast<long long>(stod(elat))/10;
    ePoint.lon = static_cast<long long>(stod(elon))/10;

      // c is guaranteed to be 'R', no need to error check

      // get the points closest to the two points we read
      int start = findClosest(sPoint, points), end = findClosest(ePoint, points);

      // run dijkstra's algorithm, this is the unoptimized version that
      // does not stop when the end is reached but it is still fast enough
    unordered_map<int, PIL> tree;
    dijkstra(graph, start, tree);

      // NOTE: in Part II you will use a different communication protocol than Part I
      // So edit the code below to implement this protocol

    // no path
    if (tree.find(end) == tree.end()) {
      output = "";    // if there's no path, the output is nothing.
    }
    else {
      // read off the path by stepping back through the search tree
      list<int> path;
      while (end != start) {
        path.push_front(end);
        end = tree[end].first;
      }
      path.push_front(start);
      for (int v : path) {
        output = to_string(points[v].lat).insert(2,".") + 
            " "+ to_string(points[v].lon).insert(4,".") + "\n";   // loop through the path list to convert the checkpoints to string

        int n = output.length();    // the size of the output
        char char_output[n];    
        strcpy(char_output, output.c_str()); // convert string output to array 
        write(out, char_output, sizeof(char_output));// write the char array output the outpipe
      }
    }

    write(out, "E\n", 2); // print "E" to the end of the outpipe
    
  }
  close(in);
  close(out);

  unlink(inpipe);
  unlink(outpipe);
  return 0;
}
