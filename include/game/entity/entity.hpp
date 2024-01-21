#pragma once
#include "../../common.hpp"
#include "../../util/counter.hpp"
#include <cstdint>

namespace wild
{
class world;
struct entity_pos : public vec3d
{
	world *world;
	float yaw;
	float pitch;
	// has the entity's yaw and pitch changed since the last tick?
	// this is updated in entity#tick and consumed in game#tick_entities.
	bool changed_look;
};

enum class entity_type
{
	PLAYER
};

// any entity that has an id and position (so all entities). e.g., player, pig,
// zombie, arrow, etc
class entity
{
  protected:
	entity(entity_type type) : type(type) {}
	// every entity that's created gets a sequentially generated entity id
	static counter<int32_t> id_counter;
	bool _remove = false;

  public:
	virtual ~entity() = default;
	entity_type type;
	// the entity's position on the server
	wild::entity_pos pos;

	int32_t id = id_counter.next();

	// move the entity by passing a difference in move
	void move_diff(double dx, double dy, double dz);

	// move the entity to a location.
	void move(double x, double y, double z);

	// holds how much the player's position changed before sending an entity
	// relative move/entity look and relative move packet. every tick, it checks
	// if this value is eligible to be sent in a packet (for small values, we
	// can't send the packet because int8_t is too nondescriptive) for example,
	// if dpos.x * 32 >= 1, set dpos.x to 0 and send a rel move packet.
	vec3d dpos;

	// return pos (which is aligned each tick) + dpos, to produce
	// the logical real position of this entity.
	vec3d get_pos() const;

	void tick_position();

	virtual void tick() { this->tick_position(); }

	void remove() { this->_remove = true; }

	bool marked_for_removal() const { return this->_remove; }
};
} // namespace wild
