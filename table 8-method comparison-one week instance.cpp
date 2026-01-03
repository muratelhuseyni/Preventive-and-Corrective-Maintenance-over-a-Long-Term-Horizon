#include "Common.h"
#include "Heuristic.h"
#include <iostream>
#include <fstream>
#include <string>
#include <fstream>
#include <algorithm>
#include <functional>  
#include <array>
#include <vector>
#include <numeric> 
#include <map>
#include <math.h>
#include <random>
using namespace std;

struct Job
{
	int ID;
	int rj;
	int Ymax;
	int dj;
	int pj;

	Job(int IDin, int rjIn, int YmaxIn, int djIn, int pjIn)
	{
		ID = IDin;
		rj = rjIn;
		Ymax = YmaxIn;
		dj = djIn;
		pj = pjIn;
	}
};

struct Base
{
	int id;
	int jobid;
	int starttime;
	int length;
	int finish;
	int parid;
	int type;
	int rj;

	Base(int IDIn, int jobIDIn, int startIn, int lengthIn, int finishIn, int typeIn, int rjIn/*int paridIn*/)
	{
		id = IDIn;
		jobid = jobIDIn;
		starttime = startIn;
		length = lengthIn;
		finish = finishIn;
		type = typeIn;
		rj = rjIn;
		//parid = paridIn;
	}

	Base()
	{
	}
};

struct ParentChild
{
	int parid;
	int childid;

	ParentChild(int parIDIn, int childIDIn)
	{
		parid = parIDIn;
		childid = childIDIn;
	}
};

struct desccpj
{
	inline bool operator() (int*& a1, int*& a2) //ascorder
	{
		return a1[2] > a2[2];
	}
};

struct ascdj
{
	inline bool operator() (int*& a1, int*& a2) //ascorder
	{
		return a1[1] < a2[1];
	}
};

struct ascrj
{
	inline bool operator() (Job& a1, Job& a2) //ascorder
	{
		return a1.rj < a2.rj;
	}
};

struct ascstart
{
	inline bool operator() (Base& a1, Base& a2) //ascorder
	{
		return a1.starttime< a2.starttime;
	}
};


typedef vector<vector<Job>> JobInt;

