#include "Branch.h"

Branch::Branch(){
}

void Branch::set(int tipo, int v, int x, int y){
	this->tipo_branch = tipo;
	this->v = v;
	this->x = x;
	this->y = y;
}