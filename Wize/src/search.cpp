#include "stdio.h"
#include "defs.h"
#include <iostream>
#include <algorithm>
#include "string.h"
#include "tinycthread.h"
#include <cstdint>


// For LMR
const int FullDepthMoves = 4;
const int ReductionLimit = 3;

int rootDepth;
thrd_t workerThreads[MAXTHREADS];




// Check if the time is up or interrupt from GUI
static void CheckUp(S_SEARCHINFO *info) {
	
	// check if more than Maxtime passed and we have to stop
	if ((info->timeset) && (GetTimeMs() > info->stoptime))
		info->stopped = TRUE;
}

static bool StopEarly(S_SEARCHINFO* info)
{
	// check if we used all the nodes/movetime we had or if we used more than our lowerbound of time
	if ((info->timeset) && (GetTimeMs() > info->optstoptime)) {
		info->stopped = TRUE;
		return true;
	}
	return false;
}


// Find the best score in the remaning moves, move ordering
static void PickNextMove(int moveNum, S_MOVELIST* list) {

	S_MOVE temp;
	int index = 0;
	int bestScore = -INF_BOUND;
	int bestNum = moveNum;

	// Find best score
	for (index = moveNum; index < list->count; ++index) {
		if (list->moves[index].score > bestScore) {
			bestScore = list->moves[index].score;
			bestNum = index;
		}
	}
	// Reorder movelist
	temp = list->moves[moveNum];
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}


// Search for a position repetition
static int IsRepetition(const S_BOARD* pos) {
	int index = 0;

	// Starting from last fiftymove update
	for (index = pos->hisPly - pos->fiftyMove; index < pos->hisPly - 1; ++index) {
		
		// Index outside of bounds
		ASSERT(index >= 0 && index < MAXGAMEMOVES);
		
		if (pos->posKey == pos->history[index].posKey) {
			return TRUE;
		}
	}

	return FALSE;
}


// Clear search information for a new search
static void ClearForSearch(S_BOARD* pos, S_SEARCHINFO* info, S_HASHTABLE *table) {
	
	// Define indices
	int index = 0;
	int index2 = 0;

	// Clear search hisotry
	for (index = 0; index < 13; ++index) {
		for (index2 = 0; index2 < BRD_SQ_NUM; ++index2) {
			pos->searchHistory[index][index2] = 0;
		}
	}

	// Clear killer moves
	for (index = 0; index < 2; ++index) {
		for (index2 = 0; index2 < MAXDEPTH; ++index2) {
			pos->searchKillers[index][index2] = 0;
		}
	}

	//table->overWrite = 0;
	table->hit = 0;
	table->cut = 0;
	++table->currentAge; // Incremement current hash table age
	pos->ply = 0; // Set search ply to 0

	// Reset search info
	info->stopped = 0;
	info->nodes = 0;
	info->fh = 0;
	info->fhf = 0;

}


// Prevent horizon effect with Quiescence search
static int Quiescence(int alpha, int beta, S_BOARD* pos, S_SEARCHINFO* info) {
	
	// Valid board
	ASSERT(CheckBoard(pos));
	if (info->stopped) {
		return 0;
	}


	++info->nodes; // Increment nodes

	// Repetition
	if (IsRepetition(pos) || pos->fiftyMove >= 100) {
		return CONTEMPT;
	}

	int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);

	// Max Depth
	if (pos->ply > MAXDEPTH - 1) {
		return InCheck ? 0 : EvalPosition(pos);
	}

	// Mate distance pruning
	alpha = std::max(alpha, -mate_value + pos->ply);
	beta = std::min(beta, mate_value - pos->ply - 1);
	if (alpha >= beta)
		return alpha;

	int Score = EvalPosition(pos);
	// Standing pat
	if (Score >= beta) {
		return beta;
	}
	if (Score > alpha) {
		alpha = Score;
	}

	// Generate captures
	S_MOVELIST list[1];
	GenerateAllCaptures(pos, list);

	// Variable definitions
	int MoveNum = 0;
	int Legal = 0;

	Score = -INF_BOUND;
	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		// Move ordering
		PickNextMove(MoveNum, list);

		// Not legal move
		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue;
		}

		++Legal;
		Score = -Quiescence(-beta, -alpha, pos, info); // Recursively call function
		TakeMove(pos);

		// Check status
		if (info->stopped == TRUE) {
			return 0;
		}

		// New alpha score
		if (Score > alpha) {
			if (Score >= beta) {
				if (Legal == 1) {
					++info->fhf; // Searched the best move first
				}
				++info->fh;

				return beta;
			}
			alpha = Score;
		}
	}
	return alpha;
}