int Getmin(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

void GetChildren(const vector<ParentChild>& ParChildAs, const Base& rootbase, vector<Base>& Children, const vector<Base>& BaseAs)
{
	for each (ParentChild parchild in ParChildAs)
	{
		if (rootbase.id == parchild.parid)
		{
			for each (Base b in BaseAs)
			{
				if (b.id == parchild.childid)
				{
					Children.push_back(b);
					break;
				}
			}
		}
	}

}

vector <Base> GetDescendants(Base& rootbase, int trackid, vector<ParentChild>& ParChildAs, vector<Base>& BaseAs, int m)
{
	vector<Base> Children;
	//base case
	if (trackid == m - 1)
	{
		Children.push_back(rootbase);
		return Children;
	}

	GetChildren(ParChildAs, rootbase, Children, BaseAs);
	vector<Base> tempChildren;
	//tempChildren = Children;

	for (auto& child : Children)
	{
		vector<Base> temp = GetDescendants(child, trackid + 1, ParChildAs, BaseAs, m);
		tempChildren.reserve(tempChildren.size() + distance(temp.begin(), temp.end()));
		tempChildren.insert(tempChildren.end(), temp.begin(), temp.end());
	}

	tempChildren.push_back(rootbase);
	return tempChildren;
}

void DatatoFile(int* SLAdaily, int n, int totaltt, int day, const vector<Job>& Jcurcor, const vector<Job>& Jcurprv, int** prevprdtms, int preventnum, int m, ofstream& dataday)
{
	for (int t = 0; t < m; ++t)
	{
		if (prevprdtms[t][0] != -1)
		{
			int job = prevprdtms[t][0];
			int type = prevprdtms[t][1];
			int delay = prevprdtms[t][2];

			dataday << job << "\t" << type << "\t" << delay << "\t" << t*preventnum << "\n";
		}
	}

	for each(Job job in Jcurcor)
		dataday << job.ID << "\t" << job.pj << "\t" << job.rj << "\t" << job.dj << "\t"
		<< job.Ymax << "\n";

	dataday << "\n";
	for each(Job job in Jcurprv)
		dataday << job.ID << "\t" << job.pj << "\t" << job.rj << "\t" << job.dj << "\t"
		<< job.Ymax << "\n";

}

struct ascpj
{
	inline bool operator() (int*& a1, int*& a2) //ascorder
	{
		return a1[1] < a2[1];
	}
};

void MIPstart4(IloNumVarArray& startVar, IloNumArray& startVal, IloModel& model, Heuristic* heur, int s, int n, const int tt, int m,
	const BoolVarArray2& y, const IloNumVarArray& P, const BoolVarArray3& x, const BoolVarArray3& e, const BoolVarArray3& a, const BoolVarArray3& b,
	const NumVarArray2& E, const NumVarArray2& T, const NumVarArray2& C, const NumVarArray2& Sj, const IloNumVarArray& S,
	const IloNumVarArray& F, const NumVarArray3& L, const NumVarArray3& B, const BoolVarArray2& np,
	const BoolVarArray3& o, const IloNumVarArray& deltaYmaxplus, const IloNumVarArray& deltaminusSLA, vector<int>& preventives, vector<int>& correctives, vector<int>& tardyjobs, int* r, int* pj, int* d, int* Ymax, int* SLA)
{
	heur->SortBase(heur->BaseAs, "jobid");
	int realjobnumber = n;
	n = preventives.size() + correctives.size();
	s = n*m;
	sort(preventives.begin(), preventives.end());
	//sort(correctives.begin(), correctives.end());
	vector<vector<int>> ajt; // tt+1
	vector<vector<int>> ojt;

	vector<int> jobs;
	for (int i = 0; i < heur->BaseAs.size(); ++i)
		jobs.push_back(heur->BaseAs[i].jobid);

	sort(jobs.begin(), jobs.end());

	int type;

	for (int i = 0; i < static_cast<int>(heur->BaseAs.size()); i++)
	{
		int j = heur->BaseAs[i].jobid;
		int start = heur->BaseAs[i].starttime;
		int finish = heur->BaseAs[i].finish;

		if (binary_search(preventives.begin(), preventives.end(), j))
			type = 0;
		else
			type = 1;

		int size = tt + 2;
		int* ajvals = new int[size];
		ajvals[0] = j;
		ajvals[1] = type;
		int* ojvals = new int[size];
		ojvals[0] = j;
		ojvals[1] = type;

		for (int t = 0; t < tt; t++)
		{
			//bu blok tamam!
			if (t < start)
			{
				startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(0);
				ajvals[t + 2] = 0;
				startVar.add(e[type][j][t]);
				startVal.add(0);
				/*model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 0);
				model.add(e[type][j][t] == 0);*/
			}

			//problem?

			else if (t == start)
			{
				startVar.add(b[type][j][t]);
				startVal.add(1);
				startVar.add(e[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(1);

				/*model.add(b[type][j][t] == 1);
				model.add(a[type][j][t] == 1);
				model.add(e[type][j][t] == 0);*/
			}


			else if (t > start && t < finish)
			{
				startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(e[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(1);
				/*model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 1);
				model.add(e[type][j][t] == 0);*/
			}


			else if (t == finish)
			{
				startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(e[type][j][t]);
				startVal.add(1);
				startVar.add(a[type][j][t]);
				startVal.add(0);
				ajvals[t + 2] = 0;
				/*model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 0);
				model.add(e[type][j][t] == 1);*/
			}

			else if (t > finish + 1)
			{
				startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(e[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(0);
				/*ajvals[t + 2] = 0;
				model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 0);
				model.add(e[type][j][t] == 0);*/
			}

		}

		//int b = heur->BaseAs[i].id;

		int b = heur->BaseAs[i].id;
		startVar.add(S[b]);
		startVal.add(heur->BaseAs[i].starttime);
		startVar.add(P[b]);
		startVal.add(heur->BaseAs[i].length);
		startVar.add(F[b]);
		startVal.add(heur->BaseAs[i].finish);
		startVar.add(C[type][j]);
		startVal.add(finish);
		startVar.add(Sj[type][j]);
		startVal.add(start);


		/*model.add(S[b] == heur->BaseAs[i].starttime);
		model.add(P[b] == heur->BaseAs[i].length);
		model.add(F[b] == heur->BaseAs[i].finish);
		model.add(C[type][j] == finish);
		model.add(Sj[type][j] == start);*/

		int deltaymaxrhs = Ymax[j] - d[j];

		if (type == 0)
		{
			if (start < d[j])
			{
				int val = d[j] - start;
				startVar.add(E[0][j]);
				startVal.add(val);
				startVar.add(T[0][j]);
				startVal.add(0);
				startVar.add(deltaYmaxplus[j]);
				startVal.add(0);
				/*model.add(E[0][j] == val);
				model.add(T[0][j] == 0);
				model.add(deltaYmaxplus[j] == 0);*/
			}

			else if (start > d[j])
			{
				/*int val = start - d[j];
				model.add(E[0][j] == 0);
				model.add(T[0][j] == val);
				if (val - deltaymaxrhs > 0)
				model.add(deltaYmaxplus[j] == val - deltaymaxrhs);
				else
				model.add(deltaYmaxplus[j] == 0);*/

				startVar.add(E[0][j]);
				startVal.add(0);
				int val = start - d[j];
				startVar.add(T[0][j]);
				startVal.add(val);
				if (val - deltaymaxrhs > 0)
				{
					startVar.add(deltaYmaxplus[j]);
					startVal.add(val - deltaymaxrhs);
				}

				else
				{
					startVar.add(deltaYmaxplus[j]);
					startVal.add(0);
				}
			}

			else
			{
				startVar.add(E[0][j]);
				startVal.add(0);
				startVar.add(T[0][j]);
				startVal.add(0);
				startVar.add(deltaYmaxplus[j]);
				startVal.add(0);
				/*model.add(E[0][j] == 0);
				model.add(T[0][j] == 0);
				model.add(deltaYmaxplus[j] == 0);*/
			}

			if (start > Ymax[j])
			{
				int baslangic;
				if (Ymax[j] < 0)
					baslangic = 0;
				else
					baslangic = Ymax[j];
				for (int t = 0; t < start; ++t)
				{
					if (t < baslangic)
					{
						ojvals[t + 2] = 0;
						/*ojvals[t + 2] = 0;
						startVar.add(o[0][j][t]);
						startVal.add(0);*/
						//model.add(o[0][j][t] == 0);
					}

					else
					{
						ojvals[t + 2] = 1;
						ojvals[t + 2] = 1;
						startVar.add(o[0][j][t]);
						startVal.add(1);
						//model.add(o[0][j][t] == 1);
					}
				}
			}
			else
			{
				for (int t = 0; t < tt; ++t)
				{
					ojvals[t + 2] = 0;
					ojvals[t + 2] = 0;
					startVar.add(o[0][j][t]);
					startVal.add(0);
					/*model.add(o[0][j][t] == 0);*/
				}

			}
		}

		if (type == 1)
		{
			for (int t = 0; t < start; ++t)
			{
				startVar.add(o[1][j][t]);
				startVal.add(1);
			}
		}

		/*model.add(x[type][b][j] == 1);
		model.add(L[type][b][j] == heur->BaseAs[i].finish);
		model.add(B[type][b][j] == heur->BaseAs[i].starttime);*/

		/*startVar.add(x[type][b][j]);
		startVal.add(1);
		startVar.add(L[type][b][j]);
		startVal.add(heur->BaseAs[i].finish);
		startVar.add(B[type][b][j]);
		startVal.add(heur->BaseAs[i].starttime);*/

		for (int t = 0; t < m; ++t)
			for (int k = t*n; k < (t + 1)*n - t; ++k)
			{
				if (k == b)
				{
					startVar.add(x[type][b][j]);
					startVal.add(1);
					startVar.add(L[type][b][j]);
					startVal.add(heur->BaseAs[i].finish);
					startVar.add(B[type][b][j]);
					startVal.add(heur->BaseAs[i].starttime);
					break;
				}

				/*startVar.add(x[type][k][j]);
				startVal.add(0);
				startVar.add(L[type][k][j]);
				startVal.add(0);
				startVar.add(B[type][k][j]);
				startVal.add(0);*/

				///*model.add(x[type][k][j] == 0);
				//model.add(L[type][k][j] == 0);
				//model.add(B[type][k][j] == 0);*/
			}

		vector<int> entry;
		//entry.resize(size);
		entry.assign(ajvals, ajvals + size);
		ajt.push_back(entry);
		delete[] ajvals;
		entry.clear();
		entry.assign(ojvals, ojvals + size);
		ojt.push_back(entry);
		delete[] ojvals;
		entry.clear();
	}

	//yab - Kab set
	map <int, int> parentjob;
	for (int j = 0; j < static_cast<int>(heur->ParChildAs.size()); j++)
	{
		/*array <int, 2> k = { heur->ParChildAs[j].parid, heur->ParChildAs[j].childid };
		parentjob.push_back(k);*/
		int b = heur->ParChildAs[j].parid;
		int a = heur->ParChildAs[j].childid;
		startVar.add(y[a][b]);
		startVal.add(1);
		parentjob[a] = b;

	}

	///at most one root
	//for (int t = 0; t < m - 1; ++t)
	//	for (int b = t*n; b < (t + 1)*n - t; ++b)
	//	{
	//		int dist = b - t*n + 1;
	//		for (int a = (t + 1)*n; a < (t + 2)*n - dist - t; ++a)
	//		{
	//			if (parentjob.size() > 0)
	//			{
	//				if (parentjob.find(a) == parentjob.end()) {
	//					// not found
	//					startVar.add(y[a][b]);
	//					startVal.add(0);
	//				}
	//				else {
	//					// found
	//					int par = parentjob[a];
	//					if (b == par)
	//						continue;
	//					else
	//					{
	//						startVar.add(y[a][b]);
	//						startVal.add(0);
	//					}
	//				}
	//			}
	//			else
	//			{
	//				startVar.add(y[a][b]);
	//				startVal.add(0);
	//			}
	//		}
	//	}

	//np_j=1 for unassigned jobs
	sort(jobs.begin(), jobs.end());
	for (int j : preventives)
	{
		if (binary_search(jobs.begin(), jobs.end(), j))
		{
			startVar.add(np[0][j]);
			startVal.add(0);
			/*model.add(np[0][j] == 0);*/
			continue;
		}

		//np=1

		int size = tt + 2;
		int* ajvals = new int[size];
		ajvals[0] = j;
		ajvals[1] = 0;
		int* ojvals = new int[size];
		ojvals[0] = j;
		ojvals[1] = 0;

		for (int t = 0; t < tt; t++)
		{
			startVar.add(b[0][j][t]);
			startVal.add(0);
			startVar.add(e[0][j][t]);
			startVal.add(0);
			startVar.add(a[0][j][t]);
			startVal.add(0);
			ajvals[t + 2] = 0;
			/*model.add(b[0][j][t] == 0);
			model.add(a[0][j][t] == 0);
			model.add(e[0][j][t] == 0);*/
		}

		if (d[j] > tt) // çizelge next periyotta olacak
		{
			startVar.add(E[0][j]);
			startVal.add(d[j] - tt);
			startVar.add(T[0][j]);
			startVal.add(0);
			startVar.add(deltaYmaxplus[j]);
			startVal.add(0);
			/*model.add(E[0][j] == d[j] - tt);
			model.add(T[0][j] == 0);
			model.add(deltaYmaxplus[j] == 0);*/
		}

		else //dj içeride
		{
			startVar.add(E[0][j]);
			startVal.add(0);
			int deltaymaxrhs = Ymax[j] - d[j];
			int val = tt - d[j];
			startVar.add(T[0][j]);
			startVal.add(val);

			if (val - deltaymaxrhs > 0)
			{
				startVar.add(deltaYmaxplus[j]);
				startVal.add(val - deltaymaxrhs);
			}

			else
			{
				startVar.add(deltaYmaxplus[j]);
				startVal.add(0);
			}
			/*int deltaymaxrhs = Ymax[j] - d[j];
			int val = tt - d[j];
			model.add(E[0][j] == 0);
			model.add(T[0][j] == val);
			if (val - deltaymaxrhs > 0)
			model.add(deltaYmaxplus[j] == val - deltaymaxrhs);
			else
			model.add(deltaYmaxplus[j] == 0);*/
		}

		//o_jt ve ymaxviol atamaları düşünülebilir
		startVar.add(np[0][j]);
		startVal.add(1);
		//model.add(np[0][j] == 1);
		//np ones
		startVar.add(C[0][j]);
		startVal.add(0);
		startVar.add(Sj[0][j]);
		startVal.add(0);

		//model.add(C[0][j] == 0);
		//model.add(Sj[0][j] == 0);

		if (tt > Ymax[j])
		{
			int baslangic;
			if (Ymax[j] < 0)
				baslangic = 0;
			else
				baslangic = Ymax[j];
			for (int t = 0; t < tt; ++t)
			{
				if (t < baslangic)
				{
					ojvals[t + 2] = 0;
					startVar.add(o[0][j][t]);
					startVal.add(0);
					/*ojvals[t + 2] = 0;
					startVar.add(o[0][j][t]);
					startVal.add(0);*/
					//model.add(o[0][j][t] == 0);
				}

				else
				{
					ojvals[t + 2] = 1;
					startVar.add(o[0][j][t]);
					startVal.add(1);
					/*startVar.add(o[0][j][t]);
					startVal.add(1);
					ojvals[t + 2] = 1;*/
					//model.add(o[0][j][t] == 1);
				}

			}
		}

		else
		{
			for (int t = 0; t < tt; ++t)
			{
				ojvals[t + 2] = 0;
				startVar.add(o[0][j][t]);
				startVal.add(0);
				/*startVar.add(o[0][j][t]);
				startVal.add(0);
				ojvals[t + 2] = 0;*/
				//model.add(o[0][j][t] == 0);
			}
		}

		vector<int> entry;
		//entry.resize(size);
		entry.assign(ajvals, ajvals + size);
		ajt.push_back(entry);
		delete[] ajvals;
		entry.clear();
		entry.assign(ojvals, ojvals + size);
		ojt.push_back(entry);
		delete[] ojvals;
		entry.clear();
	}

	for (int j : correctives)
	{
		if (binary_search(jobs.begin(), jobs.end(), j))
		{
			startVar.add(np[1][j]);
			startVal.add(0);
			//model.add(np[1][j] == 0);
			continue;
		}

		int size = tt + 2;
		int* ajvals = new int[size];
		ajvals[0] = j;
		ajvals[1] = 1;
		int* ojvals = new int[size];
		ojvals[0] = j;
		ojvals[1] = 1;

		//np=1 ones
		startVar.add(np[1][j]);
		startVal.add(1);
		startVar.add(C[1][j]);
		startVal.add(0);
		startVar.add(Sj[1][j]);
		startVal.add(0);
		/*model.add(np[1][j] == 1);
		model.add(C[1][j] == 0);
		model.add(Sj[1][j] == 0);*/

		if (d[j] > tt)
		{
			startVar.add(T[1][j]);
			startVal.add(0);
			//model.add(T[1][j] == 0);
		}

		else //dj içeride
		{
			int val = tt - d[j];
			startVar.add(T[1][j]);
			startVal.add(val);
			//model.add(T[1][j] == val);
		}


		for (int t = 0; t < tt; t++)
		{
			startVar.add(b[1][j][t]);
			startVal.add(0);
			startVar.add(e[1][j][t]);
			startVal.add(0);
			startVar.add(a[1][j][t]);
			startVal.add(0);
			//ajvals[t + 2] = 0;
			/*model.add(b[1][j][t] == 0);
			model.add(a[1][j][t] == 0);
			model.add(e[1][j][t] == 0);*/
		}

		if (tt > d[j])
		{
			int baslangic;
			if (d[j] < 0)
				baslangic = 0;
			else
				baslangic = d[j];
			for (int t = 0; t < tt; ++t)
			{
				if (t < baslangic)
				{
					ojvals[t + 2] = 0;
					startVar.add(o[1][j][t]);
					startVal.add(0);
					/*ojvals[t + 2] = 0;
					startVar.add(o[1][j][t]);
					startVal.add(0);*/
					//model.add(o[1][j][t] == 0);
				}

				else
				{
					ojvals[t + 2] = 1;
					startVar.add(o[1][j][t]);
					startVal.add(1);
					/*ojvals[t + 2] = 1;
					startVar.add(o[1][j][t]);
					startVal.add(1);*/
					//model.add(o[1][j][t] == 1);
				}
			}
		}

		vector<int> entry;
		//entry.resize(size);
		entry.assign(ajvals, ajvals + size);
		ajt.push_back(entry);
		delete[] ajvals;
		entry.clear();
		entry.assign(ojvals, ojvals + size);
		ojt.push_back(entry);
		delete[] ojvals;
		entry.clear();
	}

	////deltasla value entries
	for (int t = 0; t < tt; ++t)
	{
		int sumoanda = 0;
		for (int i = 0; i < ajt.size(); ++i)
		{
			int val = ajt[i][t + 2];
			if (val < 0)
				val = 0;
			/*cout << val << "\n";*/
			sumoanda += val;
		}


		for (int i = 0; i < ojt.size(); ++i)
		{
			int val = ojt[i][t + 2];
			int type = ojt[i][1];
			int j = ojt[i][0];
			if (val < 0)
			{
				/*startVar.add(o[type][j][t]);
				startVal.add(0);*/
				//model.add(o[type][j][t] == 0);
				val = 0;
				/*cout << val << "\n";*/
				sumoanda += val;
			}

		}
	}

}

void MIPstart41(IloNumVarArray& startVar, IloNumArray& startVal, IloModel& model, Heuristic* heur, int s, int n, const int tt, int m,
	const BoolVarArray2& y, const IloNumVarArray& P, const BoolVarArray3& x, const BoolVarArray3& e, const BoolVarArray3& a, const BoolVarArray3& b,
	const NumVarArray2& E, const NumVarArray2& T, const NumVarArray2& C, const NumVarArray2& Sj, const IloNumVarArray& S,
	const IloNumVarArray& F, const NumVarArray3& L, const NumVarArray3& B, const BoolVarArray2& np,
	const BoolVarArray3& o, const IloNumVarArray& deltaYmaxplus, const IloNumVarArray& deltaminusSLA, vector<int>& preventives, vector<int>& correctives, vector<int>& tardyjobs, int* r, int* pj, int* d, int* Ymax, int* SLA)
{
	heur->SortBase(heur->BaseAs, "jobid");
	int realjobnumber = n;
	n = preventives.size() + correctives.size();
	s = n*m;
	sort(preventives.begin(), preventives.end());
	//sort(correctives.begin(), correctives.end());
	vector<vector<int>> ajt; // tt+1
	vector<vector<int>> ojt;

	vector<int> jobs;
	for (int i = 0; i < heur->BaseAs.size(); ++i)
		jobs.push_back(heur->BaseAs[i].jobid);

	sort(jobs.begin(), jobs.end());

	int type;

	for (int i = 0; i < static_cast<int>(heur->BaseAs.size()); i++)
	{
		int j = heur->BaseAs[i].jobid;

		int start = heur->BaseAs[i].starttime;
		int finish = heur->BaseAs[i].finish;

		if (binary_search(preventives.begin(), preventives.end(), j))
			type = 0;
		else
			type = 1;

		for (int t = 0; t < tt; t++)
		{
			//bu blok tamam!
			if (t < start)
			{
				/*startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(0);
				ajvals[t + 2] = 0;
				startVar.add(e[type][j][t]);
				startVal.add(0);*/
				model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 0);
				model.add(e[type][j][t] == 0);
			}

			//problem?

			else if (t == start)
			{
				/*startVar.add(b[type][j][t]);
				startVal.add(1);
				startVar.add(e[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(1);*/

				model.add(b[type][j][t] == 1);
				model.add(a[type][j][t] == 1);
				model.add(e[type][j][t] == 0);
			}


			else if (t > start && t < finish)
			{
				/*startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(e[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(1);*/
				model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 1);
				model.add(e[type][j][t] == 0);
			}


			else if (t == finish)
			{
				/*startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(e[type][j][t]);
				startVal.add(1);
				startVar.add(a[type][j][t]);
				startVal.add(0);*/
				model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 0);
				model.add(e[type][j][t] == 1);
			}

			else if (t > finish + 1)
			{
				/*startVar.add(b[type][j][t]);
				startVal.add(0);
				startVar.add(e[type][j][t]);
				startVal.add(0);
				startVar.add(a[type][j][t]);
				startVal.add(0);*/
				model.add(b[type][j][t] == 0);
				model.add(a[type][j][t] == 0);
				model.add(e[type][j][t] == 0);
			}

		}

		int b = heur->BaseAs[i].id;


		model.add(x[type][b][j] == 1);
		model.add(L[type][b][j] == heur->BaseAs[i].finish);
		model.add(B[type][b][j] == heur->BaseAs[i].starttime);



		model.add(S[b] == heur->BaseAs[i].starttime);
		model.add(P[b] == heur->BaseAs[i].length);
		model.add(F[b] == heur->BaseAs[i].finish);
		model.add(C[type][j] == finish);
		model.add(Sj[type][j] == start);

		int deltaymaxrhs = Ymax[j] - d[j];

		int size = tt + 2;
		int* ajvals = new int[size];
		ajvals[0] = j;
		ajvals[1] = type;
		int* ojvals = new int[size];
		ojvals[0] = j;
		ojvals[1] = type;



		if (type == 0)
		{
			if (start < d[j])
			{
				int val = d[j] - start;
				/*startVar.add(E[0][j]);
				startVal.add(val);
				startVar.add(T[0][j]);
				startVal.add(0);
				startVar.add(deltaYmaxplus[j]);
				startVal.add(0);*/
				model.add(E[0][j] == val);
				model.add(T[0][j] == 0);
				model.add(deltaYmaxplus[j] == 0);
			}

			else if (start > d[j])
			{
				int val = start - d[j];
				model.add(E[0][j] == 0);
				model.add(T[0][j] == val);
				if (val - deltaymaxrhs > 0)
					model.add(deltaYmaxplus[j] == val - deltaymaxrhs);
				else
					model.add(deltaYmaxplus[j] == 0);

				/*startVar.add(E[0][j]);
				startVal.add(0);
				int val = start - d[j];
				startVar.add(T[0][j]);
				startVal.add(val);
				if (val - deltaymaxrhs > 0)
				{
				startVar.add(deltaYmaxplus[j]);
				startVal.add(val - deltaymaxrhs);
				}

				else
				{
				startVar.add(deltaYmaxplus[j]);
				startVal.add(0);
				}*/
			}

			else
			{
				/*startVar.add(E[0][j]);
				startVal.add(0);
				startVar.add(T[0][j]);
				startVal.add(0);
				startVar.add(deltaYmaxplus[j]);
				startVal.add(0);*/
				model.add(E[0][j] == 0);
				model.add(T[0][j] == 0);
				model.add(deltaYmaxplus[j] == 0);
			}

			if (start > Ymax[j])
			{
				int baslangic;
				if (Ymax[j] < 0)
					baslangic = 0;
				else
					baslangic = Ymax[j];
				for (int t = 0; t < start; ++t)
				{
					if (t < baslangic)
					{
						ojvals[t + 2] = 0;
						/*ojvals[t + 2] = 0;
						startVar.add(o[0][j][t]);
						startVal.add(0);*/
						model.add(o[0][j][t] == 0);
					}

					else
					{
						/*ojvals[t + 2] = 1;
						ojvals[t + 2] = 1;
						startVar.add(o[0][j][t]);
						startVal.add(1);*/
						model.add(o[0][j][t] == 1);
					}
				}
			}
			else
			{
				for (int t = 0; t < tt; ++t)
				{
					/*ojvals[t + 2] = 0;
					ojvals[t + 2] = 0;
					startVar.add(o[0][j][t]);
					startVal.add(0);*/
					model.add(o[0][j][t] == 0);
				}

			}
		}

		if (type == 1)
		{
			if (start > d[j])
			{
				int baslangic;
				if (d[j] < 0)
					baslangic = 0;
				else
					baslangic = d[j];
				for (int t = 0; t < start; ++t)
				{
					if (t < baslangic)
					{
						ojvals[t + 2] = 0;
						/*startVar.add(o[1][j][t]);
						startVal.add(0);*/
						/*startVar.add(o[1][j][t]);
						startVal.add(0);
						ojvals[t + 2] = 0;*/
						/*if (t == baslangic - 1)
						continue;
						model.add(o[1][j][t] == 0);*/
						model.add(o[1][j][t] == 0);
					}

					else
					{
						/*startVar.add(o[1][j][t]);
						startVal.add(1);*/
						/*ojvals[t + 2] = 1;
						startVar.add(o[1][j][t]);
						startVal.add(1);*/
						model.add(o[1][j][t] == 1);
					}
				}
			}
		}

		/*model.add(x[type][b][j] == 1);
		model.add(L[type][b][j] == heur->BaseAs[i].finish);
		model.add(B[type][b][j] == heur->BaseAs[i].starttime);*/

		/*startVar.add(x[type][b][j]);
		startVal.add(1);
		startVar.add(L[type][b][j]);
		startVal.add(heur->BaseAs[i].finish);
		startVar.add(B[type][b][j]);
		startVal.add(heur->BaseAs[i].starttime);*/



		//vector<int> entry;
		////entry.resize(size);
		//entry.assign(ajvals, ajvals + size);
		//ajt.push_back(entry);
		//delete[] ajvals;
		//entry.clear();
		//entry.assign(ojvals, ojvals + size);
		//ojt.push_back(entry);
		//delete[] ojvals;
		//entry.clear();
	}

	//yab - Kab set
	map <int, int> parentjob;
	for (int j = 0; j < static_cast<int>(heur->ParChildAs.size()); j++)
	{
		/*array <int, 2> k = { heur->ParChildAs[j].parid, heur->ParChildAs[j].childid };
		parentjob.push_back(k);*/
		int b = heur->ParChildAs[j].parid;
		int a = heur->ParChildAs[j].childid;
		/*startVar.add(y[a][b]);
		startVal.add(1);*/

		model.add(y[a][b] == 1);
		parentjob[a] = b;

	}

	///at most one root
	//for (int t = 0; t < m - 1; ++t)
	//	for (int b = t*n; b < (t + 1)*n - t; ++b)
	//	{
	//		int dist = b - t*n + 1;
	//		for (int a = (t + 1)*n; a < (t + 2)*n - dist - t; ++a)
	//		{
	//			if (parentjob.size() > 0)
	//			{
	//				if (parentjob.find(a) == parentjob.end()) {
	//					// not found
	//					startVar.add(y[a][b]);
	//					startVal.add(0);
	//				}
	//				else {
	//					// found
	//					int par = parentjob[a];
	//					if (b == par)
	//						continue;
	//					else
	//					{
	//						startVar.add(y[a][b]);
	//						startVal.add(0);
	//					}
	//				}
	//			}
	//			else
	//			{
	//				startVar.add(y[a][b]);
	//				startVal.add(0);
	//			}
	//		}
	//	}

	//np_j=1 for unassigned jobs
	sort(jobs.begin(), jobs.end());
	for (int j : preventives)
	{
		if (binary_search(jobs.begin(), jobs.end(), j))
		{
			/*startVar.add(np[0][j]);
			startVal.add(0);*/
			model.add(np[0][j] == 0);
			continue;
		}

		//np=1

		int size = tt + 2;
		int* ajvals = new int[size];
		ajvals[0] = j;
		ajvals[1] = 0;
		int* ojvals = new int[size];
		ojvals[0] = j;
		ojvals[1] = 0;

		for (int t = 0; t < tt; t++)
		{
			/*startVar.add(b[0][j][t]);
			startVal.add(0);
			startVar.add(e[0][j][t]);
			startVal.add(0);
			startVar.add(a[0][j][t]);
			startVal.add(0);*/
			ajvals[t + 2] = 0;
			model.add(b[0][j][t] == 0);
			model.add(a[0][j][t] == 0);
			model.add(e[0][j][t] == 0);
		}

		if (d[j] > tt) // çizelge next periyotta olacak
		{
			/*startVar.add(E[0][j]);
			startVal.add(d[j] - tt);
			startVar.add(T[0][j]);
			startVal.add(0);
			startVar.add(deltaYmaxplus[j]);
			startVal.add(0);*/
			model.add(E[0][j] == d[j] - tt);
			model.add(T[0][j] == 0);
			model.add(deltaYmaxplus[j] == 0);
		}

		else //dj içeride
		{
			/*startVar.add(E[0][j]);
			startVal.add(0);
			int deltaymaxrhs = Ymax[j] - d[j];
			int val = tt - d[j];
			startVar.add(T[0][j]);
			startVal.add(val);

			if (val - deltaymaxrhs > 0)
			{
			startVar.add(deltaYmaxplus[j]);
			startVal.add(val - deltaymaxrhs);
			}

			else
			{
			startVar.add(deltaYmaxplus[j]);
			startVal.add(0);
			}*/
			int deltaymaxrhs = Ymax[j] - d[j];
			int val = tt - d[j];
			model.add(E[0][j] == 0);
			model.add(T[0][j] == val);
			if (val - deltaymaxrhs > 0)
				model.add(deltaYmaxplus[j] == val - deltaymaxrhs);
			else
				model.add(deltaYmaxplus[j] == 0);
		}

		//o_jt ve ymaxviol atamaları düşünülebilir
		/*startVar.add(np[0][j]);
		startVal.add(1);*/
		model.add(np[0][j] == 1);
		//np ones
		/*startVar.add(C[0][j]);
		startVal.add(0);
		startVar.add(Sj[0][j]);
		startVal.add(0);*/

		model.add(C[0][j] == 0);
		model.add(Sj[0][j] == 0);

		if (tt > Ymax[j])
		{
			int baslangic;
			if (Ymax[j] < 0)
				baslangic = 0;
			else
				baslangic = Ymax[j];
			for (int t = 0; t < tt; ++t)
			{
				if (t < baslangic)
				{
					ojvals[t + 2] = 0;
					/*startVar.add(o[0][j][t]);
					startVal.add(0);*/
					/*ojvals[t + 2] = 0;
					startVar.add(o[0][j][t]);
					startVal.add(0);*/
					model.add(o[0][j][t] == 0);
				}

				else
				{
					ojvals[t + 2] = 1;
					/*startVar.add(o[0][j][t]);
					startVal.add(1);*/
					/*startVar.add(o[0][j][t]);
					startVal.add(1);
					ojvals[t + 2] = 1;*/
					model.add(o[0][j][t] == 1);
				}

			}
		}

		else
		{
			for (int t = 0; t < tt; ++t)
			{
				ojvals[t + 2] = 0;
				/*startVar.add(o[0][j][t]);
				startVal.add(0);*/
				/*startVar.add(o[0][j][t]);
				startVal.add(0);
				ojvals[t + 2] = 0;*/
				model.add(o[0][j][t] == 0);
			}
		}

		vector<int> entry;
		//entry.resize(size);
		entry.assign(ajvals, ajvals + size);
		ajt.push_back(entry);
		delete[] ajvals;
		entry.clear();
		entry.assign(ojvals, ojvals + size);
		ojt.push_back(entry);
		delete[] ojvals;
		entry.clear();
	}

	for (int j : correctives)
	{
		if (binary_search(jobs.begin(), jobs.end(), j))
		{
			/*startVar.add(np[1][j]);
			startVal.add(0);*/
			model.add(np[1][j] == 0);
			continue;
		}

		int size = tt + 2;
		int* ajvals = new int[size];
		ajvals[0] = j;
		ajvals[1] = 1;
		int* ojvals = new int[size];
		ojvals[0] = j;
		ojvals[1] = 1;

		//np=1 ones
		/*startVar.add(np[1][j]);
		startVal.add(1);
		startVar.add(C[1][j]);
		startVal.add(0);
		startVar.add(Sj[1][j]);
		startVal.add(0);*/
		model.add(np[1][j] == 1);
		model.add(C[1][j] == 0);
		model.add(Sj[1][j] == 0);

		if (d[j] > tt)
		{
			/*startVar.add(T[1][j]);
			startVal.add(0);*/
			model.add(T[1][j] == 0);
		}

		else //dj içeride
		{
			int val = tt - d[j];
			/*startVar.add(T[1][j]);
			startVal.add(val);*/
			model.add(T[1][j] == val);
		}


		for (int t = 0; t < tt; t++)
		{
			/*startVar.add(b[1][j][t]);
			startVal.add(0);
			startVar.add(e[1][j][t]);
			startVal.add(0);
			startVar.add(a[1][j][t]);
			startVal.add(0);*/
			//ajvals[t + 2] = 0;
			model.add(b[1][j][t] == 0);
			model.add(a[1][j][t] == 0);
			model.add(e[1][j][t] == 0);
		}

		if (tt > d[j])
		{
			int baslangic;
			if (d[j] < 0)
				baslangic = 0;
			else
				baslangic = d[j];
			for (int t = 0; t < tt; ++t)
			{
				if (t < baslangic)
				{
					ojvals[t + 2] = 0;
					/*startVar.add(o[1][j][t]);
					startVal.add(0);*/
					/*ojvals[t + 2] = 0;
					startVar.add(o[1][j][t]);
					startVal.add(0);*/
					model.add(o[1][j][t] == 0);
				}

				else
				{
					ojvals[t + 2] = 1;
					/*startVar.add(o[1][j][t]);
					startVal.add(1);*/
					/*ojvals[t + 2] = 1;
					startVar.add(o[1][j][t]);
					startVal.add(1);*/
					model.add(o[1][j][t] == 1);
				}
			}
		}

		vector<int> entry;
		//entry.resize(size);
		entry.assign(ajvals, ajvals + size);
		ajt.push_back(entry);
		delete[] ajvals;
		entry.clear();
		entry.assign(ojvals, ojvals + size);
		ojt.push_back(entry);
		delete[] ojvals;
		entry.clear();
	}

	////deltasla value entries
	for (int t = 0; t < tt; ++t)
	{
		int sumoanda = 0;
		for (int i = 0; i < ajt.size(); ++i)
		{
			int val = ajt[i][t + 2];
			if (val < 0)
				val = 0;
			/*cout << val << "\n";*/
			sumoanda += val;
		}


		for (int i = 0; i < ojt.size(); ++i)
		{
			int val = ojt[i][t + 2];
			int type = ojt[i][1];
			int j = ojt[i][0];
			if (val < 0)
			{
				/*startVar.add(o[type][j][t]);
				startVal.add(0);*/
				//model.add(o[type][j][t] == 0);
				val = 0;
				/*cout << val << "\n";*/
				sumoanda += val;
			}

		}
	}

}


void DeleteJobsbuffer(vector<Job>& Jint, vector<Job>& Jcor, IloCplex& cplex, BoolVarArray3& x, BoolVarArray2& np, NumVarArray2& C, NumVarArray2& Sj, IloNumVarArray& S, IloNumVarArray& F, int** prevprdtms, int preventnum, int m, vector<vector<Base>> cand)
{
	//cplex'i çıkar, diğer BaseAs'r göre oluşturduğun yeni başlangıç amanlarını ekle, buraya bir arguman olarak gelsin ve o argümanın içinden  bilgileri al
	vector<Job> tempJcurprv = Jint;
	vector<int> occupiedtracks;
	//......

	for each(vector<Base> bases in cand)
		for each(Base bas in bases)
		{
			int j = bas.jobid;
			int finish = bas.finish;
			int start = bas.starttime;
			int type = bas.type;
			int b = bas.id;
			int length = bas.length;

			int size = static_cast<int>(Jint.size());

			if (type == 0)
			{
				if (finish <= 24)
				{
					for (int i = 0; i < size; ++i)
					{
						if (j == Jint[i].ID)
						{
							swap(Jint[i], Jint.back());
							Jint.pop_back();
							break;
						}
					}
					continue;
				}

				else if (start < 24)
				{
					int t = floor(b / preventnum);

					prevprdtms[t][0] = j;
					prevprdtms[t][1] = 0;

					//if (start + length <)

					prevprdtms[t][3] = start + length - 24;


					for (int i = 0; i < size; ++i)
					{
						if (j == Jint[i].ID)
						{
							/*if (start + length <= 24.05)
							prevprdtms[t][2] = finish - 24;
							else
							prevprdtms[t][2] = start + length - 24;*/

							//Jint[i].pj = prevprdtms[t][2];
							if (start + length <= 24.05)
								prevprdtms[t][2] = 1; //sembolik
							else
								prevprdtms[t][2] = start + length - 24;

							Jint[i].pj = prevprdtms[t][2];

							break;
						}
					}

					occupiedtracks.push_back(t);
				}

			}

			else
			{
				size = Jcor.size();

				if (finish <= 24)
				{
					for (int i = 0; i < size; ++i)
					{
						if (j == Jcor[i].ID)
						{
							swap(Jcor[i], Jcor.back());
							Jcor.pop_back();
							break;
						}
					}
					//continue;
				}

				else if (start < 24)
				{
					int t = floor(b / preventnum);

					prevprdtms[t][0] = j;
					prevprdtms[t][1] = 1;

					prevprdtms[t][3] = start + length - 24;

					for (int i = 0; i < size; ++i)
					{
						if (j == Jcor[i].ID)
						{
							int pj = Jcor[i].pj;

							/*if (start + length <= 24.05)
							prevprdtms[t][2] = finish - 24;
							else
							prevprdtms[t][2] = start + length - 24;*/

							//Jcor[i].pj = prevprdtms[t][2];*/

							if (start + length <= 24.05)
								prevprdtms[t][2] = 1; //sembolik
							else
								prevprdtms[t][2] = start + length - 24;

							Jcor[i].pj = prevprdtms[t][2];
							break;
						}
					}

					occupiedtracks.push_back(t);

				}
			}

		}

	sort(occupiedtracks.begin(), occupiedtracks.end());

	for (int t = 0; t < m; ++t)
	{
		if (binary_search(occupiedtracks.begin(), occupiedtracks.end(), t))
			continue;

		prevprdtms[t][0] = -1;
	}


}

vector<vector<Base>> Write(const IloModel& model, const IloCplex& cplex, const IloEnv& env, ofstream& myfile, int n, int begweek, int endweek, int realbegweek, int warmup, int week, int tt, int m, int s, double& time,
	double* cumtime, double* cum, int pr, int sol, int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, int buffer, const NumVarArray3& B,
	const BoolVarArray3& o, const BoolVarArray2& y, const IloNumVarArray& P, const BoolVarArray3& x, const BoolVarArray3& e, const BoolVarArray3& a, const BoolVarArray3& bb,
	const NumVarArray2& E, const NumVarArray2& T, const NumVarArray2& C, const NumVarArray2& Sj, const IloNumVarArray& S, const IloNumVarArray& F,
	const NumVarArray3& L, const BoolVarArray2& np /*IloNumVarArray& np*/, const IloNumVarArray& deltaminusSLA, const IloNumVarArray& deltaYmaxplus, const IloNumVarArray& coroutservice, ofstream& datarunfile,
	vector<Job>& Jcurcor, vector<Job>& Jcurprv, vector<int> initials, int* SLA, int** prevprdtms)

	//bazı varları continious yaparak yeni bir write func oluşturabilirsin!
{
	double curtime = cplex.getCplexTime() - time;
	vector<Base> BaseAs;
	vector<ParentChild> ParChildAs;

	double l = 0;
	double ll = 0;
	int tard_0 = 0;
	int tard_1 = 0;
	int earl = 0;
	double SLAviolation = 0;
	double Ymaxviolation = 0;
	double wait = 0;
	double load = 0;

	int prevsize = Jcurprv.size();
	int corsize = Jcurcor.size();

	vector<int> countpark;
	countpark.resize(tt, 0);

	vector<int> realcountpark;
	realcountpark.resize(tt, 0);

	int preventnum = Jcurprv.size() + Jcurcor.size();

	typedef vector<vector<Base>> Candidates;
	Candidates cand;

	if (initials.size()>1)
		sort(initials.begin(), initials.end());

	for each (Job job in Jcurprv)
	{
		int j = job.ID;

		double sj = cplex.getValue(Sj[0][j]);
		double npj = cplex.getValue(np[0][j]);
		double fj = cplex.getValue(C[0][j]);

		if (sj <= 23.95 + buffer && npj <= 0.01)
		{
			Base bas;
			for (int t = 0; t < m; ++t)
				for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
				{
					int dist = b - t*preventnum + 1;
					if (cplex.getValue(x[0][b][j]) >= 0.001)
					{
						bas.id = b;
						bas.jobid = j;
						bas.starttime = sj;
						bas.finish = cplex.getValue(C[0][j]);
						bas.length = job.pj;
						bas.type = 0;
						bas.rj = job.rj - realbegweek;

						if (t <= m - 2)
						{
							for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
							{
								if (cplex.getValue(y[a][b]) >= 0.001)
									ParChildAs.emplace_back(b, a);
							}
						}

						break;
					}
				}

			BaseAs.push_back(bas);
		}
	}

	for each (Job job in Jcurcor)
	{
		int j = job.ID;

		double sj = cplex.getValue(Sj[1][j]);
		double npj = cplex.getValue(np[1][j]);
		double fj = cplex.getValue(C[1][j]);

		if (sj <= 23.95 + buffer && npj <= 0.01)
			//if (fj <= tt - 1 && npj <= 0.01)
		{
			//++count;
			Base bas;
			for (int t = 0; t < m; ++t)
				for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
				{
					int dist = b - t*preventnum + 1;
					if (cplex.getValue(x[1][b][j]) >= 0.001)
					{
						bas.id = b;
						bas.jobid = j;
						bas.starttime = sj;
						bas.finish = cplex.getValue(C[1][j]);

						bas.length = job.pj;
						bas.type = 1;
						bas.rj = job.rj - realbegweek;

						if (t <= m - 2)
						{
							for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
							{
								if (cplex.getValue(y[a][b]) >= 0.001)
								{
									ParChildAs.emplace_back(b, a);
								}
							}
						}

						break;
					}
				}

			BaseAs.push_back(bas);
		}
	}

	//only to write comput times
	/*if (BaseAs.size() == 0)
	{
	if (week >= 60)
	{
	myfile << week + 1 << "\t" << to_string(prevsize) << "\t" << to_string(corsize) << "\t" << SLAviolation << "\t"
	<< Ymaxviolation << "\t" << tard_0 << "\t" << tard_1 << "\t" << earl << "\t"
	<< wait << "\t" << to_string(load)
	<< "\t" << setprecision(2) << fixed << to_string(curtime);
	myfile << "\n";
	}
	return cand;
	}*/

	//Fine-tune finish times
	vector<Base> sametrack;
	for (int i = 0; i < BaseAs.size(); i++)
	{
		if (floor(BaseAs[i].id / preventnum) == 0)
			sametrack.push_back(BaseAs[i]);
	}

	sort(sametrack.begin(), sametrack.end(), ascstart());
	sort(BaseAs.begin(), BaseAs.end(), ascstart());
	cand.resize(sametrack.size());  // burada i ve j blokları için elimizdeki algı uygulayalım. //9.12.1

	//8.12.17 //descendant belirleme
	int* basebuffer = new int[m*preventnum];

	for (int i = 0; i < m*preventnum; ++i)
		basebuffer[i] = buffer;

	for (int k = 0; k < sametrack.size(); ++k)
	{
		for (int tr = 0; tr < m; ++tr)
		{
			vector<Base> Children;

			if (tr == 0)
			{
				int pullbackamount;
				if (k == 0)
					pullbackamount = min(sametrack[0].starttime, buffer);
				else
				{
					Base root;
					for each(Base basee in cand[k - 1])
					{
						int b = basee.id;
						if (b < preventnum)
						{
							root = basee;
							break;
						}
					}
					pullbackamount = min(sametrack[k].starttime - root.finish, buffer); //burası problem
				}

				GetChildren(ParChildAs, sametrack[k], Children, BaseAs);

				if (Children.size() > 0)
				{
					sort(Children.begin(), Children.end(), ascstart());
					vector<Base> roottoleaf = GetDescendants(sametrack[k], tr, ParChildAs, BaseAs, m); //sametrack[k] şu anki root
					//disttofirstchild = Children[0].starttime - sametrack[k].starttime;
					cand[k] = roottoleaf;
				}

				else
					cand[k].push_back(sametrack[k]);

				for (int i = 0; i < cand[k].size(); ++i)
				{
					int b = cand[k][i].id;
					if (cand[k][i].starttime - pullbackamount >= cand[k][i].rj)
					{
						cand[k][i].starttime -= pullbackamount;
						cand[k][i].finish -= pullbackamount;
						basebuffer[b] -= pullbackamount;
					}

					else
					{
						int disttoymin = cand[k][i].starttime - cand[k][i].rj;
						cand[k][i].starttime -= disttoymin;
						cand[k][i].finish -= disttoymin;
						basebuffer[b] -= disttoymin;
					}
				}

				Children.clear();

			}

			else if ((m == 3 && tr == m - 2) || (m == 2 && tr == m - 1)) //intermediate or last track
			{
				//for (int k = 0; k < sametrack.size(); ++k)
				//{
				Base root;
				int baseindex = 0;
				for each(Base basee in cand[k])
				{
					int b = basee.id;
					if (b < preventnum)
					{
						root = basee;
						break;
					}
					++baseindex;
				}

				GetChildren(ParChildAs, root, Children, cand[k]);

				if (Children.size() == 0)
					continue;
				vector<Base> descendants;

				sort(Children.begin(), Children.end(), ascstart());

				int disttobegin = Children[0].starttime - root.starttime; //children içinde de candk-i'le denk gelenleri dikkate al
				if (disttobegin == 0)
				{
					Children.clear();
					continue;
				}

				for (int i = 0; i < Children.size(); ++i)
				{
					/////////////////////////
					int pullbackamount;
					int b = Children[i].id;
					if (i == 0)
						pullbackamount = min(disttobegin, basebuffer[b]);
					else
						pullbackamount = min(Children[i].starttime - Children[i - 1].finish, basebuffer[b]);

					if (pullbackamount == 0)
						continue;

					descendants = GetDescendants(Children[i], tr, ParChildAs, cand[k], m); //Children[i] şu anki root

					for (int ii = 0; ii < descendants.size(); ++ii)
					{
						int bb = descendants[ii].id;
						if (descendants[ii].starttime - pullbackamount >= descendants[ii].rj)
						{
							descendants[ii].starttime -= pullbackamount;
							descendants[ii].finish -= pullbackamount; //burada alttaki işlerin de finish time'nı update et!								
							basebuffer[bb] -= pullbackamount;
						}

						else
						{
							int disttoymin = descendants[ii].starttime - descendants[ii].rj;
							descendants[ii].starttime -= disttoymin;
							descendants[ii].finish -= disttoymin;
							basebuffer[bb] -= disttoymin;
						}
					}
					////////////////777

					for (int j = 0; j < cand[k].size(); ++j)
					{
						for each(Base bas in descendants)
						{
							if (cand[k][j].id == bas.id)
							{
								cand[k][j] = bas;
								if (cand[k][j].finish < root.finish  && bas.id == Children[i].id/*&& bas.id == descendants.back().id*/) //m=3'ken 2. ve 3. trackteki joblar aday
								{
									if (root.starttime + root.length > cand[k][j].finish)
										root.finish = root.starttime + root.length;
									else
										root.finish = cand[k][j].finish;
								}

							}
						}
					}

					descendants.clear();
				}

				cand[k][baseindex] = root;
				Children.clear();
				//}
			}

			else //last track
			{
				//for (int k = 0; k < sametrack.size(); ++k)
				//{
				Base root1;

				int baseindex1 = 0;

				for each(Base basee in cand[k])
				{
					int b = basee.id;
					if (b < preventnum)
					{
						root1 = basee;
						break;
					}

					++baseindex1;
				}

				GetChildren(ParChildAs, root1, Children, cand[k]);

				if (Children.size() == 0)
					continue;

				vector<Base> GrandChildren;

				for (int j = 0; j < Children.size(); ++j)
				{
					GetChildren(ParChildAs, Children[j], GrandChildren, cand[k]);
					if (GrandChildren.size() == 0)
						continue;

					sort(GrandChildren.begin(), GrandChildren.end(), ascstart());

					int baseindex2 = 0;
					Base root = Children[j];

					for each(Base basee in cand[k])
					{
						if (root.id == basee.id)
							break;
						++baseindex2;
					}

					int disttobegin = GrandChildren[0].starttime - root.starttime; //children içinde de candk-i'le denk gelenleri dikkate al

					for (int ii = 0; ii < GrandChildren.size(); ++ii)
					{
						int pullbackamount;
						int b = GrandChildren[ii].id;
						if (ii == 0)
							pullbackamount = min(disttobegin, basebuffer[b]);
						else
							pullbackamount = min(GrandChildren[ii].starttime - GrandChildren[ii - 1].finish, basebuffer[b]);

						if (pullbackamount == 0)
						{
							//GrandChildren.clear();
							continue;
						}

						if (GrandChildren[ii].starttime - pullbackamount >= GrandChildren[ii].rj)
						{
							GrandChildren[ii].starttime -= pullbackamount;
							GrandChildren[ii].finish -= pullbackamount;
							//int bb = GrandChildren[ii].id;
							basebuffer[b] -= pullbackamount;
						}

						else
						{
							int disttoymin = GrandChildren[ii].starttime - GrandChildren[ii].rj;
							GrandChildren[ii].starttime -= disttoymin;
							GrandChildren[ii].finish -= disttoymin;
							//int bb = GrandChildren[ii].id;
							basebuffer[b] -= disttoymin;
						}

					}

					for (int ii = 0; ii < cand[k].size(); ++ii)
					{
						//bool found = false;
						for each(Base bas in GrandChildren)
						{
							if (cand[k][ii].id == bas.id)
							{
								cand[k][ii] = bas;
								//3. track ve aşağısındaki finish timelar update edilmeli!! //13.12.17

								if (cand[k][ii].finish < root.finish && bas.id == GrandChildren.back().id)
								{
									if (root.starttime + root.length > cand[k][ii].finish)
									{
										root.finish = root.starttime + root.length;
										if (root.finish > root1.finish)
											root1.finish = root.finish;
									}

									else
									{
										root.finish = cand[k][ii].finish;
										root1.finish = cand[k][ii].finish;
									}


								}
								////cand[k][baseindex] = root;
								//break;
							}
						}
					}

					cand[k][baseindex1] = root1;
					cand[k][baseindex2] = root;

					GrandChildren.clear();

				}

				Children.clear();
			}
		}
	}

	//starttime>=24 olanları canddan silelim!
	Candidates tempcand;
	tempcand.resize(cand.size());

	for (int k = 0; k < cand.size(); ++k)
		for each(Base bas in cand[k])
		{
			if (bas.starttime <= 23.95)
				tempcand[k].push_back(bas);
		}

	cand = tempcand;

	Base root;
	root.id = -1;
	Base lastchild;
	lastchild.id = -1;
	int baseindex = 0;
	int lastchildindex = 0; //indexlere bakılacak!
	int kindex = 0;

	for (int k = 0; k < cand.size(); ++k)
	{
		vector<Base> Children;

		if (cand[k].size() == 0)
			continue;

		root = sametrack[k];
		baseindex = 0;
		lastchildindex = 0;

		//int basetrackindex = 0;

		for each(Base basee in cand[k])
		{
			if (root.id == basee.id)
			{
				root = basee;
				kindex = k;
				break;
			}

			++baseindex;
		}

		if (root.finish <= 24.00 || root.starttime >= 24.00)
		{
			baseindex = 0;
			root.id = -1;
			continue;
		}


		GetChildren(ParChildAs, root, Children, cand[k]);

		if (m == 2)
		{
			if (Children.size() > 0)
			{
				lastchild = Children.back();
				for each(Base basee in cand[k])
				{
					if (lastchild.id == basee.id)
					{
						lastchild = basee;
						break;
					}

					++lastchildindex;
				}
			}
			else
				root.finish = root.starttime + root.length;

			lastchild.finish = lastchild.starttime + lastchild.length;
			//o zaman 2. ve 1. track kaldı
			if (lastchild.finish >= root.starttime + root.length)
				root.finish = lastchild.finish;
			else
				root.finish = root.starttime + root.length;
		}

		else
		{
			if (Children.size() > 0)
			{
				lastchild = Children.back();
				for each(Base basee in cand[k])
				{
					if (lastchild.id == basee.id)
					{
						lastchild = basee;
						break;
					}

					++lastchildindex;
				}

				vector<Base> GrandChildren;
				GetChildren(ParChildAs, lastchild, GrandChildren, cand[k]);

				if (GrandChildren.size() == 0)
				{
					lastchild.finish = lastchild.starttime + lastchild.length;
					//o zaman 2. ve 1. track kaldı
					if (lastchild.finish >= root.starttime + root.length)
						root.finish = lastchild.finish;
					else
						root.finish = root.starttime + root.length;
				}

				else
				{
					//yukarıdan aşağıya ilişkiler bütünü
					//grandchildren - lastchild ilişkisi
					//last child - root ilişkisi
					Base lastgrandchild = GrandChildren.back();
					if (lastgrandchild.finish > lastchild.starttime + lastchild.length)
						lastchild.finish = lastgrandchild.finish;
					else
						lastchild.finish = lastchild.starttime + lastchild.length;

					if (lastchild.finish >= root.starttime + root.length)
						root.finish = lastchild.finish;
					else
						root.finish = root.starttime + root.length;
				}
			}

			else //sadece ilk trackte araç var
				root.finish = root.starttime + root.length;
		}
	}

	if (root.id >= 0)
		cand[kindex][baseindex] = root;
	if (lastchild.id >= 0)
		cand[kindex][lastchildindex] = lastchild;

	/*if (week < 60)
		return cand;*/

	vector<int> scheduled;

	//statistics - new finishtime vs de hesaplarından sonra en son hesaplanır
	if (cand.size() > 0)
	{
		for (int k = 0; k < cand.size(); ++k)
			for (int i = 0; i < cand[k].size(); ++i)
			{
				//calculate new statistics(tardiness vs bu kadar geriye al)
				int j = cand[k][i].jobid;
				int sj = cand[k][i].starttime;
				int type = cand[k][i].type;
				int finish = cand[k][i].finish;
				int length = cand[k][i].length;

				if (sj <= 23.95)
				{
					scheduled.push_back(j);
					//count stays in park with out of service
					for (int t = cand[k][i].starttime; t < min(cand[k][i].finish, tt); ++t)
					{
						countpark[t]++; //stay
						realcountpark[t]++; //for load
					}

					if (type == 0)
					{
						//counts out of service
						for each (Job job in Jcurprv)
						{
							if (j == job.ID)
							{
								int yjmax = job.Ymax - realbegweek;
								if (yjmax <= 0)
								{
									for (int t = 0; t < sj; ++t)
										countpark[t]++;
								}

								else
									for (int t = yjmax; t < sj; ++t)
										countpark[t]++;
								break;
							}
						}
					}

					else
					{
						for each (Job job in Jcurcor)
						{
							if (j == job.ID)
							{
								for (int t = 0; t < sj; ++t)
									countpark[t]++;
								break;
							}
						}
					}

					if (binary_search(initials.begin(), initials.end(), j))
					{
						////fixlenmiş waitleri yazacağız
						for (int t = 0; t < m; ++t)
						{
							int job = prevprdtms[t][0];
							if (j == job)
							{
								int previousfinish = prevprdtms[t][3];
								//int adjustedfinish = previousfinish - 24;
								int dummypj = prevprdtms[t][2];

								if (previousfinish <= 0) //dummypj ye kadar
								{
									//wait += dummypj;
									if (finish <= 24)
										wait += finish;
									else
										wait += 24;
								}

								else
								{
									if (finish <= 24)
										wait += finish - previousfinish;
									else if (previousfinish <24)
										wait += 24 - previousfinish;
								}

								break;
							}

						}

						continue;
					}

					//other statistics

					if (finish <= 24)
						wait += finish - (sj + length); //regular

					if (sj <= 23.95 && finish >= 24.05)
					{
						if (sj + length < 24)
							wait += 24 - (sj + length);
					}

					int pullbackamount = buffer - basebuffer[cand[k][i].id];

					if (type == 0)
					{
						double viol = cplex.getValue(deltaYmaxplus[j]) - pullbackamount;
						if (viol <= 0)
							viol = 0;

						//before
						int jtard = cplex.getValue(T[0][j]);
						int jearl = cplex.getValue(E[0][j]);

						if (jearl >= 0 && jtard == 0)
							jearl += pullbackamount;
						else if (jtard>0)
						{
							jtard -= pullbackamount;
							if (jtard < 0)
							{
								jearl += -jtard;
								jtard = 0;
							}

						}

						Ymaxviolation += viol;
						tard_0 += jtard;
						earl += jearl;
					}

					else //jcurcor yazılacak
					{
						double jtard1 = cplex.getValue(T[1][j]);
						jtard1 -= pullbackamount;
						if (jtard1 < 0)
							jtard1 = 0;
						tard_1 += jtard1;
					}
				}
			}
	}

	std::sort(scheduled.begin(), scheduled.end());

	//countpark- others
	for each (Job job in Jcurcor)
	{
		int j = job.ID;
		if (binary_search(scheduled.begin(), scheduled.end(), j))
			continue;

		//24'ten sonra başlaacak, o nedenle 24'e kadar 0
		for (int t = 0; t < 24; ++t)
			countpark[t]++;
	}

	for each (Job job in Jcurprv)
	{
		int j = job.ID;
		if (binary_search(scheduled.begin(), scheduled.end(), j))
			continue;

		//int sj = cplex.getValue()
		int yjmax = job.Ymax - realbegweek;

		if (yjmax <= 0)
		{
			for (int t = 0; t < 24; ++t)
				countpark[t]++;
		}

		else
			for (int t = yjmax; t < 24; ++t)
				countpark[t]++;
	}

	//countpark-others	
	for (int t = 0; t < 24; ++t)
	{
		int fark = preventnum - countpark[t] - (SLA[t] - (n - preventnum));
		if (fark < 0)
			SLAviolation -= fark;
	}

	for (int i = 0; i < 24; ++i)
		l += realcountpark[i];

	load = (l / (m * 24)) * 100;

	string type;
	if (sol == 0 && week >= 0)
	{
		myfile << week + 1 << "\t" << to_string(prevsize) << "\t" << to_string(corsize) << "\t" << SLAviolation << "\t"
			<< Ymaxviolation << "\t" << tard_0 << "\t" << tard_1 << "\t" << earl << "\t"
			<< wait << "\t" << to_string(load)
			<< "\t" << setprecision(2) << fixed << to_string(cplex.getMIPRelativeGap()) << "\t" << setprecision(2) << fixed << curtime;
		myfile << "\n";
	}

	//time = cplex.getCplexTime();
	delete[] basebuffer;
	//delete[] countpark;
	return cand;
}

vector<vector<Base>> Writealternatif(const IloModel& model, const IloCplex& cplex, const IloEnv& env, ofstream& myfile, int n, int begweek, int endweek, int realbegweek, int warmup, int week, int tt, int m, int s, double& time,
	double* cumtime, double* cum, int pr, int sol, int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, const NumVarArray3& B,
	const BoolVarArray3& o, const BoolVarArray2& y, const IloNumVarArray& P, const BoolVarArray3& x, const BoolVarArray3& e, const BoolVarArray3& a, const BoolVarArray3& bb,
	const NumVarArray2& E, const NumVarArray2& T, const NumVarArray2& C, const NumVarArray2& Sj, const IloNumVarArray& S, const IloNumVarArray& F,
	const NumVarArray3& L, const BoolVarArray2& np /*IloNumVarArray& np*/, const IloNumVarArray& deltaminusSLA, const IloNumVarArray& deltaYmaxplus, const IloNumVarArray& coroutservice, ofstream& datarunfile,
	vector<Job>& Jcurcor, vector<Job>& Jcurprv, vector<int> initials, int* SLA)

	//bazı varları continious yaparak yeni bir write func oluşturabilirsin!
{
	vector<Base> BaseAs;
	vector<ParentChild> ParChildAs;

	typedef vector<vector<Base>> Candidates;
	Candidates cand;

	double l = 0;
	double ll = 0;
	int tard_0 = 0;
	int tard_1 = 0;
	int earl = 0;
	double SLAviolation = 0;
	double Ymaxviolation = 0;

	vector<int> countpark;
	countpark.resize(tt, 0);

	vector<int> realcountpark;
	countpark.resize(tt, 0);

	int buffer = 10;
	//int buffer = 0;
	int preventnum = Jcurprv.size() + Jcurcor.size();

	if (initials.size()>1)
		sort(initials.begin(), initials.end());

	int count = 0;

	for each (Job job in Jcurprv)
	{
		int j = job.ID;

		double sj = cplex.getValue(Sj[0][j]);
		double npj = cplex.getValue(np[0][j]);
		double fj = cplex.getValue(C[0][j]);

		//if (sj <= 24+buffer-1 && npj <= 0.01)
		//if (sj <= 24 && npj <= 0.01)

		//if (sj <= 23.95 + buffer - 1 && npj <= 0.01)
		//if (sj <= 23.95 + 2*buffer && npj <= 0.01)

		if (sj <= 23.95)
			++count;

		//if (fj <= tt - 1 && npj <= 0.01)
		if (sj <= 24 + buffer - 1 && npj <= 0.01)
		{
			Base bas;
			for (int t = 0; t < m; ++t)
				for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
				{
					int dist = b - t*preventnum + 1;
					if (cplex.getValue(x[0][b][j]) >= 0.001)
					{
						bas.id = b;
						bas.jobid = j;
						bas.starttime = sj;
						bas.finish = cplex.getValue(C[0][j]);
						bas.length = job.pj;
						bas.type = 0;
						bas.rj = job.rj - realbegweek;

						if (t <= m - 2)
						{
							for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
							{
								if (cplex.getValue(y[a][b]) >= 0.001)
								{
									ParChildAs.emplace_back(b, a);
								}
							}
						}

						break;
					}
				}

			BaseAs.push_back(bas);
		}
	}

	if (count == 0)
		return cand;

	int prevsize = Jcurprv.size();
	int corsize = Jcurcor.size();

	double curtime = cplex.getCplexTime() - time;

	//8.12.17 //descendant belirleme

	int* basebuffer = new int[m*preventnum];

	for (int i = 0; i < m*preventnum; ++i)
		basebuffer[i] = buffer;

	vector<Base> sametrack;

	//for (int tr = m - 1; tr >= 0; --tr) //tr=track index - m-2: son bloktan bir aşağısı: buna bakılmalı!

	for (int i = 0; i < BaseAs.size(); i++)
	{
		if (floor(BaseAs[i].id / preventnum) == 0)
			sametrack.push_back(BaseAs[i]);
	}

	sort(sametrack.begin(), sametrack.end(), ascstart());
	sort(BaseAs.begin(), BaseAs.end(), ascstart());
	cand.resize(sametrack.size());  // burada i ve j blokları için elimizdeki algı uygulayalım. //9.12.1

	for (int k = 0; k < sametrack.size(); ++k)
	{
		for (int tr = 0; tr < m; ++tr)
		{
			vector<Base> Children;

			if (tr == 0)
			{
				int pullbackamount;
				if (k == 0)
					pullbackamount = min(sametrack[0].starttime, buffer);
				else
				{
					Base root;
					for each(Base basee in cand[k - 1])
					{
						int b = basee.id;
						if (b < preventnum)
						{
							root = basee;
							break;
						}
					}
					pullbackamount = min(sametrack[k].starttime - root.finish, buffer); //burası problem
				}

				GetChildren(ParChildAs, sametrack[k], Children, BaseAs);

				if (Children.size() > 0)
				{
					sort(Children.begin(), Children.end(), ascstart());
					vector<Base> roottoleaf = GetDescendants(sametrack[k], tr, ParChildAs, BaseAs, m); //sametrack[k] şu anki root
					//disttofirstchild = Children[0].starttime - sametrack[k].starttime;
					cand[k] = roottoleaf;
				}

				else
					cand[k].push_back(sametrack[k]);

				for (int i = 0; i < cand[k].size(); ++i)
				{
					int b = cand[k][i].id;
					if (cand[k][i].starttime - pullbackamount >= cand[k][i].rj)
					{
						cand[k][i].starttime -= pullbackamount;
						cand[k][i].finish -= pullbackamount;
						basebuffer[b] -= pullbackamount;
					}

					else
					{
						int disttoymin = cand[k][i].starttime - cand[k][i].rj;
						cand[k][i].starttime -= disttoymin;
						cand[k][i].finish -= disttoymin;
						basebuffer[b] -= disttoymin;
					}
				}

				Children.clear();
				//++k;
				//}
			}

			else if ((m == 3 && tr == m - 2) || (m == 2 && tr == m - 1)) //intermediate or last track
			{
				//for (int k = 0; k < sametrack.size(); ++k)
				//{
				Base root;
				int baseindex = 0;
				for each(Base basee in cand[k])
				{
					int b = basee.id;
					if (b < preventnum)
					{
						root = basee;
						break;
					}
					++baseindex;
				}

				GetChildren(ParChildAs, root, Children, cand[k]);

				if (Children.size() == 0)
					continue;
				vector<Base> descendants;

				sort(Children.begin(), Children.end(), ascstart());

				int disttobegin = Children[0].starttime - root.starttime; //children içinde de candk-i'le denk gelenleri dikkate al
				if (disttobegin == 0)
				{
					Children.clear();
					continue;
				}

				int i = 0;
				while (i < Children.size())
				{
					/////////////////////////
					int pullbackamount;
					int b = Children[i].id;
					if (i == 0)
						pullbackamount = min(disttobegin, basebuffer[b]);
					else
						pullbackamount = min(Children[i].starttime - Children[i - 1].finish, basebuffer[b]);

					if (pullbackamount == 0)
					{
						//Children.clear();
						++i;
						continue;
					}

					descendants = GetDescendants(Children[i], tr, ParChildAs, cand[k], m); //Children[i] şu anki root

					for (int ii = 0; ii < descendants.size(); ++ii)
					{
						int bb = descendants[ii].id;
						if (descendants[ii].starttime - pullbackamount >= descendants[ii].rj)
						{
							descendants[ii].starttime -= pullbackamount;
							descendants[ii].finish -= pullbackamount; //burada alttaki işlerin de finish time'nı update et!								
							basebuffer[bb] -= pullbackamount;
						}

						else
						{
							int disttoymin = descendants[ii].starttime - descendants[ii].rj;
							descendants[ii].starttime -= disttoymin;
							descendants[ii].finish -= disttoymin;
							basebuffer[bb] -= disttoymin;
						}
					}
					////////////////777

					for (int j = 0; j < cand[k].size(); ++j)
					{
						for each(Base bas in descendants)
						{
							if (cand[k][j].id == bas.id)
							{
								cand[k][j] = bas;
								if (cand[k][j].finish < root.finish  && bas.id == Children[i].id/*&& bas.id == descendants.back().id*/) //m=3'ken 2. ve 3. trackteki joblar aday
								{
									if (root.starttime + root.length > cand[k][j].finish)
										root.finish = root.starttime + root.length;
									else
										root.finish = cand[k][j].finish;
								}

							}
						}
					}

					descendants.clear();

					++i;
				}

				cand[k][baseindex] = root;
				Children.clear();
				//}
			}

			else //last track
			{
				//for (int k = 0; k < sametrack.size(); ++k)
				//{
				Base root1;

				int baseindex1 = 0;

				for each(Base basee in cand[k])
				{
					int b = basee.id;
					if (b < preventnum)
					{
						root1 = basee;
						break;
					}

					++baseindex1;
				}

				GetChildren(ParChildAs, root1, Children, cand[k]);

				if (Children.size() == 0)
					continue;

				vector<Base> GrandChildren;

				for (int j = 0; j < Children.size(); ++j)
				{
					GetChildren(ParChildAs, Children[j], GrandChildren, cand[k]);
					if (GrandChildren.size() == 0)
						continue;

					sort(GrandChildren.begin(), GrandChildren.end(), ascstart());

					int baseindex2 = 0;
					Base root = Children[j];

					for each(Base basee in cand[k])
					{
						if (root.id == basee.id)
							break;
						++baseindex2;
					}

					int disttobegin = GrandChildren[0].starttime - root.starttime; //children içinde de candk-i'le denk gelenleri dikkate al

					for (int ii = 0; ii < GrandChildren.size(); ++ii)
					{
						int pullbackamount;
						int b = GrandChildren[ii].id;
						if (ii == 0)
							pullbackamount = min(disttobegin, basebuffer[b]);
						else
							pullbackamount = min(GrandChildren[ii].starttime - GrandChildren[ii - 1].finish, basebuffer[b]);

						if (pullbackamount == 0)
						{
							//GrandChildren.clear();
							continue;
						}

						if (GrandChildren[ii].starttime - pullbackamount >= GrandChildren[ii].rj)
						{
							GrandChildren[ii].starttime -= pullbackamount;
							GrandChildren[ii].finish -= pullbackamount;
							//int bb = GrandChildren[ii].id;
							basebuffer[b] -= pullbackamount;
						}

						else
						{
							int disttoymin = GrandChildren[ii].starttime - GrandChildren[ii].rj;
							GrandChildren[ii].starttime -= disttoymin;
							GrandChildren[ii].finish -= disttoymin;
							//int bb = GrandChildren[ii].id;
							basebuffer[b] -= disttoymin;
						}

					}

					for (int ii = 0; ii < cand[k].size(); ++ii)
					{
						//bool found = false;
						for each(Base bas in GrandChildren)
						{
							if (cand[k][ii].id == bas.id)
							{
								cand[k][ii] = bas;
								//3. track ve aşağısındaki finish timelar update edilmeli!! //13.12.17

								if (cand[k][ii].finish < root.finish && bas.id == GrandChildren.back().id)
								{
									if (root.starttime + root.length > cand[k][ii].finish)
									{
										root.finish = root.starttime + root.length;
										if (root.finish > root1.finish)
											root1.finish = root.finish;
									}

									else
									{
										root.finish = cand[k][ii].finish;
										root1.finish = cand[k][ii].finish;
									}


								}
								////cand[k][baseindex] = root;
								//break;
							}
						}
					}

					cand[k][baseindex1] = root1;
					cand[k][baseindex2] = root;

					GrandChildren.clear();

				}

				Children.clear();
				//}
			}
		}
	}


	//statistics - new finishtime vs de hesaplarından sonra en son hesaplanır

	if (sametrack.size() > 0)
	{
		for (int k = 0; k < sametrack.size(); ++k)
			for (int i = 0; i < cand[k].size(); ++i)
			{

				//calculate new statistics(tardiness vs bu kadar geriye al)
				int j = cand[k][i].jobid;
				int sj = cand[k][i].starttime;

				if (sj <= 23.95)
				{

					//count stays in park with out of service
					for (int t = cand[k][i].starttime; t < min(cand[k][i].finish, tt); ++t)
					{
						countpark[t]++;
						realcountpark[t]++;
					}

					//counts out of service
					for each (Job job in Jcurprv)
					{
						if (j == job.ID)
						{
							int yjmax = job.Ymax - realbegweek;
							if (yjmax < 0)
							{
								for (int t = 0; t < sj; ++t)
									countpark[t]++;
							}

							else
								for (int t = yjmax; t < sj; ++t)
									countpark[t]++;
							break;
						}
					}

					if (binary_search(initials.begin(), initials.end(), j))
						continue;

					//other statistics

					int pullbackamount = buffer - basebuffer[cand[k][i].id];

					double viol = cplex.getValue(deltaYmaxplus[j]) - pullbackamount;
					if (viol <= 0)
						viol = 0;

					//before
					int jtard = cplex.getValue(T[0][j]);
					int jearl = cplex.getValue(E[0][j]);

					if (jearl >= 0 && jtard == 0)
						jearl += pullbackamount;
					else if (jtard>0)
					{
						jtard -= pullbackamount;
						if (jtard < 0)
						{
							jearl += -jtard;
							jtard = 0;
						}

					}

					Ymaxviolation += viol;
					tard_0 += jtard;
					earl += jearl;
				}
			}
	}

	for (int t = 0; t < 24; ++t)
	{
		int fark = preventnum - countpark[t] - (SLA[t] - (n - preventnum));
		if (fark < 0)
			SLAviolation -= fark;
	}

	for (int i = 0; i < 24; ++i)
		l += realcountpark[i];

	double load = (l / (m * 24)) * 100;

	string type;
	if (sol == 0 && week >= 60)
	{
		myfile << week + 1 << "\t" << to_string(prevsize) << "\t" << to_string(corsize) << "\t" << SLAviolation << "\t"
			<< Ymaxviolation << "\t" << tard_0 << "\t" << tard_1 << "\t" << earl << "\t"
			<< to_string(cplex.getObjValue()) << "\t" << to_string(load)
			<< "\t" << setprecision(2) << fixed << to_string(curtime) << "\t" << cplex.getMIPRelativeGap() * 100 << "\t\t";
	}

	myfile << "\n";

	time = cplex.getCplexTime();
	delete[] basebuffer;
	//delete[] countpark;
	return cand;
}

void Writeclear(const IloModel& model, const IloCplex& cplex, const IloEnv& env, ofstream& myfile, int n, int begweek, int endweek, int realbegweek, int warmup, int week, int tt, int m, int s, double& time,
	double* cumtime, double* cum, int pr, int sol, int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, const NumVarArray3& B,
	const BoolVarArray3& o, const BoolVarArray2& y, const IloNumVarArray& P, const BoolVarArray3& x, const BoolVarArray3& e, const BoolVarArray3& a, const BoolVarArray3& bb,
	const NumVarArray2& E, const NumVarArray2& T, const NumVarArray2& C, const NumVarArray2& Sj, const IloNumVarArray& S, const IloNumVarArray& F,
	const IloNumVarArray& K, const NumVarArray3& L, const BoolVarArray2& np /*IloNumVarArray& np*/, const IloNumVarArray& deltaminusSLA, const IloNumVarArray& deltaYmaxplus, const IloNumVarArray& coroutservice, ofstream& datarunfile,
	vector<Job>& Jcurcor, vector<Job>& Jcurprv)

	//bazı varları continious yaparak yeni bir write func oluşturabilirsin!
{
	//warmup -= realbegweek;
	n = Jcurcor.size() + Jcurprv.size();

	/*Finds load - load wrt "m" not actual SLA!*/
	double l = 0;
	int tard = 0;

	for each (Job job in Jcurprv)
		for (int t = begweek; t < endweek; ++t)
		{
			int j = job.ID;

			l += cplex.getValue(a[0][j][t]);
		}

	for each (Job job in Jcurcor)
		for (int t = begweek; t < endweek; ++t)
		{
			/*if (t <= warmup)
			continue;*/
			int j = job.ID;
			l += cplex.getValue(a[1][j][t]);
		}

	double load = 0;
	load = l / (m*tt) * 100;

	cum[sol] += load;

	double SLAviolation = 0;
	for (int t = begweek; t < 24; ++t)
	{
		double viol = cplex.getValue(deltaminusSLA[t]);
		myfile << "deltaSLA[" + to_string(t) + "]=" << to_string(viol) + "\n";
		SLAviolation += viol;
	}

	for (int t = 0; t < m; ++t)
		for (int b = t*n; b < (t + 1)*n - t; ++b)
		{
			for each (Job job in Jcurprv)
			{
				int j = job.ID;
				if (cplex.getValue(x[0][b][j]) >= 0.001)
				{
					myfile << "x[0][" + to_string(b) + "][" + to_string(j) + "]=" << to_string(cplex.getValue(x[0][b][j])) + "\n";
					double baslangic = cplex.getValue(B[0][b][j]);
					double bitis = cplex.getValue(F[b]);
					double blockbas = cplex.getValue(S[b]);

					myfile << "B[0][" + to_string(b) + "][" + to_string(j) + "]=" << to_string(baslangic) + "\n";
					myfile << "S[" + to_string(b) + "]=" << to_string(blockbas) + "\n";
					myfile << "F[" + to_string(b) + "]=" << to_string(bitis) + "\n";
				}

			}

			for each (Job job in Jcurcor)
			{
				int j = job.ID;

				if (cplex.getValue(x[1][b][j]) >= 0.001)
				{
					myfile << "x[1][" + to_string(b) + "][" + to_string(j) + "]=" << to_string(cplex.getValue(x[1][b][j])) + "\n";

					double baslangic = cplex.getValue(B[1][b][j]);
					double bitis = cplex.getValue(F[b]);
					double blockbas = cplex.getValue(S[b]);

					myfile << "B[1][" + to_string(b) + "][" + to_string(j) + "]=" << to_string(baslangic) + "\n";
					myfile << "S[" + to_string(b) + "]=" << to_string(blockbas) + "\n";
					myfile << "F[" + to_string(b) + "]=" << to_string(bitis) + "\n";
				}


			}
		}

	myfile << "\n";


	for (int t = 0; t < m - 1; ++t)
		for (int b = t*n; b < (t + 1)*n - t; ++b)
		{
			int dist = b - t*n + 1;
			for (int a = (t + 1)*n; a < (t + 2)*n - dist - t; ++a)
			{
				if (cplex.getValue(y[a][b]) >= 0.001)
				{
					myfile << "y[" + to_string(a) + "][" + to_string(b) + "]=" << to_string(cplex.getValue(y[a][b])) + "\n";
				}
			}
		}

	for each (Job job in Jcurprv)
	{
		int j = job.ID;
		myfile << "T[0][" + to_string(j) + "]" << to_string(cplex.getValue(T[0][j])) + "\n";
		myfile << "E[0][" + to_string(j) + "]" << to_string(cplex.getValue(E[0][j])) + "\n";
	}
	myfile << "\n";

	double Ymaxviolation = 0;
	int earl = 0;
	for each (Job job in Jcurprv)
	{
		int j = job.ID;
		if (cplex.getValue(np[0][j]) >= 0.0001)
			myfile << "np[0][" + to_string(j) + "]" << to_string(cplex.getValue(np[0][j])) + "\n";

		for (int t = 0; t < tt; ++t)
			if (cplex.getValue(a[0][j][t]) >= 0.0001)
				myfile << "a[0][" + to_string(j) + "][" + to_string(t) + "]=" << to_string(cplex.getValue(a[0][j][t])) + "\n";


		/*myfile << "Sj[0][" + to_string(j) + "]=" << to_string(cplex.getValue(Sj[0][j])) + "\n";
		myfile << "Cj[0][" + to_string(j) + "]=" << to_string(cplex.getValue(C[0][j]) ) + "\n";

		for (int t = 0; t < tt; ++t)
		if (cplex.getValue(bb[0][j][t]) >= 0.0001)
		myfile << "b[0][" + to_string(j) + "][" + to_string(t) + "]=" << to_string(cplex.getValue(bb[0][j][t])) + "\n";

		for (int t = 0; t < tt; ++t)
		if (cplex.getValue(a[0][j][t]) >= 0.0001)
		myfile << "a[0][" + to_string(j) + "][" + to_string(t) + "]=" << to_string(cplex.getValue(a[0][j][t])) + "\n";

		for (int t = 0; t < tt; ++t)
		if (cplex.getValue(o[0][j][t]) >= 0.0001)
		myfile << "o[0][" + to_string(j) + "][" + to_string(t) + "]=" << to_string(cplex.getValue(o[0][j][t])) + "\n";*/

		double viol = cplex.getValue(deltaYmaxplus[j]);
		if (cplex.getValue(deltaYmaxplus[j]) >= 0.0001)
			myfile << "deltaYmaxplus[" + to_string(j) + "]=" << to_string(viol) + "\n";
		int jtard = cplex.getValue(T[0][j]);
		int jearl = cplex.getValue(E[0][j]);

		if (cplex.getValue(deltaYmaxplus[j]) >= 0.0001)
			myfile << "deltaYmaxplus[" + to_string(j) + "]=" << to_string(viol) + "\n";
		Ymaxviolation += viol;
		tard += jtard;
		earl += jearl;
	}



	for each (Job job in Jcurcor)
	{
		int j = job.ID;
		if (cplex.getValue(np[1][j]) >= 0.0001)
			myfile << "np[1][" + to_string(j) + "]" << to_string(cplex.getValue(np[1][j])) + "\n";
		myfile << "Sj[1][" + to_string(j) + "]=" << to_string(cplex.getValue(Sj[1][j])) + "\n";
		myfile << "Cj[1][" + to_string(j) + "]=" << to_string(cplex.getValue(C[1][j])) + "\n";

		if (cplex.getValue(T[1][j]) >= 0.0001)
			myfile << "T[1][" + to_string(j) + "]=" << to_string(cplex.getValue(T[1][j])) + "\n";
		int jtard = cplex.getValue(T[1][j]);
		tard += jtard;

		/*for (int t = 0; t < tt; ++t)
		if (cplex.getValue(bb[1][j][t]) >= 0.0001)
		myfile << "b[1][" + to_string(j) + "][" + to_string(t) + "]=" << to_string(cplex.getValue(bb[1][j][t])) + "\n";

		for (int t = 0; t < tt; ++t)
		if (cplex.getValue(a[1][j][t]) >= 0.0001)
		myfile << "a[1][" + to_string(j) + "][" + to_string(t) + "]=" << to_string(cplex.getValue(a[1][j][t])) + "\n";

		for (int t = 0; t < tt; ++t)
		if (cplex.getValue(o[1][j][t]) >= 0.0001)
		myfile << "o[1][" + to_string(j) + "][" + to_string(t) + "]=" << to_string(cplex.getValue(o[1][j][t])) + "\n";*/


	}

	cumSLAviol[sol] += SLAviolation;
	cumYmaxviol[sol] += Ymaxviolation;
	int prevsize = Jcurprv.size();
	int corsize = Jcurcor.size();

	double curtime = cplex.getCplexTime() - time;
	cumtime[sol] += curtime;

	double gap = cplex.getMIPRelativeGap();
	cumgap[sol] += gap;

	int nodes = cplex.getNnodes();
	cumnodes[sol] += nodes;

	string type;
	if (sol == 0)
	{
		type = "basic";
		myfile << week + 1 << "\t" << type << "\t" << to_string(prevsize) << "\t" << to_string(corsize) << "\t" << SLAviolation << "\t" << Ymaxviolation << "\t" << tard << "\t"
			<< earl << "\t" << to_string(cplex.getObjValue()) << "\t" << to_string(cplex.getMIPRelativeGap()) << "\t" << to_string(load) << "\t" << setprecision(2) << fixed << to_string(curtime) << "\t" <<
			to_string(cplex.getNnodes()) << "\t\t\t";
	}
	else
	{
		type = "improved";
		myfile << week + 1 << "\t" << type << "\t" << to_string(prevsize) << "\t" << to_string(corsize) << "\t" << SLAviolation << "\t" << Ymaxviolation << "\t" << tard << "\t"
			<< earl << "\t" << to_string(cplex.getObjValue()) << "\t" << to_string(cplex.getMIPRelativeGap()) << "\t"
			<< to_string(load) << "\t" << setprecision(2) << fixed << to_string(curtime) << "\t" << to_string(cplex.getNnodes()) << "\n";

	}

	time = cplex.getCplexTime();
	//model.remove(SLAconstraints);
}

void Writesade(const IloModel& model, const IloCplex& cplex, const IloEnv& env, ofstream& myfile, int n, int begweek, int endweek, int realbegweek, int warmup, int week, int tt, int m, int s, double& time,
	double* cumtime, double* cum, int pr, int sol, int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, const NumVarArray3& B,
	const BoolVarArray3& o, const BoolVarArray2& y, const IloNumVarArray& P, const BoolVarArray3& x, const BoolVarArray3& e, const BoolVarArray3& a, const BoolVarArray3& bb,
	const NumVarArray2& E, const NumVarArray2& T, const NumVarArray2& C, const NumVarArray2& Sj, const IloNumVarArray& S, const IloNumVarArray& F,
	const IloNumVarArray& K, const NumVarArray3& L, const BoolVarArray2& np /*IloNumVarArray& np*/, const IloNumVarArray& deltaminusSLA, const IloNumVarArray& deltaYmaxplus, const IloNumVarArray& coroutservice, ofstream& datarunfile,
	vector<Job>& Jcurcor, vector<Job>& Jcurprv, vector<int> initials)

	//bazı varları continious yaparak yeni bir write func oluşturabilirsin!
{
	//warmup -= realbegweek;
	n = Jcurcor.size() + Jcurprv.size();

	sort(initials.begin(), initials.end());

	/*Finds load - load wrt "m" not actual SLA!*/
	double l = 0;
	double prevtard = 0;
	double cortard = 0;

	for each (Job job in Jcurprv)
		for (int t = begweek; t < 24; ++t)
		{
			/*if (t <= warmup)
			continue;*/
			int j = job.ID;

			l += cplex.getValue(a[0][j][t]);
		}

	for each (Job job in Jcurcor)
		for (int t = begweek; t < 24; ++t)
		{
			/*if (t <= warmup)
			continue;*/
			int j = job.ID;
			l += cplex.getValue(a[1][j][t]);
		}

	double load = 0;
	load = l / (m * 24) * 100;

	cum[sol] += load;

	double SLAviolation = 0;
	for (int t = begweek; t < 24; ++t)
	{
		double viol = cplex.getValue(deltaminusSLA[t]);
		SLAviolation += viol;
	}

	double Ymaxviolation = 0;
	int earl = 0;
	for each (Job job in Jcurprv)
	{
		int j = job.ID;

		double sj = cplex.getValue(Sj[0][j]);

		double nextper = cplex.getValue(np[0][j]);

		if (sj >= 24.00 || nextper == 1)
			continue;

		if (binary_search(initials.begin(), initials.end(), j))
			continue;

		double viol = cplex.getValue(deltaYmaxplus[j]);
		int jtard = cplex.getValue(T[0][j]);
		int jearl = cplex.getValue(E[0][j]);


		Ymaxviolation += viol;
		prevtard += jtard;
		earl += jearl;
	}

	for each (Job job in Jcurcor)
	{
		int j = job.ID;

		double sj = cplex.getValue(Sj[1][j]);

		double nextper = cplex.getValue(np[1][j]);

		if (sj >= 24.00 || nextper == 1)
			continue;

		if (binary_search(initials.begin(), initials.end(), j))
			continue;


		int jtard = cplex.getValue(T[1][j]);
		cortard += jtard;
	}

	cumSLAviol[sol] += SLAviolation;
	cumYmaxviol[sol] += Ymaxviolation;
	int prevsize = Jcurprv.size();
	int corsize = Jcurcor.size();

	double curtime = cplex.getCplexTime() - time;
	cumtime[sol] += curtime;


	string type;
	if (sol == 0)
	{
		type = "improved";
		myfile << week + 1 << "\t" << to_string(prevsize) << "\t" << to_string(corsize) << "\t" << SLAviolation << "\t" << Ymaxviolation << "\t" << prevtard << "\t"
			<< cortard << "\t" << earl << "\t" << 0 << "\t"
			<< to_string(load) << "\t" << setprecision(2) << fixed << to_string(curtime) << "\t" << 100 << "\n";

	}

	time = cplex.getCplexTime();
	//model.remove(SLAconstraints);
}


void SetupModel(int maxYmin, int maxYmax, int minYmin, vector <int> mmin, int s, int n, int tt, int m, int M, int* SLA, int* pj, int realbegweek, int begweek, int endweek, IloEnv& env, IloModel& model, IloCplex& cplex, BoolVarArray3& o,
	BoolVarArray2& y, IloBoolVarArray& dummy, BoolVarArray2& z, IloNumVarArray& P, BoolVarArray3& x, BoolVarArray3& e, BoolVarArray3& a, BoolVarArray3& b, NumVarArray2& E, NumVarArray2& T, NumVarArray2& C, NumVarArray2& Sj, IloNumVarArray& S,
	IloNumVarArray& F, IloNumVarArray& K, NumVarArray3& L, NumVarArray3& B, const /*IloNumVarArray& np,*/ BoolVarArray2& np, IloNumVarArray& deltaminusSLA, IloNumVarArray& deltaYmaxplus/*prevjob*/, IloNumVarArray& coroutservice/*corjob*/, vector<Job>& Jint, vector<Job>& Jcor,
	vector<int>& preventives, vector<int>& tardyjobs)
{
	sort(tardyjobs.begin(), tardyjobs.end());
	int preventnum = Jint.size() + Jcor.size();
	int left;
	if (minYmin < realbegweek)
		left = 0;
	else
		left = minYmin;

	//corrective out of service

	for each (Job job in Jcor)
	{
		int j = job.ID;
		int ub = job.rj - realbegweek;
		if (ub < 0)
			ub = 0;
		//valid inequlity?
		/*for (int t = begweek; t < ub; ++t)
		model.add(o[1][j][t] == 0);*/
		// - ise sıfır yapalım!
		for (int i = ub; i < tt; ++i)
		{
			IloExpr exp(env);
			for (int t = ub; t <= i; ++t)
				exp += b[1][j][t];
			model.add(1 - exp == o[1][j][i]);
			exp.end();
		}
		////valid inequality
		//int outstart = job.dj - realbegweek + 1;
		//if (outstart < 0)
		//	outstart = 0;
		//for (int i = outstart; i < endweek; ++i)
		//	model.add(a[1][j][i] + o[1][j][i] <= 1);
	}

	//new2 - preventive out of service
	for each (Job job in Jint)
	{
		int j = job.ID;
		if (job.Ymax - realbegweek < endweek)
		{
			int maintstart = -1;
			if (job.rj - realbegweek < begweek)
				maintstart = begweek;
			else
				maintstart = job.rj - realbegweek;

			int outstart;
			if (job.Ymax - realbegweek <= begweek)
				outstart = 0;
			else
				outstart = job.Ymax - realbegweek;

			//ymax öncesi o_jt = 0
			//valid inequality?
			/*for (int t = begweek; t <= outstart - 1; ++t)
			model.add(o[0][j][t] == 0);*/

			////sonrasında, beg'den itibaren başlayan b_jt o_jt'yi ymax'tan itibaren 0 yapabilir!

			for (int i = outstart; i < endweek; ++i)
			{
				IloExpr exp(env);
				for (int t = maintstart; t <= i; ++t)
					exp += b[0][j][t];
				model.add(1 - exp == o[0][j][i]); //i burada i geliyor baştan, bunu düzenlemek lazım!
				exp.end();
				//model.add(a[0][j][i] + o[0][j][i] <= 1); //new ineq
			}

		}
		//else  //job.ymax - realbegweek >= endweek
		//{
		//	for (int t = begweek; t < endweek; ++t)
		//		model.add(o[0][j][t] == 0);
		//	//model.add(deltaYmaxplus[j] == 0); //maybe redundant
		//}
	}

	//Job assignment constraints-prev
	for each (Job job in Jint)
	{
		int j = job.ID;
		IloExpr jbasnmntexp(env);
		/*if(binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int b = 0; b < s; ++b)
			jbasnmntexp += x[0][b][j];
		model.add(jbasnmntexp + np[0][j] == 1);
		jbasnmntexp.end();
	}

	//Job assignment constraints-cor
	for each (Job job in Jcor)
	{
		int j = job.ID;
		IloExpr jbasnmntexp(env);
		/*if(binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int b = 0; b < s; ++b)
			jbasnmntexp += x[1][b][j];
		model.add(jbasnmntexp + np[1][j] == 1);
		jbasnmntexp.end();
	}

	//base  assignmnts
	for (int b = 0; b < s; ++b)
	{
		IloExpr baseexp(env);
		for each(Job job in Jint)
		{
			int j = job.ID;
			baseexp += x[0][b][j];
		}
		for each(Job job in Jcor)
		{
			int j = job.ID;
			baseexp += x[1][b][j];
		}

		model.add(baseexp <= 1);
		baseexp.end();
	}


	//makespan base-track determination-1
	for (int b = 0; b < s; ++b)
	{
		IloExpr mkspnexp(env);
		for each(Job job in Jint)
		{
			int j = job.ID;
			mkspnexp += x[0][b][j] * job.pj;
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			mkspnexp += x[1][b][j] * job.pj;
		}

		model.add(mkspnexp <= P[b]);
		mkspnexp.end();
	}

	////Makespan det-2
	//for (int t = 0; t < m - 1; ++t)
	//	for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
	//	{
	//		IloExpr linexp(env);
	//		for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
	//			linexp += K[a][b];
	//		model.add(linexp <= P[b]);
	//		linexp.end();
	//	}

	//at most one root
	for (int t = 0; t < m - 1; ++t)
		for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
		{
			IloExpr linexp(env);
			for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
				linexp += y[a][b];
			model.add(linexp <= 1);
			linexp.end();
		}

	//for (int t = 0; t < m - 1; ++t)
	//	for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
	//	{
	//		IloExpr linexp(env);
	//		for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
	//		{
	//			model.add(K[a][b] >= P[a] - /*(m - t)*mmin[0]*/ M * (1 - y[a][b])); //buna M için alternatif deneme!
	//			linexp.end();
	//		}
	//	}

	// symmetry breaking - assign to first blocks
	for (int i = 0; i < m; ++i)
		for (int a = i*preventnum; a < (i + 1)*preventnum - 1; ++a)
		{
			IloExpr exp(env);
			IloExpr exp2(env);
			for each(Job job in Jint)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				exp += x[0][a][j];
				exp2 += x[0][a + 1][j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				exp += x[1][a][j];
				exp2 += x[1][a + 1][j];
			}

			model.add(exp >= exp2);
		}

	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
		{
			IloExpr exp(env);
			IloExpr exp2(env);
			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
			{
				IloExpr exp(env);
				IloExpr exp2(env);
				for each(Job job in Jint)
				{
					int j = job.ID;
					/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
					continue;*/
					exp += x[0][b][j];
					exp2 += x[0][a][j];
				}

				for each(Job job in Jcor)
				{
					int j = job.ID;
					/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
					continue;*/
					exp += x[1][b][j];
					exp2 += x[1][a][j];
				}

				model.add(y[a][b] <= exp);//root'ta job varsa root da vardır!
				model.add(y[a][b] <= exp2);
				exp.end();
				exp2.end();
			}
			//cout << "y[" + to_string(a) + "][" + to_string(b) + "]\n";
		}

	//child varsa root da vardır! -- linking constraint
	for (int t = 0; t < m - 1; ++t)
		for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
		{
			IloExpr linexp(env);
			for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
				linexp += y[a][b];
			for each(Job job in Jint)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				model.add(x[0][a][j] <= linexp);
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				model.add(x[1][a][j] <= linexp);
			}

			linexp.end();
		}

	//Start time lb and finish time of base 
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
			{
				model.add(S[b] <= S[a] + M*(1 - y[a][b]));
				model.add(F[a] <= F[b] + M*(1 - y[a][b]));
			}

	//29.03.2017
	for (int t = m - 1; t < m; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
		{
			IloExpr exp(env);
			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][b][j] * pj[j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][b][j] * pj[j];
			}

			model.add(F[b] <= S[b] + exp);
			exp.end();
		}

	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
		{
			IloExpr exp(env);

			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][b][j] * pj[j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][b][j] * pj[j];
			}

			IloExpr exp2(env);
			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
				exp2 += y[a][b];

			model.add(F[b] <= S[b] + exp + M*exp2);
			exp.end();
			exp2.end();
		}

	//IloRangeArray tardyparents = CreateRangeArray(env, (m-1)*preventnum^2, "BlockFinish", 0, IloInfinity);

	//29.03 part2
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum; ++b)
		{
			IloExpr exp(env);

			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][b][j] * pj[j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][b][j] * pj[j];
			}

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum; ++a)
			{
				model.add(z[a][b] <= y[a][b]);
				model.add(F[b] >= F[a] - M*(1 - y[a][b]) - M*(1 - z[a][b]));
				model.add(F[b] <= S[b] + exp + M*(1 - y[a][b]) + M*z[a][b]);
				//model.add(F[a] >= F[b] - M*(1 - y[a][b]) - M*z[a][b]);
			}

			exp.end();
		}


	//Following base start after previous base
	for (int t = 0; t < m; t++)
		for (int b = t*preventnum; b < (t + 1)*preventnum - 1; ++b)
			model.add(S[b + 1] >= F[b]);

	//Block completion time
	for (int b = 0; b < s; ++b)
		model.add(F[b] >= S[b] + P[b]);

	//Block completion time - from rj
	int count = 0;
	for (int b = 0; b < s; ++b)
	{
		IloExpr exp(env);
		for each(Job job in Jint)
		{
			int j = job.ID;
			if (job.rj - realbegweek < begweek)
				continue;
			exp += x[0][b][j] * (job.rj - realbegweek);
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			if (job.rj - realbegweek < begweek)
				continue;
			exp += x[1][b][j] * (job.rj - realbegweek);
		}
		model.add(F[b] >= exp + P[b]);
	}

	//Job start time linearization
	for (int b = 0; b < s; ++b)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			//new method, from tard min
			/*if (binary_search(tardyjobs.begin(), tardyjobs.end(), j))
			continue;*/
			model.add(B[0][b][j] <= S[b]);
		}
		for each(Job job in Jcor)
		{
			int j = job.ID;
			//new method, from tard min
			/*if (binary_search(tardyjobs.begin(), tardyjobs.end(), j))
			continue;*/
			model.add(B[1][b][j] <= S[b]);
		}
	}

	for (int b = 0; b < s; ++b)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
			continue;*/
			model.add(B[0][b][j] >= S[b] - M*(1 - x[0][b][j]));
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
			continue;*/
			model.add(B[1][b][j] >= S[b] - M*(1 - x[1][b][j]));
		}
	}

	for (int b = 0; b < s; ++b)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
			continue;*/
			model.add(B[0][b][j] <= M*x[0][b][j]);
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
			continue;*/
			model.add(B[1][b][j] <= M*x[1][b][j]);
		}
	}

	//job start time >= ready time
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		/*IloExpr exp(env);
		for (int b = 0; b < s; ++b)
		exp += B[b][j];*/
		if (job.dj - realbegweek >= begweek)
			model.add((job.rj - realbegweek) * (1 - np[0][j]) <= Sj[0][j]);
		/*exp.end();*/
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		/*IloExpr exp(env);
		for (int b = 0; b < s; ++b)
		exp += B[b][j];*/
		if (job.dj - realbegweek >= begweek)
			model.add((job.rj - realbegweek) * (1 - np[1][j]) <= Sj[1][j]);
		/*exp.end();*/
	}

	//Çıkış zamanı için due date hesaplama!
	//Job completion time linearization
	for (int b = 0; b < s; ++b)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			model.add(L[0][b][j] <= F[b]);
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			model.add(L[1][b][j] <= F[b]);
		}
	}


	for (int b = 0; b < s; ++b)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			model.add(L[0][b][j] >= F[b] - M*(1 - x[0][b][j]));
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			model.add(L[1][b][j] >= F[b] - M*(1 - x[1][b][j]));
		}
	}

	for (int b = 0; b < s; ++b)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			model.add(L[0][b][j] <= M*x[0][b][j]);
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			model.add(L[1][b][j] <= M*x[1][b][j]);
		}
	}

	//Tardiness - cj obj
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (job.dj - realbegweek > endweek)
		continue;*/
		//model.add(T[0][j] >= Sj[0][j] - (job.dj - realbegweek) - tt*np[0][j]);
		model.add(T[0][j] >= Sj[0][j] + endweek - (job.dj - realbegweek) - tt*(1 - np[0][j]));
	}

	//corrective tardinessı önemli değil, couroutservice zaten bunu sağlıyor!
	for each(Job job in Jcor)
	{
		int j = job.ID;
		//model.add(T[1][j] >= Sj[1][j] - (job.dj - realbegweek) - tt*np[1][j]);
		model.add(T[1][j] >= Sj[1][j] + endweek - (job.dj - realbegweek) - tt*(1 - np[1][j]));
	}

	//valid1 - E[j] upper bound?
	//for each(Job job in Jint)
	//{
	//	int j = job.ID;
	//	if (job.dj - realbegweek >= begweek)
	//		model.add(E[0][j] <= (job.dj - realbegweek) /** (1 - np[j])*/);
	//}

	//preventive earliness
	for each(Job job in Jint)
	{
		int j = job.ID;
		//either-or
		/*if (binary_search(tardyjobs.begin(), tardyjobs.end(), j))
		model.add(E[0][j] == 0);
		else if (job.dj - realbegweek >= begweek && job.dj - realbegweek <= endweek)
		model.add(E[0][j] >= job.dj - realbegweek - Sj[0][j] - M*np[0][j]);
		else if (job.dj - realbegweek > endweek)
		{*/
		model.add(E[0][j] >= (job.dj - realbegweek) - Sj[0][j] - tt*np[0][j]);
		/*}*/
	}


	//Tardiness - 2 - upper bound
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(correctives.begin(), correctives.end(), job))
		continue;*/

		if (job.dj - realbegweek >= begweek)
		{
			/*int ub;
			if (job.dj - realbegweek < tt)
			ub = Getmin(job.Ymax - realbegweek, tt);
			else
			ub = job.Ymax;*/
			model.add(T[0][j] - deltaYmaxplus[j] <= job.Ymax - job.dj /**(1 - np[j])*/);
			//model.add(T[0][j] - deltaYmaxplus[j] <= job.Ymax - job.dj/**(1 - np[j])*/);
		}
	}

	//end time of maintenance
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += t*e[0][j][t];
		model.add(exp == C[0][j]);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += t*e[1][j][t];
		model.add(exp == C[1][j]);
		exp.end();
	}

	//maint begin
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int b = 0; b < s; ++b)
			exp += B[0][b][j];
		model.add(Sj[0][j] == exp);
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int b = 0; b < s; ++b)
			exp += B[1][b][j];
		model.add(Sj[1][j] == exp);
	}

	//maint completion j
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int b = 0; b < s; ++b)
			exp += L[0][b][j];
		model.add(C[0][j] == exp);
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int b = 0; b < s; ++b)
			exp += L[1][b][j];
		model.add(C[1][j] == exp);
	}

	// maint begn = park begin -- patlarsa >=
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += t*b[0][j][t];
		model.add(exp == Sj[0][j]);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += t*b[1][j][t];
		model.add(exp == Sj[1][j]);
		exp.end();
	}

	//hangar sched block for maint of job j
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		IloExpr exp2(env);
		IloExpr exp3(env);
		for (int b = 0; b < s; ++b)
		{
			exp += L[0][b][j];
			exp2 += B[0][b][j];
		}
		for (int t = begweek; t < endweek; ++t)
			exp3 += a[0][j][t];
		model.add(exp3 == exp - exp2);
		exp.end();
		exp2.end();
		exp3.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		IloExpr exp2(env);
		IloExpr exp3(env);
		for (int b = 0; b < s; ++b)
		{
			exp += L[1][b][j];
			exp2 += B[1][b][j];
		}
		for (int t = begweek; t < endweek; ++t)
			exp3 += a[1][j][t];
		model.add(exp3 == exp - exp2);
		exp.end();
		exp2.end();
		exp3.end();
	}

	// job must begin somewhere
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += b[0][j][t];
		model.add(exp + np[0][j] == 1);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += b[1][j][t];
		model.add(exp + np[1][j] == 1);
		exp.end();
	}

	// job must end somewhere
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += e[0][j][t];
		model.add(exp + np[0][j] == 1);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += e[1][j][t];
		model.add(exp + np[1][j] == 1);
		exp.end();
	}

	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int t = begweek; t < endweek - 1; ++t)
		{
			IloExpr exp(env);
			//for (int i = 0; i <= t; ++i)
			for (int i = begweek; i <= t + 1; ++i)
				exp += e[0][j][i];
			model.add(a[0][j][t + 1] <= 1 - exp);
			exp.end();
		}

	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int t = begweek; t < endweek - 1; ++t)
		{
			IloExpr exp(env);
			//for (int i = 0; i <= t; ++i)
			for (int i = begweek; i <= t + 1; ++i)
				exp += e[1][j][i];
			model.add(a[1][j][t + 1] <= 1 - exp);
			exp.end();
		}
	}

	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int t = begweek; t < endweek; ++t)
		{
			IloExpr exp(env);
			for (int i = begweek; i <= t; ++i)
				exp += b[0][j][i];
			//if (t == 0)
			model.add(a[0][j][t] <= exp);
			//else
			//model.add(a[j][t - 1] <= exp);
			exp.end();
		}
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int t = begweek; t < endweek; ++t)
		{
			IloExpr exp(env);
			for (int i = begweek; i <= t; ++i)
				exp += b[1][j][i];
			//if (t == 0)
			model.add(a[1][j][t] <= exp);
			//else
			//model.add(a[j][t - 1] <= exp);
			exp.end();
		}
	}

	//Bunu sonra düzgünce yazalım!
	//changed inequality - from SLA lazy constraints
	// bir de lazy yapıp bakılmalı!

	int noncriticsize = n - preventnum;
	for (int t = begweek; t < endweek; t++)
	{
		IloExpr exp(env);

		for each(Job job in Jint)
		{
			int j = job.ID;
			exp += -a[0][j][t];
			exp += -o[0][j][t];
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			exp += -a[1][j][t];
			exp += -o[1][j][t];
		}

		exp += deltaminusSLA[t];
		model.add(exp >= SLA[t] - n);
		exp.end();
	}
}


