#ifndef ANALYSE_DATA_H__
#define ANALYSE_DATA_H__

#include "milpData.h"
#include "RT.h"

void extractRunnableData();
void computeChainRt(const std::vector<Label> &s);

struct Label_listing {
	std::vector<unsigned int> labels_r;
	std::vector<unsigned int> labels_r_access;
	std::vector<unsigned int> labels_w;
	std::vector<unsigned int> labels_w_access;
};

struct Event_Chain_RT {
	unsigned int id;
	std::vector<unsigned int> cpu_id;
	std::vector<double> exec_time;
	std::vector<double> bestRt;
	std::vector<double> worstRt;
	std::vector<uint64_t> period;
	std::vector<unsigned int> prio;
	std::vector<std::string> run_names;
	std::vector<unsigned int> task_id;
	std::vector<Label_listing> labels;
};

extern std::vector<Event_Chain_RT> EventChainsRt;

void printRt(const std::vector<Event_Chain_RT> &e);


#endif // !ANALYSE_DATA_H__
