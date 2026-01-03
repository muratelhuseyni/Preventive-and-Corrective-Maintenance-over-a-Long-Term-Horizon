#include "Heuristic.h"
#include <iostream>
#include <algorithm> 
#include <fstream>
#include <map>

Heuristic::Heuristic(int mIn, int nIn, double limitIn, int ttin, const int nnin, int tin, int* pjin
	, int* din, int* Ymaxin, int* rin, int* SLAin, int neighIn, vector<int> preventivesIn, vector<int> correctivesIn, vector<int> criticsIn,
	int penymaxplusIn, int penminusIn, int pencorIn, vector<int> initialsIn)
{
	/*tardearl = nullptr;*/
	/*tardearl = tardearlIn;
	tardearl.resize(nn);*/
	m = mIn;
	n = nIn;
	tt = ttin;
	nn = nnin;
	ti = tin;
	//pj = pjin;
	//d = din;
	//Ymax = Ymaxin;
	limit = limitIn;
	//r = rin;
	//SLA = SLAin;
	pj = new int[nn];
	for (int i = 0; i < nn; ++i)
		pj[i] = pjin[i];

	d = new int[nn];
	for (int i = 0; i < nn; ++i)
		d[i] = din[i];

	Ymax = new int[nn];
	for (int i = 0; i < nn; ++i)
		Ymax[i] = Ymaxin[i];

	r = new int[nn];
	for (int i = 0; i < nn; ++i)
		r[i] = rin[i];

	SLA = new int[tt];
	for (int i = 0; i < tt; ++i)
		SLA[i] = SLAin[i];


	neigh = neighIn;
	preventives = preventivesIn;
	correctives = correctivesIn;
	critics = criticsIn;
	penymaxplus = penymaxplusIn;
	penminus = penminusIn;
	pencor = pencorIn;
	initials = initialsIn;
}

//copy constructor
Heuristic::Heuristic(const Heuristic& other)
{
	/*tardearl = nullptr;*/
	m = other.m;
	n = other.n;
	tt = other.tt;
	nn = other.nn;
	ti = other.ti;
	penymaxplus = other.penymaxplus;
	penminus = other.penminus;
	pencor = other.pencor;
	preventives = other.preventives;
	correctives = other.correctives;
	critics = other.critics;
	neigh = other.neigh;
	pj = new int[nn];
	for (int i = 0; i < nn; ++i)
		pj[i] = other.pj[i];

	d = new int[nn];
	for (int i = 0; i < nn; ++i)
		d[i] = other.d[i];

	Ymax = new int[nn];
	for (int i = 0; i < nn; ++i)
		Ymax[i] = other.Ymax[i];

	r = new int[nn];
	for (int i = 0; i < nn; ++i)
		r[i] = other.r[i];

	SLA = new int[tt];
	for (int i = 0; i < tt; ++i)
		SLA[i] = other.SLA[i];

	base = new int[m];
	for (int i = 0; i < m; ++i)
		base[i] = other.base[i];

	Hangar = other.Hangar;
	BaseAs = other.BaseAs;
	ParChildAs = other.ParChildAs;
	//delete[] tardearl;
	//tardearl = new int*[nn]; //yeni adreste yaratmak için. Böylece deep copy(CLONE) olur!
	//for (int i = 0; i < nn; i++)
	//	tardearl[i] = other.tardearl[i];
	tardearl = other.tardearl;
	obj = other.obj;
	initials = other.initials;
}

Heuristic::~Heuristic()
{
	std::cout << "destructor~ called" << endl;
	//neww //2/15/17
	delete[] Ymax;
	delete[] d;
	delete[] pj;
	delete[] r;
	delete[] SLA;
}

Heuristic::Heuristic()
{
	/*tardearl = nullptr;*/
}

void Heuristic::SortBase(vector<Base>& BaseAs, string type)
{
	if (type == "jobid")
		sort(BaseAs.begin(), BaseAs.end(), basejobid());
	else
		sort(BaseAs.begin(), BaseAs.end(), baseid());
}

