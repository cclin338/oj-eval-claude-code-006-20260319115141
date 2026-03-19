#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <algorithm>
#include <random>
#include <chrono>

extern int rows;         // The count of rows of the game map.
extern int columns;      // The count of columns of the game map.
extern int total_mines;  // The count of mines of the game map.

// You MUST NOT use any other external variables except for rows, columns and total_mines.

// Internal game state tracking
std::vector<std::vector<char>> known_map;  // Current known state: '?' = unknown, '0'-'8' = number, '@' = marked mine, 'X' = visited mine
std::vector<std::vector<bool>> is_safe;    // Cells known to be safe (not mines)
std::vector<std::vector<bool>> is_mine;    // Cells known to be mines
std::vector<std::vector<bool>> is_visited; // Cells that have been visited
std::vector<std::vector<bool>> is_marked;  // Cells that have been marked as mines

// Game statistics
int unknown_cells;
int marked_mines;
int visited_cells;

// Random number generator for random moves
std::mt19937 rng;

/**
 * @brief The definition of function Execute(int, int, bool)
 *
 * @details This function is designed to take a step when player the client's (or player's) role, and the implementation
 * of it has been finished by TA. (I hope my comments in code would be easy to understand T_T) If you do not understand
 * the contents, please ask TA for help immediately!!!
 *
 * @param r The row coordinate (0-based) of the block to be visited.
 * @param c The column coordinate (0-based) of the block to be visited.
 * @param type The type of operation to a certain block.
 * If type == 0, we'll execute VisitBlock(row, column).
 * If type == 1, we'll execute MarkMine(row, column).
 * If type == 2, we'll execute AutoExplore(row, column).
 * You should not call this function with other type values.
 */
void Execute(int r, int c, int type);

// Helper function implementations

/**
 * @brief Update knowledge base based on current map state
 */
void updateKnowledge() {
  // Simple constraint propagation
  // For each visited cell, check if we can deduce anything
  const int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  bool changed = true;
  while (changed) {
    changed = false;

    for (int i = 0; i < rows; i++) {
      for (int j = 0; j < columns; j++) {
        if (is_visited[i][j] && known_map[i][j] >= '0' && known_map[i][j] <= '8') {
          int number = known_map[i][j] - '0';

          // Count unknown and marked neighbors
          int unknown_count = 0;
          int marked_count = 0;
          std::vector<std::pair<int, int>> unknown_neighbors;

          for (int d = 0; d < 8; d++) {
            int ni = i + dx[d];
            int nj = j + dy[d];
            if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
              if (known_map[ni][nj] == '?') {
                unknown_count++;
                unknown_neighbors.push_back({ni, nj});
              } else if (known_map[ni][nj] == '@') {
                marked_count++;
              }
            }
          }

          // Rule 1: If number equals marked_count, all unknown neighbors are safe
          if (number == marked_count && unknown_count > 0) {
            for (auto [ni, nj] : unknown_neighbors) {
              if (!is_safe[ni][nj]) {
                is_safe[ni][nj] = true;
                is_mine[ni][nj] = false;
                changed = true;
              }
            }
          }

          // Rule 2: If number - marked_count == unknown_count, all unknown neighbors are mines
          if (number - marked_count == unknown_count && unknown_count > 0) {
            for (auto [ni, nj] : unknown_neighbors) {
              if (!is_mine[ni][nj]) {
                is_mine[ni][nj] = true;
                is_safe[ni][nj] = false;
                changed = true;
              }
            }
          }
        }
      }
    }
  }
}

/**
 * @brief Find a cell known to be safe
 * @return Coordinates of safe cell, or (-1, -1) if none found
 */
std::pair<int, int> findSafeCell() {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (is_safe[i][j] && !is_visited[i][j] && !is_marked[i][j] && known_map[i][j] == '?') {
        return {i, j};
      }
    }
  }
  return {-1, -1};
}

/**
 * @brief Find an obvious mine to mark
 * @return Coordinates of mine cell, or (-1, -1) if none found
 */
std::pair<int, int> findObviousMine() {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (is_mine[i][j] && !is_marked[i][j] && known_map[i][j] == '?') {
        return {i, j};
      }
    }
  }
  return {-1, -1};
}