// NegaMax Alpha Beta pruning search
static int AlphaBeta(int alpha, int beta, int depth, S_BOARD* pos, S_SEARCHINFO* info, S_HASHTABLE * table, int DoNull) {


	// Valid board
	ASSERT(CheckBoard(pos));
	ASSERT(beta > alpha);
	ASSERT(depth >= 0);
	// At terminal node


	int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);

	if (InCheck) depth = std::max(1, depth + 1);
	if (depth <= 0) {
		return Quiescence(alpha, beta, pos, info);
		// return EvalPosition(pos);
	}

	// Check time elapsed
	if (info->nodes & 2047) {
		CheckUp(info);

	}
	if (info->stopped) {
		return 0;
	}
	++info->nodes;

	// Check for draw or repetition
	if ((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) {
		return CONTEMPT;
	}

	if (pos->ply > MAXDEPTH - 1) {
		if (InCheck) {
			return EvalPosition(pos);

		}
		else {
			return 0;
		}
	}

	// Mate distance pruning
	alpha = std::max(alpha, -mate_value + pos->ply);
	beta = std::min(beta, mate_value - pos->ply - 1);
	if (alpha >= beta)
		return alpha;

	int Score = -INF_BOUND;
	int PvMove = NOMOVE;


	// Probe hash entry
	if (ProbeHashEntry(pos, table, &PvMove, &Score, alpha, beta, depth) == TRUE) {
		+table->cut;
		return Score;
	}

	// Null move
	if (DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 1) && depth >= 4) {
		MakeNullMove(pos);
		Score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, table, FALSE);
		TakeNullMove(pos);
		if (info->stopped == TRUE) {
			return 0;
		}

		if (Score >= beta){
			if (abs(Score) < ISMATE) {
				Score = beta;
			}
			++info->nullCut;
			return Score;
		}
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	// Variable definitions
	int MoveNum = 0;
	int Legal = 0;
	int OldAlpha = alpha;
	int BestMove = NOMOVE;

	int BestScore = -INF_BOUND;
	Score = -INF_BOUND;

	// Prioritize Principal variation move
	if (PvMove != NOMOVE) {
		for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
			if (list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
				//printf("Pv move found \n");
				break;
			}
		}
	}

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		// Move ordering
		PickNextMove(MoveNum, list);

		// Not legal move
		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue;
		}

		++Legal;


		// TRYING Late Move Reduction
		if (MoveNum == 0) { // First move, use full-window search
			Score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, table, TRUE);
		}
		else {
			if (MoveNum >= FullDepthMoves && depth >= ReductionLimit and not (InCheck or list->moves[MoveNum].move & MFLAGCAP)) {
				Score = -AlphaBeta(-beta, -alpha, depth - 2, pos, info, table, TRUE);
			}
			else Score = alpha + 1;// Hack to ensure that full-depth search is done.

			if (Score > alpha) {
				Score = -AlphaBeta(-(alpha + 1), -alpha, depth - 1, pos, info, table, TRUE); // Research with full window
				if (Score > alpha && Score < beta)
					Score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, table, TRUE);
			}
		}


		//Score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE); // Recursively call function
		TakeMove(pos);

		// Check status
		if (info->stopped == TRUE) {
			return 0;
		}

		// New best score
		if (Score > BestScore) {
			BestScore = Score;
			BestMove = list->moves[MoveNum].move;
			// New alpha score
			if (Score > alpha) {
				if (Score >= beta) {
					if (Legal == 1) {
						++info->fhf; // Searched the best move first
					}
					++info->fh;

					// Check for capture, adjust killers
					if (!(list->moves[MoveNum].move & MFLAGCAP)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
					}
					
					StoreHashEntry(pos, table, BestMove, beta, HFBETA, depth); // Store hash entry of position

					return beta;
				}
				alpha = Score;

				// Check for capture, adjust Hueristic
				if (!(list->moves[MoveNum].move & MFLAGCAP)) {
					pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth; // Prioritizes moves closer to root
				}
			}
		}
	}

	// Checkmate and stalemate
	if (Legal == 0) {
		if (InCheck) {
			return (- mate_value + pos->ply); // Checkmate
		}
		else {
			return CONTEMPT; // Stalemate
		}
	}

	// Store best move for new alpha value
	if (alpha != OldAlpha) {
		StoreHashEntry(pos, table, BestMove, BestScore, HFEXACT, depth); // Store hash entry of position
	}
	else {
		StoreHashEntry(pos, table, BestMove, alpha, HFALPHA, depth); // Store hash entry of position
	}

	return alpha;
}