void SetupModel42(int maxYmin, int maxYmax, int minYmin, vector <int> mmin, int s, int n, int tt, int m, int M, int* SLA, int* pj, int realbegweek, int begweek, int endweek, IloEnv& env, IloModel& model, IloCplex& cplex, BoolVarArray3& o,
	BoolVarArray2& y, IloBoolVarArray& dummy, BoolVarArray2& z, IloNumVarArray& P, BoolVarArray3& x, BoolVarArray3& e, BoolVarArray3& a, BoolVarArray3& b, NumVarArray2& E, NumVarArray2& T, NumVarArray2& C, NumVarArray2& Sj, IloNumVarArray& S,
	IloNumVarArray& F, IloNumVarArray& K, NumVarArray3& L, NumVarArray3& B, const /*IloNumVarArray& np,*/ BoolVarArray2& np, IloNumVarArray& deltaminusSLA, IloNumVarArray& deltaYmaxplus/*prevjob*/, IloNumVarArray& coroutservice/*corjob*/, vector<Job>& Jint, vector<Job>& Jcor,
	vector<int>& preventives, vector<int>& tardyjobs)
{
	sort(tardyjobs.begin(), tardyjobs.end());
	int preventnum = Jint.size() + Jcor.size();
	int left;
	if (minYmin < realbegweek)
		left = 0;
	else
		left = minYmin;

	int dist;
	int tot = 0;
	int childub = 0;

	for (int i = mmin.size() - 1; i > 0; --i)
	{
		tot += mmin[i];
		++childub;

		if (tot < mmin[0])
		{

			continue;
		}
		else
			break;
	}

	////5.29.17
	//for (int t = 0; t < m - 1; ++t)
	//	for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
	//	{
	//		dist = b - t*preventnum + 1;
	//		int count = 0;
	//		int threshold = dist*childub;

	//		for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
	//		{
	//			++count;
	//			if (count > threshold)
	//			{
	//				model.add(y[a][b] == 0);
	//				model.add(z[a][b] == 0);
	//			}

	//		}
	//	}

	//corrective out of service
	for each (Job job in Jcor)
	{
		int j = job.ID;
		int ub = job.rj - realbegweek;
		if (ub < 0)
			ub = 0;
		//valid inequlity?
		/*for (int t = begweek; t < ub; ++t)
		model.add(o[1][j][t] == 0);*/
		// - ise sıfır yapalım!
		for (int i = ub; i < tt; ++i)
		{
			IloExpr exp(env);
			for (int t = ub; t <= i; ++t)
				exp += b[1][j][t];
			model.add(1 - exp == o[1][j][i]);
			exp.end();
		}
	}

	//new2 - preventive out of service
	for each (Job job in Jint)
	{
		int j = job.ID;
		if (job.Ymax - realbegweek < endweek)
		{
			int maintstart = -1;
			if (job.rj - realbegweek < begweek)
				maintstart = begweek;
			else
				maintstart = job.rj - realbegweek;

			int outstart;
			if (job.Ymax - realbegweek <= begweek)
				outstart = 0;
			else
				outstart = job.Ymax - realbegweek;

			//ymax öncesi o_jt = 0
			//valid inequality?
			/*for (int t = begweek; t <= outstart - 1; ++t)
			model.add(o[0][j][t] == 0);*/

			////sonrasında, beg'den itibaren başlayan b_jt o_jt'yi ymax'tan itibaren 0 yapabilir!

			for (int i = outstart; i < endweek; ++i)
			{
				IloExpr exp(env);
				for (int t = maintstart; t <= i; ++t)
					exp += b[0][j][t];
				model.add(1 - exp == o[0][j][i]); //i burada i geliyor baştan, bunu düzenlemek lazım!
				exp.end();
				//model.add(a[0][j][i] + o[0][j][i] <= 1); //new ineq
			}

		}
	}

	//Job assignment constraints-prev
	for each (Job job in Jint)
	{
		int j = job.ID;
		IloExpr jbasnmntexp(env);
		/*if(binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int i = 0; i < m; ++i)
			for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
				jbasnmntexp += x[0][b][j];
		model.add(jbasnmntexp + np[0][j] == 1);
		jbasnmntexp.end();
	}

	//Job assignment constraints-cor
	for each (Job job in Jcor)
	{
		int j = job.ID;
		IloExpr jbasnmntexp(env);
		/*if(binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		for (int i = 0; i < m; ++i)
			for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
				jbasnmntexp += x[1][b][j];
		model.add(jbasnmntexp + np[1][j] == 1);
		jbasnmntexp.end();
	}

	//base  assignmnts
	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			IloExpr baseexp(env);
			for each(Job job in Jint)
			{
				int j = job.ID;
				baseexp += x[0][b][j];
			}
			for each(Job job in Jcor)
			{
				int j = job.ID;
				baseexp += x[1][b][j];
			}

			model.add(baseexp <= 1);
			baseexp.end();
		}

	int ub;


	// symmetry breaking - assign to first blocks
	for (int i = 0; i < m; ++i)
		for (int a = i*preventnum; a < (i + 1)*preventnum - 1 - i; ++a)
		{
			IloExpr exp(env);
			IloExpr exp2(env);
			for each(Job job in Jint)
			{
				int j = job.ID;

				exp += x[0][a][j];
				exp2 += x[0][a + 1][j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;

				exp += x[1][a][j];
				exp2 += x[1][a + 1][j];
			}

			model.add(exp >= exp2);
		}

	//18
	for each(Job job in Jint)
	{
		int j = job.ID;
		int rj = job.rj - realbegweek;
		if (rj > 0)
			model.add(Sj[0][j] >= rj*(1 - np[0][j]));
	}

	////job start time >= ready time
	//for each(Job job in Jint)
	//{
	//	int j = job.ID;
	//	/*IloExpr exp(env);
	//	for (int b = 0; b < s; ++b)
	//	exp += B[b][j];*/
	//	if (job.rj - realbegweek > begweek)
	//		model.add((job.rj - realbegweek) * (1 - np[0][j]) <= Sj[0][j]);
	//	/*exp.end();*/
	//}

	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			dist = b - t*preventnum + 1;

			int count = 0;
			int threshold = dist*childub;

			IloExpr exp(env);
			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][b][j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][b][j];
			}

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
					model.add(y[a][b] <= exp); //root'ta job varsa root da vardır!				
			}

			exp.end();
		}

	//child varsa y_ab 1 root seçer -- //sonra gelecegiz.
	for (int t = 0; t < m - 1; ++t)
		for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - (t + 1); ++a)
		{
			int count = 0;
			dist = a - (t + 1)*preventnum + 1;
			IloExpr exp(env);
			IloExpr linexp(env);
			for (int b = t*preventnum; b < (t + 1)*preventnum - dist - t; ++b)
			{
				linexp += y[a][b];
				++count;
			}

			if (count == 0)
			{
				//model.add(linexp == 0);
				continue;
			}

			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][a][j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][a][j];
			}

			model.add(exp == linexp); //20.2.2017
			linexp.end();
			exp.end();
		}

	//29.03.2017-last track
	/*for (int t = m - 1; t < m; ++t)
	for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
	{
	IloExpr exp(env);
	for each(Job job in Jint)
	{
	int j = job.ID;
	exp += x[0][b][j] * pj[j];
	}

	for each(Job job in Jcor)
	{
	int j = job.ID;
	exp += x[1][b][j] * pj[j];
	}

	model.add(F[b] <= S[b] + exp);
	exp.end();
	}*/

	//20.2.18 - disag1
	for (int t = m - 1; t < m; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				model.add(L[0][b][j] <= B[0][b][j] + x[0][b][j] * job.pj);
			}
			for each(Job job in Jcor)
			{
				int j = job.ID;
				model.add(L[1][b][j] <= B[1][b][j] + x[1][b][j] * job.pj);
			}
		}

	////////5.22.17
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			IloExpr exp(env);
			dist = b - t*preventnum + 1;

			int count = 0;
			int threshold = dist*childub;

			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][b][j] * pj[j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][b][j] * pj[j];
			}

			IloExpr exp2(env);

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
					exp2 += y[a][b]; //root'ta job varsa root da vardır!

			}

			model.add(F[b] <= S[b] + exp + M*exp2);
			exp.end();
			exp2.end();
		}

	//IloRangeArray tardyparents = CreateRangeArray(env, (m-1)*preventnum^2, "BlockFinish", 0, IloInfinity);

	//29.03 part2
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			dist = b - t*preventnum + 1;
			IloExpr exp(env);

			int count = 0;
			int threshold = dist*childub;

			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][b][j] * pj[j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][b][j] * pj[j];
			}

			//20.06.17-3
			//z_ab=either-or dummy variable

			IloExpr exp2(env);

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
				{
					model.add(z[a][b] <= y[a][b]);

					exp2 += z[a][b];

					model.add(F[b] >= F[a] - M*(1 - y[a][b]));
					model.add(F[b] <= F[a] + M*(1 - z[a][b]));

					//20.06.17 //needed to set true s_a value
					model.add(S[a] <= S[b] + exp - 1 + M*(1 - y[a][b]));
				}
			}

			model.add(exp2 + dummy[b] == 1);

			//model.add(F[b] >= S[b] + exp);
			model.add(F[b] <= S[b] + exp + M*(1 - dummy[b]));

			exp.end();
			exp2.end();
		}

	//Start time lb and finish time of base 
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			dist = b - t*preventnum + 1;

			int count = 0;
			int threshold = dist*childub;

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
				{
					model.add(S[b] <= S[a] + M*(1 - y[a][b]));
				}

			}
		}

	////5.29.17
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			dist = b - t*preventnum + 1;
			int count = 0;
			int threshold = dist*childub;

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
				{
					model.add(y[a][b] == 0);
					model.add(z[a][b] == 0);
				}

			}
		}


	////4.8.17
	for (int t = 1; t < m; ++t)
	{
		int dist_x = 0;
		for (int i = 0; i < preventnum - t; ++i)
		{
			int toplam;
			int a = ceil((i + 1) / childub);
			if (t == 1)
				toplam = (i + 1) + a;
			else
				toplam = (i + 1) + a + ceil(a / childub);

			if (toplam > preventnum)
			{
				dist_x = i;
				break;
			}
		}

		if (dist_x > 0)
		{
			int baslangic = dist_x + t*preventnum;

			for (int b = baslangic; b < (t + 1)*preventnum - t; ++b)
			{
				for each(Job job in Jint)
				{
					int j = job.ID;
					model.add(x[0][b][j] == 0);
				}
				for each(Job job in Jcor)
				{
					int j = job.ID;
					model.add(x[1][b][j] == 0);

				}

			}
		}

	}


	//Following base start after previous base
	for (int t = 0; t < m; t++)
		for (int b = t*preventnum; b < (t + 1)*preventnum - 1 - t; ++b)
			model.add(S[b + 1] >= F[b]);

	//Block completion time
	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
			model.add(F[b] >= S[b] + P[b]);

	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			IloExpr exp(env);

			for each(Job job in Jint)
			{
				int j = job.ID;
				exp += x[0][b][j] * pj[j];
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp += x[1][b][j] * pj[j];
			}

			model.add(P[b] >= exp);
			exp.end();
		}


	//Block completion time - from rj
	int count = 0;
	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			IloExpr exp(env);
			for each(Job job in Jint)
			{
				int j = job.ID;
				if (job.rj - realbegweek > begweek)
					exp += x[0][b][j] * (job.rj - realbegweek);
				//continue;
			}

			model.add(S[b] >= exp);
		}

	//Job start time linearization
	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				//new method, from tard min
				/*if (binary_search(tardyjobs.begin(), tardyjobs.end(), j))
				continue;*/
				model.add(B[0][b][j] <= S[b]);
			}
			for each(Job job in Jcor)
			{
				int j = job.ID;
				//new method, from tard min
				/*if (binary_search(tardyjobs.begin(), tardyjobs.end(), j))
				continue;*/
				model.add(B[1][b][j] <= S[b]);
			}
		}

	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				model.add(B[0][b][j] >= S[b] - M*(1 - x[0][b][j]));
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				model.add(B[1][b][j] >= S[b] - M*(1 - x[1][b][j]));
			}
		}

	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				model.add(B[0][b][j] <= M*x[0][b][j]);
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
				continue;*/
				model.add(B[1][b][j] <= M*x[1][b][j]);
			}
		}




	//Çıkış zamanı için due date hesaplama!
	//Job completion time linearization
	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				model.add(L[0][b][j] <= F[b]);
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				model.add(L[1][b][j] <= F[b]);
			}
		}


	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				model.add(L[0][b][j] >= F[b] - M*(1 - x[0][b][j]));
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				model.add(L[1][b][j] >= F[b] - M*(1 - x[1][b][j]));
			}
		}

	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				model.add(L[0][b][j] <= M*x[0][b][j]);
			}

			for each(Job job in Jcor)
			{
				int j = job.ID;
				model.add(L[1][b][j] <= M*x[1][b][j]);
			}
		}

	//Tardiness - cj obj
	for each(Job job in Jint)
	{
		int j = job.ID;
		model.add(T[0][j] >= Sj[0][j] + endweek - (job.dj - realbegweek) - tt*(1 - np[0][j]));
	}

	//corrective tardinessı önemli değil, couroutservice zaten bunu sağlıyor!
	for each(Job job in Jcor)
	{
		int j = job.ID;
		model.add(T[1][j] >= Sj[1][j] + endweek - (job.dj - realbegweek) - tt*(1 - np[1][j]));
	}


	//preventive earliness
	for each(Job job in Jint)
	{
		int j = job.ID;
		if (binary_search(tardyjobs.begin(), tardyjobs.end(), j))
		{
			//model.add(E[0][j] == 0);
			continue;
		}

		model.add(E[0][j] >= (job.dj - realbegweek) - Sj[0][j] - tt*np[0][j]);
	}


	//Tardiness - 2 - upper bound
	for each(Job job in Jint)
	{
		int j = job.ID;
		model.add(T[0][j] - deltaYmaxplus[j] <= job.Ymax - job.dj);
	}

	//end time of maintenance
	for each(Job job in Jint)
	{
		int j = job.ID;
		int ub = job.rj - realbegweek;
		if (ub < 0)
			ub = 0;

		IloExpr exp(env);
		for (int t = ub + job.pj; t < endweek; ++t)
			exp += t*e[0][j][t];
		model.add(exp == C[0][j]);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = job.pj; t < endweek; ++t)
			exp += t*e[1][j][t];
		model.add(exp == C[1][j]);
		exp.end();
	}

	//maint begin
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int i = 0; i < m; ++i)
			for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
				exp += B[0][b][j];
		model.add(Sj[0][j] == exp);
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int i = 0; i < m; ++i)
			for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
				exp += B[1][b][j];
		model.add(Sj[1][j] == exp);
	}

	//maint completion j
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int i = 0; i < m; ++i)
			for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
				exp += L[0][b][j];
		model.add(C[0][j] == exp);
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int i = 0; i < m; ++i)
			for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
				exp += L[1][b][j];
		model.add(C[1][j] == exp);
	}

	// maint begn = park begin -- patlarsa >=
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		int ub = job.rj - realbegweek;
		if (ub < 0)
			ub = 0;
		for (int t = ub; t < endweek; ++t)
			exp += t*b[0][j][t];
		model.add(exp == Sj[0][j]);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += t*b[1][j][t];
		model.add(exp == Sj[1][j]);
		exp.end();
	}

	//// job must begin somewhere
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		int ub = job.rj - realbegweek;
		if (ub < 0)
			ub = 0;
		for (int t = ub; t < endweek; ++t)
			exp += b[0][j][t];
		model.add(exp + np[0][j] == 1);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = begweek; t < endweek; ++t)
			exp += b[1][j][t];
		model.add(exp + np[1][j] == 1);
		exp.end();
	}

	// job must end somewhere
	for each(Job job in Jint)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		int ub = job.rj - realbegweek;
		if (ub < 0)
			ub = 0;
		for (int t = ub + job.pj; t < endweek; ++t)
			exp += e[0][j][t];
		model.add(exp + np[0][j] == 1);
		exp.end();
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		/*if (binary_search(coorrectvprvntvdatas.begin(), coorrectvprvntvdatas.end(), j))
		continue;*/
		IloExpr exp(env);
		for (int t = job.pj; t < endweek; ++t)
			exp += e[1][j][t];
		model.add(exp + np[1][j] == 1);
		exp.end();
	}

	//27 //aşağıdaki iki constraint yerine!
	for each(Job job in Jint)
	{
		int j = job.ID;
		ub = job.rj - realbegweek;
		int dj = job.dj - realbegweek;
		if (ub < 0)
			ub = 0;
		//if (ub + job.pj>endweek && dj > tt) //çünkü np_j=1
		//	continue;
		for (int t = ub; t < endweek; ++t)
		{
			IloExpr exp1(env);
			IloExpr exp2(env);
			for (int i = ub; i <= t; ++i)
				exp1 += b[0][j][i];

			for (int i = ub + job.pj; i <= t; ++i)
				exp2 += e[0][j][i];

			model.add(a[0][j][t] == exp1 - exp2);
			exp1.end();
			exp2.end();
		}
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		ub = job.rj - realbegweek;
		int dj = job.dj - realbegweek;
		if (ub < 0)
			ub = 0;

		for (int t = ub; t < endweek; ++t)
		{
			IloExpr exp1(env);
			IloExpr exp2(env);
			for (int i = ub; i <= t; ++i)
				exp1 += b[1][j][i];

			for (int i = ub + job.pj; i <= t; ++i)
				exp2 += e[1][j][i];

			model.add(a[1][j][t] == exp1 - exp2);
			exp1.end();
			exp2.end();
		}
	}

	int noncriticsize = n - preventnum;
	int initservice = 0;

	for (int t = begweek; t < endweek; t++)
	{
		//for each(Job job in Jint)
		//{
		//	int j = job.ID;
		//	if (t < job.rj - realbegweek)
		//	{
		//		model.add(a[0][j][t] == 0);
		//		model.add(o[0][j][t] == 0);
		//		++initservice;
		//	}

		//}

		//if (noncriticsize + initservice >= SLA[t]) //if i can satisfy all sla from noncritics, then it becomes redundant. But we change the logic
		//{
		//	model.add(deltaminusSLA[t] == 0);
		//	initservice = 0;
		//}

		IloExpr exp(env);

		for each(Job job in Jint)
		{
			int j = job.ID;
			exp += -a[0][j][t];
			exp += -o[0][j][t];
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			exp += -a[1][j][t];
			exp += -o[1][j][t];
		}

		exp += deltaminusSLA[t];
		model.add(exp >= SLA[t] - n);
		exp.end();
	}
}

