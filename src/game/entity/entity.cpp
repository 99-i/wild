#include "game/entity/entity.hpp"
#include "common.hpp"
#include "game/world/world.hpp"
#include "net/client.hpp"
#include "net/packet/clientbound_packet.hpp"

namespace wild
{
	counter<int32_t> entity::id_counter = counter<int32_t>();

	void entity::tick_position()
	{
		int8_t dx =
			(int8_t)(std::abs(this->dpos.x * 32) >= 1 ? this->dpos.x * 32.0
													  : 0);
		int8_t dy =
			(int8_t)(std::abs(this->dpos.y * 32) >= 1 ? this->dpos.y * 32.0
													  : 0);
		int8_t dz =
			(int8_t)(std::abs(this->dpos.z * 32) >= 1 ? this->dpos.z * 32.0
													  : 0);
		std::vector<clientbound_packet> packets_to_send;
		packet_builder position_packet(0x14);
		position_packet.append_i32(this->id);
		if (dx + dy + dz == 0)
			{
				// if we shouldn't send position packets...
				if (this->pos.changed_look)
					{
						// should we send the look packet?
						position_packet.set_id(0x16);
						position_packet.append_i8((this->pos.yaw / 360) * 255);
						position_packet.append_i8((this->pos.pitch / 360) *
												  255);
						// hate my use of goto here but in this case it actually
						// makes the code look better imo.
						goto end;
					}
				else
					{
						// we shouldn't send anything. send the default empty
						// "this" packet.
						goto end;
					}
			}
		// todo: add support for teleport packet in here.
		else
			{
				// we should send some sort of position packet.
				position_packet.set_id(0x15)
					.append_i8(dx)
					.append_i8(dy)
					.append_i8(dz);
				if (dx != 0)
					{
						bool negative = (this->dpos.x < 0 ? true : false);
						double multiplier = negative ? -0.03125 : 0.03125;
						double largest_multiple =
							std::floor(this->dpos.x / multiplier) * multiplier;
						this->dpos.x -= largest_multiple;
						this->pos.x += largest_multiple;
					}
				if (dy != 0)
					{
						bool negative = (this->dpos.y < 0 ? true : false);
						double multiplier = negative ? -0.03125 : 0.03125;
						double largest_multiple =
							std::floor(this->dpos.y / multiplier) * multiplier;
						this->dpos.y -= largest_multiple;
						this->pos.y += largest_multiple;
					}
				if (dz != 0)
					{
						bool negative = (this->dpos.z < 0 ? true : false);
						double multiplier = negative ? -0.03125 : 0.03125;
						double largest_multiple =
							std::floor(this->dpos.z / multiplier) * multiplier;
						this->dpos.z -= largest_multiple;
						this->pos.z += largest_multiple;
					}

				if (this->pos.changed_look)
					{
						//..should we send a look packet as well?
						position_packet.set_id(0x17)
							.append_i8((this->pos.yaw / 360) * 255)
							.append_i8((this->pos.pitch / 360) * 255);
					}
			}

	end:
		packets_to_send.push_back(position_packet.build());
		if (this->pos.changed_look)
			{
				packets_to_send.push_back(
					packet_builder(0x19)
						.append_i32(this->id)
						.append_i8((this->pos.yaw / 360) * 255)
						.build());
				this->pos.changed_look = false;
			}
		for (auto player : this->pos.world->get_players())
			{
				if (player != this)
					{
						player->_client.send_packets(packets_to_send);
					}
			}
	}

	void entity::move_diff(double dx, double dy, double dz)
	{
		this->dpos.x += dx;
		this->dpos.y += dy;
		this->dpos.z += dz;
	}

	void entity::move(double x, double y, double z)
	{
		this->dpos.x = x - this->pos.x;
		this->dpos.y = y - this->pos.y;
		this->dpos.z = z - this->pos.z;
	}

	vec3d entity::get_pos() const { return this->pos + this->dpos; }
} // namespace wild
