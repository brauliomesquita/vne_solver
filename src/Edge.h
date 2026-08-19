#ifndef EDGE_H
#define EDGE_H

class Edge {
    int id, orig, dest;
    double bw, delay;

  public:

  	Edge();
	Edge (int id, int o, int d, double b, double del);
     int getId();    
     int getOrig();    
     int getDest();    
     double getBW();    
     double getDelay();
};

#endif