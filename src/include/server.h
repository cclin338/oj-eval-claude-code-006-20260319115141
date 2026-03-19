#ifndef SERVER_H
#define SERVER_H

#include <cstdlib>
#include <iostream>
#include <vector>
#include <queue>
#include <utility>

/*
 * You may need to define some global variables for the information of the game map here.
 * Although we don't encourage to use global variables in real cpp projects, you may have to use them because the use of
 * class is not taught yet. However, if you are member of A-class or have learnt the use of cpp class, member functions,
 * etc., you're free to modify this structure.
 */
int rows;         // The count of rows of the game map. You MUST NOT modify its name.
int columns;      // The count of columns of the game map. You MUST NOT modify its name.
int total_mines;  // The count of mines of the game map. You MUST NOT modify its name. You should initialize this
                  // variable in function InitMap. It will be used in the advanced task.
int game_state;  // The state of the game, 0 for continuing, 1 for winning, -1 for losing. You MUST NOT modify its name.

// Additional global variables to track game state
std::vector<std::vector<bool>> mine_map;     // true if cell has mine
std::vector<std::vector<bool>> visited_map;  // true if cell has been visited
std::vector<std::vector<bool>> marked_map;   // true if cell is marked as mine
std::vector<std::vector<int>> mine_count_map; // mine count for each cell (0-8)
int visited_count;    // number of visited non-mine cells
int marked_mine_count; // number of correctly marked mines

/**
 * @brief The definition of function InitMap()
 *
 * @details This function is designed to read the initial map from stdin. For example, if there is a 3 * 3 map in which
 * mines are located at (0, 1) and (1, 2) (0-based), the stdin would be
 *     3 3
 *     .X.
 *     ...
 *     ..X
 * where X stands for a mine block and . stands for a normal block. After executing this function, your game map
 * would be initialized, with all the blocks unvisited.
 */
void InitMap() {
  std::cin >> rows >> columns;

  // Initialize all maps
  mine_map.resize(rows, std::vector<bool>(columns, false));
  visited_map.resize(rows, std::vector<bool>(columns, false));
  marked_map.resize(rows, std::vector<bool>(columns, false));
  mine_count_map.resize(rows, std::vector<int>(columns, 0));

  visited_count = 0;
  marked_mine_count = 0;
  total_mines = 0;
  game_state = 0;

  // Read the map
  for (int i = 0; i < rows; i++) {
    std::string line;
    std::cin >> line;
    for (int j = 0; j < columns; j++) {
      if (line[j] == 'X') {
        mine_map[i][j] = true;
        total_mines++;
      } else {
        mine_map[i][j] = false;
      }
    }
  }

  // Calculate mine counts for each non-mine cell
  const int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (!mine_map[i][j]) {
        int count = 0;
        for (int d = 0; d < 8; d++) {
          int ni = i + dx[d];
          int nj = j + dy[d];
          if (ni >= 0 && ni < rows && nj >= 0 && nj < columns && mine_map[ni][nj]) {
            count++;
          }
        }
        mine_count_map[i][j] = count;
      } else {
        mine_count_map[i][j] = 0; // Mine cells don't need count
      }
    }
  }
}

/**
 * @brief The definition of function VisitBlock(int, int)
 *
 * @details This function is designed to visit a block in the game map. We take the 3 * 3 game map above as an example.
 * At the beginning, if you call VisitBlock(0, 0), the return value would be 0 (game continues), and the game map would
 * be
 *     1??
 *     ???
 *     ???
 * If you call VisitBlock(0, 1) after that, the return value would be -1 (game ends and the players loses) , and the
 * game map would be
 *     1X?
 *     ???
 *     ???
 * If you call VisitBlock(0, 2), VisitBlock(2, 0), VisitBlock(1, 2) instead, the return value of the last operation
 * would be 1 (game ends and the player wins), and the game map would be
 *     1@1
 *     122
 *     01@
 *
 * @param r The row coordinate (0-based) of the block to be visited.
 * @param c The column coordinate (0-based) of the block to be visited.
 *
 * @note You should edit the value of game_state in this function. Precisely, edit it to
 *    0  if the game continues after visit that block, or that block has already been visited before.
 *    1  if the game ends and the player wins.
 *    -1 if the game ends and the player loses.
 *
 * @note For invalid operation, you should not do anything.
 */