int Heuristic::Objectard(int j, int begin, int duetime, int Ymax, int type)
{
	int dif = begin - duetime;

	if (dif > 1000)
		int a = 2;

	int tard, earl;
	int obj = 0;

	if (dif > 0)
	{
		tard = dif;
		earl = 0;
	}

	else if (dif < 0)
	{
		earl = -dif;
		tard = 0;
	}

	else
	{
		tard = 0;
		earl = 0;
	}

	if (type == 0)
	{
		if (begin > Ymax)
		{
			int deltaymaxplus = begin - Ymax;
			obj += penymaxplus*deltaymaxplus;
		}

		if (rtune == 0)
			obj += tard + earl;
		else
			obj += coef * (pencor - penymaxplus) * tard + earl;
	}

	else
	{
		obj += tard*pencor;
	}

	return obj;
}

void Heuristic::Interchange(vector<Base>& sametrack, vector<vector<Base>>&cnd, int& obj, vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int type)
{
	//cnd: basesets
	//baseset: children of specific base
	//base: each base in baseset
	int criticsize = critics.size();
	if (type == 1)
	{
		int change = 0;
		int tempid;
		int parentcount = 0;
		for (auto& baseset : cnd) //check: kopyasýysa ok, gerçekse sorun var
		{
			if (baseset.size() < 2)
				continue;
			bool improve = false;
			do{
				for (int i = 0; i < baseset.size() - 1; ++i)
					for (int j = i + 1; j < baseset.size(); ++j)
					{
						improve = false;
						auto tempbaseset = baseset;
						int delta = tempbaseset[j].length - tempbaseset[i].length;
						/*if (delta == 0)
						continue;*/

						int obj = 0; //Objectard(int j, int begin, int duetime)
						for (int k = i; k <= j; ++k)
						{
							int tur;
							int job = baseset[k].jobid;
							if (binary_search(preventives.begin(), preventives.end(), job))
								tur = 0;
							else
								tur = 1;
							obj += Objectard(baseset[k].jobid, tempbaseset[k].starttime, d[tempbaseset[k].jobid], Ymax[tempbaseset[k].jobid], tur); //0 is a type, you wil lchange it
						}

						//DO changes - if both of them at same time
						int tur1;
						int tur2;


						tempid = tempbaseset[i].jobid;

						//20.01i2017 - new
						if (binary_search(preventives.begin(), preventives.end(), tempid)) //for - i
							tur1 = 0;
						else
							tur1 = 1;

						if (binary_search(preventives.begin(), preventives.end(), tempbaseset[j].jobid)) //for - i
							tur2 = 0;
						else
							tur2 = 1;

						if (tur1 != tur2)
							continue;

						tempbaseset[i].jobid = tempbaseset[j].jobid;
						tempbaseset[i].length = pj[tempbaseset[i].jobid];
						tempbaseset[i].finish = tempbaseset[i].starttime + tempbaseset[i].length;


						tempbaseset[j].jobid = tempid; //swap is done
						tempbaseset[j].starttime += delta;
						tempbaseset[j].length = pj[tempbaseset[j].jobid];
						tempbaseset[j].finish = tempbaseset[j].starttime + tempbaseset[j].length;

						if (tempbaseset[j].starttime >= sametrack[parentcount].starttime + pj[sametrack[parentcount].id]) //swap feasibility-1
							continue;

						if (tempbaseset[i].starttime < r[tempbaseset[i].jobid]) //swap feasibility-2
							continue;

						bool infs = false;
						//infeas check

						for (int l = i + 1; l < j; ++l)
						{
							tempbaseset[l].starttime += delta;
							tempbaseset[l].finish += delta;

							if (tempbaseset[l].starttime < r[tempbaseset[l].jobid])
							{
								infs = true;
								break;
							}
						}

						if (infs)
							continue;

						int obj2 = 0;
						for (int k = i; k <= j; ++k)
						{
							int tur;
							int job = tempbaseset[k].jobid;
							if (binary_search(preventives.begin(), preventives.end(), job))
								tur = 0;
							else
								tur = 1;
							obj2 += Objectard(tempbaseset[k].jobid, tempbaseset[k].starttime, d[tempbaseset[k].jobid], Ymax[tempbaseset[k].jobid], tur); // 0 is a type
						}

						if (obj - obj2 > 0)
						{
							baseset = tempbaseset;
							improve = true;
							++change;
						}
						if (improve)
							break;
					}
			} while (improve);
			++parentcount;
		}

		if (change > 0)
		{
			for (auto baseset : cnd)
				for (auto base : baseset)
					for (int i = 0; i < BaseAs.size(); ++i)
					{
						if (base.id == BaseAs[i].id)
						{
							BaseAs[i] = base;
							break;
						}
					}

			obj = 0;
			vector<vector<int>> Hangarr;
			for each (Base base in BaseAs)
			{
				int tur;
				int job = base.jobid;
				if (binary_search(preventives.begin(), preventives.end(), job))
					tur = 0;
				else
					tur = 1;
				//tardearl[base.jobid] = EarlTardCalc(base.jobid, base.starttime, d[base.jobid]);
				critics.erase(remove(critics.begin(), critics.end(), job), critics.end());
				obj += Objectard(job, base.starttime, d[job], Ymax[job], tur);
			}

			for (int job : critics)
			{
				int tur;
				int t = tt;
				//tardearl[job] = EarlTardCalc(job, t, d[job]);
				if (binary_search(preventives.begin(), preventives.end(), job))
					tur = 0;
				else
					tur = 1;

				obj += Objectard(job, t, d[job], Ymax[job], tur);
			}

			CalculateObj(obj, Hangarr);
		}
	}

}

