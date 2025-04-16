#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>
#include <random>
#include <chrono>

class TicTacToe
{
private:
    std::array<std::array<char, 3>, 3> board;
    std::mutex board_mutex;
    std::condition_variable turn_cv;
    char current_player;
    bool game_over;
    char winner;

public:
    TicTacToe() : current_player('X'), game_over(false), winner(' ')
    {
        for (auto &row : board)
            row.fill(' ');
    }

    void display_board()
    {
        // Limpar tela antes de exibir o tabuleiro
#ifdef _WIN32
        system("cls"); // Windows
#else
        system("clear"); // Linux/Mac
#endif
        std::cout << "\nTabuleiro:\n";
        for (int i = 0; i < 3; ++i)
        {
            std::cout << " ";
            for (int j = 0; j < 3; ++j)
            {
                std::cout << board[i][j];
                if (j < 2)
                    std::cout << " | ";
            }
            std::cout << "\n";
            if (i < 2)
                std::cout << "---+---+---\n";
        }
        std::cout << std::endl;
    }

    bool make_move(char player, int row, int col)
    {
        std::unique_lock<std::mutex> lock(board_mutex);

        // Libera caso o jogo já tenha acabado
        if (game_over)
        {
            turn_cv.notify_all();
            return false;
        }

        // Espera até ser a vez do jogador OU o jogo terminar
        turn_cv.wait(lock, [&]()
                     { return player == current_player || game_over; });

        if (game_over)
        {
            turn_cv.notify_all();
            return false;
        }

        // Jogada inválida
        if (board[row][col] != ' ')
            return false;

        // Faz a jogada
        board[row][col] = player;
        std::cout << "Jogador " << player << " jogou em (" << row << ", " << col << ")\n";
        display_board();

        // Verifica se alguém venceu ou se houve empate
        if (check_win(player))
        {
            game_over = true;
            winner = player;
        }
        else if (check_draw())
        {
            game_over = true;
            winner = 'D';
        }
        else
        {
            current_player = (current_player == 'X') ? 'O' : 'X';
        }

        // Libera a outra thread
        turn_cv.notify_all();
        return true;
    }

    bool check_win(char player)
    {
        for (int i = 0; i < 3; ++i)
            if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) ||
                (board[0][i] == player && board[1][i] == player && board[2][i] == player))
                return true;

        return (board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
               (board[0][2] == player && board[1][1] == player && board[2][0] == player);
    }

    bool check_draw()
    {
        for (const auto &row : board)
            for (char cell : row)
                if (cell == ' ')
                    return false;
        return true;
    }

    bool is_game_over()
    {
        std::lock_guard<std::mutex> lock(board_mutex);
        return game_over;
    }

    char get_winner()
    {
        std::lock_guard<std::mutex> lock(board_mutex);
        return winner;
    }
};

class Player
{
private:
    TicTacToe &game;
    char symbol;
    std::string strategy;

public:
    Player(TicTacToe &g, char s, std::string strat) : game(g), symbol(s), strategy(strat) {}

    void play()
    {
        if (strategy == "sequencial")
            play_sequential();
        else if (strategy == "aleatorio")
            play_random();
    }

private:
    void play_sequential()
    {
        while (!game.is_game_over())
        {
            for (int i = 0; i < 3 && !game.is_game_over(); ++i)
            {
                for (int j = 0; j < 3 && !game.is_game_over(); ++j)
                {
                    if (game.make_move(symbol, i, j))
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    }
                    if (game.is_game_over())
                        return; // Evita travamento
                }
            }
        }
    }

    void play_random()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 2);

        while (!game.is_game_over())
        {
            int i = dist(gen);
            int j = dist(gen);
            if (game.make_move(symbol, i, j))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
            if (game.is_game_over())
                return; // Evita travamento
        }
    }
};

int main()
{
    TicTacToe jogo;

    Player jogador1(jogo, 'X', "sequencial");
    Player jogador2(jogo, 'O', "aleatorio");

    std::thread t1(&Player::play, &jogador1);
    std::thread t2(&Player::play, &jogador2);

    t1.join();
    t2.join();

    char vencedor = jogo.get_winner();
    if (vencedor == 'D')
        std::cout << "\nEmpate!\n";
    else
        std::cout << "\nJogador " << vencedor << " venceu!\n";

    return 0;
}
