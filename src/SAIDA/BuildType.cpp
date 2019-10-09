#include "BuildType.h"

namespace BWAPI
{
	// NAMES
	template <>
	const std::string Type<InitialBuildType, InitialBuildTypes::Enum::UnknownBuild>::typeNames[InitialBuildTypes::Enum::MAX] =
	{
		"Zerg_4_Drone",
		"Zerg_5_Drone",
		"Zerg_9_Drone",
		"Zerg_9_Hat",
		"Zerg_9_OverPool",
		"Zerg_9_Balup",
		"Zerg_12_Pool",
		"Zerg_12_Hat",
		"Zerg_12_Ap",
		"Zerg_sunken_rush",
		"Zerg_4_Drone_Real",
		"Toss_1g_dragoon",
		"Toss_2g_zealot",
		"Toss_2g_dragoon",
		"Toss_1g_forward",
		"Toss_2g_forward",
		"Toss_1g_double",
		"Toss_pure_double",
		"Toss_forge",
		"Toss_4probes",
		"Toss_cannon_rush",
		"Terran_2b",
		"Terran_1b_forward",
		"Terran_2b_forward",
		"Terran_bunker_rush",
		"Terran_1b_double",
		"Terran_1fac",
		"Terran_pure_double",
		"Terran_4scv",
		"UnknownBuild"
	};

	namespace InitialBuildTypes
	{
		MyBot_TYPEDEF(InitialBuildType, Zerg_4_Drone);
		MyBot_TYPEDEF(InitialBuildType, Zerg_5_Drone);
		MyBot_TYPEDEF(InitialBuildType, Zerg_9_Drone);
		MyBot_TYPEDEF(InitialBuildType, Zerg_9_Hat);
		MyBot_TYPEDEF(InitialBuildType, Zerg_9_OverPool);
		MyBot_TYPEDEF(InitialBuildType, Zerg_9_Balup);
		MyBot_TYPEDEF(InitialBuildType, Zerg_12_Pool);
		MyBot_TYPEDEF(InitialBuildType, Zerg_12_Hat);
		MyBot_TYPEDEF(InitialBuildType, Zerg_12_Ap);
		MyBot_TYPEDEF(InitialBuildType, Zerg_sunken_rush);
		MyBot_TYPEDEF(InitialBuildType, Zerg_4_Drone_Real);
		MyBot_TYPEDEF(InitialBuildType, Toss_1g_dragoon);
		MyBot_TYPEDEF(InitialBuildType, Toss_2g_zealot);
		MyBot_TYPEDEF(InitialBuildType, Toss_2g_dragoon);
		MyBot_TYPEDEF(InitialBuildType, Toss_1g_forward);
		MyBot_TYPEDEF(InitialBuildType, Toss_2g_forward);
		MyBot_TYPEDEF(InitialBuildType, Toss_1g_double);
		MyBot_TYPEDEF(InitialBuildType, Toss_pure_double);
		MyBot_TYPEDEF(InitialBuildType, Toss_forge);
		MyBot_TYPEDEF(InitialBuildType, Toss_4probes);
		MyBot_TYPEDEF(InitialBuildType, Toss_cannon_rush);
		MyBot_TYPEDEF(InitialBuildType, Terran_2b);
		MyBot_TYPEDEF(InitialBuildType, Terran_1b_forward);
		MyBot_TYPEDEF(InitialBuildType, Terran_2b_forward);
		MyBot_TYPEDEF(InitialBuildType, Terran_bunker_rush);
		MyBot_TYPEDEF(InitialBuildType, Terran_1b_double);
		MyBot_TYPEDEF(InitialBuildType, Terran_1fac);
		MyBot_TYPEDEF(InitialBuildType, Terran_pure_double);
		MyBot_TYPEDEF(InitialBuildType, Terran_4scv);
		MyBot_TYPEDEF(InitialBuildType, UnknownBuild);
	};

	InitialBuildType::InitialBuildType(int id) : Type(id) {}

	// NAMES
	template <>
	const std::string Type<MainBuildType, MainBuildTypes::Enum::UnknownMainBuild>::typeNames[MainBuildTypes::Enum::MAX] =
	{
		"Zerg_main_zergling",
		"Zerg_main_maybe_mutal",
		"Zerg_main_hydra",
		"Zerg_main_lurker",
		"Zerg_main_mutal",
		"Zerg_main_fast_mutal",
		"Zerg_main_hydra_mutal",
		"Zerg_main_queen_hydra",
		"Toss_range_up_dra_push",
		"Toss_2gate_dra_push",
		"Toss_pure_tripple",
		"Toss_drop",
		"Toss_reaver_drop",
		"Toss_dark",
		"Toss_dark_drop",
		"Toss_1base_fast_zealot",
		"Toss_2base_zealot_dra",
		"Toss_mass_production",
		"Toss_Scout",
		"Toss_fast_carrier",
		"Toss_arbiter",
		"Toss_arbiter_carrier",
		"Terran_1fac_double",
		"Terran_1fac_1star",
		"Terran_1fac_2star",
		"Terran_2fac",
		"Terran_2fac_double",
		"Terran_2fac_1star",
		"Terran_3fac",
		"UnknownMainBuild"
	};

