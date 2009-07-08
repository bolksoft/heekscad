// SolveSketch.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Sketch.h"
#include "../interface/ObjList.h"
#include "SolveSketch.h"
#include "ConstrainedObject.h"
#include "EndedObject.h"
#include "HArc.h"
#include "HCircle.h"
#include "../sketchsolve/src/solve.h"

//This file traverses the tree associated with a sketch and creates and linear
//data structure suitable for use by sketch solve. If the solver is succesful,
//the new points are written into the corresponding heeksobj's. 


arc GetArc(HArc* a);
circle GetCircle(HCircle* a);
line GetLineFromEndedObject(EndedObject* eobj);
point GetPoint(HPoint* point);
void AddPointConstraints(HPoint* point);
int solvewpoints(double  **parms,int nparms, constraint * cons, int consLength, int isFine);

std::vector<double*> params;
std::set<double*> paramset;
std::vector<constraint> constraints;
std::set<Constraint*> cons;

void debugprint(std::string s)
{
	wxLogDebug(wxString(s.c_str(),wxConvUTF8));
}

void SolveSketch(CSketch* sketch)
{
	SolveSketch(NULL,sketch,NULL);
}

void SolveSketch(CSketch* sketch, HeeksObj* dragged, void* whichpoint)
{
	params.clear();
	paramset.clear();
	constraints.clear();
	cons.clear();

	//Try going up the tree until we get a sketch
	HeeksObj* psketch = dragged;
	sketch = dynamic_cast<CSketch*>(psketch);
	while(psketch && !sketch)
	{
		psketch = psketch->Owner();
		sketch = dynamic_cast<CSketch*>(psketch);
	}

	if(!sketch)
		return; //couldn't find one

	HeeksObj* obj = sketch->GetFirstChild();
	while(obj)
	{
		ConstrainedObject* cobj = (dynamic_cast<ConstrainedObject*>(obj));
		EndedObject *eobj = (dynamic_cast<EndedObject*>(obj));
		if(cobj)
		{
			cobj->LoadToDoubles();
			if(obj->GetType() == ArcType)
			{
				arc a = GetArc((HArc*)obj);
				constraint c;
				c.type = arcRules;
				c.arc1 = a;
				constraints.push_back(c);
			}

			if(cobj->absoluteangleconstraint)
			{
				line l = GetLineFromEndedObject(eobj);
				constraint c;
				c.line1 = l;
				c.type = vertical;
				if(cobj->absoluteangleconstraint->m_angle == AbsoluteAngleHorizontal)
					c.type = horizontal;
				constraints.push_back(c);
			}

			if(cobj->linelengthconstraint)
			{
				line l = GetLineFromEndedObject(eobj);
				constraint c;
				c.line1 = l;
				c.type = lineLength;
				c.parameter = &cobj->linelengthconstraint->m_length;
				constraints.push_back(c);
			}

			if(cobj->radiusconstraint)
			{
				arc a = GetArc((HArc*)obj);
				constraint c;
				c.arc1 = a;
				c.type = arcRadius;
				c.parameter = &cobj->radiusconstraint->m_length;
				constraints.push_back(c);
			}

			if(eobj)
			{
				if(eobj->A)
					AddPointConstraints(eobj->A);
				if(eobj->B)
					AddPointConstraints(eobj->B);
			}
	
			std::list<Constraint*>::iterator it;
			for(it = cobj->constraints.begin(); it!=cobj->constraints.end(); ++it)
			{
				//check if constraint is already processed
				//uses set of pointers for easy comparison
				Constraint *con = *it;
				if(cons.find(con)!=cons.end())
					continue;

				switch(con->m_type)
				{
					case EqualRadiusConstraint:
					case ConcentricConstraint:
					{ 
						constraint c;
						bool has_arc = false;
						c.type = equalRadiusCircArc;

						if(dynamic_cast<HArc*>(con->m_obj1))
						{
							c.arc1 = GetArc((HArc*)con->m_obj1);
							has_arc = true;
						}
						else
							c.circle1 = GetCircle((HCircle*)con->m_obj1);

						if(dynamic_cast<HArc*>(con->m_obj2))
							if(has_arc)
							{
								c.arc2 = GetArc((HArc*)con->m_obj2);
								c.type = equalRadiusArcs;
							}
							else
								c.arc1 = GetArc((HArc*)con->m_obj2);
						else
							if(has_arc)
								c.circle1 = GetCircle((HCircle*)con->m_obj2);
							else
							{
								c.circle2 = GetCircle((HCircle*)con->m_obj2);
								c.type = equalRadiusCircles;
							}

						if(con->m_type == ConcentricConstraint)
						{
							switch(c.type)
							{	
								case equalRadiusCircArc: c.type = concentricCircArc; break;
								case equalRadiusArcs: c.type = concentricArcs; break;
								case equalRadiusCircles: c.type = concentricCircles; break;
							}
						}
						cons.insert(con);
						constraints.push_back(c);
					}
					break;

					case PointOnArcMidpointConstraint:
					case PointOnArcConstraint:
					{ 
						constraint c;
						c.arc1 = GetArc((HArc*)con->m_obj1);
						c.point1 = GetPoint((HPoint*)con->m_obj2);
						c.type = pointOnArc;
						if(con->m_type == PointOnArcMidpointConstraint)
							c.type = pointOnArcMidpoint;
						cons.insert(con);
						constraints.push_back(c);
					}
					break;

					case LineTangentConstraint:
					{
						constraint c;
						c.type = tangentToArc;
						if(dynamic_cast<HArc*>(con->m_obj1))
							c.arc1 = GetArc((HArc*)con->m_obj1);
						else
						{
							c.circle1 = GetCircle((HCircle*)con->m_obj1);
							c.type = tangentToCircle;
						}
						c.line1 = GetLineFromEndedObject((EndedObject*)con->m_obj2);
						cons.insert(con);
						constraints.push_back(c);
					}
					break;
					case PointOnLineMidpointConstraint:
					case PointOnLineConstraint:
					{
						constraint c;
						c.type = pointOnLine;
						if(con->m_type == PointOnLineMidpointConstraint)
							c.type = pointOnLineMidpoint;
						c.point1 = GetPoint((HPoint*)con->m_obj2);
						c.line1 = GetLineFromEndedObject((EndedObject*)con->m_obj1);
						constraints.push_back(c);
					}
					break;
					case ParallelLineConstraint:
					case PerpendicularLineConstraint:
					case EqualLengthConstraint:
					case ColinearConstraint:
					{
						line l = GetLineFromEndedObject(eobj);
						constraint c;
						c.line1 = l;
											
						if((ConstrainedObject*)con->m_obj1 == cobj)
							c.line2 = GetLineFromEndedObject((EndedObject*)con->m_obj2);
						else
							c.line2 = GetLineFromEndedObject((EndedObject*)con->m_obj1);
						switch(con->m_type)
						{
							case ParallelLineConstraint: c.type = parallel; break;
							case PerpendicularLineConstraint: c.type = perpendicular; break;
							case EqualLengthConstraint: c.type = equalLegnth; break;
							case ColinearConstraint: c.type = colinear; break;
						}
						constraints.push_back(c);
						cons.insert(con);
					}
					break;
				}
			}
		}
		obj = sketch->GetNextChild();
	}

	//Get the soft constraint
	if(dragged)
	{
		EndedObject* eobj = dynamic_cast<EndedObject*>(dragged);
		if(whichpoint == &eobj->A)
		{
			//was point A
		}
		else
		{
			//was point B
		}
	}

//	if(constraints.size() == 0)
		// no contraints //still might be arcs
//		return;

	if(solvewpoints(&params[0],params.size(),&constraints[0],constraints.size(),fine))
		//No result
	{return;}

	obj = sketch->GetFirstChild();
	while(obj)
	{
		ConstrainedObject* cobj = (dynamic_cast<ConstrainedObject*>(obj));
		EndedObject *eobj = (EndedObject*)obj;
		if(cobj)
		{
			eobj->LoadFromDoubles();
		}
		obj = sketch->GetNextChild();
	}
	
}