void Heuristic::GetChildren(const vector<ParentChild>& ParChildAs, const Base& rootbase, vector<Base>& Children)
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

vector <Heuristic::Base> Heuristic::GetDescendants(Base& rootbase, int trackid, vector<ParentChild>& ParChildAs, vector<Base>& BaseAs)
{
	vector<Base> Children;
	//base case
	if (trackid == m - 1)
	{
		Children.push_back(rootbase);
		return Children;
	}

	GetChildren(ParChildAs, rootbase, Children);
	vector<Base> tempChildren;
	//tempChildren = Children;

	for (auto& child : Children)
	{
		vector<Base> temp = GetDescendants(child, trackid + 1, ParChildAs, BaseAs);
		tempChildren.reserve(tempChildren.size() + distance(temp.begin(), temp.end()));
		tempChildren.insert(tempChildren.end(), temp.begin(), temp.end());
	}

	tempChildren.push_back(rootbase);
	return tempChildren;
}

void Heuristic::Improve()
{
	vector<Base> Children;
	typedef vector<vector<Base>> Candidates;
	Candidates cand;

	vector<Base> sametrack;

	int tr = m - 1;

	for (int i = 0; i < BaseAs.size(); i++)
	{
		if (floor(BaseAs[i].id / n) == tr - 1)
			sametrack.push_back(BaseAs[i]);
	}

	cand.resize(sametrack.size());
	for (int k = 0; k < sametrack.size(); ++k) //sametrack - 1 track öncedek atalar
	{

		GetChildren(ParChildAs, sametrack[k], Children);
		if (Children.size() < 2)
		{
			Children.clear();
			continue;
		}
		cand[k] = Children;
		Children.clear();
	}

	Interchange(sametrack, cand, obj, BaseAs, ParChildAs, 1);
	cand.clear();
}

pair<int, int> Heuristic::EarlTardCalc(int j, int begin, int duetime)
{
	//int tarearl[2];

	int dif = begin - duetime;
	int tard, earl;
	if (dif > 0)
	{
		tard = dif;
		earl = 0;
	}

	else if (dif < 0)
	{
		earl = -dif;
		tard = 0;
	}

	else
	{
		tard = 0;
		earl = 0;
	}

	/*obj += tard + earl;*/
	/*tarearl[0] = tard;
	tarearl[1] = earl;*/
	pair <int, int> p(tard, earl);
	return p;
}