int AspirationWindowSearch(int prev_eval, int depth, S_BOARD* pos, S_SEARCHINFO* info, S_HASHTABLE * table) {
	int Score = 0;

	//We set an expected window for the score at the next search depth, this window is not 100% accurate so we might need to try a bigger window and re-search the position
	int delta = 50;
	// define initial alpha beta bounds
	int alpha = -INF_BOUND;
	int beta = INF_BOUND;

	// only set up the windows is the search depth is bigger or equal than Aspiration_Depth to avoid using windows when the search isn't accurate enough
	if (depth >= 3) {
		alpha = std::max(-INF_BOUND, prev_eval - delta);
		beta = std::min(prev_eval + delta, INF_BOUND);
	}

	//Stay at current depth if we fail high/low because of the aspiration windows
	while (true) {
		Score = AlphaBeta(alpha, beta, depth, pos, info, table, TRUE);


		CheckUp(info);
		if (info->stopped) {
			break;
		}

		// stop calculating and return best move so far
		if (info->stopped) break;

		// we fell outside the window, so try again with a bigger window, if we still fail after we just search with a full window
		if ((Score <= alpha)) {
			beta = (alpha + beta) / 2;
			alpha = std::max(-INF_BOUND, Score - delta);
		}

		// we fell outside the window, so try again with a bigger window, if we still fail after we just search with a full window
		else if ((Score >= beta)) {
			beta = std::min(Score + delta, INF_BOUND);
		}
		else break;
		delta *= 1.44;
	}
	return Score;
}



int SearchPosition_Thread(void* data) {
	S_SEARCH_THREAD_DATA* searchData = (S_SEARCH_THREAD_DATA*)data;
	S_BOARD* pos = (S_BOARD *) malloc(sizeof(S_BOARD)); // Allocate memory for board

	memcpy(pos, searchData->originalPosition, sizeof(S_BOARD)); // Copying board
	SearchPosition(pos, searchData->info, searchData->ttable);
	free(pos); // Free memory

	return 0;
}