void VisitBlock(int r, int c) {
  // Check if coordinates are valid
  if (r < 0 || r >= rows || c < 0 || c >= columns) {
    return;
  }

  // If already visited or marked, no effect
  if (visited_map[r][c] || marked_map[r][c]) {
    return;
  }

  // If it's a mine, game over
  if (mine_map[r][c]) {
    visited_map[r][c] = true;
    game_state = -1;
    return;
  }

  // Visit this cell (non-mine)
  visited_map[r][c] = true;
  visited_count++;

  // If cell has 0 mine count, recursively visit all adjacent unvisited, unmarked cells
  if (mine_count_map[r][c] == 0) {
    const int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    // Use BFS to visit all adjacent 0-count cells
    std::queue<std::pair<int, int>> q;
    q.push({r, c});

    while (!q.empty()) {
      auto [cr, cc] = q.front();
      q.pop();

      // Check all 8 neighbors
      for (int d = 0; d < 8; d++) {
        int nr = cr + dx[d];
        int nc = cc + dy[d];
        if (nr >= 0 && nr < rows && nc >= 0 && nc < columns &&
            !visited_map[nr][nc] && !marked_map[nr][nc]) {

          // If neighbor is a mine, skip (shouldn't visit mines)
          if (mine_map[nr][nc]) {
            continue;
          }

          // Visit the neighbor
          visited_map[nr][nc] = true;
          visited_count++;

          // If neighbor also has 0 mine count, add to queue
          if (mine_count_map[nr][nc] == 0) {
            q.push({nr, nc});
          }
        }
      }
    }
  }

  // Check for win condition: all non-mine cells visited
  int total_non_mine_cells = rows * columns - total_mines;
  if (visited_count == total_non_mine_cells) {
    game_state = 1;
  } else {
    game_state = 0;
  }
}

/**
 * @brief The definition of function MarkMine(int, int)
 *
 * @details This function is designed to mark a mine in the game map.
 * If the block being marked is a mine, show it as "@".
 * If the block being marked isn't a mine, END THE GAME immediately. (NOTE: This is not the same rule as the real
 * game) And you don't need to
 *
 * For example, if we use the same map as before, and the current state is:
 *     1?1
 *     ???
 *     ???
 * If you call MarkMine(0, 1), you marked the right mine. Then the resulting game map is:
 *     1@1
 *     ???
 *     ???
 * If you call MarkMine(1, 0), you marked the wrong mine(There's no mine in grid (1, 0)).
 * The game_state would be -1 and game ends immediately. The game map would be:
 *     1?1
 *     X??
 *     ???
 * This is different from the Minesweeper you've played. You should beware of that.
 *
 * @param r The row coordinate (0-based) of the block to be marked.
 * @param c The column coordinate (0-based) of the block to be marked.
 *
 * @note You should edit the value of game_state in this function. Precisely, edit it to
 *    0  if the game continues after visit that block, or that block has already been visited before.
 *    1  if the game ends and the player wins.
 *    -1 if the game ends and the player loses.
 *
 * @note For invalid operation, you should not do anything.
 */
void MarkMine(int r, int c) {
  // Check if coordinates are valid
  if (r < 0 || r >= rows || c < 0 || c >= columns) {
    return;
  }

  // If already visited or marked, no effect
  if (visited_map[r][c] || marked_map[r][c]) {
    return;
  }

  // Mark the cell
  marked_map[r][c] = true;

  // Check if marked cell is actually a mine
  if (mine_map[r][c]) {
    marked_mine_count++;
    game_state = 0; // Game continues

    // Check for win condition (all non-mine cells visited)
    int total_non_mine_cells = rows * columns - total_mines;
    if (visited_count == total_non_mine_cells) {
      game_state = 1;
    }
  } else {
    // Marked a non-mine cell - immediate game over
    game_state = -1;
  }
}

/**
 * @brief The definition of function AutoExplore(int, int)
 *
 * @details This function is designed to auto-visit adjacent blocks of a certain block.
 * See README.md for more information
 *
 * For example, if we use the same map as before, and the current map is:
 *     ?@?
 *     ?2?
 *     ??@
 * Then auto explore is available only for block (1, 1). If you call AutoExplore(1, 1), the resulting map will be:
 *     1@1
 *     122
 *     01@
 * And the game ends (and player wins).
 */
