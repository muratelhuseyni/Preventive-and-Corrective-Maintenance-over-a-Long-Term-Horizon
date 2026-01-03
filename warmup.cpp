#include "Common.h"
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
	int eventtime;
	int leavetime;

	Job(int IDin, int rjIn, int YmaxIn, int djIn, int pjIn, int eventtimeIn, int leavetimeIn)
	{
		leavetime = leavetimeIn;
		eventtime = eventtimeIn;
		ID = IDin;
		rj = rjIn;
		Ymax = YmaxIn;
		dj = djIn;
		pj = pjIn;
	}
};

int main()
{
	try
	{
		typedef vector<vector<Job*>> JobInt;
		mt19937 rngmt;
		
		//sonra buradaki araç sayısını arttırırsın!
		int k = 0;
		int day = 7;
		const int tt = 24 * day; //7 gün	
		int alpha = 1;
		int postpone = -1; //maint length

		//old
		int pjob[2] = { 8, 8 };
		int tau[2] = { 2508, 10068}; //1 and 2 month
		//int kappa[2];
		int pcorjob;
		double perturb;
		double spareratio = -1;
		double corrat = 0;
		int v1 = 0;
		double pilength = 0;
		int n = 0;	

		double slascale[2] = { 1,1 };
		int ind = 0;

		//min
			//for (int kappa = 0; kappa < 1; ++kappa)       //process variab
			//	for (int allow = 1; allow < 2; ++allow)		//v1 - prev intvl variab
			//		for (int scale = 0; scale < 1; ++scale)  //n scale 
			//			for (int pi = 1; pi < 2; ++pi)		//main type
			//				for (int corlev = 0; corlev < 1; ++corlev)				// corlev
			//					for (int level = 0; level < 1; ++level)					// preventlevel
			//						for (int mtbf = 1; mtbf < 2; ++mtbf)					// cor frequency

		//////////max
			for (int kappa = 1; kappa < 2; ++kappa)       //process variab
				for (int allow = 0; allow < 1; ++allow)		//v1 - prev intvl variab
					for (int scale = 1; scale < 2; ++scale)  //n scale 
						for (int pi = 0; pi < 1; ++pi)		//main type
							for (int corlev = 1; corlev < 2; ++corlev)				// corlev
								for (int level = 1; level < 2; ++level)					// preventlevel
									for (int mtbf = 0; mtbf < 1; ++mtbf)
		
		//////ortanca
		//for (int kappa = 1; kappa < 2; ++kappa)       //process variab
		//	for (int allow = 0; allow < 1; ++allow)		//v1 - prev intvl variab
		//		for (int scale = 0; scale < 1; ++scale)  //n scale 
		//			for (int pi = 1; pi < 2; ++pi)		//main type
		//				for (int corlev = 0; corlev < 1; ++corlev)				// corlev
		//					for (int level = 0; level < 1; ++level)					// preventlevel
		//						for (int mtbf = 0; mtbf < 1; ++mtbf)

		////maxa yakın
		//for (int kappa = 0; kappa < 1; ++kappa)       //process variab
		//	for (int allow = 0; allow < 1; ++allow)		//v1 - prev intvl variab
		//		for (int scale = 1; scale < 2; ++scale)  //n scale 
		//			for (int pi = 1; pi < 2; ++pi)		//main type
		//				for (int corlev = 0; corlev < 1; ++corlev)				// corlev
		//					for (int level = 0; level < 1; ++level)					// preventlevel
		//						for (int mtbf = 0; mtbf < 1; ++mtbf)
			{	

				int initseed = std::time(NULL);

				initseed = 1512143656;

				rngmt.seed(initseed);

				//number = to_string(slascale);
				int SLAdaily[24] = { 100, 100, 100, 100, 100, 100, 78, 78, 78, 78, 78, 78, 96, 96, 96, 96, 96, 96, 0, 0, 0, 0, 0, 0 }; //+1 dor heap corruption

				for (int i = 0; i < 24; ++i)
						SLAdaily[i] = ceil(SLAdaily[i] * slascale[scale]);		
					
				int SLAdailymax = *max_element(SLAdaily, SLAdaily + 24);
				n = ceil(SLAdailymax*1.05); // to be tight 2: dist to slaistmax

			/*for (int t = 0; t < 24; ++t)
			cout << SLAist[t] << endl;*/
		
				string ktype;
				string SLAtype;
				
				JobInt Jint;
				JobInt Jcor;

				switch (pi)
				{
					case 0: //vi=10%, bunu sonra yazalım!
						//k = 3 * 24;
						ktype = "month_";
						postpone = 25*24;
						break;
					case 1:
						//k = 6 * 24;
						postpone = 30*24;
						//pjob = 8; //1.5 aylık da yapabiliriz														
						ktype = "2month_";
						break;
				}

				//allow duruyor!
				switch (allow)
				{
					case 0:
						v1 = ceil(0.10*postpone);
						break;

					case 1:
						v1 = ceil(0.20*postpone);
						break;
				}

				switch (corlev)
				{
					case 0:
						pcorjob = 10;
						break;

					case 1:
						pcorjob = 10;
						break;
				}

				ofstream myfile( to_string(ind) + "_" +  "kap_" + to_string(kappa) + "allow_" 
					+ to_string(allow) + "scale_" + to_string(scale) + "pi_" + to_string(pi)+ "corlev_" + to_string(corlev) + "level_" + to_string(level) 
					+ "mtbf_" + to_string(mtbf) + ".txt");

					int* r = new int[n+1];
					//time determination
					// 3.42 years = 30,000 t pers
					double year = 1;
					double weeks = (double)year*52;
					int totaltt = tt*weeks;
			
					int* count = new int[totaltt+1]; // +1 to avoid heap corruption error

					for (int i = 0; i < totaltt; ++i)
						count[i] = 0;

					double cumulavg;
					//Preventive Job Interval generation 
					for (int j = 0; j < n; j++)
					{
						int Ymax = 0;
						int Ymin = 0;
						/*int Yminprev = 0;*/
						int i = 0;
						vector<Job*> jobjint;
					
						//uniform_int_distribution<int> runif(0, postpone);

						uniform_int_distribution<int> runif(0, 168*4);

						//r[j] = rand() % (postpone); //onceden olmuş, ilk gelişi max postpone kadar gelecek şekilde yap.
						r[j] = runif(rngmt);

						//r[j] = postpone - k + rand() % (2 * k + 1); //ilk beta - pi_min-pi_max arasına düşer
						while (true)
						{
							if (i == 0)
								Ymin = r[j];
							else
							{
								uniform_int_distribution<int> v1unif(0, v1);
								int vchange = runif(rngmt);
								Ymin += postpone + vchange;
							}
								
							Ymax = Ymin + 2 * v1; //2=ymaxcoef_1
							if (Ymax > totaltt) // to avoid over interval generation
								break;
							int d = ceil(Ymin + 1.5*k);    //dcoef = 1.5

							uniform_int_distribution<int> punif(0, 2);
							int pchange = punif(rngmt);
							int p = pjob[level] + pchange;


							uniform_int_distribution<int> enterrangeunif(0, Ymax - Ymin);
							int enterrange = enterrangeunif(rngmt);
							int renter = Ymin + enterrange;

							int rleave = renter + p;	
							Job* enterjob = new Job(j, Ymin, Ymax, d, p, renter, rleave);
							jobjint.push_back(enterjob);
							++i;
						}

						Jint.push_back(jobjint);
						jobjint.clear();
					}

					////generate corrective jobs - Jcor
					for (int j = 0; j < n; j++)
					{
						int Ymin = 0;
						int Yminprev = 0;
						vector<Job*> jobjint;
						while (true)
						{
							//int p = rand() % 4 + 9; //9 - 12 sa + perturb later
							//int p = 5 + rand() % (int)ceil(5 / 2 + 1);

							uniform_int_distribution<int> postunif(0, 2);
							int pchange = r[j] = postunif(rngmt);
							int p = pcorjob + pchange;					

							while (true)
							{
								//double u = static_cast<double>(rand()) / RAND_MAX; //generate unirandom number	 //mt ile generate et!						

								uniform_real_distribution<double> unirandist(0.0, 1.0);
								double u = unirandist(rngmt);

								int intar = ceil(-tau[mtbf] * log(1 - u));
								if (Yminprev + p < Ymin + intar)
								{
									Ymin += intar;
									break;
								}
							}

							if (Ymin >= totaltt)
								break;
									
							int Ymax = Ymin + 24 * alpha;

							//int renter = Ymin + rand() % (Ymax - Ymin + 1);

							uniform_int_distribution<int> enterrangeunif(0, Ymax - Ymin);
							int enterrange = enterrangeunif(rngmt);
							int renter = Ymin + enterrange;
							
							int rleave = renter + p;
							if (renter > totaltt)
								break;
							else if (rleave > totaltt)
								rleave = totaltt;

							/*for (int i = Ymin; i <= rleave; ++i)
								count[i]++;*/

							int d = Ymin;
							//int p = 9;
							/*if (i == ceil(z) - 1 && Ymin > maxY)
							maxY = Ymin;*/
							Job* enterjob = new Job(j, Ymin, Ymax, d, p, renter, rleave);
							jobjint.push_back(enterjob);

							//jobjint.emplace_back(j, Ymin, Ymax, d, p, renter, rleave); //Job(int IDin, int rjIn, int YmaxIn, int djIn, int pjIn)
							Yminprev = Ymin;
						}

						Jcor.push_back(jobjint);
						jobjint.clear();
					}

					//detect dependancy between groups
					for (int t = 0; t < totaltt; ++t)
					for (int k = 0; k < Jint.size(); ++k) //each tram
						for (int l = 0; l < Jint[k].size(); ++l) //each job in tram k
						{
							Job* prevjob = Jint[k][l];
							//bool found = false;
							if (t >= prevjob->eventtime &&  t <= prevjob->leavetime)
							{
								for each (Job* corjob in Jcor[k])
								{
									if (t >= corjob->eventtime && t <= corjob->leavetime) //hangisinin rjsi küçükse diğerini sil!
									{
										if (corjob->eventtime < prevjob->eventtime)
										{
											//delete prev
											Jint[k].erase(remove(Jint[k].begin(), Jint[k].end(), prevjob), Jint[k].end());
											break;
										}
										else
										{ 
											//delete cor
											Jcor[k].erase(remove(Jcor[k].begin(),Jcor[k].end(),corjob), Jcor[k].end());
											break;
										}
									}
								}

							}
						}
					
					// Count stays
					for (int k = 0; k < Jint.size(); ++k) //each tram
						for (int l = 0; l < Jint[k].size(); ++l)
						{
							Job* prevjob = Jint[k][l];
							for (int t = prevjob->eventtime; t <= prevjob->leavetime; ++t)
								count[t]++;
							delete prevjob; //assign also irrelevant values to Jint_{kl}
						}

					for (int k = 0; k < Jcor.size(); ++k) //each tram
						for (int l = 0; l < Jcor[k].size(); ++l)
						{
							Job* corjob = Jcor[k][l];
							for (int t = corjob->eventtime; t <= corjob->leavetime; ++t)
								count[t]++;
							delete corjob;
						}

					//calculate cum avg
					double avg = 0;
					for (int t = 0; t < totaltt; ++t)
					{
						avg += count[t];
						cumulavg = avg / (t + 1);
						myfile << t << "\t" << cumulavg << "\n";
					}								

					myfile.close();					
					Jint.clear();
					Jcor.clear();
					++ind;	
					delete[] count;
					delete[] r;
			}
		}

	catch (IloException& exception)
	{
		cout << exception.getMessage() << endl;
	}

	return 0;
}

