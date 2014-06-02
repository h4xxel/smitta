#ifndef __SMITTA_H_
#define __SMITTA_H_

#include <stdbool.h>

typedef enum CellState CellState;
enum CellState {
	CELL_STATE_IMMUNE = -2,
	CELL_STATE_DEAD = -1,
	CELL_STATE_NORMAL = 0,
	/*Freshly infected cells cannot infect others the same day*/
	CELL_STATE_INFECTED,
};

typedef struct Cell Cell;
struct Cell {
	CellState state;
	int cure_day;
};

typedef struct Simulation Simulation;
struct Simulation {
	Cell *matrix;
	int matrix_size;
	
	unsigned int random_seed;
	
	struct {
		int days_max;
		int days_min;
		
		int infection_probability;
		int death_probability;
	} infection;
	
	bool interactive;
	bool quiet;
	int delay;
	
	struct {
		struct {
			int infected;
			int dead;
			int cured;
		} last_step;
		
		struct {
			int infected;
			int dead;
			int immune;
		} total;
		
		int days_lapsed;
	} statistic;
};

int simulation_genereate_cure_day(Simulation *sim);
void simulation_visualize_stdout(Simulation *sim);
bool simulation_infect(Simulation *sim, int x, int y);
void simulation_step(Simulation *sim);
void simulation_run(Simulation *sim, void (*visualize)(Simulation *));
#endif