bool SolveModel(double& z1, int* SLAdaily, int totaltt, ofstream& myfile, int warmup, int dayindex, double& time, double* cumtime, double* cum, int pr, int sol,
	int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, ofstream& datarunfile, vector<int>& critics, int pencor, int penminus, int penymaxplus,
	int maxYmin, int maxYmax, int minYmin, int buffer, vector <int> mmin, int s, int n, int tt, int m, int M, int* SLA, int* pj, int realbegweek, int begweek, int endweek, vector<Job>& Jint, vector<Job>& Jcor,
	vector<int>& preventives, vector<int>& tardyjobs, vector<int>& correctives, int* d, int* Ymax, int* rr, int limit, int** prevprdtms, int bf, int coef)
{
	//3 indices: (type)(job)(time) type0 - prev; 1 - corm
	IloEnv env;
	IloModel model(env);

	IloBoolVarArray dummy = CreateBoolVarArray(env, s, "dummy");

	BoolVarArray2 y = CreateBoolVarArray2(env, s, s, "y"); //yab
	BoolVarArray2 z = CreateBoolVarArray2(env, s, s, "z"); //zab
	BoolVarArray3 x = CreateBoolVarArray3(env, 2, s, n, "x"); // 
	BoolVarArray3 e = CreateBoolVarArray3(env, 2, n, tt, "e"); // 1 at maint j exit, 0 else
	BoolVarArray3 a = CreateBoolVarArray3(env, 2, n, tt, "a"); //1 at maint, 0 else
	BoolVarArray3 b = CreateBoolVarArray3(env, 2, n, tt, "b"); // 1 at maint j begin, 0 else
	BoolVarArray3 o = CreateBoolVarArray3(env, 2, n, tt, "o");   //out of service
	BoolVarArray2 np = CreateBoolVarArray2(env, 2, n, "np");   //to next period
	IloNumVarArray P = CreateNumVarArray(env, s, "P", 0, IloInfinity); //block makespan
	NumVarArray2 E = CreateNumVarArray2(env, 2, n, "E", 0, IloInfinity);
	NumVarArray2 T = CreateNumVarArray2(env, 2, n, "T", 0, IloInfinity);//tardiness
	NumVarArray2 C = CreateNumVarArray2(env, 2, n, "C", 0, IloInfinity); //Job completion
	NumVarArray2 Sj = CreateNumVarArray2(env, 2, n, "Sj", 0, IloInfinity); //Job start
	IloNumVarArray S = CreateNumVarArray(env, s, "S", 0, IloInfinity); // Start time of block b
	IloNumVarArray F = CreateNumVarArray(env, s, "F", 0, IloInfinity); // Finish time of block b

	//Linearization vars
	IloNumVarArray K = CreateNumVarArray(env, s, "K", 0, IloInfinity); // Finish time of block b
	NumVarArray3 L = CreateNumVarArray3(env, 2, s, n, "L", 0, IloInfinity); // ENd bj
	NumVarArray3 B = CreateNumVarArray3(env, 2, s, n, "B", 0, IloInfinity); //linearization var - begin bj

	IloNumVarArray deltaminusSLA = CreateNumVarArray(env, tt, "deltaSLA-", 0, IloInfinity); //sladundersatisfaction
	IloNumVarArray deltaYmaxplus = CreateNumVarArray(env, n, "deltaYmax+", 0, IloInfinity); //prevjobs
	IloNumVarArray coroutservice = CreateNumVarArray(env, n, "coroutserv", 0, IloInfinity); //corjobs

	NumVarArray2 Tj = CreateNumVarArray2(env, 2, n, "Tj", 0, IloInfinity);//finsh tardiness

	IloExpr objExp(env);

	if (bf == 2)
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += coef * (pencor - penymaxplus)*T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}
	else
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		objExp += pencor*T[1][j];
	}

	for (int t = 0; t < tt; ++t)
		objExp += penminus*deltaminusSLA[t];

	IloObjective obj = IloMinimize(env, objExp, "obj");
	model.add(obj);
	objExp.end();
	IloCplex cplex(model);
	int criticsize = critics.size();

	//initials
	vector<int> initials;

	int maxnontard = -1;
	int mintard = 1000;

	int preventnum = Jint.size() + Jcor.size();


	vector<int> initbasenums;
	//for (int i = 0; i < m; ++i)

	if (dayindex > 0)
	{
		//prvprdtms'tan ilgili şeyleri çekip model.add dersin

		for (int t = 0; t < m; ++t)
		{
			if (prevprdtms[t][0] != -1)
			{
				int job = prevprdtms[t][0];
				int type = prevprdtms[t][1];
				int delay = prevprdtms[t][2];
				//int start = prevprdtms[t][3];

				initials.push_back(job);
				initbasenums.push_back(t*preventnum);

				model.add(x[type][t*preventnum][job] == 1);

				//2.11.17-1
				for (int i = 0; i < m; ++i)
					for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
					{
						if (b != t*preventnum)
							model.add(x[type][b][job] == 0);
					}

				model.add(L[type][t*preventnum][job] >= delay);
				model.add(B[type][t*preventnum][job] == 0);
			}
		}
	}


	SetupModel(maxYmin, maxYmax, minYmin, mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek, env, model, cplex, o, y, dummy, z, P, x, e, a, b, E, T, C, Sj, S, F, K, L, B, np, deltaminusSLA, deltaYmaxplus, coroutservice, Jint, Jcor, preventives, tardyjobs);

	//cplex.setParam(IloCplex::TiLim, 10);

	cplex.setParam(IloCplex::TiLim, 60*2);

	//double curtime = cplex.getCplexTime() - time;

	time = cplex.getCplexTime();

	IloBool success = cplex.solve();
	double elapsedtime = cplex.getCplexTime() - time;
	//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);

	if (success && cplex.isPrimalFeasible())
	{
		/*vector<vector<Base>> cand = Write(model, cplex, env, myfile, n, begweek, endweek, realbegweek, warmup, dayindex, tt, m, s, time, cumtime, cum, pr, sol, cumSLAviol, cumYmaxviol, cumgap, cumnodes, buffer
		, B, o, y, P, x, e, a, b, E, T, C, Sj, S, F, L, np, deltaminusSLA, deltaYmaxplus, coroutservice, datarunfile, Jcor, Jint, initials, SLA, prevprdtms);

		//DeleteJobsbuffer(Jint, Jcor, cplex, x, np, C, Sj, S, F, prevprdtms, preventnum, m, cand);*/

		myfile << Jint.size() << "\t" << Jcor.size() << "\t" << elapsedtime << "\t" << cplex.getObjValue() << "\t" << cplex.getMIPRelativeGap() * 100 << "\t";

	}

	else
	{
		// olmadı instanceları buraya yazdır!		
		ofstream infsbleinst("infsble.txt");
		//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);
		cplex.exportModel("infeasible.lp");

		myfile << "INFEASIBLE!!!!" << endl;
		myfile << "success = " << success << endl;
		myfile << "cplex.isPrimalFeasible() = " << cplex.isPrimalFeasible() << endl;
		myfile.close();
		infsbleinst.close();
		return true;

	}

	cplex.end();
	model.end();
	env.end();
	//delete[] tracklimit;
	return false;
}

