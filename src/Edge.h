#ifndef EDGE_H
#define EDGE_H

class Edge {
    int id, orig, dest;
    double bw, delay;

  public:

  	Edge();
	Edge (int id, int o, int d, double b, double del);
     int getId() const;
     int getOrig() const;
     int getDest() const;
     double getBW() const;
     double getDelay() const;
};

#endif
