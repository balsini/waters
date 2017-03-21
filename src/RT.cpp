#include "RT.h"

#include <cmath>
#include <iostream>
#include <cassert>

using namespace std;

int64_t min_slack(const std::vector<Label> &s)
{
  int64_t min_slack = INT64_MAX;
  int64_t min_slack_core;

  try {
    for (unsigned int i=0; i<4; ++i) {
      min_slack_core = ADRT(CPU[i]);
      if (min_slack_core < min_slack)
        min_slack = min_slack_core;
    }
  } catch (string e) {
    cerr << e << endl;
    exit(-1);
  }

  return min_slack;
}

static inline uint64_t Ii(const Task &k, uint64_t t)
{
  uint64_t result = 0;
  uint64_t ceiling;
  uint64_t C_j;
  double T_j;

  for (Task const &tj : CPU[k.cpu_id]) {
    if (tj.prio > k.prio) {
      T_j = tj.period;
      C_j = tj.wcet;

      ceiling = ceil(t / T_j);
      result += ceiling * C_j;
    }
  }

  return result;
}

uint64_t Ri1(const Task &k)
{
  uint64_t R, R_old;

  R = k.wcet;
  do {
    R_old = R;
    R = k.wcet + Ii(k, R_old);
  } while (R != R_old);

  return R;
}

double Utilization(const vector<Task> &CPU)
{
  double r = 0.0;

  for (Task const &t : CPU)
    r += t.wcet / t.period;

  return r;
}

int64_t ADRT(vector<Task> &tasks)
{
  double utilization = Utilization(tasks);
  int64_t min_slack = INT64_MAX;

  //cout << "Utilization on CPU: " << utilization << endl;

  if (utilization >= 1)
    throw(std::string("utilization >= 1. Interference cannot converge, EXITING."));

  for (unsigned int i=0; i<tasks.size(); ++i) {
    uint64_t r_i1, r_ij, r_ij_1, r_ij_old;
    uint64_t Rij;

    r_i1 = Ri1(tasks[i]);

    if (r_i1 <= tasks[i].period) {
      Rij = r_i1;
    } else {
      cerr << "Not sure about the correctness" << endl;
      unsigned int j=1;

      r_ij_1 = r_i1;
      do {
        j++;
        r_ij = r_ij_1 + tasks[i].wcet;

        do {
          r_ij_old = r_ij;
          r_ij = j * tasks[i].wcet + Ii(tasks[i], r_ij);
        } while (r_ij != r_ij_old);

        Rij = r_ij - (j - 1) * tasks[i].period;
      } while (r_ij <= j * tasks[i].period);
    }
    tasks[i].response_time = Rij;

    int64_t slack = tasks[i].deadline - tasks[i].response_time;

    if (min_slack > slack)
      min_slack = slack;
  }

  return min_slack;
}