//void Heuristic::PreventInfeas(vector<int>Jcand, int* pj, int* Ymax, int& lastitem, int t, int& count)
//{
//	//int violjob = -1;
//	//INFEASIBILITY PREVENTION	
//
//	for (int j = 0; j < Jcand.size(); j++)
//	{
//		if (t + pj[lastitem] > Ymax[Jcand[j]])
//		{
//			//violjob = Jcand[j];
//			count++;
//			break;
//		}
//	}
//
//	if (count > 0)
//	{
//		Jcand.erase(remove(Jcand.begin(), Jcand.end(), lastitem), Jcand.end());
//		vector <array<int, 2>> pjtemp;
//		for (int k = 0; k < Jcand.size(); k++)
//		{
//			array <int, 2> a = { Jcand[k], pj[Jcand[k]] };
//			pjtemp.push_back(a);
//		}
//
//		if (pjtemp.size() >1)
//			sort(pjtemp.begin(), pjtemp.end(), descpj2()); //desc pj order- pop back
//
//		while (pjtemp.size() > 0)
//		{
//			int size = static_cast<int>(pjtemp.size());
//			lastitem = pjtemp.back()[0];
//			int sayac = 0;
//
//			if (size > 1)
//			{
//				for (int j = 0; j < size; j++)
//				{
//					if (t + pj[lastitem] > Ymax[pjtemp[j][0]] && lastitem != pjtemp[j][0])
//					{
//						//violjob = Jcand[j];
//						sayac++;
//						pjtemp.pop_back();
//						break;
//					}
//
//				}
//
//				if (sayac == 0) //violate yok, second check!
//				{
//					int tard = ObjectiveTardCalc(lastitem, t, obj, d[lastitem]).first;
//					if (tard > Ymax[lastitem] - d[lastitem]) //LAST ITEM silinmeli!
//					{
//						pjtemp.pop_back();
//						continue;
//					}
//					count = 0;
//					break;
//				}
//			}
//
//			else
//			{
//				if (t <= Ymax[lastitem])
//					count = 0;
//				break;
//			}
//
//		}
//	}
//}

bool Heuristic::HangarAssign(int lastitem, int minslackjob, int& totpj, vector<vector<int>>& Hangar, int t, int m, vector<int>& Jcand, int criticsize, int* SLA, int* Ymax, int* pj, int base[], int jobtimes[][2], vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int* d)
{
	jobtimes[lastitem][0] = t; //bj time set
	jobtimes[lastitem][1] = t + pj[lastitem]; //ej time set

	int baseid = Hangar[t].size()*criticsize + base[Hangar[t].size()];
	if (baseid < 0)
		return true;
	BaseAs.emplace_back(baseid, lastitem, t, -1, -1); //id,jobid,Sb,Pb,Fb,parentid of this base

	if (Hangar[t].size() > 0)
	{
		int parentid = -1;
		for (int i = BaseAs.size() - 1; i >= 0; i--)
		{
			if (BaseAs[i].jobid == Hangar[t][Hangar[t].size() - 1])
			{
				parentid = BaseAs[i].id;
				break;
			}
		}
		ParChildAs.emplace_back(parentid, baseid); //parentid=this block, childid=baseid çünkü daha yerleþtirmedim.
		//bu dpdate'i tüm joblar için yap!!!
		if (jobtimes[lastitem][1] > jobtimes[Hangar[t][Hangar[t].size() - 1]][1])
		{
			for (int j = 0; j < Hangar[t].size(); j++)
			{
				if (jobtimes[Hangar[t][j]][1] < jobtimes[lastitem][1])
					jobtimes[Hangar[t][j]][1] = jobtimes[lastitem][1];
			}
			//e_j updated!
		}
	}

	if (mode == 0)
		base[Hangar[t].size()]++;
	Hangar[t].push_back(lastitem);
	//return false;
}

