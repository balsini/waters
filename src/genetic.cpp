#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <chrono>
#include <algorithm>
#include <ctime>

#include "optimization.h"
#include "genetic.h"
#include "RT.h"
#include "milpData.h"

using namespace std;

typedef std::vector<std::pair<Solution, double> > GeneticPopulation;

std::vector<RAM> ramg[pop];
std::random_device rd;

void sortPopulation(GeneticPopulation &population)
{
  std::sort(population.begin(), population.end(),
            [&](const std::pair<Solution, double> &a, const std::pair<Solution, double> &b) {
    return a.second < b.second;
  });
}

double evaluatePopulation(GeneticPopulation &population)
{
  double mean_fitness = 0;

  for (unsigned int p = 0; p < population.size(); ++p) {
    population[p].second = computeResponseTime(population[p].first);
    mean_fitness += population[p].second;
  }

  sortPopulation(population);

  return mean_fitness / pop;
}

Solution ComputeNewSolutionLight(const Solution &s, unsigned int maximum)
{
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<int> dist_label(0, s.size()-1);
  std::uniform_int_distribution<int> dist_ram(0, 4);
  std::uniform_int_distribution<int> dist_noise(1, maximum);
  Solution newSol(s);
  unsigned int noise = dist_noise(generator);

  for (unsigned int i=0; i<noise; ++i)
    newSol[dist_label(generator)].ram = static_cast<RAM_LOC>(1 << dist_ram(generator));

  return newSol;
}

Solution ComputeNewSolutionRAM2RAM(const Solution &s, unsigned int maximum)
{
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<int> dist_ram(0, 4);
  std::uniform_int_distribution<int> dist_noise(1, maximum);
  Solution newSol(s);
  unsigned int noise = dist_noise(generator);
  RAM_LOC dst;
  RAM_LOC src;
  unsigned int src_pos, dst_pos;

  src = static_cast<RAM_LOC>(1 << dist_ram(generator));
  do {
    dst = static_cast<RAM_LOC>(1 << dist_ram(generator));
  } while (dst == src);

  std::vector<Label *> src_label;

  for (Label &l : newSol) {
    if (l.ram == src)
      src_label.push_back(&l);
  }


  for (int i=0; (i < noise) && (i + 1 < src_label.size()); ++i) {
    std::uniform_int_distribution<int> dist_label_src(0, src_label.size()-1);

    src_pos = dist_label_src(generator);
    src_label[src_pos]->ram = dst;
    src_label.erase(src_label.begin() + src_pos);
  }

  return newSol;
}

Solution ComputeNewSolutionRAM2Others(const Solution &s, unsigned int maximum)
{
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<int> dist_ram(0, 4);
  std::uniform_int_distribution<int> dist_noise(1, maximum);
  Solution newSol(s);
  unsigned int noise = dist_noise(generator);
  RAM_LOC dst;
  RAM_LOC src;

  src = static_cast<RAM_LOC>(1 << dist_ram(generator));

  std::vector<Label *> src_label;

  for (Label &l : newSol) {
    if (l.ram == src)
      src_label.push_back(&l);
  }

  for (int i=0; (i < noise) && (i + 1 < src_label.size()); ++i) {
    std::uniform_int_distribution<int> dist_label_src(0, src_label.size()-1);
    int dst_pos = dist_label_src(generator);

    do {
      dst = static_cast<RAM_LOC>(1 << dist_ram(generator));
    } while (dst == src);

    src_label[dst_pos]->ram = dst;
    src_label.erase(src_label.begin() + dst_pos);
  }

  return newSol;
}

void performMutation(GeneticPopulation &s, unsigned int start, unsigned int end)
{
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<int> choose_mutation(0, 100);
  unsigned int mutation_kind;

  for (unsigned int p = start; p < end; ++p) {
    mutation_kind = choose_mutation(generator);
    if (mutation_kind < 5)
      s[p].first = ComputeNewSolutionRAM2RAM(s[p].first, 100);
    else if (mutation_kind < 15)
      s[p].first = ComputeNewSolutionRAM2Others(s[p].first, 200);
    else
      s[p].first = ComputeNewSolutionLight(s[p].first, 100);
  }
}

Solution crossover(const Solution &a, const Solution &b)
{
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<int> split_dist(0, a.size() - 1);
  Solution newSol;
  newSol.resize(0);
  const unsigned int splits_size = 200;
  std::vector<unsigned int> splits;

  for (unsigned int i = 0; i<splits_size; ++i)
    splits.push_back(split_dist(generator));
  std::sort(splits.begin(), splits.end());

  for (unsigned int i=0; i<splits.size(); ++i) {

    for (unsigned int j=newSol.size(); j<splits[i]; ++j) {
      newSol.push_back(a[j]);
    }

    ++i;
    if (i>=splits.size())
      break;

    for (unsigned int j=newSol.size(); j<splits[i]; ++j) {
      newSol.push_back(b[j]);
    }
  }

  while (newSol.size() < a.size())
    newSol.push_back(a[newSol.size()]);

  return newSol;
}