bool SolveModelCut(double& z1, int* SLAdaily, int totaltt, ofstream& myfile, int warmup, int dayindex, double& time, double* cumtime, double* cum, int pr, int sol,
	int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, ofstream& datarunfile, vector<int>& critics, int pencor, int penminus, int penymaxplus,
	int maxYmin, int maxYmax, int minYmin, int buffer, vector <int> mmin, int s, int n, int tt, int m, int M, int* SLA, int* pj, int realbegweek, int begweek, int endweek, vector<Job>& Jint, vector<Job>& Jcor,
	vector<int>& preventives, vector<int>& tardyjobs, vector<int>& correctives, int* d, int* Ymax, int* rr, int limit, int** prevprdtms, int bf, int coef)
{
	//3 indices: (type)(job)(time) type0 - prev; 1 - corm
	IloEnv env;
	IloModel model(env);

	IloBoolVarArray dummy = CreateBoolVarArray(env, s, "dummy");

	BoolVarArray2 y = CreateBoolVarArray2(env, s, s, "y"); //yab
	BoolVarArray2 z = CreateBoolVarArray2(env, s, s, "z"); //zab
	BoolVarArray3 x = CreateBoolVarArray3(env, 2, s, n, "x"); // 
	BoolVarArray3 e = CreateBoolVarArray3(env, 2, n, tt, "e"); // 1 at maint j exit, 0 else
	BoolVarArray3 a = CreateBoolVarArray3(env, 2, n, tt, "a"); //1 at maint, 0 else
	BoolVarArray3 b = CreateBoolVarArray3(env, 2, n, tt, "b"); // 1 at maint j begin, 0 else
	BoolVarArray3 o = CreateBoolVarArray3(env, 2, n, tt, "o");   //out of service
	BoolVarArray2 np = CreateBoolVarArray2(env, 2, n, "np");   //to next period
	IloNumVarArray P = CreateNumVarArray(env, s, "P", 0, IloInfinity); //block makespan
	NumVarArray2 E = CreateNumVarArray2(env, 2, n, "E", 0, IloInfinity);
	NumVarArray2 T = CreateNumVarArray2(env, 2, n, "T", 0, IloInfinity);//tardiness
	NumVarArray2 C = CreateNumVarArray2(env, 2, n, "C", 0, IloInfinity); //Job completion
	NumVarArray2 Sj = CreateNumVarArray2(env, 2, n, "Sj", 0, IloInfinity); //Job start
	IloNumVarArray S = CreateNumVarArray(env, s, "S", 0, IloInfinity); // Start time of block b
	IloNumVarArray F = CreateNumVarArray(env, s, "F", 0, IloInfinity); // Finish time of block b

	//Linearization vars
	IloNumVarArray K = CreateNumVarArray(env, s, "K", 0, IloInfinity); // Finish time of block b
	NumVarArray3 L = CreateNumVarArray3(env, 2, s, n, "L", 0, IloInfinity); // ENd bj
	NumVarArray3 B = CreateNumVarArray3(env, 2, s, n, "B", 0, IloInfinity); //linearization var - begin bj

	IloNumVarArray deltaminusSLA = CreateNumVarArray(env, tt, "deltaSLA-", 0, IloInfinity); //sladundersatisfaction
	IloNumVarArray deltaYmaxplus = CreateNumVarArray(env, n, "deltaYmax+", 0, IloInfinity); //prevjobs
	IloNumVarArray coroutservice = CreateNumVarArray(env, n, "coroutserv", 0, IloInfinity); //corjobs

	NumVarArray2 Tj = CreateNumVarArray2(env, 2, n, "Tj", 0, IloInfinity);//finsh tardiness

	IloExpr objExp(env);

	if (bf == 2)
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += coef * (pencor - penymaxplus)*T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}
	else
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		objExp += pencor*T[1][j];
	}

	for (int t = 0; t < tt; ++t)
		objExp += penminus*deltaminusSLA[t];

	IloObjective obj = IloMinimize(env, objExp, "obj");
	model.add(obj);
	objExp.end();
	IloCplex cplex(model);
	int criticsize = critics.size();

	//initials
	vector<int> initials;

	int maxnontard = -1;
	int mintard = 1000;

	int preventnum = Jint.size() + Jcor.size();


	vector<int> initbasenums;
	//for (int i = 0; i < m; ++i)

	if (dayindex > 0)
	{
		//prvprdtms'tan ilgili şeyleri çekip model.add dersin

		for (int t = 0; t < m; ++t)
		{
			if (prevprdtms[t][0] != -1)
			{
				int job = prevprdtms[t][0];
				int type = prevprdtms[t][1];
				int delay = prevprdtms[t][2];
				//int start = prevprdtms[t][3];

				initials.push_back(job);
				initbasenums.push_back(t*preventnum);

				model.add(x[type][t*preventnum][job] == 1);

				//2.11.17-1
				for (int i = 0; i < m; ++i)
					for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
					{
						if (b != t*preventnum)
							model.add(x[type][b][job] == 0);
					}

				model.add(L[type][t*preventnum][job] >= delay);
				model.add(B[type][t*preventnum][job] == 0);
			}
		}
	}

	vector<int> PW;

	for each(Job job in Jint)
	{
		if (job.rj <= realbegweek + mmin[0]) //if r_j <= begweek + pmax
			PW.push_back(job.ID);
	}

	sort(PW.begin(), PW.end());

	int blocks = Getmin(tardyjobs.size(), m);
	int ilkblocks = blocks;

	//bool termin = true;
	if (tardyjobs.size() > 0 && initials.size() == 0)
	{
		int base = 0;
		while (blocks > 0)
		{
			IloExpr exp1(env);
			//IloExpr exp2(env);

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp1 += x[1][base][j];
				//model.add(B[1][base][j] == begweek);
			}

			for each(Job job in Jint)
			{
				int j = job.ID;
				//exp2 += x[0][base][j];
				//model.add(B[0][base][j] == 0);

				if (binary_search(PW.begin(), PW.end(), j))
					exp1 += x[0][base][j];
				else
				{
					model.add(x[0][base][j] == 0);
					//model.add(L[0][base][j] == 0);
				}
			}

			//model.add(S[base] == begweek);
			model.add(exp1 == 1);
			//model.add(exp2 == 0);

			if (blocks < ilkblocks)
			{
				model.add(y[base][base - preventnum] == 1);
			}

			exp1.end();
			//exp2.end();
			blocks--;
			base += preventnum;
		}
	}


	for each(Job job in Jint)
	{
		if (binary_search(tardyjobs.begin(), tardyjobs.end(), job.ID))
		{
			if (pj[job.ID] < mintard)
				mintard = pj[job.ID];
			continue;
		}

		int j = job.ID;
		if (pj[j] > maxnontard)
			maxnontard = pj[j];
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		if (pj[j] < mintard)
			mintard = pj[j];
	}

	int dist;

	int childub = 0;
	int tot = 0;

	for (int i = mmin.size() - 1; i > 0; --i)
	{
		tot += mmin[i];
		++childub;
		if (tot < mmin[0])
			continue;
		else
			break;
	}

	for (int i = 0; i > 0; --i)
	{
		tot += mmin[i];
		++childub;
		if (tot < mmin[0])
			continue;
		else
			break;
	}

	SetupModel42(maxYmin, maxYmax, minYmin, mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek, env, model, cplex, o, y, dummy, z, P, x, e, a, b, E, T, C, Sj, S, F, K, L, B, np, deltaminusSLA, deltaYmaxplus, coroutservice, Jint, Jcor, preventives, tardyjobs);

	int m1 = tt;
	//int ub;
	////28
	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				int pj = job.pj;
				model.add(L[0][b][j] >= B[0][b][j] + pj*x[0][b][j]);
			}
			for each(Job job in Jcor)
			{
				int j = job.ID;
				int pj = job.pj;
				model.add(L[1][b][j] >= B[1][b][j] + pj*x[1][b][j]);
			}
		}

	////////////////////////////////
	//02.05.17 //lemma 0.5

	vector<int> NP;
	for each(Job job in Jint)
	{
		int j = job.ID;
		if (Ymax[j] > tt)
			NP.push_back(j);
	}

	//newww //11.05.17
	for (int j : NP)
		model.add(deltaYmaxplus[j] == 0);


	////29
	for each(Job job in Jint)
	{
		int j = job.ID;
		int pj = job.pj;

		model.add(C[0][j] >= Sj[0][j] + pj*(1 - np[0][j]));
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		int pj = job.pj;

		model.add(C[1][j] >= Sj[1][j] + pj*(1 - np[1][j]));
	}


	int noncriticsize = n - preventnum;
	int initservice = 0;

	for (int t = begweek; t < endweek; t++)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			int ub = job.rj - realbegweek;
			if (ub < 0)
				ub = 0;

			if (t < ub)
			{
				model.add(a[0][j][t] == 0);
				model.add(b[0][j][t] == 0);
				//model.add(o[0][j][t] == 0);
				++initservice;
			}

			if (t< job.Ymax - realbegweek)
				model.add(o[0][j][t] == 0);

			if (t<ub + job.pj)
				model.add(e[0][j][t] == 0);

			if (t >= tt - job.pj)
				model.add(b[0][j][t] == 0);
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			int ub = job.rj - realbegweek;
			if (ub < 0)
				ub = 0;

			if (t<ub + job.pj)
				model.add(e[1][j][t] == 0);

			if (t >= tt - job.pj)
				model.add(b[1][j][t] == 0);
		}

		if (noncriticsize + initservice >= SLA[t]) //if i can satisfy all sla from noncritics, then it becomes redundant. But we change the logic
			model.add(deltaminusSLA[t] == 0);

		initservice = 0;
	}

	//10.11.2017 - performansına tam karar verilemedi?

	////5.24.17
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			int say = 0;
			dist = b - t*preventnum + 1;
			IloExpr exp(env);

			int count = 0;
			int threshold = dist*childub;
			int posb;
			int kalan;

			if (childub != 0)
			{
				if (t == 0)
					posb = dist;
				else
					posb = dist + ceil(dist / childub);

				int candidate = min(preventnum - posb, childub);
				kalan = max(candidate, 0);
			}


			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
				{
					++say;
					exp += y[a][b];
				}

			}

			if (say == 0 || say <= childub)
			{
				exp.end();
				continue;
			}

			else
				model.add(exp <= kalan);//root'ta job varsa root da vardır!
			exp.end();
		}

	for (int t = 0; t < m; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			for each(Job job in Jint)
				cplex.setPriority(x[0][b][job.ID], 3);

			for each(Job job in Jcor)
				cplex.setPriority(x[1][b][job.ID], 3);
		}

	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			dist = b - t*preventnum + 1;

			int count = 0;
			int threshold = dist*childub;

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
				{
					cplex.setPriority(y[a][b], 2);
					cplex.setPriority(z[a][b], 1);
				}

			}
		}

	//works fine //18.07.17
	cplex.setParam(IloCplex::Probe, 2);
	cplex.setParam(IloCplex::Param::MIP::Strategy::PresolveNode, 2);

	if (tardyjobs.size() > 1 && initials.size() == 0)
	{
		//for last two track: baslangic = m-2: m=3 iken kötü performans verdi	
		for (int t = 0; t < m - 1; ++t)
			for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
			{
				IloExpr exp1(env);
				IloExpr exp3(env);

				dist = b - t*preventnum + 1;

				for each (Job job in Jcor)
				{
					int j = job.ID;
					exp1 += x[1][b][j] * pj[j];
					exp3 += x[1][b][j];
				}

				for (int job : tardyjobs)
				{
					if (binary_search(preventives.begin(), preventives.end(), job))
					{
						exp1 += x[0][b][job] * pj[job];
						exp3 += x[0][b][job];
					}
				}

				for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
				{
					IloExpr exp2(env);
					IloExpr expotherchild(env);

					for each (Job job in Jcor)
					{
						int j = job.ID;
						exp2 += x[1][a][j] * pj[j];
					}

					for (int job : tardyjobs)
					{
						if (binary_search(preventives.begin(), preventives.end(), job))
							exp2 += x[0][a][job] * pj[job];
					}


					model.add(exp1 >= exp2 - (mmin[0]) * (1 - y[a][b]) - (mmin[0]) * (1 - exp3));

					exp2.end();
					expotherchild.end();

				}

				exp1.end();
				exp3.end();
			}
	}

	if (tardyjobs.size() > 1 && initials.size() > 0)
	{

		for (int t = 0; t < m - 1; ++t)
			for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
			{
				if (binary_search(initbasenums.begin(), initbasenums.end(), b))
					continue;

				IloExpr exp1(env);
				IloExpr exp3(env);

				dist = b - t*preventnum + 1;

				for each (Job job in Jcor)
				{
					int j = job.ID;
					exp1 += x[1][b][j] * pj[j];
					exp3 += x[1][b][j];
				}

				for (int job : tardyjobs)
				{
					if (binary_search(preventives.begin(), preventives.end(), job))
					{
						exp1 += x[0][b][job] * pj[job];
						exp3 += x[0][b][job];
					}
				}

				for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
				{
					if (binary_search(initbasenums.begin(), initbasenums.end(), a))
						continue;

					IloExpr exp2(env);
					IloExpr expotherchild(env);

					for each (Job job in Jcor)
					{
						int j = job.ID;
						exp2 += x[1][a][j] * pj[j];
					}

					for (int job : tardyjobs)
					{
						if (binary_search(preventives.begin(), preventives.end(), job))
							exp2 += x[0][a][job] * pj[job];
					}

					model.add(exp1 >= exp2 - (mmin[0]) * (1 - y[a][b]) - (mmin[0]) * (1 - exp3));
					exp2.end();
					expotherchild.end();

				}

				exp1.end();
				exp3.end();
			}

	}

	cplex.setParam(IloCplex::TiLim, 60 * 2);

	//cplex.setParam(IloCplex::TiLim, 20);

	time = cplex.getCplexTime();

	IloBool success = cplex.solve();
	//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);
	double elapsedtime = cplex.getCplexTime() - time;

	if (success && cplex.isPrimalFeasible())
	{
		/*vector<vector<Base>> cand = Write(model, cplex, env, myfile, n, begweek, endweek, realbegweek, warmup, dayindex, tt, m, s, time, cumtime, cum, pr, sol, cumSLAviol, cumYmaxviol, cumgap, cumnodes, buffer
		, B, o, y, P, x, e, a, b, E, T, C, Sj, S, F, L, np, deltaminusSLA, deltaYmaxplus, coroutservice, datarunfile, Jcor, Jint, initials, SLA, prevprdtms);

		DeleteJobsbuffer(Jint, Jcor, cplex, x, np, C, Sj, S, F, prevprdtms, preventnum, m, cand);*/

		myfile << elapsedtime << "\t" << cplex.getObjValue() << "\t" << cplex.getMIPRelativeGap() * 100 << "\t";

		//myfile << cplex.getNnodes() << "\t";
	}

	else
	{
		// olmadı instanceları buraya yazdır!		
		ofstream infsbleinst("infsble.txt");
		//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);
		cplex.exportModel("infeasible.lp");

		myfile << "INFEASIBLE!!!!" << endl;
		myfile << "success = " << success << endl;
		myfile << "cplex.isPrimalFeasible() = " << cplex.isPrimalFeasible() << endl;
		myfile.close();
		infsbleinst.close();
		return true;

	}

	cplex.end();
	model.end();
	env.end();
	//delete[] tracklimit;
	return false;
}

