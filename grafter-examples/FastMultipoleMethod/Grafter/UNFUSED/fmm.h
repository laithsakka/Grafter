/*************************************************************************************************
 * Copyright (C) 2017, Nikhil Hegde, Jianqiao Liu, Kirshanthan Sundararajah, Milind Kulkarni, and 
 * Purdue University. All Rights Reserved. See Copyright.txt
*************************************************************************************************/
#ifndef FMM_H
#define FMM_H
#include "fmm_types.h"
#include<stdio.h>

Vertex* CreateVertex();
int ConstructQuadTreeAndTraverse(Point* points, Box& boundingBox, int numPoints, bool hasMoreData);
int BuildSubTree_Quad(Vertex* subtreeRoot, Point* point, Vec& center, double dia, int depth, int DOR, bool clonePoint);
void TraverseUp(void* inParam, int start, int end, void* outParam);
void TraverseDown_Recursive(Vertex* node, int step);
void TraverseDownFused_Recursive(Vertex* node);
void GetInteractionList(Vertex* node, std::vector<Vertex*>& wellSeparatedNodes, bool neighbors);
bool AreAdjacent(Box& b1, Box& b2);
double KernelFn(double x1, double y1, double x2, double y2);
void parallel_for(thread_function func, int start, int end, void* inParam, void* outParam);
void read_point(FILE *in, Point *p, long int label);
void read_input(char* inputFile, Point* pointArr, long int numPoints);
int BuildSubTree_Quad(Vertex* subtreeRoot, Point* point, Vec& center, double dia, int depth, int DOR, bool clonePoint);
int ComputeChildNumber(const Vec& cofm, const Point* child);
void ComputeBoundingBoxParams(const Point* nbodies, long int numPoints, Vec& center, double& dia);
void ComputeBoundingBoxParams(Box& box, Vec& center, double& dia);
void GetLeafNodes(Vertex* node);

#ifdef METRICS
class subtreeStats
{
public:
	long int footprint;
	int numnodes;
	subtreeStats(){footprint=0;numnodes=0;}
};
void getSubtreeStats(Vertex* ver, subtreeStats* stats);
void printLoadDistribution();
#endif


#endif