void AutoExplore(int r, int c) {
  // Check if coordinates are valid
  if (r < 0 || r >= rows || c < 0 || c >= columns) {
    return;
  }

  // Auto-explore can only target visited non-mine grids (grids showing numbers)
  if (!visited_map[r][c] || mine_map[r][c]) {
    return;
  }

  // Count marked mines around the target
  const int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  int marked_around = 0;
  for (int d = 0; d < 8; d++) {
    int nr = r + dx[d];
    int nc = c + dy[d];
    if (nr >= 0 && nr < rows && nc >= 0 && nc < columns && marked_map[nr][nc]) {
      marked_around++;
    }
  }

  // If number of marked mines around equals its mine count, visit all non-mine neighbors
  if (marked_around == mine_count_map[r][c]) {
    bool game_over = false;

    for (int d = 0; d < 8; d++) {
      int nr = r + dx[d];
      int nc = c + dy[d];
      if (nr >= 0 && nr < rows && nc >= 0 && nc < columns) {
        // Only visit non-marked, non-visited cells
        if (!marked_map[nr][nc] && !visited_map[nr][nc]) {
          // Simulate visiting this cell
          if (mine_map[nr][nc]) {
            // Hit a mine - game over
            visited_map[nr][nc] = true;
            game_over = true;
          } else {
            // Visit non-mine cell
            visited_map[nr][nc] = true;
            visited_count++;

            // If cell has 0 mine count, we need to recursively visit
            if (mine_count_map[nr][nc] == 0) {
              // Use a separate BFS for this cell
              std::queue<std::pair<int, int>> q;
              q.push({nr, nc});

              while (!q.empty()) {
                auto [cr, cc] = q.front();
                q.pop();

                // Check all 8 neighbors of this cell
                for (int dd = 0; dd < 8; dd++) {
                  int nrr = cr + dx[dd];
                  int ncc = cc + dy[dd];
                  if (nrr >= 0 && nrr < rows && ncc >= 0 && ncc < columns &&
                      !visited_map[nrr][ncc] && !marked_map[nrr][ncc]) {

                    if (mine_map[nrr][ncc]) {
                      // Hit a mine - game over
                      visited_map[nrr][ncc] = true;
                      game_over = true;
                    } else {
                      visited_map[nrr][ncc] = true;
                      visited_count++;

                      if (mine_count_map[nrr][ncc] == 0) {
                        q.push({nrr, ncc});
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    if (game_over) {
      game_state = -1;
    } else {
      // Check for win condition
      int total_non_mine_cells = rows * columns - total_mines;
      if (visited_count == total_non_mine_cells) {
        game_state = 1;
      } else {
        game_state = 0;
      }
    }
  }
}

/**
 * @brief The definition of function ExitGame()
 *
 * @details This function is designed to exit the game.
 * It outputs a line according to the result, and a line of two integers, visit_count and marked_mine_count,
 * representing the number of blocks visited and the number of marked mines taken respectively.
 *
 * @note If the player wins, we consider that ALL mines are correctly marked.
 */
void ExitGame() {
  if (game_state == 1) {
    std::cout << "YOU WIN!" << std::endl;
    // If player wins, all mines are considered correctly marked
    std::cout << visited_count << " " << total_mines << std::endl;
  } else if (game_state == -1) {
    std::cout << "GAME OVER!" << std::endl;
    std::cout << visited_count << " " << marked_mine_count << std::endl;
  }
  exit(0);  // Exit the game immediately
}

/**
 * @brief The definition of function PrintMap()
 *
 * @details This function is designed to print the game map to stdout. We take the 3 * 3 game map above as an example.
 * At the beginning, if you call PrintMap(), the stdout would be
 *    ???
 *    ???
 *    ???
 * If you call VisitBlock(2, 0) and PrintMap() after that, the stdout would be
 *    ???
 *    12?
 *    01?
 * If you call VisitBlock(0, 1) and PrintMap() after that, the stdout would be
 *    ?X?
 *    12?
 *    01?
 * If the player visits all blocks without mine and call PrintMap() after that, the stdout would be
 *    1@1
 *    122
 *    01@
 * (You may find the global variable game_state useful when implementing this function.)
 *
 * @note Use std::cout to print the game map, especially when you want to try the advanced task!!!
 */
void PrintMap() {
  // Check if game is won - if so, show @ for all mine grids
  bool game_won = (game_state == 1);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (marked_map[i][j]) {
        // Marked cell
        if (mine_map[i][j]) {
          std::cout << '@';
        } else {
          std::cout << 'X'; // Marked non-mine (game would be over already)
        }
      } else if (visited_map[i][j]) {
        // Visited cell
        if (mine_map[i][j]) {
          std::cout << 'X';
        } else {
          std::cout << mine_count_map[i][j];
        }
      } else {
        // Unvisited cell
        if (game_won && mine_map[i][j]) {
          // Game won: show @ for all mine grids regardless of marked status
          std::cout << '@';
        } else {
          std::cout << '?';
        }
      }
    }
    std::cout << std::endl;
  }
}

#endif
