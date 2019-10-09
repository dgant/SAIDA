EntryName                                    : Stormbreaker
Contact Person Name (First, Last)            : Mingqiang Li
Contact Person Email Address:                : li_mingqiang_ok@aliyun.com
Affiliation                                  : Independent
Race (in full game)                          : Terran
DLL or ProxyBot                              : DLL
BWAPI Version (3.7.4 or 4.X)                 : 4.1.2
GPU compute required? (GTX 1080ti)           : NO
Compile info                                 : Stormbreaker.sln in visual studio 2013 with visual studio 2013 compiler。compile the Stormbreaker project.


To Compile:
- Bwapi 4.1.2 & BWTA (set environment viariables with names BWAPI_DIR & BWTA_DIR)
- Open src/Stormbreaker.sln in VS2013
- Select Release mode
- Build the Stormbreaker project
- Output will go to src/Release_DLL/dll/Stormbreaker.dll

Tournament Setup:
- Copy Stormbreaker.dll、DLmodel_stateValue、DLmodel_policy (or the files in dll directory) to the tournament Stormbreaker/AI folder
- Set bots: {"BotName": "Stormbreaker", "Race": "Terran", "BotType": "dll", "BWAPIVersion": "BWAPI_412"}