bool SolveModelonlyHeur(double& z1, int* SLAdaily, int totaltt, ofstream& myfile, ofstream& infsbleinst, int warmup, int dayindex, double& time, double* cumtime, double* cum, int pr, int sol,
	int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, ofstream& datarunfile, vector<int>& critics, int pencor, int penminus, int penymaxplus,
	int maxYmin, int maxYmax, int minYmin, int buffer, vector <int> mmin, int s, int n, int tt, int m, int M, int* SLA, int* pj, int realbegweek, int begweek, int endweek, vector<Job>& Jint, vector<Job>& Jcor,
	vector<int>& preventives, vector<int>& tardyjobs, vector<int>& correctives, int* d, int* Ymax, int* rr, int limit, int** prevprdtms, int bf, int coef)
{
	//3 indices: (type)(job)(time) type0 - prev; 1 - corm
	IloEnv env;
	IloModel model(env);

	IloBoolVarArray dummy = CreateBoolVarArray(env, s, "dummy");

	BoolVarArray2 y = CreateBoolVarArray2(env, s, s, "y"); //yab
	BoolVarArray2 z = CreateBoolVarArray2(env, s, s, "z"); //zab
	BoolVarArray3 x = CreateBoolVarArray3(env, 2, s, n, "x"); // 
	BoolVarArray3 e = CreateBoolVarArray3(env, 2, n, tt, "e"); // 1 at maint j exit, 0 else
	BoolVarArray3 a = CreateBoolVarArray3(env, 2, n, tt, "a"); //1 at maint, 0 else
	BoolVarArray3 b = CreateBoolVarArray3(env, 2, n, tt, "b"); // 1 at maint j begin, 0 else
	BoolVarArray3 o = CreateBoolVarArray3(env, 2, n, tt, "o");   //out of service
	BoolVarArray2 np = CreateBoolVarArray2(env, 2, n, "np");   //to next period
	IloNumVarArray P = CreateNumVarArray(env, s, "P", 0, IloInfinity); //block makespan
	NumVarArray2 E = CreateNumVarArray2(env, 2, n, "E", 0, IloInfinity);
	NumVarArray2 T = CreateNumVarArray2(env, 2, n, "T", 0, IloInfinity);//tardiness
	NumVarArray2 C = CreateNumVarArray2(env, 2, n, "C", 0, IloInfinity); //Job completion
	NumVarArray2 Sj = CreateNumVarArray2(env, 2, n, "Sj", 0, IloInfinity); //Job start
	IloNumVarArray S = CreateNumVarArray(env, s, "S", 0, IloInfinity); // Start time of block b
	IloNumVarArray F = CreateNumVarArray(env, s, "F", 0, IloInfinity); // Finish time of block b

	//Linearization vars
	IloNumVarArray K = CreateNumVarArray(env, s, "K", 0, IloInfinity); // Finish time of block b
	NumVarArray3 L = CreateNumVarArray3(env, 2, s, n, "L", 0, IloInfinity); // ENd bj
	NumVarArray3 B = CreateNumVarArray3(env, 2, s, n, "B", 0, IloInfinity); //linearization var - begin bj

	IloNumVarArray deltaminusSLA = CreateNumVarArray(env, tt, "deltaSLA-", 0, IloInfinity); //sladundersatisfaction
	IloNumVarArray deltaYmaxplus = CreateNumVarArray(env, n, "deltaYmax+", 0, IloInfinity); //prevjobs
	IloNumVarArray coroutservice = CreateNumVarArray(env, n, "coroutserv", 0, IloInfinity); //corjobs

	NumVarArray2 Tj = CreateNumVarArray2(env, 2, n, "Tj", 0, IloInfinity);//finsh tardiness

	IloExpr objExp(env);

	if (bf == 2)
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += coef * (pencor - penymaxplus)*T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}
	else
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		objExp += pencor*T[1][j];
	}

	for (int t = 0; t < tt; ++t)
		objExp += penminus*deltaminusSLA[t];

	IloObjective obj = IloMinimize(env, objExp, "obj");
	model.add(obj);
	objExp.end();
	IloCplex cplex(model);
	int criticsize = critics.size();

	//initials
	vector<int> initials;

	int maxnontard = -1;
	int mintard = 1000;

	int preventnum = Jint.size() + Jcor.size();

	vector<int> initbasenums;
	//for (int i = 0; i < m; ++i)

	if (dayindex > 0)
	{
		//prvprdtms'tan ilgili şeyleri çekip model.add dersin

		for (int t = 0; t < m; ++t)
		{
			if (prevprdtms[t][0] != -1)
			{
				int job = prevprdtms[t][0];
				int type = prevprdtms[t][1];
				int delay = prevprdtms[t][2];
				//int start = prevprdtms[t][3];

				initials.push_back(job);
				initbasenums.push_back(t*preventnum);

				model.add(x[type][t*preventnum][job] == 1);

				//2.11.17-1
				for (int i = 0; i < m; ++i)
					for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
					{
						if (b != t*preventnum)
							model.add(x[type][b][job] == 0);
					}

				model.add(L[type][t*preventnum][job] >= delay);
				model.add(B[type][t*preventnum][job] == 0);
			}
		}
	}

	SetupModel(maxYmin, maxYmax, minYmin, mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek, env, model, cplex, o, y, dummy, z, P, x, e, a, b, E, T, C, Sj, S, F, K, L, B, np, deltaminusSLA, deltaYmaxplus, coroutservice, Jint, Jcor, preventives, tardyjobs);

	///////////////////////////////////////////

	double start = cplex.getTime();
	int neigh = 0;
	Heuristic* pinc;
	int count = 0;

	int neighbest = neigh;
	int heurcriticsize = critics.size();

	//heur = 1; //dif_t içerir, slackdesc-ascpj değil!!!
	//heur = 1; //dif_t içerir, slackdesc-ascpj değil!!!

	start = cplex.getTime();
	neigh = 0;
	count = 0;

	neighbest = neigh;
	heurcriticsize = critics.size();

	bool devam = false;

	while (true)
	{
		if (devam == false)
		{
			pinc = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
			pinc->heur = 1;


			if (bf == 2)
			{
				pinc->rtune = 1;
				pinc->coef = coef;
			}

			else
				pinc->rtune = 0;

			if (pinc->RunHeuristic())
			{
				neigh += 5;
				if (pinc->obj < 0)
				{
					//delete pinc;
					break;
				}

				Heuristic* pinc2 = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
				pinc2->heur = 1;
				if (bf == 2)
				{
					pinc2->rtune = 1;
					pinc2->coef = coef;
				}

				else
					pinc2->rtune = 0;

				if (pinc2->RunHeuristic())
				{
					if (pinc->obj <= pinc2->obj || pinc2->obj < 0)
					{
						delete pinc2;
						break;
					}

					else
					{
						delete pinc;
						neighbest = neigh;
						pinc = new Heuristic(*pinc2);
						delete pinc2;
						devam = true;
					}
				}
			}
		}

		else
		{
			neigh += 5;
			Heuristic* pinc2 = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
			pinc2->heur = 1;
			if (bf == 2)
			{
				pinc2->rtune = 1;
				pinc2->coef = coef;
			}
			else
				pinc2->rtune = 0;

			if (pinc2->RunHeuristic())
			{
				if (pinc->obj <= pinc2->obj || pinc2->obj < 0)
				{
					delete pinc2;
					break;
				}
				else
				{
					delete pinc;
					neighbest = neigh;
					pinc = new Heuristic(*pinc2);
					delete pinc2;
				}
			}
		}

	}

	//////later
	if (pinc->obj >= 0)
	{
		cout << "obj = " << pinc->obj << endl;
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);

		MIPstart4(startVar, startVal, model, pinc, s, n, tt, m, y, P, x, e, a, b, E, T, C, Sj, S, F, L, B, np, o, deltaYmaxplus, deltaminusSLA, preventives, correctives, tardyjobs, rr, pj, d, Ymax, SLA);
		cplex.addMIPStart(startVar, startVal);

		startVal.clear();
		startVar.clear();
	}

	else
		cout << "INFEASIBLE!!!!!!!" << endl;

	cplex.setParam(IloCplex::TiLim, 60 * 2);

	time = cplex.getCplexTime();

	IloBool success = cplex.solve();
	//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);

	double elapsedtime = cplex.getCplexTime() - time;

	if (success && cplex.isPrimalFeasible())
	{
		//myfile << elapsedtime << "\t" << cplex.getObjValue() << "\t" << cplex.getMIPRelativeGap() << "\t";

		myfile << cplex.getNnodes() << "\t";
	}

	else
	{
		// olmadı instanceları buraya yazdır!		
		ofstream infsbleinst("infsble.txt");
		//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);
		cplex.exportModel("infeasible.lp");

		myfile << "INFEASIBLE!!!!" << endl;
		myfile << "success = " << success << endl;
		myfile << "cplex.isPrimalFeasible() = " << cplex.isPrimalFeasible() << endl;
		myfile.close();
		infsbleinst.close();
		return true;

	}

	cplex.end();
	model.end();
	env.end();
	delete pinc;
	//delete[] tracklimit;
	return false;
}

