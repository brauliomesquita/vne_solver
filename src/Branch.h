#ifndef BRANCH_H
#define BRANCH_H

class Branch {
public:
	Branch();
	void set(int tipo, int v, int x, int y);

	// 1 => nó
	// 2 => aresta
	// 3 => y
	int tipo_branch;

	int v;
	int x;
	int y;

	int valor;
	
};

#endif