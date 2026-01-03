#include <vector>
#include <string>
#include <array>
#include <algorithm>
using namespace std;

class Heuristic
{
	struct Base
	{
		int id;
		int jobid;
		int starttime;
		int length;
		int finish;
		int parid;


		Base(int IDIn, int jobIDIn, int startIn, int lengthIn, int finishIn)
		{
			id = IDIn;
			jobid = jobIDIn;
			starttime = startIn;
			length = lengthIn;
			finish = finishIn;
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


	struct Job
	{
		int id;
		int rj;
		int pj;

		Job(int IDIn, int rjIn, int pjIn)
		{
			id = IDIn;
			rj = rjIn;
			pj = pjIn;
		}
	};


	struct less_than_key2
	{
		inline bool operator() (Job*& J1, Job*& J2)
		{
			return J1->pj < J2->pj;
		}
	};

	struct descpj
	{
		inline bool operator() (array<int, 3>& a1, array<int, 3>& a2) //ascorder
		{
			return a1[2] > a2[2];
		}
	};

	struct ascpj
	{
		inline bool operator() (array<int, 3>& a1, array<int, 3>& a2) //ascorder
		{
			return a1[2] < a2[2];
		}
	};

	struct lessthankey
	{
		inline bool operator() (array<int, 3>& a1, array<int, 3>& a2)
		{
			return a1[1] < a2[1];
		}
	};

	struct descpj2
	{
		inline bool operator() (array<int, 2>& a1, array<int, 2>& a2)
		{
			return a1[1] > a2[1];
		}
	};

	struct lessthankeyy
	{
		inline bool operator() (array<int, 3>& a1, array<int, 3>& a2)
		{
			return a1[1] > a2[1];
		}
	};

	struct less_than_key
	{
		inline bool operator() (Job& J1, Job& J2)
		{
			return J1.rj > J2.rj;
		}
	};

	struct basejobid
	{
		inline bool operator() (Base& B1, Base& B2)
		{
			return B1.jobid < B2.jobid;
		}
	};

	struct basestart
	{
		inline bool operator() (Base& B1, Base& B2)
		{
			return B1.starttime < B2.starttime;
		}
	};

	struct baseid
	{
		inline bool operator() (Base& B1, Base& B2)
		{
			return B1.id < B2.id;
		}
	};

	int m;
	int before;
	int n;
	int tt;
	int nn;
	int ti;
	int* pj;
	int* d;
	int* Ymax;
	int* r;
	int* SLA;
	int neigh;
	vector<int> preventives;
	vector<int> correctives;
	vector<int> critics;
	vector<vector<int>> Hangar;
	vector<int> initials;
	int* base;


	int penymaxplus;
	int penminus;
	int pencor;

	void CalculateObj(int& obj, const vector<vector<int>> Hangar);
	pair<int, int> EarlTardCalc(int j, int begin, int duetime);
	void PreventInfeas(vector<int>Jcand, int* pj, int* Ymax, int& lastitem, int t, int& count);
	bool Checkslack(vector<int> J, int* slack);
	int Objectard(int j, int begin, int duetime, int Ymax, int type);
	void Interchange(vector<Base>& sametrack,vector<vector<Base>>&cnd, int& obj, vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int type);
	vector <Base> GetDescendants(Base& rootbase, int trackid, vector<ParentChild>& ParChildAs, vector<Base>& BaseAs); //recursive oldugundan & le tanımlama!
	void GetChildren(const vector<ParentChild>& ParChildAs, const Base& rootbase, vector<Base>& Children);
	//bool HangarAssign(int lastitem, vector<vector<int>>& Hangar, int t, int m, vector<int>& Jcand, int nn, int* SLA, int* Ymax, int* pj, int base[], int jobtimes[][2], vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int* d);
	bool HangarAssign(int lastitem, int minslackjob, int& totpj, vector<vector<int>>& Hangar, int t, int m, vector<int>& Jcand, int nn, int* SLA, int* Ymax, int* pj, int base[], int jobtimes[][2], vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int* d);
	bool AssigntoHangar(vector<Job*>& TEMP, vector<vector<int>>& Hangar, int t, int m, vector<int>& Jcand, int& obj, int nn, int* SLA, int* Ymax, int* pj, int base[], int jobtimes[][2], vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int* d);
	bool AssigntoHangar2(vector <array<int, 3>>& assigncandid, int& numbertohangar, int& totpj, vector<vector<int>>& Hangar, int t, int m, vector<int>& Jcand, int& obj, int nn, int* SLA, int* Ymax, int* pj, int base[], int jobtimes[][2], vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int* d);

public:

	int obj;
	int coef = 0;
	int rtune = 0;
	int mode = 0;
	int heur = 0;
	vector<int> sequence;
	
	vector<pair<int, int>> tardearl;
	vector<ParentChild> ParChildAs;
	vector<Base> BaseAs;
	double limit;
	bool RunHeuristic();
	//void ImproveNew();
	void SortBase(vector<Base>& BaseAs, string type);
	void Improve();
	//define public func that sorts base wrt job id and baseid
	Heuristic(int mIn, int nIn, double limitIn, int ttin, const int nnin, int tin, int* pjin, int* din, int* Ymaxin, int* rin, int* SLAin, int neighIn,
		vector<int> preventivesIn, vector<int> correctivesIn, vector<int> criticsIn, int penymaxplusIn, int penminusIn, int pencorIn, vector<int> initials);
	Heuristic(const Heuristic& other);
	Heuristic();
	~Heuristic();
};