// MPI3.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include<iostream>
#include<random>
#include<ctime>
#include<fstream>
using namespace std;
#define MASTER 0               /* taskid of first task */
#define FROM_MASTER 1          /* setting a message type */
#define FROM_WORKER 2          /* setting a message type */
#define MAX_SIZE 100
void InitAnt(int& pos, int& count, int (&path)[MAX_SIZE], bool (&visited)[MAX_SIZE], int n) {
	pos = rand() % n;
	count = 1;
	path[0] = pos;
	for (int i = 0; i < n; i++) {
		visited[i] = false;
	}
	visited[pos] = true;
}
void MoveAnt(int& pos, int& count, int (&path)[MAX_SIZE], bool (&visited)[MAX_SIZE], int newPos) {
	pos = newPos;
	count++;
	path[count - 1] = newPos;
	visited[newPos] = true;
}
int main(int argc, char *argv[])
{
	int n, min, max, rank, size, source, dest, mtype, antPos, bestLenght, bestLocalLenght, ants, nodesWalked;
	int matrix[MAX_SIZE][MAX_SIZE], bestPath[MAX_SIZE], antPath[MAX_SIZE], bestLocalPath[MAX_SIZE];
	bool antVisited[MAX_SIZE];
	double pheromones[MAX_SIZE][MAX_SIZE], pheromonesChange[MAX_SIZE][MAX_SIZE], localPheromonesChange[MAX_SIZE][MAX_SIZE];
	const double TAU0 = 10, ALPHA = 1, BETA = 1, RHO = 0.5, Q = 200;
	const int ANT_COUNT = 50, ITERATIONS = 50;
	MPI_Status status;
	srand(time(0));
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (rank == 0) {
		cout << "Enter n, min distance and max distance: ";
		cin >> n >> min >> max;
		ifstream fin;
		fin.open("D:\\graph.txt");
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				fin >> matrix[i][j];
				cout << matrix[i][j] << " ";
			}
			cout << endl;
		}
		mtype = FROM_MASTER;
		for (dest = 1; dest < size; dest++) {
			MPI_Send(&n, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
		}
		double currTime = MPI_Wtime();
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				pheromones[i][j] = TAU0;
			}
		}
		bestLenght = -1;
		int antsPerRank = ANT_COUNT / (size - 1);
		int extraAnts = ANT_COUNT % (size - 1);
		double* pheromoneChange = new double[n*n];
		for (int i = 0; i < ITERATIONS; i++) {
			for (int j = 0; j < n*n; j++) {
				pheromoneChange[j] = 0;
			}
			for (dest = 1; dest < size; dest++) {
				ants = (dest <= extraAnts) ? antsPerRank + 1 : antsPerRank;
				MPI_Send(&ants, 1, MPI_INT, dest, FROM_MASTER, MPI_COMM_WORLD);
				MPI_Send(&matrix, MAX_SIZE*MAX_SIZE, MPI_INT, dest, FROM_MASTER, MPI_COMM_WORLD);
				MPI_Send(&pheromones, MAX_SIZE*MAX_SIZE, MPI_DOUBLE, dest, FROM_MASTER, MPI_COMM_WORLD);
			}
			for (int j = 1; j < size; j++) {
				MPI_Recv(&bestLocalPath, n, MPI_INT, j, FROM_WORKER, MPI_COMM_WORLD, &status);
				MPI_Recv(&bestLocalLenght, 1, MPI_INT, j, FROM_WORKER, MPI_COMM_WORLD, &status);
				MPI_Recv(&localPheromonesChange, MAX_SIZE*MAX_SIZE, MPI_DOUBLE, j, FROM_WORKER, MPI_COMM_WORLD, &status);
				if (bestLenght == -1 || bestLocalLenght < bestLenght) {
					for (int k = 0; k < n; k++) {
						bestPath[k] = bestLocalPath[k];
					}
					bestLenght = bestLocalLenght;
				}
				for (int k = 0; k < n; k++) {
					for (int t = 0; t < n; t++) {
						pheromonesChange[k][t] += localPheromonesChange[k][t];
					}
				}
			}
			for (int j = 0; j < n; j++) {
				for (int k = j; k < n; k++) {
					pheromones[j][k] *= (1 - RHO);
					if (pheromones[j][k] < TAU0)
						pheromones[j][k] = TAU0;
					pheromones[k][j] = pheromones[j][k];
				}
			}
			for (int j = 0; j < n; j++) {
				for (int k = 0; k < n; k++) {
					pheromones[j][k] += pheromoneChange[j];
				}
			}
		}
		currTime = MPI_Wtime() - currTime;
		cout << bestLenght << ":\\ ";
		for (int i = 0; i < n; i++) {
			cout << bestPath[i] << " -> ";
		}
		cout << currTime << endl;
	}
	if (rank > 0) {
		MPI_Recv(&n, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
		std::uniform_real_distribution<double> unif(0, 1);
		std::default_random_engine re;
		for (int i = 0; i < ITERATIONS; i++) {
			MPI_Recv(&ants, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
			MPI_Recv(&matrix, MAX_SIZE*MAX_SIZE, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
			MPI_Recv(&pheromones, MAX_SIZE*MAX_SIZE, MPI_DOUBLE, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
			int bestLenght = -1;
			for (int j = 0; j < n; j++) {
				for (int k = 0; k < n; k++) {
					pheromonesChange[j][k] = 0;
				}
			}
			for (int j = 0; j < ants; j++) {
				InitAnt(antPos, nodesWalked, antPath, antVisited, n); 
				int lenght = 0;
				while (nodesWalked < n) {
					double currentRnd = unif(re);
					double probSum = 0.0;
					double fullProb = 0.0;
					for (int k = 0; k < n; k++) {
						if (!antVisited[k])
							probSum += pow(pheromones[antPos][k], ALPHA)*pow(1.0 / matrix[antPos][k], BETA);
					}
					bool isFound = false;
					for (int k = 0; k < n && !isFound; k++) {
						if (!antVisited[k]) {
							fullProb += pow(pheromones[antPos][k], ALPHA)*
								pow(1.0 / matrix[antPos][k], BETA) / probSum;
							if (currentRnd < fullProb) {
								lenght += matrix[antPos][k];
								MoveAnt(antPos, nodesWalked, antPath, antVisited, k);
								isFound = true;
							}
						}
					}
				}
				lenght += matrix[antPos][antPath[0]];
				for (int k = 0; k < n; k++) {
					pheromonesChange[antPath[k]][antPath[(k + 1) % n]] += Q / lenght;
					pheromonesChange[antPath[(k + 1) % n]][antPath[k]] = pheromonesChange[antPath[k]][antPath[(k + 1) % n]];
				}
				if (bestLenght == -1 || lenght < bestLenght) {
					for (int k = 0; k < n; k++) {
						bestPath[k] = antPath[k];
					}
					bestLenght = lenght;
				}
			}
			MPI_Send(&bestPath, n, MPI_INT, 0, FROM_WORKER, MPI_COMM_WORLD);
			MPI_Send(&bestLenght, 1, MPI_INT, 0, FROM_WORKER, MPI_COMM_WORLD);
			MPI_Send(&pheromonesChange, MAX_SIZE*MAX_SIZE, MPI_DOUBLE, 0, FROM_WORKER, MPI_COMM_WORLD);
		}
	}
	MPI_Finalize();
	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
