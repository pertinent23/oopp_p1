#include "belote.hh"
#include <string>
#include <array>
#include <algorithm>

namespace {
    // Constants for magic numbers
    const int NUM_PLAYERS = 4;
    const int NUM_TRICKS = 8;
    const int NUM_CARDS = 32;

    typedef std::array<bool, NUM_CARDS> PlayerPossibleCards;
    typedef std::array<PlayerPossibleCards, NUM_PLAYERS> GameState;

    /**
     * @brief Converts a suit character to its corresponding integer index.
     * * @param suit The suit character ('s', 'h', 'd', 'c').
     * @return int The index of the suit (0 to 3), or -1 if invalid.
     */
    int get_suit_idx(const char suit) {
        switch (suit)
        {
            case 's': return 0;
            case 'h': return 1;
            case 'd': return 2;
            case 'c': return 3;
            default: return -1;
        }
    }

    /**
     * @brief Converts a rank character to its corresponding integer index.
     * * @param rank The rank character ('A', 'K', 'Q', 'J', 'T', '9', '8', '7').
     * @return int The index of the rank (0 to 7), or -1 if invalid.
     */
    int get_rank_idx(const char rank) {
        const std::string ranks = "AKQJT987";
        const auto pos = ranks.find(rank);
        return (pos != std::string::npos) ? static_cast<int>(pos) : -1;
    }

    /**
     * @brief Computes a unique ID for a given card based on its rank and suit.
     * * @param rank The rank character.
     * @param suit The suit character.
     * @return int A unique identifier for the card (0 to 31).
     */
    int get_card_id(const char rank, const char suit) {
        return get_suit_idx(suit) * 8 + get_rank_idx(rank);
    }

    /**
     * @brief Calculates the point value of a card.
     * * @param rank The rank character.
     * @param suit The suit character.
     * @param trump The current trump suit.
     * @return int The point value of the card.
     */
    int get_card_points(const char rank, const char suit, const char trump) {
        if (suit == trump) {
            switch(rank) {
                case 'J': return 20;
                case '9': return 14;
                case 'A': return 11;
                case 'T': return 10;
                case 'K': return 4;
                case 'Q': return 3;
                default: return 0;
            }
        } else {
            switch(rank) {
                case 'A': return 11;
                case 'T': return 10;
                case 'K': return 4;
                case 'Q': return 3;
                case 'J': return 2;
                default: return 0;
            }
        }
    }

    /**
     * @brief Determines the relative strength of a trump card.
     * * @param rank The rank character.
     * @return int The relative strength value (higher is stronger).
     */
    int get_trump_order(const char rank) {
        switch(rank) {
            case 'J': return 7;
            case '9': return 6;
            case 'A': return 5;
            case 'T': return 4;
            case 'K': return 3;
            case 'Q': return 2;
            case '8': return 1;
            case '7': return 0;
            default: return -1;
        }
    }

    /**
     * @brief Determines the relative strength of a non-trump card.
     * * @param rank The rank character.
     * @return int The relative strength value (higher is stronger).
     */
    int get_plain_order(const char rank) {
        switch(rank) {
            case 'A': return 7;
            case 'T': return 6;
            case 'K': return 5;
            case 'Q': return 4;
            case 'J': return 3;
            case '9': return 2;
            case '8': return 1;
            case '7': return 0;
            default: return -1;
        }
    }

    /**
     * @brief Marks a specific suit as empty for a given player.
     * * @param state The current game state tracking possible cards.
     * @param player The index of the player (0 to 3).
     * @param suit The suit to mark as empty.
     */
    void mark_suit_empty(GameState& state, const int player, const char suit) {
        const std::string all_ranks = "AKQJT987";
        for (const char r : all_ranks) {
            state[player][get_card_id(r, suit)] = false;
        }
    }

    /**
     * @brief Marks all trump cards stronger than a specific value as empty for a player.
     * * @param state The current game state.
     * @param player The index of the player.
     * @param trump The trump suit.
     * @param max_val The value of the current highest trump on the table.
     */
    void mark_higher_trumps_empty(GameState& state, const int player, const char trump, const int max_val) {
        const std::string all_ranks = "AKQJT987";
        for (const char r : all_ranks) {
            if (get_trump_order(r) > max_val) {
                state[player][get_card_id(r, trump)] = false;
            }
        }
    }

    /**
     * @brief Marks a card as globally played so no one else can hold it.
     * * @param state The current game state.
     * @param card_id The ID of the played card.
     */
    void mark_card_played(GameState& state, const int card_id) {
        for (int p = 0; p < NUM_PLAYERS; ++p) {
            state[p][card_id] = false;
        }
    }