bool Heuristic::AssigntoHangar2(vector <array<int, 3>>& assigncandid, int& numbertohangar, int& totpj, vector<vector<int>>& Hangar, int t, int m, vector<int>& Jcand, int& obj, int nn, int* SLA, int* Ymax, int* pj, int base[], int jobtimes[][2], vector<Base>& BaseAs, vector<ParentChild>& ParChildAs, int* d)
{
	bool infeas;

	if (assigncandid.size() > 1 && initials.size() == 0)
		sort(assigncandid.begin(), assigncandid.end(), ascpj()); //max pj last

	if (initials.size()> 0)
		reverse(assigncandid.begin(), assigncandid.end());

	while (true)
	{
		if (assigncandid.size() == 0 || numbertohangar == 0)
			break;

		int lastitem;

		lastitem = assigncandid.back()[0]; //take largest pj


		//else
		//{
		jobtimes[lastitem][0] = t; //bj time set
		jobtimes[lastitem][1] = t + pj[lastitem]; //ej time set

		int baseid = Hangar[t].size()*nn + base[Hangar[t].size()];
		if (baseid < 0)
			return true;
		BaseAs.emplace_back(baseid, lastitem, t, -1, -1); //id,jobid,Sb,Pb,Fb,parentid of this base

		if (Hangar[t].size() > 0)
		{
			int parentid = -1;
			for (int i = BaseAs.size() - 1; i >= 0; i--)
			{
				if (BaseAs[i].jobid == Hangar[t][Hangar[t].size() - 1])
				{
					parentid = BaseAs[i].id;
					break;
				}
			}
			ParChildAs.emplace_back(parentid, baseid); //parentid=this block, childid=baseid çünkü daha yerleþtirmedim.
			//bu dpdate'i tüm joblar için yap!!!
			if (jobtimes[lastitem][1] > jobtimes[Hangar[t][Hangar[t].size() - 1]][1])
			{
				for (int j = 0; j < Hangar[t].size(); j++)
				{
					if (jobtimes[Hangar[t][j]][1] < jobtimes[lastitem][1])
						jobtimes[Hangar[t][j]][1] = jobtimes[lastitem][1];
				}
				//e_j updated!
			}
		}

		if (mode == 0)
			base[Hangar[t].size()]++;
		Hangar[t].push_back(lastitem);

		assigncandid.pop_back();
		--numbertohangar;

	}
	return false;
}

//Baseas Public
void Heuristic::CalculateObj(int& obj, vector<vector<int>> Hangar)
{
	//fill a hangar
	if (Hangar.size() == 0)
	{
		Hangar.resize(tt);
		for (int t = 0; t < tt; ++t)
		{
			for each(Base base in BaseAs)
			{
				if (t >= base.starttime && t < base.finish)
				{
					int job = base.jobid;
					Hangar[t].push_back(job);
				}
			}
		}
	}

	int viol = 0;
	vector<vector<int>> OOS; //out of service
	OOS.resize(tt);


	for (int t = 0; t < tt; ++t)
	{
		for (int job : correctives)
		{
			bool found = true;
			for each(Base base in BaseAs)
			{
				if (job == base.jobid)
				{
					found = false;
					if (t < base.starttime)
						OOS[t].push_back(job);
					break;
				}
			}

			if (found)
				OOS[t].push_back(job);
		}

		for (int job : preventives)
		{
			bool found = true;
			for each(Base base in BaseAs)
			{
				if (job == base.jobid)
				{
					found = false;
					if (t < base.starttime && t >= Ymax[job])
						OOS[t].push_back(job);
					break;
				}
			}

			if (found && t >= Ymax[job])
				OOS[t].push_back(job);
		}

		//obj calc
		int dif_t = SLA[t] - (nn - OOS[t].size() - Hangar[t].size());

		if (dif_t > 0)
		{
			//viol += dif_t;
			obj += dif_t * penminus;
		}

	}

}