void IterativeDeepen(S_SEARCH_WORKER_DATA* workerData) {

    // Define variables
    workerData->bestMove = NOMOVE;
    int bestScore = -INF_BOUND;
    int currentDepth = 0;
    int pvMoves = 0;
    int pvNum = 0;

    // Iterative deepening
    for (currentDepth = 1; currentDepth <= workerData->info->depth; ++currentDepth) {

        rootDepth = currentDepth;
        // start aspiration window search
        bestScore = AspirationWindowSearch(bestScore, currentDepth, workerData->pos, workerData->info, workerData->ttable);

        if (workerData->threadNumber == 0) {
            if (workerData->info->stopped == TRUE) break;
            pvMoves = GetPvLine(currentDepth, workerData->pos, workerData->ttable); // Get PV line
            workerData->bestMove = workerData->pos->pvArray[0];

            long time = GetTimeMs() - workerData->info->starttime;
            uint64_t nps = workerData->info->nodes / (time + !time) * 1000; // Might be an issue

            if (bestScore > -mate_value && bestScore < -mate_score)
                std::cout << "info score mate " << std::dec << -(bestScore + mate_value) / 2 << " depth " << currentDepth << " nodes " << workerData->info->nodes <<
                          " nps " << nps << " time " << time << " pv ";

            else if (bestScore > mate_score && bestScore < mate_value)
                std::cout << "info score mate " << std::dec << (mate_value - bestScore) / 2 + 1 << " depth " << currentDepth << " nodes " << workerData->info->nodes <<
                          " nps " << nps << " time " << time << " pv ";

            else
                std::cout << "info score cp " << std::dec << bestScore << " depth " << currentDepth << " nodes " << workerData->info->nodes <<
                          " nps " << nps << " time " << time << " pv ";

            for (int count = 0; count < pvMoves; ++count) {
                // print PV move
                std::cout << PrMove(workerData->pos->pvArray[count]) << " ";
            }
            std::cout << std::endl;

            if (StopEarly(workerData->info)) {
                break;
            }
        }

    }
}

// Start a worker thread
int StartWorkerThread(void* data) {

	S_SEARCH_WORKER_DATA* workerData = (S_SEARCH_WORKER_DATA*)data;
	//printf("Thread:%d Starts\n", workerData->threadNumber);
	IterativeDeepen(workerData);
	//printf("Thread:%d Ends\n", workerData->threadNumber, workerData->depth);

	if (workerData->threadNumber == 0) {
		printf("bestmove %s\n", PrMove(workerData->bestMove));
	}
	free(workerData); // Free worker memory
	return 0;
}


// Initialize a worker thread
void SetupWorker(int threadNum, thrd_t* workerTh, S_BOARD* pos, S_SEARCHINFO* info, S_HASHTABLE* table) {

	S_SEARCH_WORKER_DATA* pWorkerData = (S_SEARCH_WORKER_DATA * ) malloc(sizeof(S_SEARCH_WORKER_DATA));
	pWorkerData->pos = (S_BOARD*) malloc(sizeof(S_BOARD));
	memcpy(pWorkerData->pos, pos, sizeof(S_BOARD));
	pWorkerData->info = info;
	pWorkerData->ttable = table;
	pWorkerData->threadNumber = threadNum;

	thrd_create(workerTh, &StartWorkerThread, (void*)pWorkerData); // Start worker thread
}



void CreateSearchWorkers(S_BOARD* pos, S_SEARCHINFO* info, S_HASHTABLE* table) {

	//printf("CreateSearchWorkers:%d\n", info->threadNum);

	for (int i = 0; i < info->threadNum; ++i) {
		SetupWorker(i, &workerThreads[i], pos, info, table);
	}
}

// Iterative deepening from depth = 1 to MAXDEPTH
void SearchPosition(S_BOARD* pos, S_SEARCHINFO* info, S_HASHTABLE *table) {

	// Define variables
	int bestMove = NOMOVE;

	ClearForSearch(pos, info, table); // Prepare for search

	if (EngineOptions->UseBook == TRUE) {
		bestMove = GetBookMove(pos);
		if (bestMove != NOMOVE) {
			std::cout << "info score cp " << 0 << " depth " << 0 << " nodes " << 0 <<
				" nps " << 0 << " time 0 \n";
			printf("bestmove %s\n", PrMove(bestMove));
		}
		else {
			EngineOptions->UseBook = FALSE;
		}
	}

	// Iterative deepening
	if (bestMove == NOMOVE) {
		EngineOptions->UseBook = FALSE;
		CreateSearchWorkers(pos, info, table);

	}

	// Join the worker threads
	for (int i = 0; i < info->threadNum; ++i) {
		thrd_join(workerThreads[i], NULL);
	}

}



