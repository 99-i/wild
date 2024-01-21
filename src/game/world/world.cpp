#include "game/world/world.hpp"
#include "game/entity/player.hpp"
#include <algorithm>
#include <plog/Log.h>

namespace wild
{
	// todo: players far away shouldn't get that packet.
	void world::tick_entities()
	{
		std::lock_guard entities_guard{this->entities_mutex};

		std::vector<entity *> removed_entities;
		for (size_t i = 0; i < this->entities.size(); i++)
			{
				entity *entity = this->entities[i];
				if (entity->marked_for_removal())
					{
						removed_entities.push_back(entity);
					}
				else
					{
						entity->tick();
					}
			}
		if (removed_entities.size() > 0)
			{
				auto it = std::remove_if(
					this->entities.begin(), this->entities.end(),
					[&](entity *entity) {
						return std::find(removed_entities.begin(),
										 removed_entities.end(),
										 entity) != removed_entities.end();
					});
				this->entities.erase(it, this->entities.end());

				for (auto player : this->get_players())
					{
						player->send_destroy_entities(removed_entities);
					}

				for (auto entity : removed_entities)
					{
						// todo: what else should we do here?
						delete entity;
					}
			}
	}

	void world::spawn_entity(entity *entity)
	{
		std::lock_guard entities_guard{this->entities_mutex};
		entity->pos.world = this;

		if (entity->type == entity_type::PLAYER)
			{
				player *player = dynamic_cast<struct player *>(entity);
				// todo: world saving player data.
				player->pos.x = 7;
				player->pos.y = 18;
				player->pos.z = 7;

				if (player == nullptr)
					{
						PLOGF << "dynamic_cast to player returned nullptr.";
						return;
					}

				player->send_join_game_packet(0, 0, 0, 100, "default");
				player->send_spawn_position_packet();
				player->send_player_abilities_packet();
				player->send_bulk_chunk_data_packet();
				player->send_move_and_look_packet();

				for (auto existing_player : this->get_players())
					{
						player->initialize_other_player(existing_player);
						existing_player->initialize_other_player(player);
					}
			}
		else
			{
				// spawn mob, spawn object, etc..
			}
		this->entities.push_back(entity);
	}

	std::vector<player *> world::get_players()
	{
		std::vector<player *> players;
		for (size_t i = 0; i < this->entities.size(); i++)
			{
				entity *entity = this->entities[i];
				if (entity != nullptr &&
					dynamic_cast<player *>(entity) != nullptr)
					{
						players.push_back(dynamic_cast<player *>(entity));
					}
			}
		return players;
	}
} // namespace wild