bool Heuristic::RunHeuristic()
{
	if (mode == 0)
	{
		int totpj = 0;
		obj = 0;

		int* slack = new int[nn];
		vector<int> Exit;    //Exited jobs from maint hangar so far
		vector<vector<int>> J; //J_t
		Hangar.resize(tt);
		J.resize(tt);

		int nextper = 0;
		sort(preventives.begin(), preventives.end());


		//Form Jcons set
		for (int i : critics)
		{
			//Jcons.emplace_back(i, r[i], pj[i]);
			slack[i] = d[i];
			for (int t = 0; t < tt; t++)
				J[t].push_back(i);
		}

		int cand = 0;
		base = new int[m];
		for (int i = 0; i < m; i++)
			base[i] = 0;
		int(*jobtimes)[2] = new int[nn][2]; //multidim array in one line!

		vector<array<int, 3>> slackdesc; // id,	
		//vector<int> urgents;
		vector<array<int, 3>> assigncandid;
		vector<array<int, 3>> firsts;

		vector<int> tempinitials = initials;
		sort(tempinitials.begin(), tempinitials.end());

		for (int t = 0; t < tt; t++)
		{
			if (t > 0 && static_cast<int> (Hangar[t - 1].size()) != 0)
			{
				//sondan baþa doðru çýkacak araçlarý belirleme
				Hangar[t] = Hangar[t - 1];
				while (true)
				{
					int size = static_cast<int> (Hangar[t].size());
					if (size != 0 && t == jobtimes[Hangar[t][size - 1]][1])
					{
						Exit.push_back(Hangar[t][size - 1]); //insert last element to exit
						Hangar[t].pop_back();
						continue;
					}
					break;
				}

				if (static_cast<int> (Exit.size()) == critics.size())
					break;

				//remove jobs that are already at hangar
				for (int k = 0; k < static_cast<int> (Hangar[t].size()); k++)
					J[t].erase(remove(J[t].begin(), J[t].end(), Hangar[t][k]), J[t].end());
			}

			//Remove already departed ones
			for (int i = 0; i < static_cast<int> (Exit.size()); i++)
				J[t].erase(remove(J[t].begin(), J[t].end(), Exit[i]), J[t].end());

			vector<int> Jcand;

			Jcand = J[t];
			// Erase jobs from Jcons if it is not within neighborhood

			if (t > 0)
			{
				for (int j = 0; j < nn; j++)
					slack[j]--;
			}

			for (int i = 0; i<static_cast<int> (J[t].size()); i++)
			{
				if (slack[J[t][i]] <= neigh)
				{
					if (t + pj[J[t][i]] >= tt)
					{
						Jcand.erase(remove(Jcand.begin(), Jcand.end(), J[t][i]), Jcand.end());
						++nextper;
					}

					else if (t < r[J[t][i]])
						Jcand.erase(remove(Jcand.begin(), Jcand.end(), J[t][i]), Jcand.end());
					else
						continue;
				}

				Jcand.erase(remove(Jcand.begin(), Jcand.end(), J[t][i]), Jcand.end());
			}


			int numbertohangar = m - static_cast<int> (Hangar[t].size());

			if (Jcand.size() < numbertohangar)
				numbertohangar = Jcand.size();

			if (t == 0)
			{
				if (numbertohangar < initials.size())
					numbertohangar = initials.size();
			}

			if (numbertohangar == 0)
				continue;

			// only Jcons satisfy SLA
			/*if (Jcand.size() < numbertohangar)
			{
			numbertohangar = Jcand.size();

			if (t == 0)
			{
			if (numbertohangar < initials.size())
			numbertohangar = initials.size();
			}

			if (numbertohangar == 0)
			continue;
			}*/


			if (Exit.size() + nextper == critics.size())
				break;
			else
				nextper = 0;

			J[t].clear();

			//update number to ha/ngar wrt remaining jobs

			vector<array<int, 3>> urgentextend;

			for (int i = 0; i< Jcand.size(); i++)
			{
				int job = Jcand[i];
				//select urgent corrective or ymaxviolated preventive cars

				if (binary_search(preventives.begin(), preventives.end(), job))
				{
					if (t >= Ymax[job])
					{
						//urgents.push_back(job);
						array <int, 3> a = { job, slack[job], pj[job] };
						urgentextend.push_back(a);
						continue;
					}

				}

				else
				{
					//urgents.push_back(job);
					array <int, 3> a = { job, slack[job], pj[job] };
					urgentextend.push_back(a);
					continue;
				}

				//and implement rest of jobs which are not urgent
				array <int, 3> a = { Jcand[i], slack[Jcand[i]], pj[Jcand[i]] };
				slackdesc.push_back(a);
			}

			if (slackdesc.size() > 1)
				sort(slackdesc.begin(), slackdesc.end(), lessthankeyy()); //desc order - slack buyuk olanı servise at!

			sort(urgentextend.begin(), urgentextend.end(), ascpj());

			//combine slackdesc + urgents
			for (int j = 0; j < urgentextend.size(); ++j)
				slackdesc.push_back(urgentextend[j]);

			if (heur == 1 && slackdesc.size() >= m)
			{
				int lastitem = slackdesc.back()[0]; //first item
				int candidfin = t + pj[lastitem];

				if (Hangar[t].size() > 0)
				{
					int fatherjob = Hangar[t][Hangar[t].size() - 1];
					if (candidfin > jobtimes[fatherjob][0] + pj[fatherjob] && Hangar[t].size() >= m - 1)
					{
						slackdesc.clear();
						assigncandid.clear();
						continue;
					}

				}
			}

			//select last numbertohangar cars		
			for (int i = 0; i < numbertohangar; ++i)
			{
				if (t == 0) //for fist initials
				{
					if (slackdesc.size() > 0)
					{
						int job = slackdesc.back()[0];
						if (binary_search(tempinitials.begin(), tempinitials.end(), job))
							slackdesc.pop_back();
						else
						{
							assigncandid.push_back(slackdesc.back());
							slackdesc.pop_back();
						}
					}

				}

				else //regular
				{
					assigncandid.push_back(slackdesc.back());
					slackdesc.pop_back();
				}
			}

			////10.3.17
			if (t == 0)
			{
				for (int job : initials) //bazen çalışıyor, bazen çalışmıyor. Bunları atamak için özel bir kod yazmalıyız!!
				{
					array <int, 3> a = { job, slack[job], pj[job] };
					firsts.push_back(a);
				}

				//numbertohangar = initials.size();
				bool infeas;
				if (initials.size() > 0)
				{
					infeas = AssigntoHangar2(firsts, numbertohangar, totpj, Hangar, t, m, Jcand, obj, n, SLA, Ymax, pj, base, jobtimes, BaseAs, ParChildAs, d); //overloaded func
					initials.clear();
				}

				if (Hangar[t].size() < m)
					infeas = AssigntoHangar2(assigncandid, numbertohangar, totpj, Hangar, t, m, Jcand, obj, n, SLA, Ymax, pj, base, jobtimes, BaseAs, ParChildAs, d); //overloaded func
			}

			else
				bool infeas = AssigntoHangar2(assigncandid, numbertohangar, totpj, Hangar, t, m, Jcand, obj, n, SLA, Ymax, pj, base, jobtimes, BaseAs, ParChildAs, d); //overloaded func

			slackdesc.clear();
			assigncandid.clear();
		}//for sonu

		vector<int> tempcritics = critics;
		//Base'e parent ve child id yerleþtirme
		for (int i = 0; i < BaseAs.size(); i++)
		{
			int job = BaseAs[i].jobid;
			int tur;
			if (binary_search(preventives.begin(), preventives.end(), job))
				tur = 0;
			else
				tur = 1;
			int t = BaseAs[i].starttime;
			obj += Objectard(job, t, d[job], Ymax[job], tur);
			tempcritics.erase(remove(tempcritics.begin(), tempcritics.end(), job), tempcritics.end());
			BaseAs[i].finish = jobtimes[BaseAs[i].jobid][1];
			BaseAs[i].length = BaseAs[i].finish - BaseAs[i].starttime;
		}

		before = obj;
		for (int job : tempcritics)
		{
			int tur;
			int t = tt;
			//tardearl[job] = EarlTardCalc(job, t, d[job]);
			if (binary_search(preventives.begin(), preventives.end(), job))
				tur = 0;
			else
				tur = 1;

			obj += Objectard(job, t, d[job], Ymax[job], tur);
		}

		CalculateObj(obj, Hangar);

		std::cout << obj << endl;
		delete[] jobtimes;
		delete[] slack;
	}

	return true;
}