	namespace MainBuildTypes
	{
		MyBot_TYPEDEF(MainBuildType, Zerg_main_zergling);
		MyBot_TYPEDEF(MainBuildType, Zerg_main_maybe_mutal);
		MyBot_TYPEDEF(MainBuildType, Zerg_main_hydra);
		MyBot_TYPEDEF(MainBuildType, Zerg_main_lurker);
		MyBot_TYPEDEF(MainBuildType, Zerg_main_mutal);
		MyBot_TYPEDEF(MainBuildType, Zerg_main_fast_mutal);
		MyBot_TYPEDEF(MainBuildType, Zerg_main_hydra_mutal);
		MyBot_TYPEDEF(MainBuildType, Zerg_main_queen_hydra);
		MyBot_TYPEDEF(MainBuildType, Toss_range_up_dra_push);
		MyBot_TYPEDEF(MainBuildType, Toss_2gate_dra_push);
		MyBot_TYPEDEF(MainBuildType, Toss_pure_tripple);
		MyBot_TYPEDEF(MainBuildType, Toss_drop);
		MyBot_TYPEDEF(MainBuildType, Toss_reaver_drop);
		MyBot_TYPEDEF(MainBuildType, Toss_dark);
		MyBot_TYPEDEF(MainBuildType, Toss_dark_drop);
		MyBot_TYPEDEF(MainBuildType, Toss_1base_fast_zealot);
		MyBot_TYPEDEF(MainBuildType, Toss_2base_zealot_dra);
		MyBot_TYPEDEF(MainBuildType, Toss_mass_production);
		MyBot_TYPEDEF(MainBuildType, Toss_Scout);
		MyBot_TYPEDEF(MainBuildType, Toss_fast_carrier);
		MyBot_TYPEDEF(MainBuildType, Toss_arbiter);
		MyBot_TYPEDEF(MainBuildType, Toss_arbiter_carrier);
		MyBot_TYPEDEF(MainBuildType, Terran_1fac_double);
		MyBot_TYPEDEF(MainBuildType, Terran_1fac_1star);
		MyBot_TYPEDEF(MainBuildType, Terran_1fac_2star);
		MyBot_TYPEDEF(MainBuildType, Terran_2fac);
		MyBot_TYPEDEF(MainBuildType, Terran_2fac_double);
		MyBot_TYPEDEF(MainBuildType, Terran_2fac_1star);
		MyBot_TYPEDEF(MainBuildType, Terran_3fac);
		MyBot_TYPEDEF(MainBuildType, UnknownMainBuild);
	};

	MainBuildType::MainBuildType(int id) : Type(id) {}

	// NAMES
	template <>
	const std::string Type<MainStrategyType, MainStrategyTypes::Enum::Eliminate>::typeNames[MainStrategyTypes::Enum::MAX] =
	{
		"WaitToBase",
		"WaitToFirstExpansion",
		"AttackAll",
		"BackAll",
		"DrawLine",
		"WaitLine",
		"Eliminate"
	};

	namespace MainStrategyTypes
	{
		MyBot_TYPEDEF(MainStrategyType, WaitToBase);
		MyBot_TYPEDEF(MainStrategyType, WaitToFirstExpansion);
		MyBot_TYPEDEF(MainStrategyType, AttackAll);
		MyBot_TYPEDEF(MainStrategyType, BackAll);
		MyBot_TYPEDEF(MainStrategyType, DrawLine);
		MyBot_TYPEDEF(MainStrategyType, WaitLine);
		MyBot_TYPEDEF(MainStrategyType, Eliminate);
	};

	MainStrategyType::MainStrategyType(int id) : Type(id) {}

	// ³» ºôµå
	template <>
	const std::string Type<MyBuildType, MyBuildTypes::Enum::Basic>::typeNames[MyBuildTypes::Enum::MAX] =
	{
		"Terran_VultureTankWraith",
		"Terran_TankGoliath",
		"Protoss_MineKiller",
		"Protoss_ZealotKiller",
		"Protoss_CannonKiller",
		"Protoss_CarrierKiller",
		"Protoss_DragoonKiller",
		"Protoss_TemplarKiller",
		
		"Protoss_FullMachanic",
		"Zerg_MMtoMachanic",
		"Basic"

	};

	namespace MyBuildTypes
	{
		MyBot_TYPEDEF(MyBuildType, Terran_VultureTankWraith);
		MyBot_TYPEDEF(MyBuildType, Terran_TankGoliath);
		MyBot_TYPEDEF(MyBuildType, Protoss_MineKiller);
		MyBot_TYPEDEF(MyBuildType, Protoss_ZealotKiller);
		MyBot_TYPEDEF(MyBuildType, Protoss_CannonKiller);
		MyBot_TYPEDEF(MyBuildType, Protoss_CarrierKiller);
		MyBot_TYPEDEF(MyBuildType, Protoss_DragoonKiller);
		MyBot_TYPEDEF(MyBuildType, Protoss_TemplarKiller);
		MyBot_TYPEDEF(MyBuildType, Protoss_FullMechanic);
		MyBot_TYPEDEF(MyBuildType, Zerg_MMtoMachanic);
		MyBot_TYPEDEF(MyBuildType, Basic);
	};

	MyBuildType::MyBuildType(int id) : Type(id) {}
}
