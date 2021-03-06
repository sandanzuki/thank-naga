#include "Game.hpp"

#include "AssignState.hpp"
#include "GenericResponses.hpp"
#include "PlayingState.hpp"
#include "RematchState.hpp"
#include "RequestVerification.hpp"
#include "SelectionState.hpp"

Game::Game(LogWriter *log, int _game_id, MapInfo *_map) : GameState(log, _game_id, NULL, NULL)
{
    map = _map;
    current_state = new AssignState(log, _game_id);
}

bool Game::tick(double time_in_seconds)
{
    // If the current_state is NULL, we're done!
    if(current_state == NULL)
    {
        return false;
    }

    // Tick things and update appropriately.
    if(!current_state->tick(time_in_seconds))
    {
        switch(current_state->get_name())
        {
            case ASSIGN:
                {
                    AssignState *as = (AssignState*) current_state;
                    player_one = as->get_player_one();
                    player_two = as->get_player_two();
                    current_state = new SelectionState(log, game_id, player_one, player_two);
                    delete as;
                }
                break;
            case SELECTION:
                {
                    SelectionState *ss = (SelectionState*) current_state;
                    current_state = new PlayingState(log, game_id, player_one, player_two,
                            ss->get_player_one_units(), ss->get_player_two_units(), map);
                    delete ss;
                }
                break;
            case PLAYING:
                {
                    delete current_state;
                    current_state = new RematchState(log, game_id, player_one, player_two);
                }
                break;
            case REMATCH:
                {
                    delete current_state;
                    current_state = new SelectionState(log, game_id, player_one, player_two);
                }
                break;
        }
        notify_state_change();
    }

    // Otherwise, just keep going!
    return true;
}

void Game::handle_request(Player *p, EventRequest *r)
{
    // Assuming we still have a valid GameState, try this!
    if(current_state != NULL)
    {
        // Get the type of EventRequest.
        string type = (*r)["type"].asString();

        // Real talk, if either Player quits, it's time to die.
        if(type.compare("PlayerQuitRequest") == 0)
        {
            log->write("[GAME] INFO: Handling PlayerQuitRequest.");
            // First remove the requesting player from the Game.
            if(player_one == p)
            {
                player_one = NULL;
                player_two->set_game_id(-1);
            }
            else if(player_two == p)
            {
                player_two == NULL;
                player_one->set_game_id(-1);
            }

            // If we're in SELECTION state, we have to delete the Units first.
            if(current_state->get_name() == SELECTION)
            {
                SelectionState *ss = (SelectionState*) current_state;
                for(Unit *u : ss->get_player_one_units())
                {
                    delete u;
                }
                for(Unit *u : ss->get_player_two_units())
                {
                    delete u;
                }
            }
            
            // Now delete the current_state and notify the remaining Player.
            delete current_state;
            current_state = NULL;
            notify_state_change();
        }

        // If the Player submits a name change request, go ahead and do it.
        else if(type.compare("PlayerRenameRequest") == 0)
        {
            if(!verify_rename_request(r))
            {
                notify_invalid_request(p->get_connection(), r);
                return;
            }
            notify_rename(p, r);
        }

        // Otherwise, pass it off to the state.
        else
        {
            current_state->handle_request(p, r);
        }
    }

    // After everything is done, delete the EventRequest.
    delete r;
}

bool Game::needs_player()
{
    return current_state != NULL && current_state->get_name() == ASSIGN;
}

void Game::notify_rename(Player *p, EventRequest *r)
{
    // Build the Event
    Event notify;
    notify["type"] = string("PlayerRenameEvent");
    notify["game_id"] = game_id;
    notify["request_id"] = (*r)["request_id"];
    notify["player_id"] = p->get_player_id();
    notify["name"] = (*r)["name"];
    send_all_players(notify);
}

void Game::notify_state_change()
{
    // Determine the state string based on state.
    string state;
    if(current_state == NULL)
    {
        state = string("GAME_OVER");
    }
    else
    {
        switch(current_state->get_name())
        {
            case ASSIGN:
                state = string("ASSIGN");
                break;
            case SELECTION:
                state = string("SELECTION");
                break;
            case PLAYING:
                state = string("PLAYING");
                break;
            case REMATCH:
                state = string("REMATCH");
                break;
            default:
                state = string("INVALID");
                break;
        }
    }

    // Build the Event
    Event notify;
    notify["type"] = string("StateChangeEvent");
    notify["game_id"] = game_id;
    notify["state"] = state;

    // Send to all connected players.
    char buffer[2048];
    memset(buffer, 0, 2048);
    sprintf(buffer, "[GAME] INFO: GameState has switched to %s.", state.c_str());
    log->write(buffer);
    send_all_players(notify);
}
