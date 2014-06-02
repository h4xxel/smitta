#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

int usleep(int usec);

#include "smitta.h"

struct InitialCellList {
	int x;
	int y;
	CellState state;
	struct InitialCellList *next;
};

static void usage(void) {
	fprintf(stderr,
		"Arguments:\n"
		"	-h		help\n"
		"\n"
		"	-s N		matrix size (NxN)\n"
		"	-p N		infection spread probability (0-100)\n"
		"	-m N		mortality rate (0-100)\n"
		"	-u N		number of days infected (upper limit)\n"
		"	-l N		number of days infected (lower limit)\n"
		"	-a X Y		add infected cell at (X, Y)\n"
		"\n"
		"	-i		interactive simulation mode\n"
		"	-q		quiet simulation mode\n"
		"	-d N		simulation step delay (µs)\n"
		"	-r N		random seed\n"
		"\n"
	);
}

static int print_error(int error) {
	switch(error) {
			case -1:
				fprintf(stderr, "Error: invalid argument, use -h for help\n");
				return -1;
			
			case -2:
				fprintf(stderr, "Error: out of memory\n");
				return -1;
			
			case -3:
				return 0;
			
			default:
				return -1;
		}
}

static int parse_arg(int argc, char **argv, Simulation *sim, struct InitialCellList **initial_cell_list) {
	/*
		Parse command line arguments
		sets up the Simulation structure and a
		linked list of all the initially infected cells
		Returns < 0 on error
	*/
	
	int i;
	struct InitialCellList *tmp;
	
	if(!sim)
		return -1;
	
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-')
			return -1;
			
		switch(argv[i][1]) {
			case 'h':
				usage();
				return -3;
			
			case 'i':
				sim->interactive = true;
				break;
			
			case 'q':
				sim->quiet = true;
				break;
			
			case 'd':
				if(argc <= i + 1)
					return -1;
				
				sim->delay = atoi(argv[++i]);
				break;
			
			case 'r':
				if(argc <= i + 1)
					return -1;
				
				sim->random_seed = atoi(argv[++i]);
				break;
			
			case 's':
				if(argc <= i + 1)
					return -1;
				
				free(sim->matrix);
				sim->matrix_size = atoi(argv[++i]);
				break;
			
			case 'p':
				if(argc <= i + 1)
					return -1;
				
				sim->infection.infection_probability = atoi(argv[++i]);
				break;
			
			case 'm':
				if(argc <= i + 1)
					return -1;
				
				sim->infection.death_probability = atoi(argv[++i]);
				break;
			
			case 'u':
				if(argc <= i + 1)
					return -1;
				
				sim->infection.days_max = atoi(argv[++i]);
				break;
			
			case 'l':
				if(argc <= i + 1)
					return -1;
				
				sim->infection.days_min = atoi(argv[++i]);
				break;
			
			case 'a':
				if(argc <= i + 2)
					return -1;
				
				if(!(tmp = malloc(sizeof(struct InitialCellList))))
					return -2;
				
				tmp->x = atoi(argv[++i]);
				tmp->y = atoi(argv[++i]);
				tmp->state = CELL_STATE_INFECTED;
				tmp->next = *initial_cell_list;
				*initial_cell_list = tmp;
				break;
			
			default:
				return -1;
		}
	}
	
	return 0;
}

static int init_matrix(Simulation *sim, struct InitialCellList *initial_cell_list) {
	/*
		Allocate simulaion matrix and initialize infected cells
		from list
	*/
	if(!(sim->matrix = malloc(sizeof(Cell) * sim->matrix_size * sim->matrix_size)))
		return -2;
	memset(sim->matrix, 0, sizeof(Cell) * sim->matrix_size * sim->matrix_size);
	
	for(; initial_cell_list; initial_cell_list = initial_cell_list->next) {
		if(initial_cell_list->x >= sim->matrix_size || initial_cell_list->y >= sim->matrix_size || initial_cell_list->x <0 || initial_cell_list->y < 0) {
			fprintf(stderr, 
				"Warning: infected cell at (%i, %i) is outside bounds and cannot be added\n", 
				initial_cell_list->x, initial_cell_list->y
			);
			continue;
		}
		
		sim->matrix[(sim->matrix_size * initial_cell_list->y) + initial_cell_list->x].state = initial_cell_list->state;
		sim->matrix[(sim->matrix_size * initial_cell_list->y) + initial_cell_list->x].cure_day = simulation_genereate_cure_day(sim);
		
		sim->statistic.total.infected++;
	}
	return 0;
}

int simulation_genereate_cure_day(Simulation *sim) {
	/*
		Generate a date within the bounds a cell will be cured
	*/
	return sim->statistic.days_lapsed + sim->infection.days_min + rand()%(sim->infection.days_max - sim->infection.days_min);
}

void simulation_visualize_stdout(Simulation *sim) {
	/*
		Visualization plugin for console
	*/
	int x, y;
	printf("\nDay %i\n", sim->statistic.days_lapsed);
	for(y = 0; y < sim->matrix_size; y++) {
		for(x = 0; x < sim->matrix_size; x++) {
			switch(sim->matrix[(sim->matrix_size * y) + x].state) {
				case CELL_STATE_NORMAL:
					putchar('.');
					break;
				
				case CELL_STATE_IMMUNE:
					putchar('-');
					break;
				
				case CELL_STATE_DEAD:
					putchar('x');
					break;
				
				default:
					putchar('*');
					break;
			}
			putchar(' ');
		}
		putchar('\n');
	}
	printf("\nToday:	Infected: %i  Dead: %i  Cured: %i\n", sim->statistic.last_step.infected, sim->statistic.last_step.dead, sim->statistic.last_step.cured);
	printf("Total:	Infected: %i  Dead: %i	Immune: %i\n", sim->statistic.total.infected, sim->statistic.total.dead, sim->statistic.total.immune);
}