    /**
     * @brief Calculates and assigns final scores based on the contract and game outcome.
     * * @param out The output stream to print the final scores.
     * @param tricks_won The number of tricks won by each team.
     * @param team_scores The raw scores collected by each team.
     * @param belote_points The belote/rebelote points collected by each team.
     * @param taker_team The index of the team that took the contract (0 or 1).
     */
    void resolve_final_scores(
        std::ostream& out, 
        const std::array<int, 2>& tricks_won, 
        const std::array<int, 2>& team_scores,
        const std::array<int, 2>& belote_points, 
        const int taker_team
    ) {
        
        int base_scores[2] = { team_scores[0] - belote_points[0], team_scores[1] - belote_points[1] };
        
        // Capot rule check
        if (tricks_won[0] == NUM_TRICKS) { base_scores[0] = 252; base_scores[1] = 0; }
        if (tricks_won[1] == NUM_TRICKS) { base_scores[0] = 0; base_scores[1] = 252; }

        const int defender_team = 1 - taker_team;
        std::array<int, 2> final_scores;

        // Contract check (Être dedans)
        const int taker_total = base_scores[taker_team] + belote_points[taker_team];
        const int defender_total = base_scores[defender_team] + belote_points[defender_team];

        if (taker_total < defender_total) {
            const int def_base = (tricks_won[defender_team] == NUM_TRICKS) ? 252 : 162;
            final_scores[defender_team] = def_base + belote_points[defender_team];
            final_scores[taker_team] = belote_points[taker_team];
        } else {
            final_scores[0] = base_scores[0] + belote_points[0];
            final_scores[1] = base_scores[1] + belote_points[1];
        }

        out << final_scores[0] << " " << final_scores[1] << "\n";
    }
}

bool game(std::istream& in, std::ostream& out, std::ostream& err) {
    char trump_char;
    int taker_team_input;
    
    if (!(in >> trump_char >> taker_team_input)) return true;

    const int taker_team = taker_team_input - 1;

    GameState possible_cards;
    for (int p = 0; p < NUM_PLAYERS; ++p) {
        possible_cards[p].fill(true);
    }

    std::array<int, 2> tricks_won = {0, 0};
    std::array<int, 2> belote_points = {0, 0};
    std::array<int, 2> team_scores = {0, 0};
    
    std::array<bool, NUM_PLAYERS> played_k_trump = {false, false, false, false};
    std::array<bool, NUM_PLAYERS> played_q_trump = {false, false, false, false};

    int first_player = 0;

    for (int trick = 0; trick < NUM_TRICKS; ++trick) {
        std::array<std::string, NUM_PLAYERS> cards;
        for (int i = 0; i < NUM_PLAYERS; ++i) {
            in >> cards[i];
        }

        int best_player = -1;
        int highest_trump_val = -1;
        int best_card_val = -1;
        int trick_points = 0;
        const char lead_suit = cards[0][1];

        for (int i = 0; i < NUM_PLAYERS; ++i) {
            const int current_player = (first_player + i) % NUM_PLAYERS;
            const std::string& card = cards[i];
            const char rank = card[0];
            const char suit = card[1];
            const int card_id = get_card_id(rank, suit);

            // Rule Verification: Cheat detection
            if (!possible_cards[current_player][card_id]) {
                err << "Error: player " << (current_player + 1) << " played a " << rank << " of " << suit 
                    << ", but previous tricks implied he should not have this card.\n";
                return false;
            }

            mark_card_played(possible_cards, card_id);
            trick_points += get_card_points(rank, suit, trump_char);

            // Belote/Rebelote logic
            if (suit == trump_char) {
                if (rank == 'K') played_k_trump[current_player] = true;
                if (rank == 'Q') played_q_trump[current_player] = true;
                
                if (played_k_trump[current_player] && played_q_trump[current_player]) {
                    const int team_idx = current_player % 2;
                    belote_points[team_idx] += 20;
                    team_scores[team_idx] += 20;
                    played_k_trump[current_player] = false; 
                }
            }

            // Constraints deduction
            const bool partner_is_master = (best_player != -1) && ((current_player % 2) == (best_player % 2));

            if (suit != lead_suit) {
                mark_suit_empty(possible_cards, current_player, lead_suit);

                if (!partner_is_master && suit != trump_char) {
                    mark_suit_empty(possible_cards, current_player, trump_char);
                }
            }

            const bool forced_to_play_trump = (lead_suit == trump_char) || (suit != lead_suit && !partner_is_master);
            if (forced_to_play_trump && suit == trump_char && highest_trump_val != -1) {
                const int played_val = get_trump_order(rank);
                if (played_val < highest_trump_val) {
                    mark_higher_trumps_empty(possible_cards, current_player, trump_char, highest_trump_val);
                }
            }

            // Update trick winner
            if (best_player == -1) {
                best_player = current_player;
                if (suit == trump_char) highest_trump_val = get_trump_order(rank);
                best_card_val = (suit == trump_char) ? get_trump_order(rank) : get_plain_order(rank);
            } else {
                if (suit == trump_char) {
                    const int t_val = get_trump_order(rank);
                    if (highest_trump_val == -1 || t_val > highest_trump_val) {
                        best_player = current_player;
                        highest_trump_val = t_val;
                    }
                } else if (suit == lead_suit && highest_trump_val == -1) {
                    const int p_val = get_plain_order(rank);
                    if (p_val > best_card_val) {
                        best_player = current_player;
                        best_card_val = p_val;
                    }
                }
            }
        }

        const int winning_team = best_player % 2;
        team_scores[winning_team] += trick_points;
        tricks_won[winning_team] += 1;
        
        if (trick == 7) {
            team_scores[winning_team] += 10; // Dix de der
        }

        out << team_scores[0] << " " << team_scores[1] << " " << (best_player + 1) << "\n";
        first_player = best_player;
    }

    resolve_final_scores(out, tricks_won, team_scores, belote_points, taker_team);

    return true;
}