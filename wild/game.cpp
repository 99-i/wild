#include "game.h"
#include "server.h"

constexpr int TICK_RATE = 20;

static void tick_timer_cb(uv_timer_t *timer)
{
	w_game *game = reinterpret_cast<w_game *>((char *)timer - offsetof(w_game, tick_timer));
	game->tick();
}

w_game::w_game(w_server *server) : server(server)
{
}

void w_game::start()
{
	uv_timer_init(&this->server->loop, &this->tick_timer);
	uv_timer_start(&this->tick_timer, tick_timer_cb, 0, 1000 / TICK_RATE);
}
void w_game::tick()
{
	this->lua_vm.tick();
}