bool simulation_infect(Simulation *sim, int x, int y) {
	/*
		Try to infect a single cell, based on probability
		Returns true if successfully infected, otherwise false
	*/
	/*if(x < 0 || y < 0 || x >= sim->matrix_size || y >= sim->matrix_size)
		return;*/
	
	while(x < 0)
		x = sim->matrix_size + x;
	while(y < 0)
		y = sim->matrix_size + y;
	
	x = x % sim->matrix_size;
	y = y % sim->matrix_size;
	
	if(rand()%100 > sim->infection.infection_probability)
		return false;
	
	if(sim->matrix[(sim->matrix_size * y) + x].state == CELL_STATE_NORMAL) {
		sim->matrix[(sim->matrix_size * y) + x].state = sim->statistic.days_lapsed + CELL_STATE_INFECTED;
		sim->matrix[(sim->matrix_size * y) + x].cure_day = simulation_genereate_cure_day(sim);
		return true;
	}
	return false;
}

void simulation_step(Simulation *sim) {
	/*
		Run  a single step of the simulation
	*/
	int x, y, i, j;
	enum CellState c;
	for(y = 0; y < sim->matrix_size; y++) {
		for(x = 0; x < sim->matrix_size; x++) {
			c = sim->matrix[(sim->matrix_size * y) + x].state;
			if(c < CELL_STATE_INFECTED || c >= sim->statistic.days_lapsed + CELL_STATE_INFECTED)
				continue;
			
			for(i = -1; i <= 1; i++)
				for(j = -1; j <= 1; j++)
					if(i || j)
						sim->statistic.last_step.infected += simulation_infect(sim, x + j, y + i);
					
			if(rand()%100 < sim->infection.death_probability) {
				sim->matrix[(sim->matrix_size * y) + x].state = CELL_STATE_DEAD;
				sim->statistic.last_step.dead++;
				continue;
			}
			
			if(sim->matrix[(sim->matrix_size * y) + x].cure_day == sim->statistic.days_lapsed) {
				sim->matrix[(sim->matrix_size * y) + x].state = CELL_STATE_IMMUNE;
				sim->statistic.last_step.cured++;
			}
		}
	}
	
	sim->statistic.days_lapsed++;
}

void simulation_run(Simulation *sim, void (*visualize)(Simulation *)) {
	/*
		Continously run the simulation until no more infected cells exist
	*/
	
	if(!sim->quiet)
		visualize(sim);
	
	while(sim->statistic.total.infected) {
		if(sim->interactive) {
			if(getchar() == 'q')
				return;
		} else {
			if(sim->delay)
				usleep(sim->delay);
		}
		
		sim->statistic.last_step.infected = 0;
		sim->statistic.last_step.dead = 0;
		sim->statistic.last_step.cured = 0;
		
		simulation_step(sim);
		
		sim->statistic.total.infected += sim->statistic.last_step.infected - sim->statistic.last_step.cured - sim->statistic.last_step.dead;
		sim->statistic.total.dead += sim->statistic.last_step.dead;
		sim->statistic.total.immune += sim->statistic.last_step.cured;
		
		if(!sim->quiet)
			visualize(sim);
	}
}

int main(int argc, char **argv) {
	int err;
	struct InitialCellList *initial_cell_list = NULL;
	
	/*Set up default initial conditions*/
	Simulation sim = {
		.matrix = NULL,
		.matrix_size = 10,
		
		.infection = {
			.days_max = 4,
			.days_min = 2,
			
			.infection_probability = 10,
			.death_probability = 3,
		},
		
		.interactive = false,
		.quiet = false,
		.delay = 0,
		
		.statistic = {
			.last_step = {
				.infected = 0,
				.dead = 0,
				.cured = 0,
			},
			
			.total = {
				.infected = 0,
				.dead = 0,
			},
			
			.days_lapsed = 1,
		}
	};
	sim.random_seed = (unsigned int) time(NULL);
	
	if((err = parse_arg(argc, argv, &sim, &initial_cell_list)) < 0)
		return print_error(err);
	
	if((err = init_matrix(&sim, initial_cell_list)) < 0)
		return print_error(err);
	
	srand(sim.random_seed);
	simulation_run(&sim, simulation_visualize_stdout);
	
	if(!sim.quiet)
		printf("\n\n");
	
	printf(
		"Simulation with random seed %u\n"
		"%i days\n"
		"Total infections: %i (%i%%)\n"
		"Total deaths: %i (%i%%)\n",
		sim.random_seed,
		sim.statistic.days_lapsed,
		sim.statistic.total.immune + sim.statistic.total.dead,
		100*(sim.statistic.total.immune + sim.statistic.total.dead)/(sim.matrix_size*sim.matrix_size),
		sim.statistic.total.dead,
		100*(sim.statistic.total.dead)/(sim.matrix_size*sim.matrix_size)
	);
	
	return 0;
}
