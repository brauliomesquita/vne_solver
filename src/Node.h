#ifndef NODE_H
#define NODE_H

class Node {
    
    int id;
    double x, y, cpu;
    
    public:
    Node(int id, double x, double y, double cpu);
    Node();
    int getId() const;
    double getX() const;
    double getY() const;
    double getCPU() const;

};

#endif
