#pragma once
#include <BWAPI.h>

#define MyBot_TYPEDEF(type,x) const type x(Enum::x)

namespace BWAPI
{
	// ### 순서를 절대 바꾸지 마세요. ###
	// 추가되는 전략은 가장 아래로(UnknownBuild 위로)
	// 글자수가 너무 길어 화면에 뿌려지는 글자 이상이 되면 오류 발생
	namespace InitialBuildTypes
	{
		namespace Enum
		{
			enum Enum
			{
				Zerg_4_Drone = 0,
				Zerg_5_Drone,
				Zerg_9_Drone,
				Zerg_9_Hat,
				Zerg_9_OverPool,
				Zerg_9_Balup,
				Zerg_12_Pool,
				Zerg_12_Hat,
				Zerg_12_Ap,
				Zerg_sunken_rush,
				Zerg_4_Drone_Real,
				Toss_1g_dragoon,// 1게이트 드라군 8
				Toss_2g_zealot, // 2게이트 질럿 10
				Toss_2g_dragoon, // 2게이트 드라군 9
				Toss_1g_forward,// 전진 1게이트 11
				Toss_2g_forward,// 전진 2게이트 12
				Toss_1g_double,// 1게이트 더블
				Toss_pure_double,// 쌩더블
				Toss_forge,// 포지 더블
				Toss_4probes,
				Toss_cannon_rush,
				Terran_2b,
				Terran_1b_forward,
				Terran_2b_forward,
				Terran_bunker_rush,
				Terran_1b_double,
				Terran_1fac,
				Terran_pure_double,
				Terran_4scv,
				UnknownBuild,
				MAX
			};
		};
	}

	class InitialBuildType : public Type<InitialBuildType, InitialBuildTypes::Enum::UnknownBuild>
	{
	public:
		InitialBuildType(int id = InitialBuildTypes::Enum::UnknownBuild);
	};

	namespace InitialBuildTypes
	{
		extern const InitialBuildType Zerg_4_Drone;
		extern const InitialBuildType Zerg_5_Drone;
		extern const InitialBuildType Zerg_9_Drone;
		extern const InitialBuildType Zerg_9_Hat;
		extern const InitialBuildType Zerg_9_OverPool;
		extern const InitialBuildType Zerg_9_Balup;
		extern const InitialBuildType Zerg_12_Pool;
		extern const InitialBuildType Zerg_12_Hat;
		extern const InitialBuildType Zerg_12_Ap;
		extern const InitialBuildType Zerg_sunken_rush;
		extern const InitialBuildType Zerg_4_Drone_Real;
		extern const InitialBuildType Toss_1g_dragoon;
		extern const InitialBuildType Toss_2g_zealot;
		extern const InitialBuildType Toss_2g_dragoon;
		extern const InitialBuildType Toss_1g_forward;
		extern const InitialBuildType Toss_2g_forward;
		extern const InitialBuildType Toss_1g_double;
		extern const InitialBuildType Toss_pure_double;
		extern const InitialBuildType Toss_forge;
		extern const InitialBuildType Toss_4probes;
		extern const InitialBuildType Toss_cannon_rush;
		extern const InitialBuildType Terran_2b;
		extern const InitialBuildType Terran_1b_forward;
		extern const InitialBuildType Terran_2b_forward;
		extern const InitialBuildType Terran_bunker_rush;
		extern const InitialBuildType Terran_1b_double;
		extern const InitialBuildType Terran_1fac;
		extern const InitialBuildType Terran_pure_double;
		extern const InitialBuildType Terran_4scv;
		extern const InitialBuildType UnknownBuild;
	}

	namespace MainBuildTypes
	{
		namespace Enum
		{
			enum Enum
			{
				Zerg_main_zergling = 0,
				Zerg_main_maybe_mutal,
				Zerg_main_hydra,
				Zerg_main_lurker,
				Zerg_main_mutal,
				Zerg_main_fast_mutal,
				Zerg_main_hydra_mutal,
				Zerg_main_queen_hydra,
				Toss_range_up_dra_push,
				Toss_2gate_dra_push, // 15투 or 2게이트 드라군
				Toss_pure_tripple,
				Toss_drop, // 질럿, 드라군 드랍
				Toss_reaver_drop,
				Toss_dark, // 9
				Toss_dark_drop,
				Toss_1base_fast_zealot, // 앞마당 안먹고 올인
				Toss_2base_zealot_dra,  // 앞마당 먹고 질럿드라군 올인
				Toss_mass_production,
				Toss_Scout,
				Toss_fast_carrier, //0
				Toss_arbiter,
				Toss_arbiter_carrier,
				Terran_1fac_double,
				Terran_1fac_1star,
				Terran_1fac_2star,
				Terran_2fac,
				Terran_2fac_double,
				Terran_2fac_1star,
				Terran_3fac,
				UnknownMainBuild,
				MAX
			};
		};
	}