bool SolveModelHeur1(double& z1, int* SLAdaily, int totaltt, ofstream& myfile, int warmup, int dayindex, double& time, double* cumtime, double* cum, int pr, int sol,
	int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, ofstream& datarunfile, vector<int>& critics, int pencor, int penminus, int penymaxplus,
	int maxYmin, int maxYmax, int minYmin, int buffer, vector <int> mmin, int s, int n, int tt, int m, int M, int* SLA, int* pj, int realbegweek, int begweek, int endweek, vector<Job>& Jint, vector<Job>& Jcor,
	vector<int>& preventives, vector<int>& tardyjobs, vector<int>& correctives, int* d, int* Ymax, int* rr, int limit, int** prevprdtms, int bf, int coef, ofstream& dataday)
{
	//3 indices: (type)(job)(time) type0 - prev; 1 - corm
	IloEnv env;
	IloModel model(env);

	IloBoolVarArray dummy = CreateBoolVarArray(env, s, "dummy");

	BoolVarArray2 y = CreateBoolVarArray2(env, s, s, "y"); //yab
	BoolVarArray2 z = CreateBoolVarArray2(env, s, s, "z"); //zab
	BoolVarArray3 x = CreateBoolVarArray3(env, 2, s, n, "x"); // 
	BoolVarArray3 e = CreateBoolVarArray3(env, 2, n, tt, "e"); // 1 at maint j exit, 0 else
	BoolVarArray3 a = CreateBoolVarArray3(env, 2, n, tt, "a"); //1 at maint, 0 else
	BoolVarArray3 b = CreateBoolVarArray3(env, 2, n, tt, "b"); // 1 at maint j begin, 0 else
	BoolVarArray3 o = CreateBoolVarArray3(env, 2, n, tt, "o");   //out of service
	BoolVarArray2 np = CreateBoolVarArray2(env, 2, n, "np");   //to next period
	IloNumVarArray P = CreateNumVarArray(env, s, "P", 0, IloInfinity); //block makespan
	NumVarArray2 E = CreateNumVarArray2(env, 2, n, "E", 0, IloInfinity);
	NumVarArray2 T = CreateNumVarArray2(env, 2, n, "T", 0, IloInfinity);//tardiness
	NumVarArray2 C = CreateNumVarArray2(env, 2, n, "C", 0, IloInfinity); //Job completion
	NumVarArray2 Sj = CreateNumVarArray2(env, 2, n, "Sj", 0, IloInfinity); //Job start
	IloNumVarArray S = CreateNumVarArray(env, s, "S", 0, IloInfinity); // Start time of block b
	IloNumVarArray F = CreateNumVarArray(env, s, "F", 0, IloInfinity); // Finish time of block b

	//Linearization vars
	IloNumVarArray K = CreateNumVarArray(env, s, "K", 0, IloInfinity); // Finish time of block b
	NumVarArray3 L = CreateNumVarArray3(env, 2, s, n, "L", 0, IloInfinity); // ENd bj
	NumVarArray3 B = CreateNumVarArray3(env, 2, s, n, "B", 0, IloInfinity); //linearization var - begin bj

	IloNumVarArray deltaminusSLA = CreateNumVarArray(env, tt, "deltaSLA-", 0, IloInfinity); //sladundersatisfaction
	IloNumVarArray deltaYmaxplus = CreateNumVarArray(env, n, "deltaYmax+", 0, IloInfinity); //prevjobs
	IloNumVarArray coroutservice = CreateNumVarArray(env, n, "coroutserv", 0, IloInfinity); //corjobs

	NumVarArray2 Tj = CreateNumVarArray2(env, 2, n, "Tj", 0, IloInfinity);//finsh tardiness

	IloExpr objExp(env);

	if (bf == 2)
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += coef * (pencor - penymaxplus)*T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}
	else
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		objExp += pencor*T[1][j];
	}

	for (int t = 0; t < tt; ++t)
		objExp += penminus*deltaminusSLA[t];

	IloObjective obj = IloMinimize(env, objExp, "obj");
	model.add(obj);
	objExp.end();
	IloCplex cplex(model);
	int criticsize = critics.size();

	//initials
	vector<int> initials;

	int maxnontard = -1;
	int mintard = 1000;

	int preventnum = Jint.size() + Jcor.size();


	vector<int> initbasenums;
	//for (int i = 0; i < m; ++i)

	if (dayindex > 0)
	{
		//prvprdtms'tan ilgili şeyleri çekip model.add dersin

		for (int t = 0; t < m; ++t)
		{
			if (prevprdtms[t][0] != -1)
			{
				int job = prevprdtms[t][0];
				int type = prevprdtms[t][1];
				int delay = prevprdtms[t][2];
				//int start = prevprdtms[t][3];

				initials.push_back(job);
				initbasenums.push_back(t*preventnum);

				model.add(x[type][t*preventnum][job] == 1);

				//2.11.17-1
				for (int i = 0; i < m; ++i)
					for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
					{
						if (b != t*preventnum)
							model.add(x[type][b][job] == 0);
					}

				model.add(L[type][t*preventnum][job] >= delay);
				model.add(B[type][t*preventnum][job] == 0);
			}
		}
	}


	SetupModel(maxYmin, maxYmax, minYmin, mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek, env, model, cplex, o, y, dummy, z, P, x, e, a, b, E, T, C, Sj, S, F, K, L, B, np, deltaminusSLA, deltaYmaxplus, coroutservice, Jint, Jcor, preventives, tardyjobs);

	//cplex.setParam(IloCplex::TiLim, 10);

	cplex.setParam(IloCplex::TiLim, 60 * 2);
	///////////////////////////////////////////

	double start = cplex.getTime();
	int neigh = 0;
	Heuristic* pinc;
	int count = 0;

	int neighbest = neigh;
	int heurcriticsize = critics.size();

	//heur = 1; //dif_t içerir, slackdesc-ascpj değil!!!
	//heur = 1; //dif_t içerir, slackdesc-ascpj değil!!!

	start = cplex.getTime();
	neigh = 0;
	count = 0;

	neighbest = neigh;
	heurcriticsize = critics.size();

	bool devam = false;

	while (true)
	{
		if (devam == false)
		{
			pinc = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
			pinc->heur = 1;


			if (bf == 2)
			{
				pinc->rtune = 1;
				pinc->coef = coef;
			}

			else
				pinc->rtune = 0;

			if (pinc->RunHeuristic())
			{
				neigh += 5;
				if (pinc->obj < 0)
				{
					//delete pinc;
					break;
				}

				Heuristic* pinc2 = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
				pinc2->heur = 1;
				if (bf == 2)
				{
					pinc2->rtune = 1;
					pinc2->coef = coef;
				}

				else
					pinc2->rtune = 0;

				if (pinc2->RunHeuristic())
				{
					if (pinc->obj <= pinc2->obj || pinc2->obj < 0)
					{
						delete pinc2;
						break;
					}

					else
					{
						delete pinc;
						neighbest = neigh;
						pinc = new Heuristic(*pinc2);
						delete pinc2;
						devam = true;
					}
				}
			}
		}

		else
		{
			neigh += 5;
			Heuristic* pinc2 = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
			pinc2->heur = 1;
			if (bf == 2)
			{
				pinc2->rtune = 1;
				pinc2->coef = coef;
			}
			else
				pinc2->rtune = 0;

			if (pinc2->RunHeuristic())
			{
				if (pinc->obj <= pinc2->obj || pinc2->obj < 0)
				{
					delete pinc2;
					break;
				}
				else
				{
					delete pinc;
					neighbest = neigh;
					pinc = new Heuristic(*pinc2);
					delete pinc2;
				}
			}
		}

	}

	//////later
	if (pinc->obj >= 0)
	{
		cout << "obj = " << pinc->obj << endl;
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);

		MIPstart4(startVar, startVal, model, pinc, s, n, tt, m, y, P, x, e, a, b, E, T, C, Sj, S, F, L, B, np, o, deltaYmaxplus, deltaminusSLA, preventives, correctives, tardyjobs, rr, pj, d, Ymax, SLA);
		cplex.addMIPStart(startVar, startVal);

		startVal.clear();
		startVar.clear();
	}

	else
		cout << "INFEASIBLE!!!!!!!" << endl;

	cplex.setParam(IloCplex::TiLim, 60 * 2);

	time = cplex.getCplexTime();

	IloBool success = cplex.solve();
	//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);

	double elapsedtime = cplex.getCplexTime() - time;

	if (success && cplex.isPrimalFeasible())
	{
		//myfile << elapsedtime << "\t" << cplex.getObjValue() << "\t" << cplex.getMIPRelativeGap() << "\t";

		myfile << cplex.getNnodes() << "\t";
	}

	else
	{
		// olmadı instanceları buraya yazdır!		
		ofstream infsbleinst("infsble.txt");
		//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);
		cplex.exportModel("infeasible.lp");

		myfile << "INFEASIBLE!!!!" << endl;
		myfile << "success = " << success << endl;
		myfile << "cplex.isPrimalFeasible() = " << cplex.isPrimalFeasible() << endl;
		myfile.close();
		infsbleinst.close();
		return true;

	}

	cplex.end();
	model.end();
	env.end();
	delete pinc;
	//delete[] tracklimit;
	return false;
}