/**
 * @brief Find a random unknown cell
 * @return Coordinates of random unknown cell
 */
std::pair<int, int> findRandomUnknown() {
  std::vector<std::pair<int, int>> unknown_cells_list;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (known_map[i][j] == '?' && !is_marked[i][j]) {
        unknown_cells_list.push_back({i, j});
      }
    }
  }

  if (unknown_cells_list.empty()) {
    return {-1, -1};
  }

  std::uniform_int_distribution<int> dist(0, unknown_cells_list.size() - 1);
  return unknown_cells_list[dist(rng)];
}

/**
 * @brief Calculate probability of being a mine for each unknown cell
 * @return Coordinates of cell with lowest mine probability
 */
std::pair<int, int> findBestGuess() {
  // Simple probability calculation:
  // For cells adjacent to numbers, use local information
  // For isolated cells, use global mine density

  int remaining_mines = total_mines - marked_mines;
  int remaining_unknown = unknown_cells;

  // If no remaining mines or unknown cells, return invalid
  if (remaining_unknown == 0 || remaining_mines <= 0) {
    return {-1, -1};
  }

  float global_probability = (float)remaining_mines / remaining_unknown;

  std::pair<int, int> best_cell = {-1, -1};
  float best_probability = 2.0f; // Start with probability > 1

  const int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (known_map[i][j] == '?' && !is_marked[i][j]) {
        float cell_probability = global_probability;

        // Check if cell is adjacent to any visited cells
        bool adjacent_to_number = false;
        float local_prob_sum = 0.0f;
        int local_constraints = 0;

        for (int d = 0; d < 8; d++) {
          int ni = i + dx[d];
          int nj = j + dy[d];
          if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
            if (known_map[ni][nj] >= '1' && known_map[ni][nj] <= '8') {
              adjacent_to_number = true;

              int number = known_map[ni][nj] - '0';

              // Count unknown and marked neighbors of this number cell
              int unknown_around = 0;
              int marked_around = 0;

              for (int dd = 0; dd < 8; dd++) {
                int nni = ni + dx[dd];
                int nnj = nj + dy[dd];
                if (nni >= 0 && nni < rows && nnj >= 0 && nnj < columns) {
                  if (known_map[nni][nnj] == '?') {
                    unknown_around++;
                  } else if (known_map[nni][nnj] == '@') {
                    marked_around++;
                  }
                }
              }

              // Calculate local probability for this constraint
              int remaining_mines_for_cell = number - marked_around;
              if (remaining_mines_for_cell > 0 && unknown_around > 0) {
                local_prob_sum += (float)remaining_mines_for_cell / unknown_around;
                local_constraints++;
              }
            }
          }
        }

        // If cell is adjacent to numbers, use average of local probabilities
        if (adjacent_to_number && local_constraints > 0) {
          cell_probability = local_prob_sum / local_constraints;
        }

        // Prefer cells with lower probability
        if (cell_probability < best_probability) {
          best_probability = cell_probability;
          best_cell = {i, j};
        }
      }
    }
  }

  if (best_cell.first != -1) {
    return best_cell;
  }

  // Fallback to random
  return findRandomUnknown();
}

/**
 * @brief Check if a cell should be auto-explored
 * @param r Row coordinate
 * @param c Column coordinate
 * @return True if auto-explore should be performed
 */
bool shouldAutoExplore(int r, int c) {
  if (!is_visited[r][c] || known_map[r][c] < '0' || known_map[r][c] > '8') {
    return false;
  }

  int number = known_map[r][c] - '0';
  if (number == 0) return false;

  const int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  int marked_count = 0;
  int unknown_count = 0;

  for (int d = 0; d < 8; d++) {
    int nr = r + dx[d];
    int nc = c + dy[d];
    if (nr >= 0 && nr < rows && nc >= 0 && nc < columns) {
      if (known_map[nr][nc] == '@') {
        marked_count++;
      } else if (known_map[nr][nc] == '?') {
        unknown_count++;
      }
    }
  }

  // Auto-explore if number equals marked_count and there are unknown neighbors
  return (number == marked_count && unknown_count > 0);
}

/**
 * @brief The definition of function InitGame()
 *
 * @details This function is designed to initialize the game. It should be called at the beginning of the game, which
 * will read the scale of the game map and the first step taken by the server (see README).
 */
