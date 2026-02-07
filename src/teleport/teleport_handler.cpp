#include "teleport_handler.h"
#include <thread>
#include <chrono>
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <sdk/sdk.h>
#include <game/game.h>
#include <cache/cache.h>

namespace teleport
{
	void run()
	{
		std::uint64_t last_place_id = memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::PlaceId);
		std::string last_job_id = memory->read<std::string>(game::datamodel.address + Offsets::DataModel::JobId);

		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));

			std::uint64_t fake_dm = memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer);
			rbx::instance_t dm(memory->read<std::uint64_t>(fake_dm + Offsets::FakeDataModel::RealDataModel));

			if (!dm.address || dm.get_name() != "Ugc")
			{
				last_place_id = 0;
				last_job_id = "";
				continue;
			}

			std::uint64_t place_id = memory->read<std::uint64_t>(dm.address + Offsets::DataModel::PlaceId);
			std::string job_id = memory->read<std::string>(dm.address + Offsets::DataModel::JobId);

			if ((place_id != last_place_id || job_id != last_job_id) && place_id != 0)
			{
				game::datamodel = dm;
				game::visengine = { memory->read<std::uint64_t>(memory->get_module_address() + Offsets::VisualEngine::Pointer) };
				game::workspace = { game::datamodel.find_first_child_by_class("Workspace") };
				game::players = { game::datamodel.find_first_child_by_class("Players") };
				game::local_player = { memory->read<std::uint64_t>(game::players.address + Offsets::Player::LocalPlayer) };

				{
					std::lock_guard<std::mutex> lock(cache::mtx);
					cache::cached_players.clear();
					cache::cached_local_player = {};
				}

				last_place_id = place_id;
				last_job_id = job_id;
				game::detect_game();
			}
		}
	}
}