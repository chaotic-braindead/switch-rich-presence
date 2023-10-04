#include <iostream>
#include <csignal>
#include <thread>
#include <algorithm>
#include <memory>
#include "discord.h"
#include "env.h"


namespace {
	volatile bool interrupted{ false };
}

struct DiscordState {
	std::unique_ptr<discord::Core> core;
	discord::User currentUser;
};

struct Game {
	std::string name;
	std::string largeImage;

	Game(const char* name) {
		this->name = name;
		this->largeImage = name;
		std::transform(largeImage.begin(), largeImage.end(), largeImage.begin(), [](char c) { return std::tolower(c); });
		std::replace(largeImage.begin(), largeImage.end(), ' ', '_');
		
	}
	
};

int main() {
	DiscordState state{};
	Game game("Super Smash Bros Ultimate");
	std::cout << game.largeImage;
	
	discord::Core* core{};
	auto result = discord::Core::Create(CLIENT_ID, DiscordCreateFlags_NoRequireDiscord, &core);
	state.core.reset(core);

	if (!state.core) {
		std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result)
			<< ")\n";
		std::exit(-1);
	}

	state.core->SetLogHook(
		discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
			std::cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n";
	});
	

	core->UserManager().OnCurrentUserUpdate.Connect([&state]() {
		state.core->UserManager().GetCurrentUser(&state.currentUser);
		std::cout << state.currentUser.GetUsername() << "\n";
	});

	std::signal(SIGINT, [](int) { interrupted = true; });


	discord::Activity act{};
	act.SetDetails(game.name.c_str());
	act.GetTimestamps().SetStart(std::time(0));
	act.GetAssets().SetLargeImage(game.largeImage.c_str());
	act.SetType(discord::ActivityType::Playing);
	state.core->ActivityManager().UpdateActivity(act,
		[](discord::Result res) {
			std::cout << ((res == discord::Result::Ok) ? "updated" : "failed");
	});

	do {
		state.core->RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	} while (!interrupted);
	return 0;
}