void InitGame() {
  // Initialize all your global variables!
  known_map.resize(rows, std::vector<char>(columns, '?'));
  is_safe.resize(rows, std::vector<bool>(columns, false));
  is_mine.resize(rows, std::vector<bool>(columns, false));
  is_visited.resize(rows, std::vector<bool>(columns, false));
  is_marked.resize(rows, std::vector<bool>(columns, false));

  unknown_cells = rows * columns;
  marked_mines = 0;
  visited_cells = 0;

  // Initialize random number generator
  rng.seed(std::chrono::steady_clock::now().time_since_epoch().count());

  int first_row, first_column;
  std::cin >> first_row >> first_column;
  Execute(first_row, first_column, 0);
}

/**
 * @brief The definition of function ReadMap()
 *
 * @details This function is designed to read the game map from stdin when playing the client's (or player's) role.
 * Since the client (or player) can only get the limited information of the game map, so if there is a 3 * 3 map as
 * above and only the block (2, 0) has been visited, the stdin would be
 *     ???
 *     12?
 *     01?
 */
void ReadMap() {
  // Read the current map state
  for (int i = 0; i < rows; i++) {
    std::string line;
    std::cin >> line;
    for (int j = 0; j < columns; j++) {
      known_map[i][j] = line[j];

      // Update internal state based on what we see
      if (line[j] == '?') {
        // Unknown cell
        if (is_visited[i][j]) {
          is_visited[i][j] = false;
          visited_cells--;
        }
        if (is_marked[i][j]) {
          is_marked[i][j] = false;
          marked_mines--;
        }
      } else if (line[j] >= '0' && line[j] <= '8') {
        // Visited non-mine cell
        if (!is_visited[i][j]) {
          is_visited[i][j] = true;
          visited_cells++;
        }
        is_safe[i][j] = true; // Visited cells are definitely safe
        is_mine[i][j] = false;
        is_marked[i][j] = false;
      } else if (line[j] == '@') {
        // Marked mine (correctly)
        if (!is_marked[i][j]) {
          is_marked[i][j] = true;
          marked_mines++;
        }
        is_mine[i][j] = true;
        is_safe[i][j] = false;
        is_visited[i][j] = false;
      } else if (line[j] == 'X') {
        // Either marked non-mine (wrong) or visited mine (game over)
        // In both cases, we mark it as visited
        if (!is_visited[i][j]) {
          is_visited[i][j] = true;
          visited_cells++;
        }
        is_safe[i][j] = false;
        // Don't update mine status since X could be wrong mark or actual mine
      }
    }
  }

  // Update unknown cells count
  unknown_cells = rows * columns - visited_cells - marked_mines;

  // Update knowledge after reading new map
  updateKnowledge();
}

/**
 * @brief The definition of function Decide()
 *
 * @details This function is designed to decide the next step when playing the client's (or player's) role. Open up your
 * mind and make your decision here! Caution: you can only execute once in this function.
 */
void Decide() {
  // Strategy:
  // 1. First, check for auto-explore opportunities (safe and efficient)
  // 2. Then, mark obvious mines
  // 3. Then, visit known safe cells
  // 4. Finally, make a random guess if no safe moves

  // Step 1: Check for auto-explore
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (shouldAutoExplore(i, j)) {
        Execute(i, j, 2);  // Auto-explore
        return;
      }
    }
  }

  // Step 2: Mark obvious mines
  auto mine_cell = findObviousMine();
  if (mine_cell.first != -1) {
    Execute(mine_cell.first, mine_cell.second, 1);  // Mark mine
    return;
  }

  // Step 3: Visit known safe cells
  auto safe_cell = findSafeCell();
  if (safe_cell.first != -1) {
    Execute(safe_cell.first, safe_cell.second, 0);  // Visit
    return;
  }

  // Step 4: Make an educated guess
  // Use probability calculations to choose the safest cell
  auto best_guess = findBestGuess();
  if (best_guess.first != -1) {
    Execute(best_guess.first, best_guess.second, 0);  // Visit
    return;
  }

  // Fallback: should never reach here
  Execute(0, 0, 0);
}

#endif