void performReproduction(GeneticPopulation &s, unsigned int start, unsigned int end, unsigned int sons)
{
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<int> parents_dist(start, end);

  for (unsigned int p = 0; p < sons; ++p) {

    int father = parents_dist(generator);
    int mother;
    do {
      mother = parents_dist(generator);
    } while (mother == father);

    s[pop - p - 1].first = crossover(s[mother].first, s[father].first);
  }
}

void performMitosis(GeneticPopulation &s, unsigned int start, unsigned int end, unsigned int offset)
{
  for (unsigned int p = start; p < end; ++p)
    s[offset + p] = s[start + p];
}

void InitializePopulation(GeneticPopulation &s)
{
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<int> distribution(0, 4);
  unsigned int position;
  RAM ram_temp;

  s.clear();

  for (unsigned int p=0; p<pop; ++p) {
    // Initialize rams
    for (int i = 0; i < 4; i++) {
      ram_temp.size = 131072;
      ram_temp.available = 131072;
      ramg[p].push_back(ram_temp);
    }
    ram_temp.size = 131072 * 2;
    ram_temp.available = 131072 * 2;
    ramg[p].push_back(ram_temp);

    s.push_back(std::make_pair(labels, 0.0));

    for (unsigned int i = 0; i<s[p].first.size(); ++i) {
      do {
        //do {
        position = distribution(generator);
        //	}	while (static_cast<RAM_LOC>(1 << position) == LRAM_1);
      } while (ramg[p].at(position).available - s[p].first.at(i).bitLen < 0);


      ramg[p].at(position).available -= s[p].first.at(i).bitLen;
      s[p].first.at(i).ram = static_cast<RAM_LOC>(1 << position);
    }
  }
}


std::pair<Solution, double> genetic()
{
  int termination = 1;
  uint64_t epoch = 0;

  const double TO_CLONE_F = 0.05;
  const double TO_CLONE_TO_F = 0.80;

  const double TO_REPRODUCE_F = 0.15;
  const double TO_REPRODUCE_CHILDREN_F = TO_REPRODUCE_F;

  const double TO_MUTATE_F = 1 - TO_CLONE_F - TO_REPRODUCE_CHILDREN_F;
  const double TO_MUTATE_FROM_F = TO_CLONE_F;


  const unsigned int TO_CLONE = TO_CLONE_F * pop;
  const unsigned int TO_CLONE_TO = TO_CLONE_TO_F * pop;

  const unsigned int TO_REPRODUCE = TO_REPRODUCE_F * pop;
  const unsigned int TO_REPRODUCE_CHILDREN = TO_REPRODUCE_CHILDREN_F * pop;

  const unsigned int TO_MUTATE = TO_MUTATE_F * pop;
  const unsigned int TO_MUTATE_FROM = TO_MUTATE_FROM_F * pop;


  time_t now;
  struct tm newyear;
  time(&now);  /* get current time; same as: now = time(NULL)  */
  newyear = *localtime(&now);
  newyear.tm_hour = 0; newyear.tm_min = 0; newyear.tm_sec = 0;
  newyear.tm_mon = 0;  newyear.tm_mday = 1;
  long int seconds = static_cast<long int>(difftime(now,mktime(&newyear)));

  string filename = string("result_") + std::to_string(seconds) + string(".csv");

  cout << "csv name: " << filename << endl << endl;

  GeneticPopulation population;
  Solution s_opt;
  double fit_mean, fit_opt;

  population.resize(pop);
  InitializePopulation(population);
  fit_mean = evaluatePopulation(population);

  fit_opt = population[0].second;
  s_opt = population[0].first;

  new_optimal_solution_found(fit_opt, s_opt, fit_mean, epoch);
  solution_to_csv(filename, s_opt, fit_opt, fit_mean, epoch);
  do {
    ++epoch;
    //selectParents();
    //performReproduction();
    performMitosis(population, 0, TO_CLONE, TO_CLONE_TO);
    performReproduction(population, 0, TO_REPRODUCE, TO_REPRODUCE_CHILDREN);
    performMutation(population, TO_MUTATE_FROM, TO_MUTATE_FROM + TO_MUTATE);
    //updatePopulation();

    fit_mean = evaluatePopulation(population);

    if (fit_opt > population[0].second) {
      fit_opt = population[0].second;
      s_opt = population[0].first;
      new_optimal_solution_found(fit_opt, s_opt, fit_mean, epoch);
      solution_to_csv(filename, s_opt, fit_opt, fit_mean, epoch);
    } else {
      cout << "." << endl;
    }
  } while (termination != 0);

  return std::make_pair(s_opt, fit_opt);
}

void genetic_run()
{
  cout << endl << "Performing genetic algorithm" << endl << endl;
  auto r = genetic();
  cout << "Done" << endl;
  cout << "Best solution found" << endl;
  printSolution(r.first);
  cout << "With value: " << r.second << endl;
}