bool SolveModeldenemeHeur1(double& z1, int* SLAdaily, int totaltt, ofstream& myfile, int warmup, int dayindex, double& time, double* cumtime, double* cum, int pr, int sol,
	int* cumSLAviol, int* cumYmaxviol, double* cumgap, int* cumnodes, ofstream& datarunfile, vector<int>& critics, int pencor, int penminus, int penymaxplus,
	int maxYmin, int maxYmax, int minYmin, int buffer, vector <int> mmin, int s, int n, int tt, int m, int M, int* SLA, int* pj, int realbegweek, int begweek, int endweek, vector<Job>& Jint, vector<Job>& Jcor,
	vector<int>& preventives, vector<int>& tardyjobs, vector<int>& correctives, int* d, int* Ymax, int* rr, int limit, int** prevprdtms, int bf, int coef, ofstream& dataday)
{
	//3 indices: (type)(job)(time) type0 - prev; 1 - corm
	IloEnv env;
	IloModel model(env);

	IloBoolVarArray dummy = CreateBoolVarArray(env, s, "dummy");

	BoolVarArray2 y = CreateBoolVarArray2(env, s, s, "y"); //yab
	BoolVarArray2 z = CreateBoolVarArray2(env, s, s, "z"); //zab
	BoolVarArray3 x = CreateBoolVarArray3(env, 2, s, n, "x"); // 
	BoolVarArray3 e = CreateBoolVarArray3(env, 2, n, tt, "e"); // 1 at maint j exit, 0 else
	BoolVarArray3 a = CreateBoolVarArray3(env, 2, n, tt, "a"); //1 at maint, 0 else
	BoolVarArray3 b = CreateBoolVarArray3(env, 2, n, tt, "b"); // 1 at maint j begin, 0 else
	BoolVarArray3 o = CreateBoolVarArray3(env, 2, n, tt, "o");   //out of service
	BoolVarArray2 np = CreateBoolVarArray2(env, 2, n, "np");   //to next period
	IloNumVarArray P = CreateNumVarArray(env, s, "P", 0, IloInfinity); //block makespan
	NumVarArray2 E = CreateNumVarArray2(env, 2, n, "E", 0, IloInfinity);
	NumVarArray2 T = CreateNumVarArray2(env, 2, n, "T", 0, IloInfinity);//tardiness
	NumVarArray2 C = CreateNumVarArray2(env, 2, n, "C", 0, IloInfinity); //Job completion
	NumVarArray2 Sj = CreateNumVarArray2(env, 2, n, "Sj", 0, IloInfinity); //Job start
	IloNumVarArray S = CreateNumVarArray(env, s, "S", 0, IloInfinity); // Start time of block b
	IloNumVarArray F = CreateNumVarArray(env, s, "F", 0, IloInfinity); // Finish time of block b

	//Linearization vars
	IloNumVarArray K = CreateNumVarArray(env, s, "K", 0, IloInfinity); // Finish time of block b
	NumVarArray3 L = CreateNumVarArray3(env, 2, s, n, "L", 0, IloInfinity); // ENd bj
	NumVarArray3 B = CreateNumVarArray3(env, 2, s, n, "B", 0, IloInfinity); //linearization var - begin bj

	IloNumVarArray deltaminusSLA = CreateNumVarArray(env, tt, "deltaSLA-", 0, IloInfinity); //sladundersatisfaction
	IloNumVarArray deltaYmaxplus = CreateNumVarArray(env, n, "deltaYmax+", 0, IloInfinity); //prevjobs
	IloNumVarArray coroutservice = CreateNumVarArray(env, n, "coroutserv", 0, IloInfinity); //corjobs

	NumVarArray2 Tj = CreateNumVarArray2(env, 2, n, "Tj", 0, IloInfinity);//finsh tardiness

	IloExpr objExp(env);

	if (bf == 2)
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += coef * (pencor - penymaxplus)*T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}
	else
		for each(Job job in Jint)
		{
			int j = job.ID;
			objExp += T[0][j] + E[0][j] + penymaxplus*deltaYmaxplus[j];
		}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		objExp += pencor*T[1][j];
	}

	for (int t = 0; t < tt; ++t)
		objExp += penminus*deltaminusSLA[t];

	IloObjective obj = IloMinimize(env, objExp, "obj");
	model.add(obj);
	objExp.end();
	IloCplex cplex(model);
	int criticsize = critics.size();

	//initials
	vector<int> initials;

	int maxnontard = -1;
	int mintard = 1000;

	int preventnum = Jint.size() + Jcor.size();


	vector<int> initbasenums;
	//for (int i = 0; i < m; ++i)

	if (dayindex > 0)
	{
		//prvprdtms'tan ilgili şeyleri çekip model.add dersin

		for (int t = 0; t < m; ++t)
		{
			if (prevprdtms[t][0] != -1)
			{
				int job = prevprdtms[t][0];
				int type = prevprdtms[t][1];
				int delay = prevprdtms[t][2];
				//int start = prevprdtms[t][3];

				initials.push_back(job);
				initbasenums.push_back(t*preventnum);

				model.add(x[type][t*preventnum][job] == 1);

				//2.11.17-1
				for (int i = 0; i < m; ++i)
					for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
					{
						if (b != t*preventnum)
							model.add(x[type][b][job] == 0);
					}

				model.add(L[type][t*preventnum][job] >= delay);
				model.add(B[type][t*preventnum][job] == 0);
			}
		}
	}

	vector<int> PW;

	for each(Job job in Jint)
	{
		if (job.rj <= realbegweek + mmin[0]) //if r_j <= begweek + pmax
			PW.push_back(job.ID);
	}

	sort(PW.begin(), PW.end());

	int blocks = Getmin(tardyjobs.size(), m);
	int ilkblocks = blocks;

	//bool termin = true;
	if (tardyjobs.size() > 0 && initials.size() == 0)
	{
		int base = 0;
		while (blocks > 0)
		{
			IloExpr exp1(env);
			//IloExpr exp2(env);

			for each(Job job in Jcor)
			{
				int j = job.ID;
				exp1 += x[1][base][j];
				//model.add(B[1][base][j] == begweek);
			}

			for each(Job job in Jint)
			{
				int j = job.ID;
				//exp2 += x[0][base][j];
				//model.add(B[0][base][j] == 0);

				if (binary_search(PW.begin(), PW.end(), j))
					exp1 += x[0][base][j];
				else
				{
					model.add(x[0][base][j] == 0);
					//model.add(L[0][base][j] == 0);
				}
			}

			//model.add(S[base] == begweek);
			model.add(exp1 == 1);
			//model.add(exp2 == 0);

			if (blocks < ilkblocks)
			{
				model.add(y[base][base - preventnum] == 1);
			}

			exp1.end();
			//exp2.end();
			blocks--;
			base += preventnum;
		}
	}


	for each(Job job in Jint)
	{
		if (binary_search(tardyjobs.begin(), tardyjobs.end(), job.ID))
		{
			if (pj[job.ID] < mintard)
				mintard = pj[job.ID];
			continue;
		}

		int j = job.ID;
		if (pj[j] > maxnontard)
			maxnontard = pj[j];
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		if (pj[j] < mintard)
			mintard = pj[j];
	}

	int dist;

	int childub = 0;
	int tot = 0;

	for (int i = mmin.size() - 1; i > 0; --i)
	{
		tot += mmin[i];
		++childub;
		if (tot < mmin[0])
			continue;
		else
			break;
	}

	for (int i = 0; i > 0; --i)
	{
		tot += mmin[i];
		++childub;
		if (tot < mmin[0])
			continue;
		else
			break;
	}

	SetupModel42(maxYmin, maxYmax, minYmin, mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek, env, model, cplex, o, y, dummy, z, P, x, e, a, b, E, T, C, Sj, S, F, K, L, B, np, deltaminusSLA, deltaYmaxplus, coroutservice, Jint, Jcor, preventives, tardyjobs);



	int m1 = tt;
	//int ub;
	////28
	for (int i = 0; i < m; ++i)
		for (int b = i*preventnum; b < (i + 1)*preventnum - i; ++b)
		{
			for each(Job job in Jint)
			{
				int j = job.ID;
				int pj = job.pj;
				model.add(L[0][b][j] >= B[0][b][j] + pj*x[0][b][j]);
			}
			for each(Job job in Jcor)
			{
				int j = job.ID;
				int pj = job.pj;
				model.add(L[1][b][j] >= B[1][b][j] + pj*x[1][b][j]);
			}
		}

	////////////////////////////////
	//02.05.17 //lemma 0.5

	vector<int> NP;
	for each(Job job in Jint)
	{
		int j = job.ID;
		if (Ymax[j] > tt)
			NP.push_back(j);
	}

	//newww //11.05.17
	for (int j : NP)
		model.add(deltaYmaxplus[j] == 0);


	////29
	for each(Job job in Jint)
	{
		int j = job.ID;
		int pj = job.pj;

		model.add(C[0][j] >= Sj[0][j] + pj*(1 - np[0][j]));
	}

	for each(Job job in Jcor)
	{
		int j = job.ID;
		int pj = job.pj;

		model.add(C[1][j] >= Sj[1][j] + pj*(1 - np[1][j]));
	}


	int noncriticsize = n - preventnum;
	int initservice = 0;

	for (int t = begweek; t < endweek; t++)
	{
		for each(Job job in Jint)
		{
			int j = job.ID;
			int ub = job.rj - realbegweek;
			if (ub < 0)
				ub = 0;

			if (t < ub)
			{
				model.add(a[0][j][t] == 0);
				model.add(b[0][j][t] == 0);
				//model.add(o[0][j][t] == 0);
				++initservice;
			}

			if (t< job.Ymax - realbegweek)
				model.add(o[0][j][t] == 0);

			if (t<ub + job.pj)
				model.add(e[0][j][t] == 0);

			if (t >= tt - job.pj)
				model.add(b[0][j][t] == 0);
		}

		for each(Job job in Jcor)
		{
			int j = job.ID;
			int ub = job.rj - realbegweek;
			if (ub < 0)
				ub = 0;

			if (t<ub + job.pj)
				model.add(e[1][j][t] == 0);

			if (t >= tt - job.pj)
				model.add(b[1][j][t] == 0);
		}

		if (noncriticsize + initservice >= SLA[t]) //if i can satisfy all sla from noncritics, then it becomes redundant. But we change the logic
			model.add(deltaminusSLA[t] == 0);

		initservice = 0;
	}

	//10.11.2017 - performansına tam karar verilemedi?

	////5.24.17
	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			int say = 0;
			dist = b - t*preventnum + 1;
			IloExpr exp(env);

			int count = 0;
			int threshold = dist*childub;
			int posb;
			int kalan;

			if (childub != 0)
			{
				if (t == 0)
					posb = dist;
				else
					posb = dist + ceil(dist / childub);

				int candidate = min(preventnum - posb, childub);
				kalan = max(candidate, 0);
			}


			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
				{
					++say;
					exp += y[a][b];
				}

			}

			if (say == 0 || say <= childub)
			{
				exp.end();
				continue;
			}

			else
				model.add(exp <= kalan);//root'ta job varsa root da vardır!
			exp.end();
		}

	for (int t = 0; t < m; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			for each(Job job in Jint)
				cplex.setPriority(x[0][b][job.ID], 3);

			for each(Job job in Jcor)
				cplex.setPriority(x[1][b][job.ID], 3);
		}

	for (int t = 0; t < m - 1; ++t)
		for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
		{
			dist = b - t*preventnum + 1;

			int count = 0;
			int threshold = dist*childub;

			for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
			{
				++count;
				if (count > threshold)
					break;
				else
				{
					cplex.setPriority(y[a][b], 2);
					cplex.setPriority(z[a][b], 1);
				}

			}
		}

	//works fine //18.07.17
	cplex.setParam(IloCplex::Probe, 2);
	cplex.setParam(IloCplex::Param::MIP::Strategy::PresolveNode, 2);

	if (tardyjobs.size() > 1 && initials.size() == 0)
	{
		//for last two track: baslangic = m-2: m=3 iken kötü performans verdi	
		for (int t = 0; t < m - 1; ++t)
			for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
			{
				IloExpr exp1(env);
				IloExpr exp3(env);

				dist = b - t*preventnum + 1;

				for each (Job job in Jcor)
				{
					int j = job.ID;
					exp1 += x[1][b][j] * pj[j];
					exp3 += x[1][b][j];
				}

				for (int job : tardyjobs)
				{
					if (binary_search(preventives.begin(), preventives.end(), job))
					{
						exp1 += x[0][b][job] * pj[job];
						exp3 += x[0][b][job];
					}
				}

				for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
				{
					IloExpr exp2(env);
					IloExpr expotherchild(env);

					for each (Job job in Jcor)
					{
						int j = job.ID;
						exp2 += x[1][a][j] * pj[j];
					}

					for (int job : tardyjobs)
					{
						if (binary_search(preventives.begin(), preventives.end(), job))
							exp2 += x[0][a][job] * pj[job];
					}


					model.add(exp1 >= exp2 - (mmin[0]) * (1 - y[a][b]) - (mmin[0]) * (1 - exp3));

					exp2.end();
					expotherchild.end();

				}

				exp1.end();
				exp3.end();
			}
	}

	if (tardyjobs.size() > 1 && initials.size() > 0)
	{

		for (int t = 0; t < m - 1; ++t)
			for (int b = t*preventnum; b < (t + 1)*preventnum - t; ++b)
			{
				if (binary_search(initbasenums.begin(), initbasenums.end(), b))
					continue;

				IloExpr exp1(env);
				IloExpr exp3(env);

				dist = b - t*preventnum + 1;

				for each (Job job in Jcor)
				{
					int j = job.ID;
					exp1 += x[1][b][j] * pj[j];
					exp3 += x[1][b][j];
				}

				for (int job : tardyjobs)
				{
					if (binary_search(preventives.begin(), preventives.end(), job))
					{
						exp1 += x[0][b][job] * pj[job];
						exp3 += x[0][b][job];
					}
				}

				for (int a = (t + 1)*preventnum; a < (t + 2)*preventnum - dist - t; ++a)
				{
					if (binary_search(initbasenums.begin(), initbasenums.end(), a))
						continue;

					IloExpr exp2(env);
					IloExpr expotherchild(env);

					for each (Job job in Jcor)
					{
						int j = job.ID;
						exp2 += x[1][a][j] * pj[j];
					}

					for (int job : tardyjobs)
					{
						if (binary_search(preventives.begin(), preventives.end(), job))
							exp2 += x[0][a][job] * pj[job];
					}

					model.add(exp1 >= exp2 - (mmin[0]) * (1 - y[a][b]) - (mmin[0]) * (1 - exp3));
					exp2.end();
					expotherchild.end();

				}

				exp1.end();
				exp3.end();
			}

	}

	///////////////////////////////////////////

	double start = cplex.getTime();
	int neigh = 0;
	Heuristic* pinc;
	int count = 0;

	int neighbest = neigh;
	int heurcriticsize = critics.size();

	//heur = 1; //dif_t içerir, slackdesc-ascpj değil!!!
	//heur = 1; //dif_t içerir, slackdesc-ascpj değil!!!

	start = cplex.getTime();
	neigh = 0;
	count = 0;

	neighbest = neigh;
	heurcriticsize = critics.size();

	bool devam = false;

	while (true)
	{
		if (devam == false)
		{
			pinc = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
			pinc->heur = 1;


			if (bf == 2)
			{
				pinc->rtune = 1;
				pinc->coef = coef;
			}

			else
				pinc->rtune = 0;

			if (pinc->RunHeuristic())
			{
				neigh += 5;
				if (pinc->obj < 0)
				{
					//delete pinc;
					break;
				}

				Heuristic* pinc2 = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
				pinc2->heur = 1;
				if (bf == 2)
				{
					pinc2->rtune = 1;
					pinc2->coef = coef;
				}

				else
					pinc2->rtune = 0;

				if (pinc2->RunHeuristic())
				{
					if (pinc->obj <= pinc2->obj || pinc2->obj < 0)
					{
						delete pinc2;
						break;
					}

					else
					{
						delete pinc;
						neighbest = neigh;
						pinc = new Heuristic(*pinc2);
						delete pinc2;
						devam = true;
					}
				}
			}
		}

		else
		{
			neigh += 5;
			Heuristic* pinc2 = new Heuristic(m, criticsize, limit, tt, n, tt, pj, d, Ymax, rr, SLA, neigh, preventives, correctives, critics, penymaxplus, penminus, pencor, initials);
			pinc2->heur = 1;
			if (bf == 2)
			{
				pinc2->rtune = 1;
				pinc2->coef = coef;
			}
			else
				pinc2->rtune = 0;

			if (pinc2->RunHeuristic())
			{
				if (pinc->obj <= pinc2->obj || pinc2->obj < 0)
				{
					delete pinc2;
					break;
				}
				else
				{
					delete pinc;
					neighbest = neigh;
					pinc = new Heuristic(*pinc2);
					delete pinc2;
				}
			}
		}

	}

	//////later
	if (pinc->obj >= 0)
	{
		cout << "obj = " << pinc->obj << endl;
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);

		MIPstart4(startVar, startVal, model, pinc, s, n, tt, m, y, P, x, e, a, b, E, T, C, Sj, S, F, L, B, np, o, deltaYmaxplus, deltaminusSLA, preventives, correctives, tardyjobs, rr, pj, d, Ymax, SLA);
		cplex.addMIPStart(startVar, startVal);

		startVal.clear();
		startVar.clear();
	}

	else
		cout << "INFEASIBLE!!!!!!!" << endl;

	cplex.setParam(IloCplex::TiLim, 60*2);

	//cplex.writeSolution("ahmet.sol");

	time = cplex.getCplexTime();
	IloBool success = cplex.solve();   

	double elapsedtime = cplex.getCplexTime() - time; 

	if (success && cplex.isPrimalFeasible())
	{
		/*if (dayindex >= 60)
			DatatoFile(SLAdaily, n, totaltt, dayindex + 1, Jcor, Jint, prevprdtms, preventnum, m, dataday);*/

		myfile << elapsedtime << "\t" << cplex.getObjValue() << "\t" << cplex.getMIPRelativeGap() * 100 << "\n";
		
		/*vector<vector<Base>> cand = Write(model, cplex, env, myfile, n, begweek, endweek, realbegweek, warmup, dayindex, tt, m, s, time, cumtime, cum, pr, sol, cumSLAviol, cumYmaxviol, cumgap, cumnodes, buffer
			, B, o, y, P, x, e, a, b, E, T, C, Sj, S, F, L, np, deltaminusSLA, deltaYmaxplus, coroutservice, datarunfile, Jcor, Jint, initials, SLA, prevprdtms);

		DeleteJobsbuffer(Jint, Jcor, cplex, x, np, C, Sj, S, F, prevprdtms, preventnum, m, cand);*/
	}

	else
	{
		// olmadı instanceları buraya yazdır!		
		ofstream infsbleinst("infsble.txt");
		//DatatoFile(infsbleinst, SLAdaily, n, totaltt, realbegweek, Jcor, Jint);
		cplex.exportModel("infeasible.lp");

		myfile << "INFEASIBLE!!!!" << endl;
		myfile << "success = " << success << endl;
		myfile << "cplex.isPrimalFeasible() = " << cplex.isPrimalFeasible() << endl;
		myfile.close();
		infsbleinst.close();
		return true;

	}

	cplex.end();
	model.end();
	env.end();
	delete pinc;
	//delete[] tracklimit;
	return false;
}


int main()
{
	try
	{
		//ofstream mfile("data.txt");
		ofstream datarunfile("all.txt");
		//sonra buradaki araç sayısını arttırırsın!
		int k = 0;
		int day = 7;
		const int tt = 24 * day; //7 gün	
		const int M = tt;
		const int ti = tt;
		int totvar = 0;
		int run = 1;
		/////////////////

		double time = 0;

		vector<int> preventives;
		vector<int> correctives;
		vector<int> critics;

		int* r;
		int alpha = 1;
		int penminus = 10;
		int penymaxplus = 4;
		int pencor = 5;
		int postpone[2] = { 25 * 24, 30 * 24 }; //real - maint length
		int pcor = 10;
		int pjob = 8;

		int tau[2] = { 2508, 10068 };
		int totalday = 160;

		////////////////////////////

		int m[2] = { 2, 3 };
		double vv1[2] = { 0.1, 0.2 };
		double limit = M * 10;
		//ofstream myfile("myfile.txt");

		int buffer[3] = { 0, 10, 0 };
		int coef = 4;

		int v1;

		int ust = 10;
		int alt = 1;

		ofstream myfile("myfile.txt");

		//for (int i = 0; i < 3; ++i)
		for (int i = 2; i < 3; ++i)
		{
			int* SLAdaily; 
			string pathparent;
			int m;

			//SLAdaily = new int[24]{0, 6, 10, 10, 7, 7, 7, 7, 7, 8, 12, 12, 11, 11, 10, 7, 6, 5, 5, 2, 0, 0, 0, 0};
			//pathparent = "D:\\Users\\suuser\\Desktop\\tez\\cplex tez\\19.02.2016-Ogleden sonra\\Yeni klasör\\runs\\medium\\pseudo\\cor\\real\\";
			//20.3.20: only to test one case			
			//aşağıya cor jobs da okuyucu kod koy

			switch (i)
			{
			case 0: //low
			{
				SLAdaily = new int[24]{90, 90, 90, 90, 90, 90, 84, 84, 84, 84, 84, 84, 86, 86, 86, 86, 86, 86, 0, 0, 0, 0, 0, 0};
				pathparent = "C:\\Users\\labuser\\Desktop\\cplex tez\\19.02.2016-Ogleden sonra\\Yeni klasör\\runs\\low\\pseudo\\cor\\";				
				m = 3;
			}
			break;

			case 1: //medium
			{
				SLAdaily = new int[24]{96, 96, 96, 96, 96, 96, 74, 74, 74, 74, 74, 74, 90, 90, 90, 90, 90, 90, 0, 0, 0, 0, 0, 0};
				pathparent = "C:\\Users\\labuser\\Desktop\\cplex tez\\19.02.2016-Ogleden sonra\\Yeni klasör\\runs\\medium\\pseudo\\cor\\real\\";
				m = 2;
			}
			break;

			case 2: 
			{
				/*SLAdaily = new int[24]{97, 97, 97, 97, 97, 97, 72, 72, 72, 72, 72, 72, 91, 91, 91, 91, 91, 91, 0, 0, 0, 0, 0, 0};
				pathparent = "C:\\Users\\labuser\\Desktop\\cplex tez\\19.02.2016-Ogleden sonra\\Yeni klasör\\runs\\high\\pseudo\\cor\\";
				m = 2;*/ //old sys3 //high //old-discarded

				//sys1 oldu //least load //m3_slaS_tau10068_pi720_v0.2
				/*SLAdaily = new int[24]{90, 90, 90, 90, 90, 90, 83, 83, 83, 83, 83, 83, 87, 87, 87, 87, 87, 87, 0, 0, 0, 0, 0, 0};
				pathparent = "C:\\Users\\labuser\\Desktop\\cplex tez\\19.02.2016-Ogleden sonra\\Yeni klasör\\runs\\sys1'\\pseudo\\";
				m = 3;*/

				//sys3 //m2_slaS_tau2508_pi720_v0.1
				SLAdaily = new int[24]{88, 88, 88, 88, 88, 88, 85, 85, 85, 85, 85, 85, 87, 87, 87, 87, 87, 87, 0, 0, 0, 0, 0, 0};
				pathparent = "C:\\Users\\labuser\\Desktop\\cplex tez\\19.02.2016-Ogleden sonra\\Yeni klasör\\runs\\sys3'\\pseudo\\";
				m = 2; 
			}
			break;
			}

			for (int dos = alt; dos <= ust; ++dos)
			{
				string line;
				//ifstream file(dos + ".txt");			

				string path = pathparent + to_string(dos) + ".txt";
				ifstream file(path);

				int pr = 0;
				int scale = 1;				
				int** prevprdtms = new int*[m];
				for (int i = 0; i < m; i++)
					prevprdtms[i] = new int[4]{-1, -1, -1, -1};

				string ktype;
				string SLAtype;

				JobInt Jint;
				JobInt Jcor;

				int totaltt = 168 * 40; // to diminish tail effect

				//number = to_string(slascale[scale]);
				double cum[3] = { 0, 0, 0 };
				int cumSLAviol[3] = { 0, 0, 0 };
				double cumtime[3] = { 0, 0, 0 };
				int cumschedjobs = 0;
				double cumgap[3] = { 0, 0, 0 };
				int cumnodes[3] = { 0, 0, 0 };
				int cumYmaxviol[3] = { 0, 0, 0 };

				//int SLAdaily[24] = { 100, 100, 100, 100, 100, 100, 78, 78, 78, 78, 78, 78, 96, 96, 96, 96, 96, 96, 0, 0, 0, 0, 0, 0 }; //+1 dor heap corruption								

				vector<Job> Jcurprv;
				vector<Job> Jcurcor;
				vector<Job> tempJcurprv;
				vector<Job> tempJcurcor;
				vector<int> mmin;
				vector<int> tardyjobs;
				vector<Job> Jprvnext;
				vector<Job> Jcornext;
				int* SLA = new int[tt];

				for (int dayindex = 0; dayindex < 1; ++dayindex) //16: to test last week
				{
					ofstream dataday(to_string(dayindex) + ".txt");

					int begweek = dayindex * 24;
					int endweek = begweek + tt; //w.r.t hrs
					int n = 105;

					//Gerçek büyük boyutlu sla

					for (int i = 0; i < tt; ++i)
					{
						int k = i % 24;
						SLA[i] = SLAdaily[k];
					}

					int* Ymax = new int[n];
					int* d = new int[n];
					int* rr = new int[n];
					int* pj = new int[n];

					int bosinit = 0;

					int maxYmin = -1;
					int minYmin = tt;
					int maxYmax = -1;
					int pmin = tt;

					int realbegweek=0; //olması gereken-initialize
					begweek = 0;
					endweek = tt;
					bool ayrac = false;
					double z2;

					if (file.is_open())
					{
						int count = 0;
						while (getline(file, line)) //line her bir satırı okur
						{
							istringstream Stream(line); //ID pj rj dj Ymax
							++count;

							if (line == "") //dosyadan dosyaya değişiyor. Break pointle deneyip bakılmalı!
							{
								ayrac = true;
								continue;
							}

							if (count == 1)
							{
								Stream >> realbegweek;
								continue;
							}							

							if (ayrac == false)
							{
								int job, type, delay, base, t,ymax;
								t = count - 2;
								Stream >> job >> type >> delay >> base >> ymax;

								if (type == 0)
								{
									prevprdtms[t][0] = job;
									prevprdtms[t][1] = type;
									prevprdtms[t][2] = delay;
									continue;
								}
								
								//mapping
								//Jcurcor.emplace_back(job, rj, ymax, dj, delay);
								Jcurcor.emplace_back(job, delay, ymax, base, type);
							}

							else
							{
								int ID, dj, rj, pj, Ymax;
								Stream >> ID >> pj >> rj >> dj >> Ymax;
								Jcurprv.emplace_back(ID, rj, Ymax, dj, pj);
							}

						}
						file.close();
					}

					//determine s
					int totaljobs = Jcurprv.size() + Jcurcor.size();

					for each (Job job in Jcurprv)
					{
						int pj = job.pj;
						int rj = job.rj;
						mmin.push_back(pj);
						/*if (Ymax[j] > maxYmax)
						maxYmax = Ymax[j];*/
						if (pj < pmin)
							pmin = pj;
						if (rj < minYmin)
							minYmin = rj;
						if (rj > maxYmin)
							maxYmin = rj;
					}

					for each (Job job in Jcurcor)
					{
						int pj = job.pj;
						int rj = job.rj;
						mmin.push_back(pj);
						/*if (Ymax[j] > maxYmax)
						maxYmax = Ymax[j];*/
						if (pj < pmin)
							pmin = pj;
						if (rj < minYmin)
							minYmin = rj;
						if (rj > maxYmin)
							maxYmin = rj;
					}

					//for heuristicS
					//merge two sets in preventives

					for each (Job job in Jcurcor)
					{
						int j = job.ID;
						Ymax[j] = job.Ymax - realbegweek;
						d[j] = job.dj - realbegweek;
						rr[j] = job.rj - realbegweek;
						pj[j] = job.pj;
						critics.push_back(j);
						correctives.push_back(j);
						tardyjobs.push_back(j);
					}

					for each (Job job in Jcurprv)
					{
						int j = job.ID;
						Ymax[j] = job.Ymax - realbegweek;
						d[j] = job.dj - realbegweek;
						rr[j] = job.rj - realbegweek;
						pj[j] = job.pj;
						preventives.push_back(j);
						critics.push_back(j);
					}


					for (int job : preventives)
					{
						if (Ymax[job] <= 0)
							tardyjobs.push_back(job);
					}

					int sol = 0;
					cout << "dayindex =" << dayindex << endl;

					bool infeasible = false;
					int warmup = 0;
					int bf = 0;

					int s = totaljobs * m;
					sort(mmin.begin(), mmin.end(), greater<int>()); //descending order

					//////normal model
					infeasible = SolveModel(z2, SLAdaily, totaltt, myfile, warmup, dayindex, time, cumtime, cum, pr, sol, cumSLAviol, cumYmaxviol,
						cumgap, cumnodes, datarunfile, critics, pencor, penminus, penymaxplus, maxYmin, maxYmax, minYmin, buffer[bf], mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek,
						Jcurprv, Jcurcor, preventives, tardyjobs, correctives, d, Ymax, rr, limit, prevprdtms, bf, coef);

					////////model+cut
					infeasible = SolveModelCut(z2, SLAdaily, totaltt, myfile, warmup, dayindex, time, cumtime, cum, pr, sol, cumSLAviol, cumYmaxviol,
						cumgap, cumnodes, datarunfile, critics, pencor, penminus, penymaxplus, maxYmin, maxYmax, minYmin, buffer[bf], mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek,
						Jcurprv, Jcurcor, preventives, tardyjobs, correctives, d, Ymax, rr, limit, prevprdtms, bf, coef); 

					//Heur + model2 + cuts
					infeasible = SolveModeldenemeHeur1(z2, SLAdaily, totaltt, myfile, warmup, dayindex, time, cumtime, cum, pr, sol, cumSLAviol, cumYmaxviol,
						cumgap, cumnodes, datarunfile, critics, pencor, penminus, penymaxplus, maxYmin, maxYmax, minYmin, buffer[bf], mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek,
						Jcurprv, Jcurcor, preventives, tardyjobs, correctives, d, Ymax, rr, limit, prevprdtms, bf, coef, dataday);

					//model1+heur
					/*infeasible = SolveModelHeur1(z2, SLAdaily, totaltt, myfile, warmup, dayindex, time, cumtime, cum, pr, sol, cumSLAviol, cumYmaxviol,
					cumgap, cumnodes, datarunfile, critics, pencor, penminus, penymaxplus, maxYmin, maxYmax, minYmin, buffer[bf], mmin, s, n, tt, m, M, SLA, pj, realbegweek, begweek, endweek,
					Jcurprv, Jcurcor, preventives, tardyjobs, correctives, d, Ymax, rr, limit, prevprdtms, bf, coef, dataday);*/

					
					preventives.clear();
					correctives.clear();
					critics.clear();
					mmin.clear();
					tardyjobs.clear();

					delete[] Ymax;
					delete[] d;
					delete[] pj;
					delete[] rr;

					dataday.close();
				} //week end

				Jint.clear();
				Jcor.clear();
				Jcurcor.clear();
				Jcurprv.clear();

				delete[] SLA;

				for (int i = 0; i < m; i++)
					delete[] prevprdtms[i];
				delete[] prevprdtms;
			}
		
		}

		myfile.close();
	}

	catch (IloException& exception)
	{
		cout << exception.getMessage() << endl;
	}

	return 0;
}


