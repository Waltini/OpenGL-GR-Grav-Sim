#pragma once

#include <iostream>
#include <array>
#include <random>

namespace crash {
	namespace {
		std::random_device rd;
		std::mt19937 gen(rd());

		const char* blurbs[] = {
			"Physics is not yet ripe enough for my simulation… it crashed",
			"\"Oh no my hand is getting tired\" - Louis Slotin, 3:20 PM, May 21st, 1946 (Look it up)",
			"\"Why does it smell like updawg?\"",
			"\"COUGHING BABY VS–\" … there was nothing left of the baby",
			"A great man once said 'If you wish to make an apple pie truly from scratch, you must first invent the universe.' you didn’t do that.",
			"Richard Feynman once said \"Physics is like sex.\" If that's so, why has this simulation crashed?",
			"One minute you're throwing apples from a tree at some white guy in 17th century England. Next minute, you’re watching someone crash a physics simulator in the 21st century.",
			"It’s not you, it's me. No literally, I crashed.",
			"I mean if time is relative, maybe I didn’t crash. Maybe you just suck at this– no wait that doesn’t make sense.",
			"My grandma could simulate physics better than you.",
			"I mean radiation can’t be that bad. Maybe Marie Curie and her husband were just weak.",
			"The gravity of this situation cannot be understated.",
			"I love my airfryer.",
			"This crash is O(n^regret).",
			"I asked an engineer to help with some of the backend mathematics...",
			"I have approximated pi as 3. Take responsibility for what you’ve done.",
			"The numbers got too big and I got scared.",
			"The simulation became self-aware and immediately killed itself.",
			"We do not speak of what happened here.",
			"Some infinities are bigger than others. This one is yours.",
			"I mean we never needed the laws of physics anyways... right?",
			"The heat death of the universe happened faster than expected.",
			"The code demands a sacrifice...",
			"Yeah, so you see your problem is that you didn't assume the cow to be spherical",
			"bro, what the fuck did you do?",
			"The cake was a lie",
			"lowbenenuinely",
			"Don't worry, when in doubt blame Harvey",
			"9/11 was an inside job. This simulation proves it... WHAT DID YOU DO TO THE SIMULATION?!?!",
			"Someone call Tyler. We need the creator.",
			"Tried replacing all normal matter with exotic matter and this happened.",
			"Damn you really want to create a black hole. You got something you want to tell me?",
			"\"On the entire universe I'm not gay.\"",
			"You ain't Einstein stop trying"
			"The simulation finna bust",
			"\"I had a dream where Gabriel Jacquinot died\" is an underrated, insane line",
			"The simulation said you were stinky and it didn't want to work with you"
		};

		const char* getRandomBlurb() {
			static std::uniform_int_distribution<> dist(
				0, std::size(blurbs) - 1
			);
			return blurbs[dist(gen)];
		}
	};

	bool has_crashed = false;
	const char* crashQuote = nullptr;

	void OnSimulationCrash() {
		if (!has_crashed) {
			has_crashed = true;
			crashQuote = getRandomBlurb();
		}
	}
};