void PushBack(double *v)
{
	if(paramset.find(v) == paramset.end())
	{
		params.push_back(v);
		paramset.insert(v);
	}
}

void AddPointConstraints(HPoint* point)
{
	std::list<Constraint*>::iterator it;
	for(it = point->constraints.begin(); it!= point->constraints.end(); ++it)
	{
		Constraint* con = *it;
		if(con->m_type == CoincidantPointConstraint)
		{
			constraint c;
			c.type = pointOnPoint;
			c.point1 = GetPoint((HPoint*)con->m_obj1);
			c.point2 = GetPoint((HPoint*)con->m_obj2);
			constraints.push_back(c);
			cons.insert(con);
		}
	}
}

point GetPoint(HPoint* obj)
{
		obj->LoadToDoubles();
		point p;
		p.x = &obj->mx; p.y = &obj->my;

		PushBack(p.x);
		PushBack(p.y);

		return p;
}


line GetLineFromEndedObject(EndedObject* eobj)
{
		eobj->LoadToDoubles();
		point p;
		p.x = &eobj->A->mx; p.y = &eobj->A->my;
		PushBack(p.x);
		PushBack(p.y);
		point p2;
		p2.x = &eobj->B->mx; p2.y = &eobj->B->my;
		PushBack(p2.x);
		PushBack(p2.y);

		line l;
		l.p1 = p;
		l.p2 = p2;

		return l;
}

arc GetArc(HArc* a)
{

	arc ret;
	ret.start = GetPoint(a->A);
	ret.end = GetPoint(a->B);
	ret.center = GetPoint(a->C);

	return ret;
}

circle GetCircle(HCircle* a)
{
	circle ret;
	ret.center = GetPoint(a->C);
	ret.rad = &a->m_radius;
	PushBack(ret.rad);

	return ret;
}