	class MainBuildType : public Type<MainBuildType, MainBuildTypes::Enum::UnknownMainBuild>
	{
	public:
		MainBuildType(int id = MainBuildTypes::Enum::UnknownMainBuild);
	};

	namespace MainBuildTypes
	{
		extern const MainBuildType Zerg_main_zergling;
		extern const MainBuildType Zerg_main_maybe_mutal;
		extern const MainBuildType Zerg_main_hydra;
		extern const MainBuildType Zerg_main_lurker;
		extern const MainBuildType Zerg_main_mutal;
		extern const MainBuildType Zerg_main_fast_mutal;
		extern const MainBuildType Zerg_main_hydra_mutal;
		extern const MainBuildType Zerg_main_queen_hydra;
		extern const MainBuildType Toss_range_up_dra_push;
		extern const MainBuildType Toss_2gate_dra_push;
		extern const MainBuildType Toss_pure_tripple;
		extern const MainBuildType Toss_drop;
		extern const MainBuildType Toss_reaver_drop;
		extern const MainBuildType Toss_dark;
		extern const MainBuildType Toss_dark_drop;
		extern const MainBuildType Toss_1base_fast_zealot;
		extern const MainBuildType Toss_2base_zealot_dra;
		extern const MainBuildType Toss_mass_production;
		extern const MainBuildType Toss_Scout;
		extern const MainBuildType Toss_fast_carrier;
		extern const MainBuildType Toss_arbiter;
		extern const MainBuildType Toss_arbiter_carrier;
		extern const MainBuildType Terran_1fac_double;
		extern const MainBuildType Terran_1fac_1star;
		extern const MainBuildType Terran_1fac_2star;
		extern const MainBuildType Terran_2fac;
		extern const MainBuildType Terran_2fac_double;
		extern const MainBuildType Terran_2fac_1star;
		extern const MainBuildType Terran_3fac;
		extern const MainBuildType UnknownMainBuild;
	}

	namespace MainStrategyTypes
	{
		namespace Enum {
			enum Enum {
				WaitToBase,						/// 본진 상태
				WaitToFirstExpansion,			/// 앞마당 상태
				AttackAll,						/// 적진으로 공격
				BackAll,						/// 후퇴
				DrawLine,                       /// 선긋기
				WaitLine,                       /// 선긋고 대기
				Eliminate,
				MAX
			};
		};
	}
	class MainStrategyType : public Type<MainStrategyType, MainStrategyTypes::Enum::Eliminate>
	{
	public:
		MainStrategyType(int id = MainStrategyTypes::Enum::WaitToBase);
	};

	namespace MainStrategyTypes
	{
		extern const MainStrategyType WaitToBase;
		extern const MainStrategyType WaitToFirstExpansion;
		extern const MainStrategyType AttackAll;
		extern const MainStrategyType BackAll;
		extern const MainStrategyType DrawLine;
		extern const MainStrategyType WaitLine;
		extern const MainStrategyType Eliminate;
	}

	namespace MyBuildTypes
	{
		namespace Enum {
			enum Enum {
				Terran_VultureTankWraith,
				Terran_TankGoliath,
				Protoss_MineKiller,
				Protoss_ZealotKiller,
				Protoss_CannonKiller,
				Protoss_CarrierKiller,
				Protoss_DragoonKiller,
				Protoss_TemplarKiller,
				Zerg_MMtoMachanic,
				Protoss_FullMechanic,
				Basic,
				MAX
			};
		};
	}
	class MyBuildType : public Type<MyBuildType, MyBuildTypes::Enum::Basic>
	{
	public:
		MyBuildType(int id = MyBuildTypes::Enum::Basic);
	};

	namespace MyBuildTypes
	{
		extern const MyBuildType Terran_VultureTankWraith;
		extern const MyBuildType Protoss_MineKiller;

		extern const MyBuildType Protoss_ZealotKiller;
		extern const MyBuildType Protoss_CannonKiller;
		extern const MyBuildType Protoss_CarrierKiller;
		extern const MyBuildType Protoss_DragoonKiller;
		extern const MyBuildType Protoss_TemplarKiller;
		extern const MyBuildType Zerg_MMtoMachanic;
		extern const MyBuildType Protoss_FullMechanic;
		extern const MyBuildType Terran_TankGoliath;
		extern const MyBuildType Basic;
	}
}
